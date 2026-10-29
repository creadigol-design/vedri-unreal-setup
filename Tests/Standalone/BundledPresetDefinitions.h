// Copyright vedri studio Ltd. All Rights Reserved.

// Single source of truth for the presets shipped in VedriVPToolkit/Resources/Presets.
// `make presets` writes the JSON files from these definitions; TestPreset.cpp checks the shipped
// files are byte-identical to what the serialiser produces, so they can never drift.
//
// Provider settings are left empty on purpose: each provider then uses its documented defaults from
// the installed engine or vendor plugin (no port numbers are baked into presets).

#pragma once

#include "Core/VPCoreWizardConfig.h"

#include <string>
#include <vector>

namespace VedriTest
{
	struct BundledPreset
	{
		const char* FileStem;
		Vedri::Core::WizardConfig Config;
	};

	inline std::vector<BundledPreset> MakeBundledPresets()
	{
		using namespace Vedri::Core;

		// vedri studio's own kit: Blackmagic DeckLink, 1080p25, Mo-Sys StarTracker, PYXIS 6K.
		const auto MakeVedri = [](EShootMode Mode)
		{
			WizardConfig Config = MakeDefaultConfig(Mode);
			Config.VideoIO.CardFamily = CardFamilyBlackmagic;
			Config.Timing.Rate = FrameRate{25, 1};
			Config.Timing.Res = Resolution{1920, 1080};
			Config.Tracking.ProviderId = TrackingProviderMoSys;
			Config.Lens.CameraSensorId = "bmd_pyxis_6k";
			Config.Lens.SensorWidthMm = 36.0;
			Config.Lens.SensorHeightMm = 24.0;
			return Config;
		};

		WizardConfig VedriGreen = MakeVedri(EShootMode::GreenScreen);
		VedriGreen.Name = "vedr\xC3\xAD green screen 25p Blackmagic Mo-Sys";
		VedriGreen.Description = "Green screen stage: Blackmagic DeckLink, 1080p25, genlock from SDI input, clean CG out for the external "
								 "keyer, Mo-Sys StarTracker (free Mo-Sys Live Link plugin), Blackmagic PYXIS 6K.";

		WizardConfig VedriLed = MakeVedri(EShootMode::Led);
		VedriLed.Name = "vedr\xC3\xAD LED 8x3m 25p Blackmagic Mo-Sys";
		VedriLed.Description = "LED stage: 8 m x 3 m flat wall, 2 render nodes, Blackmagic DeckLink, 1080p25, Mo-Sys StarTracker (free "
							   "Mo-Sys Live Link plugin), Blackmagic PYXIS 6K. Panel pitch is a placeholder: set it from your panel spec sheet.";
		VedriLed.Led.Wall.WidthM = 8.0;
		VedriLed.Led.Wall.HeightM = 3.0;
		VedriLed.Led.Wall.RenderNodeCount = 2;

		// Generic starting points for any studio: built-in FreeD tracking, which most tracking systems
		// can output. Everything else is the wizard default; change the card and camera in the wizard.
		WizardConfig GenericGreen = MakeDefaultConfig(EShootMode::GreenScreen);
		GenericGreen.Name = "Green screen, FreeD tracking";
		GenericGreen.Description = "Green screen stage with built-in FreeD tracking, 1080p25, clean CG out for an external keyer. "
								   "Choose your video card and camera in the wizard.";
		GenericGreen.Tracking.ProviderId = TrackingProviderFreeD;

		WizardConfig GenericLed = MakeDefaultConfig(EShootMode::Led);
		GenericLed.Name = "LED wall, FreeD tracking";
		GenericLed.Description = "LED stage with built-in FreeD tracking, 1080p25, 2 render nodes. Enter your wall size and panel "
								 "spec, video card and camera in the wizard.";
		GenericLed.Tracking.ProviderId = TrackingProviderFreeD;

		return {
			{"vedri-greenscreen-25p-blackmagic-mosys", VedriGreen},
			{"vedri-led-8x3m-25p-blackmagic-mosys", VedriLed},
			{"greenscreen-freed", GenericGreen},
			{"led-freed", GenericLed},
		};
	}
}
