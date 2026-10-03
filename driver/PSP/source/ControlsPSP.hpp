// SPDX-FileCopyrightText: 2025-2026 Sotiris Papatheodorou
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SUPER_HAXAGON_PSP_CONTROLS_PSP_HPP
#define SUPER_HAXAGON_PSP_CONTROLS_PSP_HPP

#include "Driver/Platform.hpp"

namespace SuperHaxagon {
	void initControls();

	Buttons pressedButtons();

	std::string buttonNameString(ButtonName buttonName);
}

#endif
