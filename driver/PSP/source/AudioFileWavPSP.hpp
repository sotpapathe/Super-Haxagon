// SPDX-FileCopyrightText: 2025-2026 Sotiris Papatheodorou
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SUPER_HAXAGON_PSP_AUDIO_FILE_WAV_PSP_HPP
#define SUPER_HAXAGON_PSP_AUDIO_FILE_WAV_PSP_HPP

#include <pspiofilemgr.h>

#include "AudioFilePSP.hpp"

namespace SuperHaxagon {
	struct AudioFileWav : public AudioFile {
		AudioFileWav(const std::string& path);

		~AudioFileWav();

		long read(Sample* buffer, uint32_t bufferSamples) override;

		float getTime() const override;

		bool rewind() override;

		uint32_t numSamples() const override;

		uint32_t sampleRate() const override;

		const std::string& path() const override;

		private:
		SceUID _f = -1;
		std::string _path;
		uint32_t _numSamples = 0;
		uint32_t _sampleRate = 0;
		uint32_t _sampleIndex = 0;
	};
}

#endif // SUPER_HAXAGON_PSP_AUDIO_FILE_WAV_PSP_HPP
