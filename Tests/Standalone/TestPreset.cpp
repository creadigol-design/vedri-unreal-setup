// Copyright vedri studio Ltd. All Rights Reserved.

#include "TestHarness.h"

#include "BundledPresetDefinitions.h"
#include "Core/VPCorePreset.h"

#include <fstream>
#include <sstream>
#include <string>

using namespace Vedri::Core;

namespace
{
	bool ReadFile(const std::string& Path, std::string& OutText)
	{
		std::ifstream File(Path, std::ios::binary);
		if (!File)
		{
			return false;
		}
		std::ostringstream Buffer;
		Buffer << File.rdbuf();
		OutText = Buffer.str();
		return true;
	}

	size_t CountAtLeast(const IssueList& Issues, ESeverity Minimum)
	{
		size_t Count = 0;
		for (const ValidationIssue& Issue : Issues)
		{
			Count += Issue.Severity >= Minimum ? 1u : 0u;
		}
		return Count;
	}

	WizardConfig LoadExpectingSuccess(const std::string& Text, IssueList& Issues)
	{
		WizardConfig Config;
		if (!LoadPresetFromString(Text, Config, Issues))
		{
			VedriTest::ReportFailure(__FILE__, __LINE__, "preset failed to load: " + Text);
		}
		return Config;
	}

	WizardConfig MakeEverythingNonDefault()
	{
		WizardConfig Config = MakeDefaultConfig(EShootMode::Led);
		Config.Name = "Stiwdio \xC5\xB4yn \"test\" | 8\xC3\x97" "3";
		Config.Description = "Line one\nLine two";
		Config.VideoIO.CardFamily = CardFamilyAja;
		Config.VideoIO.DeviceIndex = 1;
		Config.VideoIO.InputPort = 3;
		Config.VideoIO.OutputPort = 4;
		Config.VideoIO.GenlockSource = EGenlockSource::ReferenceInput;
		Config.Timing.Rate = FrameRate{30000, 1001};
		Config.Timing.Res = Resolution{3840, 2160};
		Config.Timing.bDropFrameTimecode = true;
		Config.VideoOut.bEnabled = false;
		Config.VideoOut.PixelFormat = EOutputPixelFormat::Rgba8Bit;
		Config.VideoOut.bKeyAndFill = true;
		Config.VideoOut.Source = EOutputSource::TrackedCineCamera;
		Config.Led.Wall.WidthM = 12.5;
		Config.Led.Wall.HeightM = 4.0;
		Config.Led.Wall.Panels.PixelPitchMm = 1.95;
		Config.Led.Wall.Panels.PixelsPerTileX = 256;
		Config.Led.Wall.Panels.PixelsPerTileY = 256;
		Config.Led.Wall.RenderNodeCount = 4;
		Config.Led.Wall.CurvatureRadiusM = 9.0;
		Config.Led.Wall.BottomHeightM = 0.3;
		Config.Led.Wall.Ceiling.bEnabled = true;
		Config.Led.Wall.Ceiling.WidthM = 6.0;
		Config.Led.Wall.Ceiling.DepthM = 4.0;
		Config.Led.Wall.Ceiling.Panels.PixelPitchMm = 3.9;
		Config.Led.Wall.Ceiling.HeightM = 5.0;
		Config.Led.Wall.Ceiling.NodeIndex = 2;
		Config.Led.bGenerateSwitchboardStub = false;
		Config.Led.bCreateDefaultLightCards = false;
		Config.Led.bCreateChromakeyMarkers = false;
		Config.Tracking.ProviderId = TrackingProviderVicon;
		Config.Tracking.ProviderSettings.Set("port", 804);
		Config.Tracking.ProviderSettings.Set("server", "10.0.0.20");
		Config.Tracking.ProviderSettings.Set("nested", JsonValue::MakeObject()).Set("flag", true);
		Config.Tracking.SubjectName = "Camera A";
		Config.Tracking.bAutoApplyOnStartup = false;
		Config.Lens.CameraSensorId = CustomCameraSensorId;
		Config.Lens.SensorWidthMm = 24.89;
		Config.Lens.SensorHeightMm = 18.66;
		Config.SelectionOverrides["setting:/Script/Engine.Engine/bSmoothFrameRate"] = false;
		Config.SelectionOverrides["asset:/Game/VPToolkit/Config/MP_VPToolkit"] = true;
		return Config;
	}
}

