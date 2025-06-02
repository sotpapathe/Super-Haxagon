// SPDX-FileCopyrightText: 2025 Sotiris Papatheodorou
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SUPER_HAXAGON_PSP_COLOR_PSP_HPP
#define SUPER_HAXAGON_PSP_COLOR_PSP_HPP

#include <array>
#include <cstdint>

namespace SuperHaxagon {
	// Change to uint16_t to use a 16-bit framebuffer. The following
	// constants and functions allow abstracting away the framebuffer bit
	// depth.
	typedef uint32_t ColorPSP;

	// The framebuffer colour format
	static constexpr unsigned PSP_FB_FMT = std::is_same_v<ColorPSP, uint16_t> ? GU_PSM_4444 : GU_PSM_8888;

	// The colour format used in draw commands.
	static constexpr unsigned PSP_GU_COLOR = std::is_same_v<ColorPSP, uint16_t> ? GU_COLOR_4444 : GU_COLOR_8888;

	// Return color packed into a 16-bit or 32-bit integer.
	template<typename ColorT>
	constexpr ColorT packColor(const Color color) {
		if constexpr (std::is_same_v<ColorT, uint16_t>) {
			return ((color.a >> 4) << 12)
				+ ((color.b >> 4) << 8)
				+ ((color.g >> 4) << 4)
				+ (color.r >> 4);
		} else {
			return (color.a << 24)
				+ (color.b << 16)
				+ (color.g << 8)
				+ color.r;
		}
	}

	constexpr ColorPSP packColorPSP(const Color color) {
		return packColor<ColorPSP>(color);
	}

	// Return the color lookup table used by the 8-bit-per-pixel font
	// textures. It treats the font texture pixel value as the alpha
	// channel and combines it with a white color.
	constexpr std::array<ColorPSP, 256> generateFontClut() {
		std::array<ColorPSP, 256> clut {};
		for (size_t i = 0; i < clut.size(); i++) {
			clut[i] = packColorPSP(Color{0xFF, 0xFF, 0xFF, static_cast<uint8_t>(i)});
		}
		return clut;
	}
}

#endif
