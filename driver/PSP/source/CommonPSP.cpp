// SPDX-FileCopyrightText: 2025-2026 Sotiris Papatheodorou
// SPDX-License-Identifier: GPL-3.0-or-later

#include "CommonPSP.hpp"

#include <pspiofilemgr.h>

namespace SuperHaxagon {
	std::vector<unsigned char> readFile(const std::string& filename) {
		std::vector<unsigned char> buf;

		SceIoStat s;
		if (sceIoGetstat(filename.c_str(), &s) < 0) {
			return buf;
		}
		try {
			buf.resize(s.st_size);
		} catch (const std::exception& e) {
			return buf;
		}

		const SceUID fd = sceIoOpen(filename.c_str(), PSP_O_RDONLY, 0777);
		if (fd < 0) {
			buf.clear();
			return buf;
		}

		if (sceIoRead(fd, buf.data(), buf.size()) != buf.size()) {
			buf.clear();
		}
		sceIoClose(fd);
		return buf;
	}
}
