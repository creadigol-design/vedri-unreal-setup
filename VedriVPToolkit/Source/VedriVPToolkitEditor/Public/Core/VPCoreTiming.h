// Copyright vedri studio Ltd. All Rights Reserved.

#pragma once

// Frame rates, video resolutions and the SDI link-class hint. Engine-independent: the editor layer
// converts FrameRate to the engine's frame-rate type.

#include "Core/VPCoreValidation.h"

#include <cstdint>
#include <string>
#include <vector>

namespace Vedri::Core
{
	/** Exact rational frame rate, e.g. 25/1 or 24000/1001 (23.976). */
	struct FrameRate
	{
		uint32_t Numerator = 25;
		uint32_t Denominator = 1;

		bool IsValid() const { return Numerator > 0 && Denominator > 0; }

		double AsDecimal() const { return IsValid() ? static_cast<double>(Numerator) / static_cast<double>(Denominator) : 0.0; }

		/** Rational equality: 50/2 == 25/1. */
		bool IsEquivalentTo(const FrameRate& Other) const
		{
			return static_cast<uint64_t>(Numerator) * Other.Denominator == static_cast<uint64_t>(Other.Numerator) * Denominator;
		}

		/** Exact field equality (what a preset stores). */
		bool operator==(const FrameRate& Other) const = default;

		/** Human label without "fps": "25", "23.976", "29.97", "59.94". Not localised (numerals only). */
		std::string ToLabel() const { return FormatNumber(AsDecimal(), 3); }
	};

	/** The rates offered in the wizard, UK broadcast default first. */
	inline const std::vector<FrameRate>& GetFrameRatePresets()
	{
		static const std::vector<FrameRate> Presets = {
			{25, 1},
			{50, 1},
			{24, 1},
			{24000, 1001},
			{30000, 1001},
			{30, 1},
			{60000, 1001},
			{60, 1},
		};
		return Presets;
	}

	inline FrameRate GetDefaultFrameRate()
	{
		return GetFrameRatePresets().front();
	}

	inline bool IsPresetFrameRate(const FrameRate& Rate)
	{
		for (const FrameRate& Preset : GetFrameRatePresets())
		{
			if (Preset.IsEquivalentTo(Rate))
			{
				return true;
			}
		}
		return false;
	}

	/** SMPTE drop-frame timecode only exists for the NTSC 29.97 and 59.94 rates. */
	inline bool SupportsDropFrameTimecode(const FrameRate& Rate)
	{
		return Rate.IsEquivalentTo(FrameRate{30000, 1001}) || Rate.IsEquivalentTo(FrameRate{60000, 1001});
	}

	struct Resolution
	{
		uint32_t Width = 1920;
		uint32_t Height = 1080;

		bool operator==(const Resolution& Other) const = default;
	};

	inline const std::vector<Resolution>& GetResolutionPresets()
	{
		static const std::vector<Resolution> Presets = {
			{1920, 1080},
			{3840, 2160},
		};
		return Presets;
	}

	inline bool IsPresetResolution(const Resolution& Res)
	{
		for (const Resolution& Preset : GetResolutionPresets())
		{
			if (Preset == Res)
			{
				return true;
			}
		}
		return false;
	}

	/** Largest single-link SDI standard needed to carry a format. */
	enum class ESdiLinkClass : uint8_t
	{
		HD_1_5G,
		SDI_3G,
		SDI_6G,
		SDI_12G,
		BeyondSingleLink
	};

	inline const char* SdiLinkClassId(ESdiLinkClass Link)
	{
		switch (Link)
		{
		case ESdiLinkClass::HD_1_5G:
			return "1.5G";
		case ESdiLinkClass::SDI_3G:
			return "3G";
		case ESdiLinkClass::SDI_6G:
			return "6G";
		case ESdiLinkClass::SDI_12G:
			return "12G";
		case ESdiLinkClass::BeyondSingleLink:
			return "beyond12G";
		}
		return "beyond12G";
	}

	/**
	 * Approximate single-link SDI class from active pixel rate, scaled from the anchors
	 * 1080p30 = 1.5G, 1080p60 = 3G, 2160p30 = 6G, 2160p60 = 12G. A 10% margin keeps DCI widths
	 * (2048 / 4096) in the same class as their 1920 / 3840 counterparts.
	 * This is a hint for the UI only; the card's own format list is authoritative.
	 */
	inline ESdiLinkClass ClassifySdiLink(const Resolution& Res, const FrameRate& Rate)
	{
		const double PixelRate = static_cast<double>(Res.Width) * static_cast<double>(Res.Height) * Rate.AsDecimal();
		const double HdAnchor = 1920.0 * 1080.0 * 30.0 * 1.1;
		if (PixelRate <= HdAnchor)
		{
			return ESdiLinkClass::HD_1_5G;
		}
		if (PixelRate <= HdAnchor * 2.0)
		{
			return ESdiLinkClass::SDI_3G;
		}
		if (PixelRate <= HdAnchor * 4.0)
		{
			return ESdiLinkClass::SDI_6G;
		}
		if (PixelRate <= HdAnchor * 8.0)
		{
			return ESdiLinkClass::SDI_12G;
		}
		return ESdiLinkClass::BeyondSingleLink;
	}
}
