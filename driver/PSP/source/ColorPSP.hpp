// SPDX-FileCopyrightText: 2025-2026 Sotiris Papatheodorou
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SUPER_HAXAGON_PSP_COLOR_PSP_HPP
#define SUPER_HAXAGON_PSP_COLOR_PSP_HPP

#include <Driver/Tools/Color.hpp>
#include <array>
#include <cstdint>

namespace SuperHaxagon {
	// Using 16 bit color doesn't offer any measurable performance
	// improvement, while complicating calling of certain functions like
	// sceGuColor().
	constexpr std::uint32_t packColor(const Color c) {
		// Due to the order of Color members and the fact that the
		// PSP's CPU is little endian, the layout of Color in memory is
		// exactly the same as the resulting uint32_t. However
		// casting Color to uint32_t is undefined behavior in C++ so we
		// have to use shifts and ors.
		return c.a << 24 | c.b << 16 | c.g << 8 | c.r;
	}

	// Return the color lookup table used by the 8-bit-per-pixel font
	// textures. It treats the font texture pixel value as the alpha
	// channel and combines it with a white color.
	constexpr std::array<std::uint32_t, 256> generateFontClut() {
		std::array<std::uint32_t, 256> clut {};
		for (size_t i = 0; i < clut.size(); i++) {
			clut[i] = packColor(Color{0xFF, 0xFF, 0xFF, static_cast<std::uint8_t>(i)});
		}
		return clut;
	}
}

#endif