VEDRI_TEST(Preset_DefaultsRoundTrip)
{
	for (const EShootMode Mode : {EShootMode::GreenScreen, EShootMode::Led})
	{
		const WizardConfig Original = MakeDefaultConfig(Mode);
		IssueList Issues;
		const WizardConfig Loaded = LoadExpectingSuccess(SavePresetToString(Original), Issues);
		EXPECT_TRUE(Loaded == Original);
		EXPECT_TRUE(Issues.empty());
	}
}

VEDRI_TEST(Preset_EveryFieldRoundTrips)
{
	const WizardConfig Original = MakeEverythingNonDefault();
	const std::string Text = SavePresetToString(Original);
	IssueList Issues;
	const WizardConfig Loaded = LoadExpectingSuccess(Text, Issues);
	EXPECT_TRUE(Loaded == Original);
	EXPECT_TRUE(Issues.empty());
	EXPECT_EQ(SavePresetToString(Loaded), Text);

	WizardConfig Green = MakeDefaultConfig(EShootMode::GreenScreen);
	Green.GreenScreen.bCreatePlateSource = false;
	Green.GreenScreen.bCreateMatteOutput = true;
	Green.GreenScreen.MatteOutputPort = 4;
	IssueList GreenIssues;
	EXPECT_TRUE(LoadExpectingSuccess(SavePresetToString(Green), GreenIssues) == Green);
}

VEDRI_TEST(Preset_OnlyActiveModeSectionIsWritten)
{
	const std::string Green = SavePresetToString(MakeDefaultConfig(EShootMode::GreenScreen));
	EXPECT_TRUE(Green.find("\"greenScreen\"") != std::string::npos);
	EXPECT_TRUE(Green.find("\"led\"") == std::string::npos);
	const std::string Led = SavePresetToString(MakeDefaultConfig(EShootMode::Led));
	EXPECT_TRUE(Led.find("\"led\"") != std::string::npos);
	EXPECT_TRUE(Led.find("\"greenScreen\"") == std::string::npos);
}

VEDRI_TEST(Preset_BundledFilesMatchDefinitionsAndValidate)
{
	const ValidationContext Context = ValidationContext::MakeDefault();
	for (const VedriTest::BundledPreset& Bundled : VedriTest::MakeBundledPresets())
	{
		const std::string Path = std::string(VEDRI_PRESET_DIR) + "/" + Bundled.FileStem + ".json";
		std::string Text;
		if (!ReadFile(Path, Text))
		{
			VedriTest::ReportFailure(__FILE__, __LINE__, "missing bundled preset " + Path + " (run `make presets`)");
			continue;
		}
		if (Text != SavePresetToString(Bundled.Config))
		{
			VedriTest::ReportFailure(__FILE__, __LINE__, Path + " differs from BundledPresetDefinitions.h (run `make presets`)");
		}
		IssueList Issues;
		const WizardConfig Loaded = LoadExpectingSuccess(Text, Issues);
		EXPECT_TRUE(Loaded == Bundled.Config);
		EXPECT_EQ(CountAtLeast(Issues, ESeverity::Warning), size_t(0));
		const IssueList Validation = ValidateConfig(Loaded, Context);
		if (CountAtLeast(Validation, ESeverity::Warning) != 0)
		{
			for (const ValidationIssue& Issue : Validation)
			{
				std::fprintf(stderr, "  %s: %s\n", Issue.Field.c_str(), Issue.Code.c_str());
			}
			VedriTest::ReportFailure(__FILE__, __LINE__, Path + " has validation warnings or errors");
		}
	}
}

