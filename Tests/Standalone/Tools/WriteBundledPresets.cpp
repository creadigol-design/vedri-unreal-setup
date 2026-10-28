// Copyright vedri studio Ltd. All Rights Reserved.

// Regenerates VedriVPToolkit/Resources/Presets/*.json from BundledPresetDefinitions.h.
// Usage: WriteBundledPresets <output directory>

#include "BundledPresetDefinitions.h"
#include "Core/VPCorePreset.h"

#include <cstdio>
#include <fstream>
#include <string>

int main(int ArgCount, char** Args)
{
	if (ArgCount != 2)
	{
		std::fprintf(stderr, "usage: %s <output directory>\n", Args[0]);
		return 2;
	}
	const std::string Directory = Args[1];
	for (const VedriTest::BundledPreset& Preset : VedriTest::MakeBundledPresets())
	{
		const std::string Path = Directory + "/" + Preset.FileStem + ".json";
		std::ofstream File(Path, std::ios::binary | std::ios::trunc);
		File << Vedri::Core::SavePresetToString(Preset.Config);
		if (!File.good())
		{
			std::fprintf(stderr, "failed to write %s\n", Path.c_str());
			return 1;
		}
		std::printf("wrote %s\n", Path.c_str());
	}
	return 0;
}
