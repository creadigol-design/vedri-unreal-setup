// Copyright vedri studio Ltd. All Rights Reserved.

#pragma once

// Preset (de)serialisation: WizardConfig <-> JSON (Saved/VPToolkit/Presets/*.json and the bundled
// presets in Resources/Presets). Reading is lenient about missing fields (they keep their defaults, so
// presets saved by older versions still load) and strict about wrong types. Unknown fields produce a
// warning so a typo in a hand-edited preset is visible rather than silently ignored.
//
// Schema changes: bump CurrentPresetSchemaVersion, add a migration step in MigratePresetJson, add a
// fixture to Tests/Standalone/Fixtures, and note it in CHANGELOG.md.

#include "Core/VPCoreJson.h"
#include "Core/VPCoreValidation.h"
#include "Core/VPCoreWizardConfig.h"

#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

namespace Vedri::Core
{
	namespace PresetDetail
	{
		class ObjectReader
		{
		public:
			ObjectReader(const JsonValue& InSource, std::string InPath, IssueList& InIssues)
				: Source(InSource), Path(std::move(InPath)), Issues(InIssues)
			{
			}

			std::string FieldPath(const char* Key) const { return Path.empty() ? std::string(Key) : Path + "." + Key; }

			void Bool(const char* Key, bool& Out)
			{
				if (const JsonValue* Value = Take(Key))
				{
					if (Value->IsBool())
					{
						Out = Value->AsBool();
					}
					else
					{
						WrongType(Key, "boolean");
					}
				}
			}

			void Number(const char* Key, double& Out)
			{
				if (const JsonValue* Value = Take(Key))
				{
					if (Value->IsNumber())
					{
						Out = Value->AsNumber();
					}
					else
					{
						WrongType(Key, "number");
					}
				}
			}

			void UInt(const char* Key, uint32_t& Out)
			{
				if (const JsonValue* Value = Take(Key))
				{
					const double Number = Value->AsNumber(-1.0);
					double IntegralPart = 0.0;
					if (Value->IsNumber() && Number >= 0.0 && Number <= 4294967295.0 && std::modf(Number, &IntegralPart) == 0.0)
					{
						Out = static_cast<uint32_t>(Number);
					}
					else
					{
						WrongType(Key, "unsignedInteger");
					}
				}
			}

			void Int(const char* Key, int32_t& Out)
			{
				if (const JsonValue* Value = Take(Key))
				{
					const double Number = Value->AsNumber(0.5);
					double IntegralPart = 0.0;
					if (Value->IsNumber() && Number >= -2147483648.0 && Number <= 2147483647.0 && std::modf(Number, &IntegralPart) == 0.0)
					{
						Out = static_cast<int32_t>(Number);
					}
					else
					{
						WrongType(Key, "integer");
					}
				}
			}

			void String(const char* Key, std::string& Out)
			{
				if (const JsonValue* Value = Take(Key))
				{
					if (Value->IsString())
					{
						Out = Value->AsString();
					}
					else
					{
						WrongType(Key, "string");
					}
				}
			}

			template <typename EnumType, size_t Count>
			void Enum(const char* Key, EnumType& Out, const EnumName<EnumType> (&Table)[Count])
			{
				if (const JsonValue* Value = Take(Key))
				{
					if (!Value->IsString())
					{
						WrongType(Key, "string");
					}
					else if (!EnumFromString(Value->AsString(), Out, Table))
					{
						AddIssue(Issues, ESeverity::Error, "preset.unknownEnumValue", FieldPath(Key), {Value->AsString()});
					}
				}
			}

			/** Returns the nested object, or nullptr when absent or not an object (the latter is reported). */
			const JsonValue* Object(const char* Key)
			{
				const JsonValue* Value = Take(Key);
				if (Value && !Value->IsObject())
				{
					WrongType(Key, "object");
					return nullptr;
				}
				return Value;
			}

