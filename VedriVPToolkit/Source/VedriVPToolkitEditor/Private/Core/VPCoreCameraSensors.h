// Copyright vedri studio Ltd. All Rights Reserved.

#pragma once

// Sensor-size presets for the tracked Cine Camera's filmback.
//
// Values are the full active sensor area from the manufacturers' published specs. Recording in a
// crop / windowed mode changes the effective filmback, which is why "custom" exists.
// TODO(confirm-spec): re-check each value against the current manufacturer spec sheet before release
// (listed in docs/WINDOWS_TODO.md, "Data to confirm").
//
// DisplayName is a product name, not translatable text; the editor shows it with
// FText::AsCultureInvariant.

#include <string>
#include <vector>

namespace Vedri::Core
{
	struct CameraSensorPreset
	{
		const char* Id;
		const char* DisplayName;
		double SensorWidthMm;
		double SensorHeightMm;
	};

	inline constexpr const char* CustomCameraSensorId = "custom";

	inline const std::vector<CameraSensorPreset>& GetCameraSensorPresets()
	{
		static const std::vector<CameraSensorPreset> Presets = {
			{"bmd_pyxis_6k", "Blackmagic PYXIS 6K", 36.0, 24.0},
			{"bmd_ursa_cine_12k_lf", "Blackmagic URSA Cine 12K LF", 35.64, 23.32},
			{"bmd_ursa_mini_pro_12k", "Blackmagic URSA Mini Pro 12K", 27.03, 14.25},
			{"bmd_ursa_mini_pro_4_6k_g2", "Blackmagic URSA Mini Pro 4.6K G2", 25.34, 14.25},
			{"bmd_pocket_6k_pro", "Blackmagic Pocket Cinema Camera 6K Pro", 23.10, 12.99},
			{"arri_alexa_35", "ARRI ALEXA 35", 27.99, 19.22},
			{"sony_venice_2_8k", "Sony VENICE 2 (8.6K)", 35.9, 24.0},
		};
		return Presets;
	}

	inline const char* GetDefaultCameraSensorId()
	{
		return "bmd_pyxis_6k";
	}

	/** nullptr for "custom" or unknown ids. */
	inline const CameraSensorPreset* FindCameraSensorPreset(const std::string& Id)
	{
		for (const CameraSensorPreset& Preset : GetCameraSensorPresets())
		{
			if (Id == Preset.Id)
			{
				return &Preset;
			}
		}
		return nullptr;
	}
}
