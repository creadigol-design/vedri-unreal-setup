// Copyright vedri studio Ltd. All Rights Reserved.

#include "TestHarness.h"

#include "Core/VPCoreChangeSet.h"

#include <map>
#include <optional>
#include <string>

using namespace Vedri::Core;

namespace
{
	ReportStrings MakeEnglishReportStrings()
	{
		ReportStrings Strings;
		Strings.Title = "vedri VP Toolkit setup report";
		Strings.GeneratedLabel = "Generated";
		Strings.PresetLabel = "Preset";
		Strings.EngineLabel = "Engine";
		Strings.ToolkitLabel = "Toolkit";
		Strings.RestartNotice = "Restart the editor to finish enabling plugins.";
		Strings.ItemHeader = "Item";
		Strings.ChangeHeader = "Change";
		Strings.ResultHeader = "Result";
		Strings.NoChanges = "Nothing to change: the project is already set up.";
		Strings.NoResultReported = "no result reported";
		Strings.CategoryHeadings = {{EChangeCategory::Plugin, "Plugins"}, {EChangeCategory::Setting, "Project settings"},
			{EChangeCategory::Asset, "Assets"}, {EChangeCategory::File, "Files"}};
		Strings.KindNames = {{EChangeKind::Create, "Create"}, {EChangeKind::Modify, "Change"}, {EChangeKind::Regenerate, "Regenerate"},
			{EChangeKind::Overwrite, "Overwrite"}, {EChangeKind::Unchanged, "Already set"}, {EChangeKind::Unavailable, "Not installed"}};
		Strings.StatusNames = {{EApplyStatus::Applied, "Done"}, {EApplyStatus::Skipped, "Skipped"}, {EApplyStatus::NotNeeded, "-"},
			{EApplyStatus::Failed, "FAILED"}};
		return Strings;
	}

	/**
	 * A fake project: settings, plugins and generated assets, plus a Preview that diffs a desired state
	 * against it and an Apply that writes the ticked changes back. Mirrors what the real actions do.
	 */
	struct FakeProject
	{
		std::map<std::string, std::string> Settings;
		std::map<std::string, PluginState> Plugins;
		std::map<std::string, TargetState> Assets;

		ChangeSet Preview(const std::string& Fingerprint) const
		{
			ChangeSet Changes;
			const auto Setting = [this](const std::string& Id) -> std::optional<std::string>
			{
				const auto Found = Settings.find(Id);
				return Found == Settings.end() ? std::nullopt : std::optional<std::string>(Found->second);
			};
			Changes.Add(DiffSetting("ConfigureTiming", "/Script/Engine.Engine/bUseFixedFrameRate", Setting("/Script/Engine.Engine/bUseFixedFrameRate"), "True"));
			Changes.Add(DiffSetting("ConfigureTiming", "/Script/Engine.Engine/FixedFrameRate", Setting("/Script/Engine.Engine/FixedFrameRate"), "25.000000"));
			for (const char* Plugin : {"LiveLink", "MediaIOFramework"})
			{
				const auto Found = Plugins.find(Plugin);
				Changes.Add(DiffPlugin("EnablePlugins", Plugin, Found == Plugins.end() ? PluginState{} : Found->second));
			}
			const std::string Profile = "/Game/VPToolkit/Config/MP_VPToolkit";
			const auto FoundAsset = Assets.find(Profile);
			Changes.Add(DiffGenerated("CreateMediaProfile", EChangeCategory::Asset, Profile, FoundAsset == Assets.end() ? TargetState{} : FoundAsset->second, Fingerprint));
			return Changes;
		}

		std::vector<ApplyResult> Apply(const ChangeSet& Changes)
		{
			std::vector<ApplyResult> Results;
			for (const Change& Item : Changes.GetChanges())
			{
				if (!IsSelectableKind(Item.Kind))
				{
					Results.push_back({Item.Key, EApplyStatus::NotNeeded, ""});
					continue;
				}
				if (!Item.bSelected)
				{
					Results.push_back({Item.Key, EApplyStatus::Skipped, ""});
					continue;
				}
				const std::string Id = Item.Key.substr(Item.Key.find(':') + 1);
				switch (Item.Category)
				{
				case EChangeCategory::Setting:
					Settings[Id] = Item.After;
					break;
				case EChangeCategory::Plugin:
					Plugins[Id].bEnabled = true;
					break;
				case EChangeCategory::Asset:
				case EChangeCategory::File:
					Assets[Id] = TargetState{true, Item.After};
					break;
				}
				Results.push_back({Item.Key, EApplyStatus::Applied, ""});
			}
			return Results;
		}
	};
}

