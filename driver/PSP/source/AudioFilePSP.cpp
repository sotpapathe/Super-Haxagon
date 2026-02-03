// SPDX-FileCopyrightText: 2025 Sotiris Papatheodorou
// SPDX-License-Identifier: GPL-3.0-or-later

#include "AudioFilePSP.hpp"

#include <pspaudiolib.h>
#include <stdlib.h>

#include "AudioFileVorbisPSP.hpp"
#include "AudioFileWavPSP.hpp"

namespace SuperHaxagon {
	uint32_t roundSamples(uint32_t samples) {
		const auto r = ldiv(samples, PSP_NUM_AUDIO_SAMPLES);
		return PSP_NUM_AUDIO_SAMPLES * (r.quot + (r.rem > 0));
	}

	std::unique_ptr<AudioFile> createAudioFile(const Platform& platform, const std::string& path) {
		auto vorbis = std::make_unique<AudioFileVorbis>(path + ".ogg");
		if (vorbis && vorbis->numSamples() > 0) {
			return vorbis;
		}
		auto wav = std::make_unique<AudioFileWav>(platform, path + ".wav");
		if (wav && wav->numSamples() > 0) {
			return wav;
		}
		return nullptr;
	}
}