VEDRI_TEST(Preset_BundledPresetsDescribeVedriStudio)
{
	const std::vector<VedriTest::BundledPreset> Bundled = VedriTest::MakeBundledPresets();
	ASSERT_TRUE(Bundled.size() == 4);
	EXPECT_EQ(std::string(Bundled[0].FileStem), "vedri-greenscreen-25p-blackmagic-mosys");
	EXPECT_EQ(std::string(Bundled[1].FileStem), "vedri-led-8x3m-25p-blackmagic-mosys");
	EXPECT_EQ(std::string(Bundled[2].FileStem), "greenscreen-freed");
	EXPECT_EQ(std::string(Bundled[3].FileStem), "led-freed");
	for (const VedriTest::BundledPreset& Preset : Bundled)
	{
		EXPECT_TRUE(Preset.Config.Timing.Rate == (FrameRate{25, 1}));
		EXPECT_TRUE(Preset.Config.Timing.Res == (Resolution{1920, 1080}));
		EXPECT_EQ(Preset.Config.VideoIO.CardFamily, "blackmagic");
		EXPECT_EQ(Preset.Config.Tracking.ProviderSettings.Size(), size_t(0)); // providers use their documented defaults
	}
	const WizardConfig& Green = Bundled[0].Config;
	EXPECT_EQ(Green.Mode, EShootMode::GreenScreen);
	EXPECT_EQ(Green.Tracking.ProviderId, "mosys");
	EXPECT_EQ(Green.Lens.CameraSensorId, "bmd_pyxis_6k");
	const WizardConfig& Led = Bundled[1].Config;
	EXPECT_EQ(Led.Mode, EShootMode::Led);
	EXPECT_EQ(Led.Tracking.ProviderId, "mosys");
	EXPECT_EQ(Led.Led.Wall.WidthM, 8.0);
	EXPECT_EQ(Led.Led.Wall.HeightM, 3.0);
	EXPECT_EQ(Led.Led.Wall.RenderNodeCount, 2u);
	EXPECT_EQ(Bundled[2].Config.Mode, EShootMode::GreenScreen);
	EXPECT_EQ(Bundled[2].Config.Tracking.ProviderId, "freed");
	EXPECT_EQ(Bundled[3].Config.Mode, EShootMode::Led);
	EXPECT_EQ(Bundled[3].Config.Tracking.ProviderId, "freed");
}

VEDRI_TEST(Preset_MissingFieldsKeepDefaults)
{
	IssueList Issues;
	const WizardConfig Loaded = LoadExpectingSuccess(R"({"schemaVersion": 1, "mode": "led", "timing": {"frameRate": {"numerator": 50}}})", Issues);
	EXPECT_TRUE(Issues.empty());
	EXPECT_EQ(Loaded.Mode, EShootMode::Led);
	EXPECT_TRUE(Loaded.Timing.Rate == (FrameRate{50, 1}));
	EXPECT_TRUE(Loaded.Led == LedConfig());
	EXPECT_TRUE(Loaded.Tracking == TrackingConfig());
}

VEDRI_TEST(Preset_UnknownFieldWarnsButLoads)
{
	IssueList Issues;
	LoadExpectingSuccess(R"({"schemaVersion": 1, "timing": {"frameRat": {}}, "extra": true})", Issues);
	EXPECT_EQ(Issues.size(), size_t(2));
	const ValidationIssue* Nested = FindIssue(Issues, "preset.unknownField");
	ASSERT_TRUE(Nested != nullptr);
	EXPECT_EQ(Nested->Severity, ESeverity::Warning);
	EXPECT_EQ(Nested->Field, "timing.frameRat");
	EXPECT_EQ(Issues[1].Field, "extra");
}