VEDRI_TEST(Changes_DiffSettingKinds)
{
	const Change Created = DiffSetting("A", "S/P", std::nullopt, "1", true);
	EXPECT_EQ(Created.Kind, EChangeKind::Create);
	EXPECT_EQ(Created.Key, "setting:S/P");
	EXPECT_TRUE(Created.bSelected);
	EXPECT_TRUE(Created.bRequiresRestart);

	const Change Same = DiffSetting("A", "S/P", std::string("1"), "1", true);
	EXPECT_EQ(Same.Kind, EChangeKind::Unchanged);
	EXPECT_FALSE(Same.bSelected);
	EXPECT_FALSE(Same.bRequiresRestart);

	const Change Modified = DiffSetting("A", "S/P", std::string("0"), "1");
	EXPECT_EQ(Modified.Kind, EChangeKind::Modify);
	EXPECT_TRUE(Modified.bSelected);
	EXPECT_EQ(*Modified.Before, "0");
}

VEDRI_TEST(Changes_DiffPluginKinds)
{
	const Change Missing = DiffPlugin("EnablePlugins", "MoSysLiveLink", PluginState{false, false});
	EXPECT_EQ(Missing.Kind, EChangeKind::Unavailable);
	EXPECT_FALSE(Missing.bSelected);
	EXPECT_FALSE(Missing.bRequiresRestart);

	const Change Disabled = DiffPlugin("EnablePlugins", "LiveLink", PluginState{true, false});
	EXPECT_EQ(Disabled.Kind, EChangeKind::Modify);
	EXPECT_TRUE(Disabled.bSelected);
	EXPECT_TRUE(Disabled.bRequiresRestart);
	EXPECT_EQ(Disabled.Key, "plugin:LiveLink");

	const Change Enabled = DiffPlugin("EnablePlugins", "LiveLink", PluginState{true, true});
	EXPECT_EQ(Enabled.Kind, EChangeKind::Unchanged);
	EXPECT_FALSE(Enabled.bRequiresRestart);
}

VEDRI_TEST(Changes_GeneratedAssetsAreNeverSilentlyReplaced)
{
	const std::string Path = "/Game/VPToolkit/Config/MP_VPToolkit";
	EXPECT_EQ(DiffGenerated("A", EChangeCategory::Asset, Path, TargetState{false, std::nullopt}, "f1").Kind, EChangeKind::Create);

	const Change Foreign = DiffGenerated("A", EChangeCategory::Asset, Path, TargetState{true, std::nullopt}, "f1");
	EXPECT_EQ(Foreign.Kind, EChangeKind::Overwrite);
	EXPECT_FALSE(Foreign.bSelected);

	const Change Ours = DiffGenerated("A", EChangeCategory::Asset, Path, TargetState{true, std::string("f1")}, "f1");
	EXPECT_EQ(Ours.Kind, EChangeKind::Unchanged);

	const Change Stale = DiffGenerated("A", EChangeCategory::Asset, Path, TargetState{true, std::string("f0")}, "f1");
	EXPECT_EQ(Stale.Kind, EChangeKind::Regenerate);
	EXPECT_TRUE(Stale.bSelected);
}