			void ReportUnknownFields() const
			{
				for (size_t Index = 0; Index < Source.Size(); ++Index)
				{
					const std::string& Key = Source.KeyAt(Index);
					bool bKnown = false;
					for (const std::string& Consumed : ConsumedKeys)
					{
						bKnown = bKnown || Consumed == Key;
					}
					if (!bKnown)
					{
						AddIssue(Issues, ESeverity::Warning, "preset.unknownField", FieldPath(Key.c_str()), {Key});
					}
				}
			}

		private:
			const JsonValue& Source;
			std::string Path;
			IssueList& Issues;
			std::vector<std::string> ConsumedKeys;

			const JsonValue* Take(const char* Key)
			{
				ConsumedKeys.emplace_back(Key);
				return Source.Find(Key);
			}

			void WrongType(const char* Key, const char* Expected)
			{
				AddIssue(Issues, ESeverity::Error, "preset.wrongType", FieldPath(Key), {Expected});
			}
		};

		inline JsonValue PanelsToJson(const PanelSpec& Panels)
		{
			JsonValue Json = JsonValue::MakeObject();
			Json.Set("pixelPitchMm", Panels.PixelPitchMm);
			Json.Set("tileWidthMm", Panels.TileWidthMm);
			Json.Set("tileHeightMm", Panels.TileHeightMm);
			Json.Set("pixelsPerTileX", Panels.PixelsPerTileX);
			Json.Set("pixelsPerTileY", Panels.PixelsPerTileY);
			return Json;
		}

		inline void PanelsFromJson(const JsonValue& Json, const std::string& Path, PanelSpec& Out, IssueList& Issues)
		{
			ObjectReader Reader(Json, Path, Issues);
			Reader.Number("pixelPitchMm", Out.PixelPitchMm);
			Reader.Number("tileWidthMm", Out.TileWidthMm);
			Reader.Number("tileHeightMm", Out.TileHeightMm);
			Reader.UInt("pixelsPerTileX", Out.PixelsPerTileX);
			Reader.UInt("pixelsPerTileY", Out.PixelsPerTileY);
			Reader.ReportUnknownFields();
		}

		inline JsonValue WallToJson(const WallSpec& Wall)
		{
			JsonValue Json = JsonValue::MakeObject();
			Json.Set("widthM", Wall.WidthM);
			Json.Set("heightM", Wall.HeightM);
			Json.Set("panels", PanelsToJson(Wall.Panels));
			Json.Set("renderNodes", Wall.RenderNodeCount);
			Json.Set("curvatureRadiusM", Wall.CurvatureRadiusM);
			Json.Set("bottomHeightM", Wall.BottomHeightM);
			JsonValue Ceiling = JsonValue::MakeObject();
			Ceiling.Set("enabled", Wall.Ceiling.bEnabled);
			Ceiling.Set("widthM", Wall.Ceiling.WidthM);
			Ceiling.Set("depthM", Wall.Ceiling.DepthM);
			Ceiling.Set("panels", PanelsToJson(Wall.Ceiling.Panels));
			Ceiling.Set("heightM", Wall.Ceiling.HeightM);
			Ceiling.Set("nodeIndex", Wall.Ceiling.NodeIndex);
			Json.Set("ceiling", std::move(Ceiling));
			return Json;
		}

		inline void WallFromJson(const JsonValue& Json, const std::string& Path, WallSpec& Out, IssueList& Issues)
		{
			ObjectReader Reader(Json, Path, Issues);
			Reader.Number("widthM", Out.WidthM);
			Reader.Number("heightM", Out.HeightM);
			if (const JsonValue* Panels = Reader.Object("panels"))
			{
				PanelsFromJson(*Panels, Reader.FieldPath("panels"), Out.Panels, Issues);
			}
			Reader.UInt("renderNodes", Out.RenderNodeCount);
			Reader.Number("curvatureRadiusM", Out.CurvatureRadiusM);
			Reader.Number("bottomHeightM", Out.BottomHeightM);
			if (const JsonValue* Ceiling = Reader.Object("ceiling"))
			{
				ObjectReader CeilingReader(*Ceiling, Reader.FieldPath("ceiling"), Issues);
				CeilingReader.Bool("enabled", Out.Ceiling.bEnabled);
				CeilingReader.Number("widthM", Out.Ceiling.WidthM);
				CeilingReader.Number("depthM", Out.Ceiling.DepthM);
				if (const JsonValue* Panels = CeilingReader.Object("panels"))
				{
					PanelsFromJson(*Panels, CeilingReader.FieldPath("panels"), Out.Ceiling.Panels, Issues);
				}
				CeilingReader.Number("heightM", Out.Ceiling.HeightM);
				CeilingReader.Int("nodeIndex", Out.Ceiling.NodeIndex);
				CeilingReader.ReportUnknownFields();
			}
			Reader.ReportUnknownFields();
		}

