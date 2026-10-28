// Copyright vedri studio Ltd. All Rights Reserved.

#pragma once

// The Summary / Apply diff model.
//
// Every setup action's Preview() reports what it would do as Change records. The wizard collects them
// into one ChangeSet, shows it grouped by category with a tick per item, and Apply() only touches the
// ticked ones. Re-running the wizard on an already-configured project yields only Unchanged items:
// that is what makes the wizard idempotent.
//
// Non-destructive rule: something the toolkit did not create is never replaced by default. Overwrite
// items start unticked and need an explicit tick (which a preset can record in SelectionOverrides).
//
// Change keys are stable strings, "<category>:<id>". The engine names below are illustrative only; the
// real ids are chosen, and verified against the engine headers, by each action in M1-M6:
//   plugin:<PluginName>                      e.g. plugin:LiveLink
//   setting:<ConfigSectionOrClass>/<Property> e.g. setting:/Script/Engine.Engine/bUseFixedFrameRate
//   asset:<ObjectPath>                       e.g. asset:/Game/VPToolkit/Config/MP_VPToolkit
//   file:<ProjectRelativePath>               e.g. file:Saved/VPToolkit/VPToolkit_SetupReport.md
// Values (Before / After) are engine export text; Core only compares them as strings.

#include "Core/VPCoreValidation.h"