VEDRI_TEST(Preset_WrongTypesAreErrors)
{
	const std::vector<std::pair<std::string, std::string>> Cases = {
		{R"({"schemaVersion": 1, "timing": {"resolution": {"width": "1920"}}})", "timing.resolution.width"},
		{R"({"schemaVersion": 1, "timing": {"resolution": {"width": 1920.5}}})", "timing.resolution.width"},
		{R"({"schemaVersion": 1, "videoIO": {"inputPort": -1}})", "videoIO.inputPort"},
		{R"({"schemaVersion": 1, "videoOut": {"enabled": "yes"}})", "videoOut.enabled"},
		{R"({"schemaVersion": 1, "tracking": []})", "tracking"},
		{R"({"schemaVersion": 1, "tracking": {"providerSettings": 40000}})", "tracking.providerSettings"},
		{R"({"schemaVersion": 1, "mode": "led", "led": {"wall": {"ceiling": {"nodeIndex": 1.5}}}})", "led.wall.ceiling.nodeIndex"},
		{R"({"schemaVersion": 1, "selectionOverrides": {"plugin:LiveLink": 1}})", "selectionOverrides.plugin:LiveLink"},
	};
	for (const auto& [Text, Field] : Cases)
	{
		WizardConfig Config;
		IssueList Issues;
		EXPECT_FALSE(LoadPresetFromString(Text, Config, Issues));
		const ValidationIssue* Issue = FindIssue(Issues, "preset.wrongType");
		if (!Issue || Issue->Field != Field || Issue->Severity != ESeverity::Error)
		{
			VedriTest::ReportFailure(__FILE__, __LINE__, "expected preset.wrongType at " + Field + " for " + Text);
		}
	}
}

VEDRI_TEST(Preset_UnknownEnumValueIsError)
{
	WizardConfig Config;
	IssueList Issues;
	EXPECT_FALSE(LoadPresetFromString(R"({"schemaVersion": 1, "mode": "virtual"})", Config, Issues));
	const ValidationIssue* Issue = FindIssue(Issues, "preset.unknownEnumValue");
	ASSERT_TRUE(Issue != nullptr);
	EXPECT_EQ(Issue->Field, "mode");
	EXPECT_EQ(Issue->Args[0], "virtual");
}

VEDRI_TEST(Preset_SchemaVersionIsChecked)
{
	WizardConfig Config;
	IssueList Missing;
	EXPECT_FALSE(LoadPresetFromString(R"({"mode": "led"})", Config, Missing));
	EXPECT_TRUE(FindIssue(Missing, "preset.missingSchemaVersion") != nullptr);

	IssueList Zero;
	EXPECT_FALSE(LoadPresetFromString(R"({"schemaVersion": 0})", Config, Zero));
	EXPECT_TRUE(FindIssue(Zero, "preset.missingSchemaVersion") != nullptr);

	IssueList Newer;
	EXPECT_FALSE(LoadPresetFromString(R"({"schemaVersion": 2})", Config, Newer));
	const ValidationIssue* Issue = FindIssue(Newer, "preset.newerSchemaVersion");
	ASSERT_TRUE(Issue != nullptr);
	EXPECT_TRUE(Issue->Args == (std::vector<std::string>{"2", "1"}));

	IssueList NotObject;
	EXPECT_FALSE(LoadPresetFromString("[1]", Config, NotObject));
	EXPECT_TRUE(FindIssue(NotObject, "preset.notAnObject") != nullptr);
}

VEDRI_TEST(Preset_InvalidJsonIsReported)
{
	WizardConfig Config;
	IssueList Issues;
	EXPECT_FALSE(LoadPresetFromString("{\"schemaVersion\": 1,", Config, Issues));
	const ValidationIssue* Issue = FindIssue(Issues, "preset.invalidJson");
	ASSERT_TRUE(Issue != nullptr);
	EXPECT_EQ(Issue->Args[0].rfind("line 1", 0), size_t(0));
}

VEDRI_TEST(Preset_FileStem)
{
	EXPECT_EQ(MakePresetFileStem("vedr\xC3\xAD LED 8\xC3\x97" "3m 25p Blackmagic"), "vedri-led-8-3m-25p-blackmagic");
	EXPECT_EQ(MakePresetFileStem("vedri LED 8x3m 25p"), "vedri-led-8x3m-25p");
	EXPECT_EQ(MakePresetFileStem("Stiwdio \xC5\xB4yn a \xC5\xB7 ac \xE1\xBA\x83 \xE1\xBB\xB3"), "stiwdio-wyn-a-y-ac-w-y");
	EXPECT_EQ(MakePresetFileStem("\xC3\x89" "CLAIR \xC3\x85" "se \xC3\xB8l"), "eclair-ase-ol");
	EXPECT_EQ(MakePresetFileStem("  --Our wall!!  "), "our-wall");
	EXPECT_EQ(MakePresetFileStem(""), "preset");
	EXPECT_EQ(MakePresetFileStem("!!!"), "preset");
	EXPECT_EQ(MakePresetFileStem("\xFF\xFE"), "preset");
	EXPECT_EQ(MakePresetFileStem(std::string(100, 'a')).size(), size_t(64));
}