		/** Upgrades older schema versions in place. Version 1 is the first; nothing to migrate yet. */
		inline void MigratePresetJson(JsonValue& Json, uint32_t FromVersion)
		{
			(void)Json;
			(void)FromVersion;
		}
	}

	inline JsonValue PresetToJson(const WizardConfig& Config)
	{
		JsonValue Json = JsonValue::MakeObject();
		Json.Set("schemaVersion", Config.SchemaVersion);
		Json.Set("name", Config.Name);
		Json.Set("description", Config.Description);
		Json.Set("mode", EnumToString(Config.Mode, ShootModeNames));

		JsonValue VideoIO = JsonValue::MakeObject();
		VideoIO.Set("cardFamily", Config.VideoIO.CardFamily);
		VideoIO.Set("deviceIndex", Config.VideoIO.DeviceIndex);
		VideoIO.Set("inputPort", Config.VideoIO.InputPort);
		VideoIO.Set("outputPort", Config.VideoIO.OutputPort);
		VideoIO.Set("genlockSource", EnumToString(Config.VideoIO.GenlockSource, GenlockSourceNames));
		Json.Set("videoIO", std::move(VideoIO));

		JsonValue Timing = JsonValue::MakeObject();
		JsonValue Rate = JsonValue::MakeObject();
		Rate.Set("numerator", Config.Timing.Rate.Numerator);
		Rate.Set("denominator", Config.Timing.Rate.Denominator);
		Timing.Set("frameRate", std::move(Rate));
		JsonValue Res = JsonValue::MakeObject();
		Res.Set("width", Config.Timing.Res.Width);
		Res.Set("height", Config.Timing.Res.Height);
		Timing.Set("resolution", std::move(Res));
		Timing.Set("dropFrameTimecode", Config.Timing.bDropFrameTimecode);
		Json.Set("timing", std::move(Timing));

		JsonValue VideoOut = JsonValue::MakeObject();
		VideoOut.Set("enabled", Config.VideoOut.bEnabled);
		VideoOut.Set("pixelFormat", EnumToString(Config.VideoOut.PixelFormat, OutputPixelFormatNames));
		VideoOut.Set("keyAndFill", Config.VideoOut.bKeyAndFill);
		VideoOut.Set("source", EnumToString(Config.VideoOut.Source, OutputSourceNames));
		Json.Set("videoOut", std::move(VideoOut));

		// Only the active mode's section is written, so a green-screen preset carries no LED wall.
		if (Config.Mode == EShootMode::GreenScreen)
		{
			JsonValue Green = JsonValue::MakeObject();
			Green.Set("createPlateSource", Config.GreenScreen.bCreatePlateSource);
			Green.Set("createMatteOutput", Config.GreenScreen.bCreateMatteOutput);
			Green.Set("matteOutputPort", Config.GreenScreen.MatteOutputPort);
			Json.Set("greenScreen", std::move(Green));
		}
		else
		{
			JsonValue Led = JsonValue::MakeObject();
			Led.Set("wall", PresetDetail::WallToJson(Config.Led.Wall));
			Led.Set("generateSwitchboardStub", Config.Led.bGenerateSwitchboardStub);
			Led.Set("createDefaultLightCards", Config.Led.bCreateDefaultLightCards);
			Led.Set("createChromakeyMarkers", Config.Led.bCreateChromakeyMarkers);
			Json.Set("led", std::move(Led));
		}

		JsonValue Tracking = JsonValue::MakeObject();
		Tracking.Set("provider", Config.Tracking.ProviderId);
		Tracking.Set("subjectName", Config.Tracking.SubjectName);
		Tracking.Set("autoApplyOnStartup", Config.Tracking.bAutoApplyOnStartup);
		Tracking.Set("providerSettings", Config.Tracking.ProviderSettings);
		Json.Set("tracking", std::move(Tracking));

		JsonValue Lens = JsonValue::MakeObject();
		Lens.Set("cameraSensor", Config.Lens.CameraSensorId);
		Lens.Set("sensorWidthMm", Config.Lens.SensorWidthMm);
		Lens.Set("sensorHeightMm", Config.Lens.SensorHeightMm);
		Json.Set("lens", std::move(Lens));

		JsonValue Overrides = JsonValue::MakeObject();
		for (const auto& [Key, bSelected] : Config.SelectionOverrides)
		{
			Overrides.Set(Key, bSelected);
		}
		Json.Set("selectionOverrides", std::move(Overrides));
		return Json;
	}

