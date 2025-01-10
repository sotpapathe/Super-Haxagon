#ifndef SUPER_HAXAGON_COMMON_PSP_HPP
#define SUPER_HAXAGON_COMMON_PSP_HPP

#include "Core/Structs.hpp"

namespace SuperHaxagon {
	static inline uint32_t packColor(const Color color) {
		// The PSP CPU is little-endian and colors are packed in the
		// ABGR order.
		return (static_cast<uint32_t>(color.a) << 24) +
			(static_cast<uint32_t>(color.b) << 16) +
			(static_cast<uint32_t>(color.g) << 8) +
			static_cast<uint32_t>(color.r);
	}
}

#endif //SUPER_HAXAGON_COMMON_PSP_HPP
