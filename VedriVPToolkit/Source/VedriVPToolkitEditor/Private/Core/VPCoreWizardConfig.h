// Copyright vedri studio Ltd. All Rights Reserved.

#pragma once

// The wizard's complete state: what the user chose on every step. This is exactly what a preset
// stores. Engine-specific meaning (which UClass a card family maps to, which plugin names a step
// enables) is resolved in the editor layer; Core only stores and validates the choices.

#include "Core/VPCoreCameraSensors.h"
#include "Core/VPCoreTiming.h"
#include "Core/VPCoreValidation.h"
#include "Core/VPCoreWallLayout.h"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace Vedri::Core
{
	inline constexpr uint32_t CurrentPresetSchemaVersion = 1;

	enum class EShootMode : uint8_t
	{
		GreenScreen,
		Led
	};

	enum class EGenlockSource : uint8_t
	{
		SdiInput,
		ReferenceInput
	};

	/** Output pixel format. The card provider maps these to its own enum and reports which it supports. */
	enum class EOutputPixelFormat : uint8_t
	{
		Yuv10Bit,
		Yuv8Bit,
		Rgb10Bit,
		Rgba8Bit
	};

	enum class EOutputSource : uint8_t
	{
		ActiveViewport,
		TrackedCineCamera
	};

	/** Stable string ids for enums, shared by the preset format and the editor. */
	template <typename EnumType>
	struct EnumName
	{
		EnumType Value;
		const char* Name;
	};

	inline constexpr EnumName<EShootMode> ShootModeNames[] = {
		{EShootMode::GreenScreen, "greenScreen"},
		{EShootMode::Led, "led"},
	};

	inline constexpr EnumName<EGenlockSource> GenlockSourceNames[] = {
		{EGenlockSource::SdiInput, "sdiInput"},
		{EGenlockSource::ReferenceInput, "referenceInput"},
	};

	inline constexpr EnumName<EOutputPixelFormat> OutputPixelFormatNames[] = {
		{EOutputPixelFormat::Yuv10Bit, "yuv10"},
		{EOutputPixelFormat::Yuv8Bit, "yuv8"},
		{EOutputPixelFormat::Rgb10Bit, "rgb10"},
		{EOutputPixelFormat::Rgba8Bit, "rgba8"},
	};

	inline constexpr EnumName<EOutputSource> OutputSourceNames[] = {
		{EOutputSource::ActiveViewport, "activeViewport"},
		{EOutputSource::TrackedCineCamera, "trackedCineCamera"},
	};

	template <typename EnumType, size_t Count>
	const char* EnumToString(EnumType Value, const EnumName<EnumType> (&Table)[Count])
	{
		for (const EnumName<EnumType>& Entry : Table)
		{
			if (Entry.Value == Value)
			{
				return Entry.Name;
			}
		}
		return "";
	}

	template <typename EnumType, size_t Count>
	bool EnumFromString(const std::string& Name, EnumType& OutValue, const EnumName<EnumType> (&Table)[Count])
	{
		for (const EnumName<EnumType>& Entry : Table)
		{
			if (Name == Entry.Name)
			{
				OutValue = Entry.Value;
				return true;
			}
		}
		return false;
	}

	/** Card family ids ("blackmagic", "aja") are open-ended: each id belongs to one IVPVideoIOProvider. */
	inline constexpr const char* CardFamilyBlackmagic = "blackmagic";
	inline constexpr const char* CardFamilyAja = "aja";

	/** Tracking source ids. "mosys" uses the Mo-Sys Live Link plugin when present, else FreeD. */
	inline constexpr const char* TrackingSourceMoSys = "mosys";
	inline constexpr const char* TrackingSourceFreeD = "freed";

	struct VideoIOConfig
	{
		std::string CardFamily = CardFamilyBlackmagic;
		/** Which card, when more than one of the family is installed. */
		uint32_t DeviceIndex = 0;
		/** 1-based connector numbers as printed on the card bracket. */
		uint32_t InputPort = 1;
		uint32_t OutputPort = 2;
		EGenlockSource GenlockSource = EGenlockSource::SdiInput;

		bool operator==(const VideoIOConfig& Other) const = default;
	};

	struct TimingConfig
	{
		FrameRate Rate = GetDefaultFrameRate();
		Resolution Res;
		bool bDropFrameTimecode = false;

		bool operator==(const TimingConfig& Other) const = default;
	};

	struct VideoOutConfig
	{
		bool bEnabled = true;
		EOutputPixelFormat PixelFormat = EOutputPixelFormat::Yuv10Bit;
		bool bKeyAndFill = false;
		EOutputSource Source = EOutputSource::ActiveViewport;

		bool operator==(const VideoOutConfig& Other) const = default;
	};

	/** Green screen: Unreal outputs clean CG; keying happens downstream on the external keyer/switcher. */
	struct GreenScreenConfig
	{
		bool bCreatePlateSource = true;
		bool bCreateMatteOutput = false;
		uint32_t MatteOutputPort = 3;

		bool operator==(const GreenScreenConfig& Other) const = default;
	};

	struct LedConfig
	{
		WallSpec Wall;
		bool bGenerateSwitchboardStub = true;
		bool bCreateDefaultLightCards = true;
		bool bCreateChromakeyMarkers = true;

		bool operator==(const LedConfig& Other) const = default;
	};

	struct TrackingConfig
	{
		std::string Source = TrackingSourceMoSys;
		/** TODO(verify-on-pc): match the engine's FreeD source default port and the StarTracker's FreeD output port. */
		uint32_t FreeDUdpPort = 40000;
		std::string SubjectName = "TrackedCamera";
		bool bAutoApplyOnStartup = true;

		bool operator==(const TrackingConfig& Other) const = default;
	};

	struct LensConfig
	{
		std::string CameraSensorId = GetDefaultCameraSensorId();
		double SensorWidthMm = 36.0;
		double SensorHeightMm = 24.0;

		bool operator==(const LensConfig& Other) const = default;
	};

	struct WizardConfig
	{
		uint32_t SchemaVersion = CurrentPresetSchemaVersion;
		std::string Name;
		std::string Description;
		EShootMode Mode = EShootMode::GreenScreen;
		VideoIOConfig VideoIO;
		TimingConfig Timing;
		VideoOutConfig VideoOut;
		GreenScreenConfig GreenScreen;
		LedConfig Led;
		TrackingConfig Tracking;
		LensConfig Lens;
		/**
		 * Per-item ticks on the Summary step that differ from the default, keyed by change key
		 * (see VPCoreChangeSet.h). true = tick an item that is unticked by default, false = untick.
		 */
		std::map<std::string, bool> SelectionOverrides;

		bool operator==(const WizardConfig& Other) const = default;
	};

	inline WizardConfig MakeDefaultConfig(EShootMode Mode)
	{
		WizardConfig Config;
		Config.Mode = Mode;
		return Config;
	}

	/** What the editor has registered; validation of open-ended ids checks against it. */
	struct ValidationContext
	{
		std::vector<std::string> KnownCardFamilies;
		std::vector<std::string> KnownTrackingSources;

		static ValidationContext MakeDefault()
		{
			ValidationContext Context;
			Context.KnownCardFamilies = {CardFamilyBlackmagic, CardFamilyAja};
			Context.KnownTrackingSources = {TrackingSourceMoSys, TrackingSourceFreeD};
			return Context;
		}
	};

	namespace ConfigDetail
	{
		inline bool Contains(const std::vector<std::string>& Values, const std::string& Value)
		{
			for (const std::string& Candidate : Values)
			{
				if (Candidate == Value)
				{
					return true;
				}
			}
			return false;
		}

		inline bool IsKnownChangeKeyPrefix(const std::string& Key)
		{
			for (const char* Prefix : {"plugin:", "setting:", "asset:", "file:"})
			{
				const size_t Length = std::char_traits<char>::length(Prefix);
				if (Key.size() > Length && Key.compare(0, Length, Prefix) == 0)
				{
					return true;
				}
			}
			return false;
		}
	}

	/** Semantic validation of a complete config. Field names are the preset JSON paths. */
	inline IssueList ValidateConfig(const WizardConfig& Config, const ValidationContext& Context)
	{
		IssueList Issues;

		// Video I/O
		if (!ConfigDetail::Contains(Context.KnownCardFamilies, Config.VideoIO.CardFamily))
		{
			AddIssue(Issues, ESeverity::Error, "videoIO.unknownCardFamily", "videoIO.cardFamily", {Config.VideoIO.CardFamily});
		}
		if (Config.VideoIO.DeviceIndex > 15)
		{
			AddIssue(Issues, ESeverity::Error, "videoIO.deviceIndexOutOfRange", "videoIO.deviceIndex", {FormatUInt(Config.VideoIO.DeviceIndex)});
		}
		const auto CheckPort = [&Issues](const char* Field, uint32_t Port)
		{
			if (Port < 1 || Port > 16)
			{
				AddIssue(Issues, ESeverity::Error, "videoIO.portOutOfRange", Field, {FormatUInt(Port)});
			}
		};
		CheckPort("videoIO.inputPort", Config.VideoIO.InputPort);
		CheckPort("videoIO.outputPort", Config.VideoIO.OutputPort);
		if (Config.VideoOut.bEnabled && Config.VideoIO.InputPort == Config.VideoIO.OutputPort)
		{
			AddIssue(Issues, ESeverity::Error, "videoIO.inputOutputSamePort", "videoIO.outputPort", {FormatUInt(Config.VideoIO.OutputPort)});
		}

		// Timing
		const FrameRate& Rate = Config.Timing.Rate;
		if (!Rate.IsValid() || Rate.AsDecimal() > 240.0)
		{
			AddIssue(Issues, ESeverity::Error, "timing.invalidFrameRate", "timing.frameRate", {FormatUInt(Rate.Numerator), FormatUInt(Rate.Denominator)});
		}
		else if (!IsPresetFrameRate(Rate))
		{
			AddIssue(Issues, ESeverity::Warning, "timing.nonStandardFrameRate", "timing.frameRate", {Rate.ToLabel()});
		}
		if (Config.Timing.bDropFrameTimecode && Rate.IsValid() && !SupportsDropFrameTimecode(Rate))
		{
			AddIssue(Issues, ESeverity::Error, "timing.dropFrameNotSupported", "timing.dropFrameTimecode", {Rate.ToLabel()});
		}
		const Resolution& Res = Config.Timing.Res;
		const bool bResolutionInRange = Res.Width >= 16 && Res.Height >= 16 && Res.Width <= 8192 && Res.Height <= 8192;
		if (!bResolutionInRange)
		{
			AddIssue(Issues, ESeverity::Error, "timing.resolutionOutOfRange", "timing.resolution", {FormatUInt(Res.Width), FormatUInt(Res.Height)});
		}
		else
		{
			// 4:2:2 chroma subsampling needs an even width.
			if (Res.Width % 2 != 0)
			{
				AddIssue(Issues, ESeverity::Error, "timing.resolutionWidthOdd", "timing.resolution.width", {FormatUInt(Res.Width)});
			}
			if (!IsPresetResolution(Res))
			{
				AddIssue(Issues, ESeverity::Info, "timing.customResolution", "timing.resolution", {FormatUInt(Res.Width), FormatUInt(Res.Height)});
			}
			if (Rate.IsValid())
			{
				const ESdiLinkClass Link = ClassifySdiLink(Res, Rate);
				AddIssue(Issues, Link == ESdiLinkClass::BeyondSingleLink ? ESeverity::Warning : ESeverity::Info, "timing.sdiLinkRequired", "timing",
					{SdiLinkClassId(Link), FormatUInt(Res.Width), FormatUInt(Res.Height), Rate.ToLabel()});
			}
		}

		// Mode specific
		if (Config.Mode == EShootMode::GreenScreen)
		{
			const GreenScreenConfig& Green = Config.GreenScreen;
			if (Green.bCreateMatteOutput)
			{
				if (Green.MatteOutputPort < 1 || Green.MatteOutputPort > 16)
				{
					AddIssue(Issues, ESeverity::Error, "videoIO.portOutOfRange", "greenScreen.matteOutputPort", {FormatUInt(Green.MatteOutputPort)});
				}
				else if (Green.MatteOutputPort == Config.VideoIO.InputPort || (Config.VideoOut.bEnabled && Green.MatteOutputPort == Config.VideoIO.OutputPort))
				{
					AddIssue(Issues, ESeverity::Error, "greenScreen.mattePortInUse", "greenScreen.matteOutputPort", {FormatUInt(Green.MatteOutputPort)});
				}
			}
		}
		else
		{
			const WallLayout Layout = ComputeWallLayout(Config.Led.Wall);
			AppendIssues(Issues, Layout.Issues, "led.wall");
		}

		// Tracking
		if (!ConfigDetail::Contains(Context.KnownTrackingSources, Config.Tracking.Source))
		{
			AddIssue(Issues, ESeverity::Error, "tracking.unknownSource", "tracking.source", {Config.Tracking.Source});
		}
		if (Config.Tracking.FreeDUdpPort < 1 || Config.Tracking.FreeDUdpPort > 65535)
		{
			AddIssue(Issues, ESeverity::Error, "tracking.udpPortOutOfRange", "tracking.freeDUdpPort", {FormatUInt(Config.Tracking.FreeDUdpPort)});
		}
		if (Config.Tracking.SubjectName.empty())
		{
			AddIssue(Issues, ESeverity::Error, "tracking.subjectNameEmpty", "tracking.subjectName");
		}

		// Lens
		const LensConfig& Lens = Config.Lens;
		if (Lens.CameraSensorId != CustomCameraSensorId)
		{
			const CameraSensorPreset* Preset = FindCameraSensorPreset(Lens.CameraSensorId);
			if (!Preset)
			{
				AddIssue(Issues, ESeverity::Warning, "lens.unknownCameraPreset", "lens.cameraSensor", {Lens.CameraSensorId});
			}
			else if (std::fabs(Preset->SensorWidthMm - Lens.SensorWidthMm) > 0.005 || std::fabs(Preset->SensorHeightMm - Lens.SensorHeightMm) > 0.005)
			{
				AddIssue(Issues, ESeverity::Warning, "lens.sensorDiffersFromPreset", "lens",
					{Preset->DisplayName, FormatNumber(Lens.SensorWidthMm, 2), FormatNumber(Lens.SensorHeightMm, 2)});
			}
		}
		if (!(Lens.SensorWidthMm > 0.0 && Lens.SensorWidthMm <= 100.0))
		{
			AddIssue(Issues, ESeverity::Error, "lens.sensorSizeOutOfRange", "lens.sensorWidthMm", {FormatNumber(Lens.SensorWidthMm, 2)});
		}
		if (!(Lens.SensorHeightMm > 0.0 && Lens.SensorHeightMm <= 100.0))
		{
			AddIssue(Issues, ESeverity::Error, "lens.sensorSizeOutOfRange", "lens.sensorHeightMm", {FormatNumber(Lens.SensorHeightMm, 2)});
		}

		// Summary-step overrides
		for (const auto& [Key, bSelected] : Config.SelectionOverrides)
		{
			(void)bSelected;
			if (!ConfigDetail::IsKnownChangeKeyPrefix(Key))
			{
				AddIssue(Issues, ESeverity::Warning, "preset.unknownSelectionKey", "selectionOverrides", {Key});
			}
		}
		return Issues;
	}
}