	/**
	 * Reads a preset. Returns false if any Error was reported; OutConfig then holds whatever could be
	 * read and must not be applied. Semantic checks (ranges, port clashes) are ValidateConfig's job.
	 */
	inline bool PresetFromJson(const JsonValue& InJson, WizardConfig& OutConfig, IssueList& Issues)
	{
		const size_t ErrorsBefore = CountIssues(Issues, ESeverity::Error);
		OutConfig = WizardConfig();
		if (!InJson.IsObject())
		{
			AddIssue(Issues, ESeverity::Error, "preset.notAnObject", "");
			return false;
		}

		const JsonValue* VersionValue = InJson.Find("schemaVersion");
		const double VersionNumber = VersionValue ? VersionValue->AsNumber(-1.0) : -1.0;
		double IntegralPart = 0.0;
		if (!VersionValue || !VersionValue->IsNumber() || VersionNumber < 1.0 || VersionNumber > 1000000.0 || std::modf(VersionNumber, &IntegralPart) != 0.0)
		{
			AddIssue(Issues, ESeverity::Error, "preset.missingSchemaVersion", "schemaVersion");
			return false;
		}
		const uint32_t Version = static_cast<uint32_t>(VersionNumber);
		if (Version > CurrentPresetSchemaVersion)
		{
			AddIssue(Issues, ESeverity::Error, "preset.newerSchemaVersion", "schemaVersion", {FormatUInt(Version), FormatUInt(CurrentPresetSchemaVersion)});
			return false;
		}
		JsonValue Json = InJson;
		if (Version < CurrentPresetSchemaVersion)
		{
			PresetDetail::MigratePresetJson(Json, Version);
		}

		PresetDetail::ObjectReader Root(Json, "", Issues);
		uint32_t IgnoredVersion = 0;
		Root.UInt("schemaVersion", IgnoredVersion);
		OutConfig.SchemaVersion = CurrentPresetSchemaVersion;
		Root.String("name", OutConfig.Name);
		Root.String("description", OutConfig.Description);
		Root.Enum("mode", OutConfig.Mode, ShootModeNames);

		if (const JsonValue* VideoIO = Root.Object("videoIO"))
		{
			PresetDetail::ObjectReader Reader(*VideoIO, "videoIO", Issues);
			Reader.String("cardFamily", OutConfig.VideoIO.CardFamily);
			Reader.UInt("deviceIndex", OutConfig.VideoIO.DeviceIndex);
			Reader.UInt("inputPort", OutConfig.VideoIO.InputPort);
			Reader.UInt("outputPort", OutConfig.VideoIO.OutputPort);
			Reader.Enum("genlockSource", OutConfig.VideoIO.GenlockSource, GenlockSourceNames);
			Reader.ReportUnknownFields();
		}

		if (const JsonValue* Timing = Root.Object("timing"))
		{
			PresetDetail::ObjectReader Reader(*Timing, "timing", Issues);
			if (const JsonValue* Rate = Reader.Object("frameRate"))
			{
				PresetDetail::ObjectReader RateReader(*Rate, "timing.frameRate", Issues);
				RateReader.UInt("numerator", OutConfig.Timing.Rate.Numerator);
				RateReader.UInt("denominator", OutConfig.Timing.Rate.Denominator);
				RateReader.ReportUnknownFields();
			}
			if (const JsonValue* Res = Reader.Object("resolution"))
			{
				PresetDetail::ObjectReader ResReader(*Res, "timing.resolution", Issues);
				ResReader.UInt("width", OutConfig.Timing.Res.Width);
				ResReader.UInt("height", OutConfig.Timing.Res.Height);
				ResReader.ReportUnknownFields();
			}
			Reader.Bool("dropFrameTimecode", OutConfig.Timing.bDropFrameTimecode);
			Reader.ReportUnknownFields();
		}

		if (const JsonValue* VideoOut = Root.Object("videoOut"))
		{
			PresetDetail::ObjectReader Reader(*VideoOut, "videoOut", Issues);
			Reader.Bool("enabled", OutConfig.VideoOut.bEnabled);
			Reader.Enum("pixelFormat", OutConfig.VideoOut.PixelFormat, OutputPixelFormatNames);
			Reader.Bool("keyAndFill", OutConfig.VideoOut.bKeyAndFill);
			Reader.Enum("source", OutConfig.VideoOut.Source, OutputSourceNames);
			Reader.ReportUnknownFields();
		}

		if (const JsonValue* Green = Root.Object("greenScreen"))
		{
			PresetDetail::ObjectReader Reader(*Green, "greenScreen", Issues);
			Reader.Bool("createPlateSource", OutConfig.GreenScreen.bCreatePlateSource);
			Reader.Bool("createMatteOutput", OutConfig.GreenScreen.bCreateMatteOutput);
			Reader.UInt("matteOutputPort", OutConfig.GreenScreen.MatteOutputPort);
			Reader.ReportUnknownFields();
		}

		if (const JsonValue* Led = Root.Object("led"))
		{
			PresetDetail::ObjectReader Reader(*Led, "led", Issues);
			if (const JsonValue* Wall = Reader.Object("wall"))
			{
				PresetDetail::WallFromJson(*Wall, "led.wall", OutConfig.Led.Wall, Issues);
			}
			Reader.Bool("generateSwitchboardStub", OutConfig.Led.bGenerateSwitchboardStub);
			Reader.Bool("createDefaultLightCards", OutConfig.Led.bCreateDefaultLightCards);
			Reader.Bool("createChromakeyMarkers", OutConfig.Led.bCreateChromakeyMarkers);
			Reader.ReportUnknownFields();
		}

		if (const JsonValue* Tracking = Root.Object("tracking"))
		{
			PresetDetail::ObjectReader Reader(*Tracking, "tracking", Issues);
			Reader.String("provider", OutConfig.Tracking.ProviderId);
			Reader.String("subjectName", OutConfig.Tracking.SubjectName);
			Reader.Bool("autoApplyOnStartup", OutConfig.Tracking.bAutoApplyOnStartup);
			// Kept as-is: the provider owns and validates these keys.
			if (const JsonValue* Settings = Reader.Object("providerSettings"))
			{
				OutConfig.Tracking.ProviderSettings = *Settings;
			}
			Reader.ReportUnknownFields();
		}

		if (const JsonValue* Lens = Root.Object("lens"))
		{
			PresetDetail::ObjectReader Reader(*Lens, "lens", Issues);
			Reader.String("cameraSensor", OutConfig.Lens.CameraSensorId);
			Reader.Number("sensorWidthMm", OutConfig.Lens.SensorWidthMm);
			Reader.Number("sensorHeightMm", OutConfig.Lens.SensorHeightMm);
			Reader.ReportUnknownFields();
		}

		if (const JsonValue* Overrides = Root.Object("selectionOverrides"))
		{
			for (size_t Index = 0; Index < Overrides->Size(); ++Index)
			{
				const JsonValue& Value = Overrides->ValueAt(Index);
				if (Value.IsBool())
				{
					OutConfig.SelectionOverrides[Overrides->KeyAt(Index)] = Value.AsBool();
				}
				else
				{
					AddIssue(Issues, ESeverity::Error, "preset.wrongType", "selectionOverrides." + Overrides->KeyAt(Index), {"boolean"});
				}
			}
		}

		Root.ReportUnknownFields();
		return CountIssues(Issues, ESeverity::Error) == ErrorsBefore;
	}

