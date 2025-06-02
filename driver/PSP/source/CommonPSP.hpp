// SPDX-FileCopyrightText: 2025 Sotiris Papatheodorou
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SUPER_HAXAGON_PSP_COMMON_PSP_HPP
#define SUPER_HAXAGON_PSP_COMMON_PSP_HPP

#include <Driver/Tools/Color.hpp>
#include <cmath>
#include <string>
#include <vector>

namespace SuperHaxagon {
	// Compute the closest power of 2 greater or equal to x.
	// https://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
	constexpr int gePowerOf2(int x) {
		static_assert(sizeof(int) == sizeof(int32_t));
		x--;
		x |= x >> 1;
		x |= x >> 2;
		x |= x >> 4;
		x |= x >> 8;
		x |= x >> 16;
		x++;
		return x + (x == 0);
	}

	// Return a buffer with the contents of filename.
	std::vector<unsigned char> readFile(const std::string& filename);
}

#endif