VEDRI_TEST(Changes_RerunIsIdempotent)
{
	FakeProject Project;
	Project.Settings["/Script/Engine.Engine/bUseFixedFrameRate"] = "False";
	Project.Plugins["LiveLink"] = PluginState{true, false};
	Project.Plugins["MediaIOFramework"] = PluginState{true, true};

	ChangeSet First = Project.Preview("fingerprint-a");
	EXPECT_EQ(First.CountSelected(), size_t(4));
	EXPECT_TRUE(First.RequiresRestart());
	Project.Apply(First);

	const ChangeSet Second = Project.Preview("fingerprint-a");
	EXPECT_EQ(Second.CountSelected(), size_t(0));
	EXPECT_FALSE(Second.RequiresRestart());
	for (const Change& Item : Second.GetChanges())
	{
		EXPECT_EQ(Item.Kind, EChangeKind::Unchanged);
	}

	// Changing a wizard setting only regenerates what depends on it.
	const ChangeSet Third = Project.Preview("fingerprint-b");
	EXPECT_EQ(Third.CountSelected(), size_t(1));
	EXPECT_EQ(Third.Find("asset:/Game/VPToolkit/Config/MP_VPToolkit")->Kind, EChangeKind::Regenerate);
}

VEDRI_TEST(Changes_UntickedItemsAreLeftAlone)
{
	FakeProject Project;
	Project.Plugins["LiveLink"] = PluginState{true, false};
	Project.Plugins["MediaIOFramework"] = PluginState{true, false};
	ChangeSet Changes = Project.Preview("f");
	EXPECT_TRUE(Changes.SetSelected("plugin:LiveLink", false));
	const std::vector<ApplyResult> Results = Project.Apply(Changes);
	EXPECT_FALSE(Project.Plugins["LiveLink"].bEnabled);
	EXPECT_TRUE(Project.Plugins["MediaIOFramework"].bEnabled);
	EXPECT_EQ(Results[2].Status, EApplyStatus::Skipped);
}

VEDRI_TEST(Changes_ConflictsAreDetected)
{
	ChangeSet Changes;
	Changes.Add(DiffSetting("ConfigureTiming", "/Script/Engine.Engine/bSmoothFrameRate", std::string("True"), "False"));
	Changes.Add(DiffSetting("HardenSettings", "/Script/Engine.Engine/bSmoothFrameRate", std::string("True"), "False"));
	EXPECT_EQ(Changes.GetChanges().size(), size_t(1));
	EXPECT_FALSE(Changes.HasConflicts());
	EXPECT_EQ(Changes.GetChanges()[0].ActionId, "ConfigureTiming");

	Changes.Add(DiffSetting("Other", "/Script/Engine.Engine/bSmoothFrameRate", std::string("True"), "True"));
	EXPECT_TRUE(Changes.HasConflicts());
	EXPECT_EQ(Changes.GetChanges().size(), size_t(1));
	const ValidationIssue* Issue = FindIssue(Changes.GetIssues(), "changes.conflict");
	ASSERT_TRUE(Issue != nullptr);
	EXPECT_TRUE(Issue->Args == (std::vector<std::string>{"ConfigureTiming", "Other", "False", "True"}));
}

VEDRI_TEST(Changes_SelectionOverridesRoundTrip)
{
	FakeProject Project;
	Project.Plugins["LiveLink"] = PluginState{true, false};
	Project.Assets["/Game/VPToolkit/Config/MP_VPToolkit"] = TargetState{true, std::nullopt}; // someone else's asset

	ChangeSet Changes = Project.Preview("f");
	EXPECT_FALSE(Changes.Find("asset:/Game/VPToolkit/Config/MP_VPToolkit")->bSelected);
	EXPECT_TRUE(Changes.SetSelected("asset:/Game/VPToolkit/Config/MP_VPToolkit", true));
	EXPECT_TRUE(Changes.SetSelected("plugin:LiveLink", false));
	EXPECT_FALSE(Changes.SetSelected("plugin:MediaIOFramework", true)); // not installed: not selectable
	EXPECT_FALSE(Changes.SetSelected("plugin:DoesNotExist", true));

	const std::map<std::string, bool> Overrides = Changes.ExtractSelectionOverrides();
	EXPECT_EQ(Overrides.size(), size_t(2));
	EXPECT_TRUE(Overrides.at("asset:/Game/VPToolkit/Config/MP_VPToolkit"));
	EXPECT_FALSE(Overrides.at("plugin:LiveLink"));

	ChangeSet Fresh = Project.Preview("f");
	Fresh.ApplySelectionOverrides(Overrides);
	EXPECT_TRUE(Fresh.ExtractSelectionOverrides() == Overrides);
	EXPECT_EQ(Fresh.CountSelected(), Changes.CountSelected());
}