	inline std::string SavePresetToString(const WizardConfig& Config)
	{
		return WriteJson(PresetToJson(Config));
	}

	inline bool LoadPresetFromString(const std::string& Text, WizardConfig& OutConfig, IssueList& Issues)
	{
		JsonValue Json;
		std::string ParseError;
		if (!ParseJson(Text, Json, ParseError))
		{
			AddIssue(Issues, ESeverity::Error, "preset.invalidJson", "", {ParseError});
			OutConfig = WizardConfig();
			return false;
		}
		return PresetFromJson(Json, OutConfig, Issues);
	}

	/**
	 * File-name stem for a preset name: lowercase ASCII, accents folded (including Welsh
	 * circumflex, acute, grave and diaeresis on w and y), everything else collapsed to '-'.
	 * "vedri LED 8x3m 25p" -> "vedri-led-8x3m-25p". Never empty, at most 64 characters.
	 */
	inline std::string MakePresetFileStem(const std::string& Name)
	{
		const auto Fold = [](uint32_t CodePoint) -> char
		{
			if ((CodePoint >= 'a' && CodePoint <= 'z') || (CodePoint >= '0' && CodePoint <= '9'))
			{
				return static_cast<char>(CodePoint);
			}
			if (CodePoint >= 'A' && CodePoint <= 'Z')
			{
				return static_cast<char>(CodePoint - 'A' + 'a');
			}
			// Latin-1 upper and lower ranges share a layout 0x20 apart.
			const uint32_t Lower = (CodePoint >= 0xC0 && CodePoint <= 0xDE && CodePoint != 0xD7) ? CodePoint + 0x20 : CodePoint;
			if (Lower >= 0xE0 && Lower <= 0xE5)
			{
				return 'a';
			}
			if (Lower == 0xE7)
			{
				return 'c';
			}
			if (Lower >= 0xE8 && Lower <= 0xEB)
			{
				return 'e';
			}
			if (Lower >= 0xEC && Lower <= 0xEF)
			{
				return 'i';
			}
			if (Lower == 0xF1)
			{
				return 'n';
			}
			if ((Lower >= 0xF2 && Lower <= 0xF6) || Lower == 0xF8)
			{
				return 'o';
			}
			if (Lower >= 0xF9 && Lower <= 0xFC)
			{
				return 'u';
			}
			if (Lower == 0xFD || Lower == 0xFF)
			{
				return 'y';
			}
			// Welsh: w/y with circumflex, grave, acute, diaeresis.
			if (CodePoint == 0x0174 || CodePoint == 0x0175 || (CodePoint >= 0x1E80 && CodePoint <= 0x1E85))
			{
				return 'w';
			}
			if (CodePoint == 0x0176 || CodePoint == 0x0177 || CodePoint == 0x0178 || CodePoint == 0x1EF2 || CodePoint == 0x1EF3)
			{
				return 'y';
			}
			return '-';
		};

		std::string Stem;
		size_t Pos = 0;
		while (Pos < Name.size())
		{
			uint32_t CodePoint = 0;
			if (!JsonDetail::DecodeUtf8(Name, Pos, CodePoint))
			{
				++Pos;
				CodePoint = '-';
			}
			const char Folded = Fold(CodePoint);
			if (Folded == '-' && (Stem.empty() || Stem.back() == '-'))
			{
				continue;
			}
			Stem.push_back(Folded);
		}
		if (Stem.size() > 64)
		{
			Stem.resize(64);
		}
		while (!Stem.empty() && Stem.back() == '-')
		{
			Stem.pop_back();
		}
		return Stem.empty() ? std::string("preset") : Stem;
	}
}
