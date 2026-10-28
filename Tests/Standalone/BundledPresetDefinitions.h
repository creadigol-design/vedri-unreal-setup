// Copyright vedri studio Ltd. All Rights Reserved.

// Single source of truth for the two presets shipped in VedriVPToolkit/Resources/Presets.
// `make presets` writes the JSON files from these definitions; TestPreset.cpp checks the shipped
// files are byte-identical to what the serialiser produces, so they can never drift.

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

		WizardConfig Green = MakeDefaultConfig(EShootMode::GreenScreen);
		Green.Name = "vedr\xC3\xAD green screen 25p Blackmagic";
		Green.Description = "Green screen stage: Blackmagic DeckLink, 1080p25, genlock from SDI input, clean CG out for the external keyer, "
							"Mo-Sys StarTracker, Blackmagic PYXIS 6K.";
		Green.VideoIO.CardFamily = CardFamilyBlackmagic;
		Green.Timing.Rate = FrameRate{25, 1};
		Green.Timing.Res = Resolution{1920, 1080};
		Green.Tracking.Source = TrackingSourceMoSys;
		Green.Lens.CameraSensorId = "bmd_pyxis_6k";
		Green.Lens.SensorWidthMm = 36.0;
		Green.Lens.SensorHeightMm = 24.0;

		WizardConfig Led = MakeDefaultConfig(EShootMode::Led);
		Led.Name = "vedr\xC3\xAD LED 8x3m 25p Blackmagic";
		Led.Description = "LED stage: 8 m x 3 m flat wall, 2 render nodes, Blackmagic DeckLink, 1080p25, Mo-Sys StarTracker, "
						  "Blackmagic PYXIS 6K. Panel pitch is a placeholder: set it from your panel spec sheet.";
		Led.VideoIO.CardFamily = CardFamilyBlackmagic;
		Led.Timing.Rate = FrameRate{25, 1};
		Led.Timing.Res = Resolution{1920, 1080};
		Led.Led.Wall.WidthM = 8.0;
		Led.Led.Wall.HeightM = 3.0;
		Led.Led.Wall.RenderNodeCount = 2;
		Led.Tracking.Source = TrackingSourceMoSys;
		Led.Lens.CameraSensorId = "bmd_pyxis_6k";
		Led.Lens.SensorWidthMm = 36.0;
		Led.Lens.SensorHeightMm = 24.0;

		return {
			{"vedri-greenscreen-25p-blackmagic", Green},
			{"vedri-led-8x3m-25p-blackmagic", Led},
		};
	}
}
