// SPDX-FileCopyrightText: 2025 Sotiris Papatheodorou
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SUPER_HAXAGON_PSP_AUDIO_BUFFER_PSP_HPP
#define SUPER_HAXAGON_PSP_AUDIO_BUFFER_PSP_HPP

#include <string>
#include <vector>

#include "AudioFilePSP.hpp"

namespace SuperHaxagon {
	// Read an audio file into memory, storing the samples as 2 channel,
	// 16-bit PCM. The number of samples stored is always a multiple of
	// PSP_NUM_AUDIO_SAMPLES, with silent samples appended after the file
	// samples to reach the necessary size.
	struct AudioBuffer {
		AudioBuffer(const std::string& path);

		std::string path;
		std::vector<Sample> samples;
	};
}

#endif // SUPER_HAXAGON_PSP_AUDIO_BUFFER_PSP_HPP
