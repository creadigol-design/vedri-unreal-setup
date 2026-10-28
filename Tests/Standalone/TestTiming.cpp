// Copyright vedri studio Ltd. All Rights Reserved.

#include "TestHarness.h"

#include "Core/VPCoreTiming.h"

using namespace Vedri::Core;

VEDRI_TEST(Timing_LabelsAreBroadcastNames)
{
	EXPECT_EQ((FrameRate{25, 1}).ToLabel(), "25");
	EXPECT_EQ((FrameRate{50, 1}).ToLabel(), "50");
	EXPECT_EQ((FrameRate{24000, 1001}).ToLabel(), "23.976");
	EXPECT_EQ((FrameRate{30000, 1001}).ToLabel(), "29.97");
	EXPECT_EQ((FrameRate{60000, 1001}).ToLabel(), "59.94");
}

VEDRI_TEST(Timing_PresetsStartWithUkDefault)
{
	const std::vector<FrameRate>& Presets = GetFrameRatePresets();
	EXPECT_EQ(Presets.size(), size_t(8));
	EXPECT_TRUE(GetDefaultFrameRate() == (FrameRate{25, 1}));
	for (const FrameRate& Rate : {FrameRate{25, 1}, FrameRate{50, 1}, FrameRate{24, 1}, FrameRate{24000, 1001}, FrameRate{30000, 1001},
			 FrameRate{30, 1}, FrameRate{60000, 1001}, FrameRate{60, 1}})
	{
		EXPECT_TRUE(IsPresetFrameRate(Rate));
	}
	EXPECT_FALSE(IsPresetFrameRate(FrameRate{48, 1}));
	EXPECT_TRUE(GetResolutionPresets().front() == (Resolution{1920, 1080}));
	EXPECT_TRUE(IsPresetResolution(Resolution{3840, 2160}));
	EXPECT_FALSE(IsPresetResolution(Resolution{2048, 1080}));
}

VEDRI_TEST(Timing_RationalEquivalence)
{
	EXPECT_TRUE((FrameRate{50, 2}).IsEquivalentTo(FrameRate{25, 1}));
	EXPECT_FALSE((FrameRate{50, 2}) == (FrameRate{25, 1}));
	EXPECT_FALSE((FrameRate{24000, 1001}).IsEquivalentTo(FrameRate{24, 1}));
	EXPECT_TRUE(IsPresetFrameRate(FrameRate{50, 2}));
	EXPECT_FALSE((FrameRate{25, 0}).IsValid());
	EXPECT_EQ((FrameRate{25, 0}).AsDecimal(), 0.0);
}

VEDRI_TEST(Timing_DropFrameOnlyForNtscThirtyAndSixty)
{
	EXPECT_TRUE(SupportsDropFrameTimecode(FrameRate{30000, 1001}));
	EXPECT_TRUE(SupportsDropFrameTimecode(FrameRate{60000, 1001}));
	EXPECT_FALSE(SupportsDropFrameTimecode(FrameRate{24000, 1001}));
	EXPECT_FALSE(SupportsDropFrameTimecode(FrameRate{25, 1}));
	EXPECT_FALSE(SupportsDropFrameTimecode(FrameRate{30, 1}));
}

VEDRI_TEST(Timing_SdiLinkClass)
{
	const Resolution Hd{1920, 1080};
	const Resolution Uhd{3840, 2160};
	EXPECT_EQ(ClassifySdiLink(Hd, FrameRate{25, 1}), ESdiLinkClass::HD_1_5G);
	EXPECT_EQ(ClassifySdiLink(Hd, FrameRate{30, 1}), ESdiLinkClass::HD_1_5G);
	EXPECT_EQ(ClassifySdiLink(Hd, FrameRate{30000, 1001}), ESdiLinkClass::HD_1_5G);
	EXPECT_EQ(ClassifySdiLink(Hd, FrameRate{50, 1}), ESdiLinkClass::SDI_3G);
	EXPECT_EQ(ClassifySdiLink(Hd, FrameRate{60, 1}), ESdiLinkClass::SDI_3G);
	EXPECT_EQ(ClassifySdiLink(Uhd, FrameRate{25, 1}), ESdiLinkClass::SDI_6G);
	EXPECT_EQ(ClassifySdiLink(Uhd, FrameRate{30, 1}), ESdiLinkClass::SDI_6G);
	EXPECT_EQ(ClassifySdiLink(Uhd, FrameRate{50, 1}), ESdiLinkClass::SDI_12G);
	EXPECT_EQ(ClassifySdiLink(Uhd, FrameRate{60, 1}), ESdiLinkClass::SDI_12G);
	EXPECT_EQ(ClassifySdiLink(Resolution{2048, 1080}, FrameRate{30, 1}), ESdiLinkClass::HD_1_5G);
	EXPECT_EQ(ClassifySdiLink(Resolution{4096, 2160}, FrameRate{30, 1}), ESdiLinkClass::SDI_6G);
	EXPECT_EQ(ClassifySdiLink(Resolution{7680, 4320}, FrameRate{25, 1}), ESdiLinkClass::BeyondSingleLink);
	EXPECT_EQ(std::string(SdiLinkClassId(ESdiLinkClass::SDI_12G)), "12G");
}
