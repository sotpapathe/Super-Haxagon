// SPDX-FileCopyrightText: 2025-2026 Sotiris Papatheodorou
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ControlsPSP.hpp"

#include <cstring>
#include <pspctrl.h>
#include <pspreg.h>

namespace SuperHaxagon {
	// Whether the cross button is used to select menu options. Otherwise
	// the circle button is used to select menu options, which is the case
	// for Japanese PSPs.
	static bool selectIsCross() {
		// Default to the non-Japanese convention on error.
		uint32_t value = 1;
		struct RegParam reg;
		reg.regtype = 1;
		std::strcpy(reg.name, SYSTEM_REGISTRY);
		reg.namelen = std::strlen(SYSTEM_REGISTRY);
		reg.unk2 = 1;
		reg.unk3 = 1;
		REGHANDLE hr;
		if (sceRegOpenRegistry(&reg, 1, &hr) == 0) {
			REGHANDLE hc;
			if (sceRegOpenCategory(hr, "/CONFIG/SYSTEM/XMB", 1, &hc) == 0) {
				sceRegGetKeyValueByName(hc, "button_assign", &value, sizeof value);
				sceRegCloseCategory(hc);
			}
			sceRegCloseRegistry(hr);
		}
		return value == 1;
	}

	// Cache the result of selectIsCross() as it won't change while the
	// game is running.
	static const bool crossSelects = selectIsCross();

	void initControls() {
		// Disable the analog stick.
		sceCtrlSetSamplingCycle(0);
		sceCtrlSetSamplingMode(PSP_CTRL_MODE_DIGITAL);
	}

	Buttons pressedButtons() {
		// PSP games typically don't have a dedicated quit button/menu
		// option and use the PlayStation button instead which brings
		// up a quit prompt from the OS.
		SceCtrlData pad;
		sceCtrlPeekBufferPositive(&pad, 1);
		Buttons buttons;
		buttons.select = pad.Buttons & (crossSelects ? PSP_CTRL_CROSS : PSP_CTRL_CIRCLE);
		buttons.back = pad.Buttons & (crossSelects ? PSP_CTRL_CIRCLE : PSP_CTRL_CROSS);
		buttons.left = pad.Buttons & PSP_CTRL_LTRIGGER || pad.Buttons & PSP_CTRL_LEFT;
		buttons.right = pad.Buttons & PSP_CTRL_RTRIGGER || pad.Buttons & PSP_CTRL_RIGHT;
		return buttons;
	}

	std::string buttonNameString(ButtonName buttonName) {
		// See pspRanges in FontPSP.cpp for the mapping between PSP
		// button glyphs and the ASCII control characters used in the
		// strings below.
		switch (buttonName) {
			case ButtonName::BACK: return crossSelects ? "\x04" : "\x05";
			case ButtonName::SELECT: return crossSelects ? "\x05" : "\x04";
			case ButtonName::LEFT: return "\x06  \x02";
			case ButtonName::RIGHT: return "\x07  \x03";
			case ButtonName::QUIT: return "\x01";
			default: return "?";
		}
	}
}