VEDRI_TEST(Validate_DefaultsAreClean)
{
	const ValidationContext Context = ValidationContext::MakeDefault();
	for (const EShootMode Mode : {EShootMode::GreenScreen, EShootMode::Led})
	{
		EXPECT_EQ(CountAtLeast(ValidateConfig(MakeDefaultConfig(Mode), Context), ESeverity::Warning), size_t(0));
	}
	const IssueList Issues = ValidateConfig(MakeDefaultConfig(EShootMode::GreenScreen), Context);
	const ValidationIssue* Link = FindIssue(Issues, "timing.sdiLinkRequired");
	ASSERT_TRUE(Link != nullptr);
	EXPECT_EQ(Link->Severity, ESeverity::Info);
	EXPECT_EQ(Link->Args[0], "1.5G");
}

VEDRI_TEST(Validate_CatchesBadChoices)
{
	const ValidationContext Context = ValidationContext::MakeDefault();
	const auto Has = [&Context](const WizardConfig& Config, const char* Code, const char* Field, ESeverity Severity)
	{
		const IssueList Issues = ValidateConfig(Config, Context);
		for (const ValidationIssue& Issue : Issues)
		{
			if (Issue.Code == Code && Issue.Field == Field && Issue.Severity == Severity)
			{
				return true;
			}
		}
		return false;
	};

	WizardConfig Config = MakeDefaultConfig(EShootMode::GreenScreen);
	Config.VideoIO.OutputPort = Config.VideoIO.InputPort;
	EXPECT_TRUE(Has(Config, "videoIO.inputOutputSamePort", "videoIO.outputPort", ESeverity::Error));
	Config.VideoOut.bEnabled = false; // no output, no clash
	EXPECT_FALSE(Has(Config, "videoIO.inputOutputSamePort", "videoIO.outputPort", ESeverity::Error));

	Config = MakeDefaultConfig(EShootMode::GreenScreen);
	Config.VideoIO.CardFamily = "deltacast";
	EXPECT_TRUE(Has(Config, "videoIO.unknownCardFamily", "videoIO.cardFamily", ESeverity::Error));
	ValidationContext WithThirdFamily = Context;
	WithThirdFamily.KnownCardFamilies.push_back("deltacast");
	EXPECT_EQ(CountIssues(ValidateConfig(Config, WithThirdFamily), ESeverity::Error), size_t(0));

	Config = MakeDefaultConfig(EShootMode::GreenScreen);
	Config.Timing.bDropFrameTimecode = true;
	EXPECT_TRUE(Has(Config, "timing.dropFrameNotSupported", "timing.dropFrameTimecode", ESeverity::Error));
	Config.Timing.Rate = FrameRate{30000, 1001};
	EXPECT_FALSE(Has(Config, "timing.dropFrameNotSupported", "timing.dropFrameTimecode", ESeverity::Error));

	Config = MakeDefaultConfig(EShootMode::GreenScreen);
	Config.Timing.Rate = FrameRate{48, 1};
	EXPECT_TRUE(Has(Config, "timing.nonStandardFrameRate", "timing.frameRate", ESeverity::Warning));
	Config.Timing.Rate = FrameRate{0, 1};
	EXPECT_TRUE(Has(Config, "timing.invalidFrameRate", "timing.frameRate", ESeverity::Error));

	Config = MakeDefaultConfig(EShootMode::GreenScreen);
	Config.Timing.Res = Resolution{1921, 1080};
	EXPECT_TRUE(Has(Config, "timing.resolutionWidthOdd", "timing.resolution.width", ESeverity::Error));
	Config.Timing.Res = Resolution{2048, 1080};
	EXPECT_TRUE(Has(Config, "timing.customResolution", "timing.resolution", ESeverity::Info));
	Config.Timing.Res = Resolution{10000, 1080};
	EXPECT_TRUE(Has(Config, "timing.resolutionOutOfRange", "timing.resolution", ESeverity::Error));
	Config.Timing.Res = Resolution{7680, 4320};
	EXPECT_TRUE(Has(Config, "timing.sdiLinkRequired", "timing", ESeverity::Warning));

	Config = MakeDefaultConfig(EShootMode::GreenScreen);
	Config.GreenScreen.bCreateMatteOutput = true;
	Config.GreenScreen.MatteOutputPort = Config.VideoIO.OutputPort;
	EXPECT_TRUE(Has(Config, "greenScreen.mattePortInUse", "greenScreen.matteOutputPort", ESeverity::Error));

	Config = MakeDefaultConfig(EShootMode::Led);
	Config.Led.Wall.WidthM = 0.0;
	EXPECT_TRUE(Has(Config, "wall.dimensionOutOfRange", "led.wall.widthM", ESeverity::Error));
	Config.Led.Wall.WidthM = 8.2;
	EXPECT_TRUE(Has(Config, "wall.sizeNotWholeTiles", "led.wall.widthM", ESeverity::Warning));

	Config = MakeDefaultConfig(EShootMode::GreenScreen);
	Config.Tracking.ProviderId = "not-a-tracker";
	EXPECT_TRUE(Has(Config, "tracking.unknownProvider", "tracking.provider", ESeverity::Error));
	for (const std::string& Known : ValidationContext::MakeDefault().KnownTrackingSources)
	{
		Config.Tracking.ProviderId = Known;
		EXPECT_FALSE(Has(Config, "tracking.unknownProvider", "tracking.provider", ESeverity::Error));
	}
	EXPECT_EQ(ValidationContext::MakeDefault().KnownTrackingSources.size(), size_t(11));
	Config.Tracking = TrackingConfig();
	Config.Tracking.ProviderSettings = JsonValue(40000);
	EXPECT_TRUE(Has(Config, "tracking.providerSettingsNotObject", "tracking.providerSettings", ESeverity::Error));
	Config.Tracking = TrackingConfig();
	Config.Tracking.SubjectName.clear();
	EXPECT_TRUE(Has(Config, "tracking.subjectNameEmpty", "tracking.subjectName", ESeverity::Error));

	Config = MakeDefaultConfig(EShootMode::GreenScreen);
	Config.Lens.SensorWidthMm = 27.03;
	EXPECT_TRUE(Has(Config, "lens.sensorDiffersFromPreset", "lens", ESeverity::Warning));
	Config.Lens.CameraSensorId = CustomCameraSensorId;
	EXPECT_EQ(CountAtLeast(ValidateConfig(Config, Context), ESeverity::Warning), size_t(0));
	Config.Lens.SensorHeightMm = 0.0;
	EXPECT_TRUE(Has(Config, "lens.sensorSizeOutOfRange", "lens.sensorHeightMm", ESeverity::Error));
	Config.Lens = LensConfig();
	Config.Lens.CameraSensorId = "red_v_raptor";
	EXPECT_TRUE(Has(Config, "lens.unknownCameraPreset", "lens.cameraSensor", ESeverity::Warning));

	Config = MakeDefaultConfig(EShootMode::GreenScreen);
	Config.SelectionOverrides["nDisplay"] = false;
	EXPECT_TRUE(Has(Config, "preset.unknownSelectionKey", "selectionOverrides", ESeverity::Warning));
}

VEDRI_TEST(Validate_SensorPresetsAreSane)
{
	for (const CameraSensorPreset& Preset : GetCameraSensorPresets())
	{
		EXPECT_TRUE(Preset.SensorWidthMm > Preset.SensorHeightMm);
		EXPECT_TRUE(Preset.SensorWidthMm > 10.0 && Preset.SensorWidthMm < 60.0);
		EXPECT_FALSE(std::string(Preset.Id) == CustomCameraSensorId);
	}
	const CameraSensorPreset* Pyxis = FindCameraSensorPreset(GetDefaultCameraSensorId());
	ASSERT_TRUE(Pyxis != nullptr);
	EXPECT_EQ(Pyxis->SensorWidthMm, 36.0);
	EXPECT_EQ(Pyxis->SensorHeightMm, 24.0);
	EXPECT_TRUE(FindCameraSensorPreset(CustomCameraSensorId) == nullptr);
}