VEDRI_TEST(Changes_GroupByCategoryKeepsOrder)
{
	FakeProject Project;
	const ChangeSet Changes = Project.Preview("f");
	const std::vector<const Change*> Settings = Changes.GetByCategory(EChangeCategory::Setting);
	ASSERT_TRUE(Settings.size() == 2);
	EXPECT_EQ(Settings[0]->Key, "setting:/Script/Engine.Engine/bUseFixedFrameRate");
	EXPECT_EQ(Changes.GetByCategory(EChangeCategory::Plugin).size(), size_t(2));
	EXPECT_EQ(Changes.GetByCategory(EChangeCategory::Asset).size(), size_t(1));
	EXPECT_TRUE(Changes.GetByCategory(EChangeCategory::File).empty());
}

VEDRI_TEST(Report_ListsEveryChangeWithItsOutcome)
{
	FakeProject Project;
	Project.Settings["/Script/Engine.Engine/bUseFixedFrameRate"] = "False";
	Project.Plugins["LiveLink"] = PluginState{true, false};
	ChangeSet Changes = Project.Preview("f");
	Changes.SetSelected("setting:/Script/Engine.Engine/FixedFrameRate", false);
	const std::vector<ApplyResult> Results = Project.Apply(Changes);

	ReportStrings Strings = MakeEnglishReportStrings();
	Strings.DescribeChange = [](const Change& Item) { return Item.Key == "plugin:LiveLink" ? std::string("Live Link | core") : std::string(); };
	const ReportContext Context{"2026-10-28T12:00:00Z", "vedri green screen 25p Blackmagic", "5.5.4", "0.1.0"};
	const std::string Report = BuildSetupReport(Changes, Results, Context, Strings);

	EXPECT_EQ(Report.rfind("# vedri VP Toolkit setup report\n", 0), size_t(0));
	EXPECT_TRUE(Report.find("- Preset: vedri green screen 25p Blackmagic\n") != std::string::npos);
	EXPECT_TRUE(Report.find("> Restart the editor to finish enabling plugins.") != std::string::npos);
	const size_t Plugins = Report.find("## Plugins");
	const size_t Settings = Report.find("## Project settings");
	const size_t Assets = Report.find("## Assets");
	EXPECT_TRUE(Plugins != std::string::npos && Plugins < Settings && Settings < Assets);
	EXPECT_TRUE(Report.find("## Files") == std::string::npos);
	EXPECT_TRUE(Report.find("| Live Link \\| core | Change | Done |") != std::string::npos);
	EXPECT_TRUE(Report.find("| plugin:MediaIOFramework | Not installed | - |") != std::string::npos);
	EXPECT_TRUE(Report.find("| setting:/Script/Engine.Engine/bUseFixedFrameRate | Change: `False` -> `True` | Done |") != std::string::npos);
	EXPECT_TRUE(Report.find("| setting:/Script/Engine.Engine/FixedFrameRate | Create: `25.000000` | Skipped |") != std::string::npos);
	EXPECT_TRUE(Report.find("| asset:/Game/VPToolkit/Config/MP_VPToolkit | Create | Done |") != std::string::npos);
}

VEDRI_TEST(Report_MissingResultIsAFailureAndEmptySetSaysSo)
{
	ChangeSet Changes;
	Changes.Add(DiffSetting("A", "S/P", std::nullopt, "1"));
	const ReportStrings Strings = MakeEnglishReportStrings();
	const std::string Report = BuildSetupReport(Changes, {}, ReportContext{}, Strings);
	EXPECT_TRUE(Report.find("| setting:S/P | Create: `1` | FAILED (no result reported) |") != std::string::npos);
	EXPECT_TRUE(Report.find("> Restart") == std::string::npos);

	const std::string Empty = BuildSetupReport(ChangeSet(), {}, ReportContext{}, Strings);
	EXPECT_TRUE(Empty.find("Nothing to change: the project is already set up.") != std::string::npos);
}