#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace Vedri::Core
{
	enum class EChangeCategory : uint8_t
	{
		Plugin,
		Setting,
		Asset,
		File
	};

	enum class EChangeKind : uint8_t
	{
		/** Does not exist yet; will be created. Ticked by default. */
		Create,
		/** Exists with a different value (settings, plugin enablement). Ticked by default. */
		Modify,
		/** An asset the toolkit generated earlier, from different settings. Ticked by default. */
		Regenerate,
		/** Exists and was not made by the toolkit. Unticked by default: never replaced silently. */
		Overwrite,
		/** Already as desired. Not selectable. */
		Unchanged,
		/** Cannot be done here (e.g. plugin not installed). Not selectable; shown with a notice. */
		Unavailable
	};

	inline const char* ChangeCategoryId(EChangeCategory Category)
	{
		switch (Category)
		{
		case EChangeCategory::Plugin:
			return "plugin";
		case EChangeCategory::Setting:
			return "setting";
		case EChangeCategory::Asset:
			return "asset";
		case EChangeCategory::File:
			return "file";
		}
		return "";
	}

	inline const char* ChangeKindId(EChangeKind Kind)
	{
		switch (Kind)
		{
		case EChangeKind::Create:
			return "create";
		case EChangeKind::Modify:
			return "modify";
		case EChangeKind::Regenerate:
			return "regenerate";
		case EChangeKind::Overwrite:
			return "overwrite";
		case EChangeKind::Unchanged:
			return "unchanged";
		case EChangeKind::Unavailable:
			return "unavailable";
		}
		return "";
	}

	inline std::string MakeChangeKey(EChangeCategory Category, const std::string& Id)
	{
		return std::string(ChangeCategoryId(Category)) + ":" + Id;
	}

	inline bool IsSelectableKind(EChangeKind Kind)
	{
		return Kind != EChangeKind::Unchanged && Kind != EChangeKind::Unavailable;
	}

	inline bool IsSelectedByDefault(EChangeKind Kind)
	{
		return Kind == EChangeKind::Create || Kind == EChangeKind::Modify || Kind == EChangeKind::Regenerate;
	}

	struct Change
	{
		/** Which setup action produced it, e.g. "EnablePlugins". */
		std::string ActionId;
		EChangeCategory Category = EChangeCategory::Setting;
		std::string Key;
		EChangeKind Kind = EChangeKind::Create;
		std::optional<std::string> Before;
		std::string After;
		bool bRequiresRestart = false;
		bool bSelected = true;

		bool operator==(const Change& Other) const = default;
	};

	/** Effective value of a setting. A setting always has a value; a missing one means "not set yet". */
	inline Change DiffSetting(const std::string& ActionId, const std::string& SettingId, const std::optional<std::string>& Current,
		const std::string& Desired, bool bRequiresRestart = false)
	{
		Change Result;
		Result.ActionId = ActionId;
		Result.Category = EChangeCategory::Setting;
		Result.Key = MakeChangeKey(EChangeCategory::Setting, SettingId);
		Result.Before = Current;
		Result.After = Desired;
		Result.Kind = !Current ? EChangeKind::Create : (*Current == Desired ? EChangeKind::Unchanged : EChangeKind::Modify);
		Result.bRequiresRestart = bRequiresRestart && Result.Kind != EChangeKind::Unchanged;
		Result.bSelected = IsSelectedByDefault(Result.Kind);
		return Result;
	}

	struct PluginState
	{
		bool bInstalled = false;
		bool bEnabled = false;
	};

	/** Enabling a plugin always needs an editor restart. */
	inline Change DiffPlugin(const std::string& ActionId, const std::string& PluginName, const PluginState& State)
	{
		Change Result;
		Result.ActionId = ActionId;
		Result.Category = EChangeCategory::Plugin;
		Result.Key = MakeChangeKey(EChangeCategory::Plugin, PluginName);
		Result.After = "enabled";
		if (!State.bInstalled)
		{
			Result.Kind = EChangeKind::Unavailable;
			Result.Before = std::string("missing");
		}
		else
		{
			Result.Before = std::string(State.bEnabled ? "enabled" : "disabled");
			Result.Kind = State.bEnabled ? EChangeKind::Unchanged : EChangeKind::Modify;
		}
		Result.bRequiresRestart = Result.Kind == EChangeKind::Modify;
		Result.bSelected = IsSelectedByDefault(Result.Kind);
		return Result;
	}

	/**
	 * What is on disk at an asset or file path. Generated assets carry the fingerprint of the settings
	 * that produced them (asset metadata tag on the PC; a header line for generated text files).
	 */
	struct TargetState
	{
		bool bExists = false;
		std::optional<std::string> GeneratorFingerprint;
	};

	inline Change DiffGenerated(const std::string& ActionId, EChangeCategory Category, const std::string& Path, const TargetState& State,
		const std::string& DesiredFingerprint)
	{
		Change Result;
		Result.ActionId = ActionId;
		Result.Category = Category;
		Result.Key = MakeChangeKey(Category, Path);
		Result.After = DesiredFingerprint;
		Result.Before = State.GeneratorFingerprint;
		if (!State.bExists)
		{
			Result.Kind = EChangeKind::Create;
		}
		else if (!State.GeneratorFingerprint)
		{
			Result.Kind = EChangeKind::Overwrite;
		}
		else if (*State.GeneratorFingerprint == DesiredFingerprint)
		{
			Result.Kind = EChangeKind::Unchanged;
		}
		else
		{
			Result.Kind = EChangeKind::Regenerate;
		}
		Result.bSelected = IsSelectedByDefault(Result.Kind);
		return Result;
	}

	class ChangeSet
	{
	public:
		/**
		 * Adds a change. Two actions may report the same key with the same After value (they agree; the
		 * first one is kept). Different After values are a conflict: an Error issue is recorded and the
		 * second change is dropped, so Apply can never run both.
		 */
		void Add(Change NewChange)
		{
			for (const Change& Existing : Changes)
			{
				if (Existing.Key == NewChange.Key)
				{
					if (Existing.After != NewChange.After)
					{
						AddIssue(ConflictIssues, ESeverity::Error, "changes.conflict", NewChange.Key,
							{Existing.ActionId, NewChange.ActionId, Existing.After, NewChange.After});
					}
					return;
				}
			}
			Changes.push_back(std::move(NewChange));
		}

		void Append(const std::vector<Change>& NewChanges)
		{
			for (const Change& NewChange : NewChanges)
			{
				Add(NewChange);
			}
		}

		const std::vector<Change>& GetChanges() const { return Changes; }
		const IssueList& GetIssues() const { return ConflictIssues; }

		const Change* Find(const std::string& Key) const
		{
			for (const Change& Candidate : Changes)
			{
				if (Candidate.Key == Key)
				{
					return &Candidate;
				}
			}
			return nullptr;
		}

		std::vector<const Change*> GetByCategory(EChangeCategory Category) const
		{
			std::vector<const Change*> Result;
			for (const Change& Candidate : Changes)
			{
				if (Candidate.Category == Category)
				{
					Result.push_back(&Candidate);
				}
			}
			return Result;
		}

		/** Returns false (and changes nothing) for unknown keys and non-selectable kinds. */
		bool SetSelected(const std::string& Key, bool bSelected)
		{
			for (Change& Candidate : Changes)
			{
				if (Candidate.Key == Key)
				{
					if (!IsSelectableKind(Candidate.Kind))
					{
						return false;
					}
					Candidate.bSelected = bSelected;
					return true;
				}
			}
			return false;
		}

		/** Applies a preset's recorded ticks. Keys that are absent or not selectable this time are ignored. */
		void ApplySelectionOverrides(const std::map<std::string, bool>& Overrides)
		{
			for (const auto& [Key, bSelected] : Overrides)
			{
				SetSelected(Key, bSelected);
			}
		}

		/** The ticks that differ from the defaults, for saving into a preset. */
		std::map<std::string, bool> ExtractSelectionOverrides() const
		{
			std::map<std::string, bool> Overrides;
			for (const Change& Candidate : Changes)
			{
				if (IsSelectableKind(Candidate.Kind) && Candidate.bSelected != IsSelectedByDefault(Candidate.Kind))
				{
					Overrides[Candidate.Key] = Candidate.bSelected;
				}
			}
			return Overrides;
		}

		size_t CountSelected() const
		{
			size_t Count = 0;
			for (const Change& Candidate : Changes)
			{
				Count += (Candidate.bSelected && IsSelectableKind(Candidate.Kind)) ? 1u : 0u;
			}
			return Count;
		}

		bool RequiresRestart() const
		{
			for (const Change& Candidate : Changes)
			{
				if (Candidate.bSelected && Candidate.bRequiresRestart && IsSelectableKind(Candidate.Kind))
				{
					return true;
				}
			}
			return false;
		}

		bool HasConflicts() const { return HasErrors(ConflictIssues); }

	private:
		std::vector<Change> Changes;
		IssueList ConflictIssues;
	};

	enum class EApplyStatus : uint8_t
	{
		Applied,
		/** Unticked by the user. */
		Skipped,
		/** Nothing to do (Unchanged or Unavailable). */
		NotNeeded,
		Failed
	};

	inline const char* ApplyStatusId(EApplyStatus Status)
	{
		switch (Status)
		{
		case EApplyStatus::Applied:
			return "applied";
		case EApplyStatus::Skipped:
			return "skipped";
		case EApplyStatus::NotNeeded:
			return "notNeeded";
		case EApplyStatus::Failed:
			return "failed";
		}
		return "";
	}

	struct ApplyResult
	{
		std::string Key;
		EApplyStatus Status = EApplyStatus::Applied;
		/** Engine-side detail for failures (log text). */
		std::string Detail;
	};

	/**
	 * Every piece of text the setup report needs. Core holds no user-facing strings: the editor fills
	 * this from LOCTEXT (so the report follows the editor language), the standalone tests fill it in
	 * English.
	 */
	struct ReportStrings
	{
		std::string Title;
		std::string GeneratedLabel;
		std::string PresetLabel;
		std::string EngineLabel;
		std::string ToolkitLabel;
		std::string RestartNotice;
		std::string ItemHeader;
		std::string ChangeHeader;
		std::string ResultHeader;
		std::string NoChanges;
		/** Shown when a ticked item has no apply result (Apply stopped early). */
		std::string NoResultReported;
		std::map<EChangeCategory, std::string> CategoryHeadings;
		std::map<EChangeKind, std::string> KindNames;
		std::map<EApplyStatus, std::string> StatusNames;
		/** Optional human label for a change; the key is shown when empty or unset. */
		std::function<std::string(const Change&)> DescribeChange;
	};

	namespace ReportDetail
	{
		/** Escapes text for a Markdown table cell. */
		inline std::string Cell(const std::string& Text)
		{
			std::string Out;
			for (const char C : Text)
			{
				if (C == '|')
				{
					Out += "\\|";
				}
				else if (C == '\n' || C == '\r')
				{
					Out.push_back(' ');
				}
				else
				{
					Out.push_back(C);
				}
			}
			return Out;
		}

		template <typename KeyType>
		std::string Lookup(const std::map<KeyType, std::string>& Names, KeyType Key, const char* Fallback)
		{
			const auto Found = Names.find(Key);
			return Found != Names.end() && !Found->second.empty() ? Found->second : std::string(Fallback);
		}
	}

	struct ReportContext
	{
		std::string TimestampUtc;
		std::string PresetName;
		std::string EngineVersion;
		std::string ToolkitVersion;
	};

	/** Builds VPToolkit_SetupReport.md: every change grouped by category, with what Apply did to it. */
	inline std::string BuildSetupReport(const ChangeSet& Changes, const std::vector<ApplyResult>& Results, const ReportContext& Context,
		const ReportStrings& Strings)
	{
		std::string Out;
		Out += "# " + Strings.Title + "\n\n";
		Out += "- " + Strings.GeneratedLabel + ": " + Context.TimestampUtc + "\n";
		Out += "- " + Strings.PresetLabel + ": " + Context.PresetName + "\n";
		Out += "- " + Strings.EngineLabel + ": " + Context.EngineVersion + "\n";
		Out += "- " + Strings.ToolkitLabel + ": " + Context.ToolkitVersion + "\n\n";

		bool bRestartNeeded = false;
		for (const ApplyResult& Result : Results)
		{
			const Change* Applied = Changes.Find(Result.Key);
			bRestartNeeded = bRestartNeeded || (Result.Status == EApplyStatus::Applied && Applied && Applied->bRequiresRestart);
		}
		if (bRestartNeeded)
		{
			Out += "> " + Strings.RestartNotice + "\n\n";
		}

		if (Changes.GetChanges().empty())
		{
			Out += Strings.NoChanges + "\n";
			return Out;
		}

		for (const EChangeCategory Category : {EChangeCategory::Plugin, EChangeCategory::Setting, EChangeCategory::Asset, EChangeCategory::File})
		{
			const std::vector<const Change*> InCategory = Changes.GetByCategory(Category);
			if (InCategory.empty())
			{
				continue;
			}
			Out += "## " + ReportDetail::Lookup(Strings.CategoryHeadings, Category, ChangeCategoryId(Category)) + "\n\n";
			Out += "| " + Strings.ItemHeader + " | " + Strings.ChangeHeader + " | " + Strings.ResultHeader + " |\n";
			Out += "|---|---|---|\n";
			for (const Change* Item : InCategory)
			{
				std::string Label = Strings.DescribeChange ? Strings.DescribeChange(*Item) : std::string();
				if (Label.empty())
				{
					Label = Item->Key;
				}
				std::string What = ReportDetail::Lookup(Strings.KindNames, Item->Kind, ChangeKindId(Item->Kind));
				if (Item->Category == EChangeCategory::Setting && Item->Kind == EChangeKind::Modify && Item->Before)
				{
					What += ": `" + *Item->Before + "` -> `" + Item->After + "`";
				}
				else if (Item->Category == EChangeCategory::Setting && Item->Kind == EChangeKind::Create)
				{
					What += ": `" + Item->After + "`";
				}

				EApplyStatus Status = IsSelectableKind(Item->Kind) ? (Item->bSelected ? EApplyStatus::Failed : EApplyStatus::Skipped) : EApplyStatus::NotNeeded;
				std::string Detail;
				bool bFound = false;
				for (const ApplyResult& Result : Results)
				{
					if (Result.Key == Item->Key)
					{
						Status = Result.Status;
						Detail = Result.Detail;
						bFound = true;
						break;
					}
				}
				std::string ResultText = ReportDetail::Lookup(Strings.StatusNames, Status, ApplyStatusId(Status));
				if (!bFound && Status == EApplyStatus::Failed)
				{
					// Ticked but Apply never reported on it: treat as a failure so it is never silently lost.
					Detail = Strings.NoResultReported;
				}
				if (!Detail.empty())
				{
					ResultText += " (" + Detail + ")";
				}
				Out += "| " + ReportDetail::Cell(Label) + " | " + ReportDetail::Cell(What) + " | " + ReportDetail::Cell(ResultText) + " |\n";
			}
			Out += "\n";
		}
		return Out;
	}
}
