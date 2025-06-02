// SPDX-FileCopyrightText: 2025-2026 Sotiris Papatheodorou
// SPDX-License-Identifier: GPL-3.0-or-later

#include "AudioFileWavPSP.hpp"

#include <algorithm>
#include <string.h>

namespace SuperHaxagon {
	struct WavHeader {
		char riff[4]; // "RIFF"
		uint32_t chunkSize;
		char wave[4]; // "WAVE"
		char format[4]; // "fmt "
		uint32_t blockSize; // 16 for PCM
		uint16_t audioFormat; // 1 for PCM
		uint16_t channels;
		uint32_t sampleRate;
		uint32_t bytesPerSec;
		uint16_t bytesPerBlock;
		uint16_t bitsPerSample;
		char data[4]; // "data"
		uint32_t dataSize;

		bool valid() {
			return strncmp(riff, "RIFF", 4) == 0
				&& strncmp(wave, "WAVE", 4) == 0
				&& strncmp(format, "fmt ", 4) == 0
				&& channels > 0
				&& sampleRate > 0
				&& bytesPerSec > 0
				&& bytesPerBlock > 0
				&& bitsPerSample > 0
				&& strncmp(data, "data", 4) == 0;
		}

		bool supported() {
			return valid()
				&& blockSize == 16
				&& audioFormat == 1
				&& channels == 2
				&& bitsPerSample == 16;
		}
	};

	AudioFileWav::AudioFileWav(const std::string& path) : _path(path) {
		_f = sceIoOpen(_path.c_str(), PSP_O_RDONLY, 0777);
		if (_f < 0) {
			return;
		}
		WavHeader header;
		const int n = sceIoRead(_f, &header, sizeof header);
		if (n != sizeof header || !header.supported()) {
			sceIoClose(_f);
			_f = -1;
			return;
		}
		_numSamples = header.dataSize / sizeof(Sample);
		_sampleRate = header.sampleRate;
		// We're ready to read the 2-channel, 16-bit PCM samples after the header.
	}

	AudioFileWav::~AudioFileWav() {
		if (_f >= 0) {
			sceIoClose(_f);
		}
	}

	long AudioFileWav::read(Sample* buffer, uint32_t bufferSamples) {
		if (_numSamples == 0) {
			// The file doesn't contain valid WAV data. Return
			// OV_EINVAL from libvorbis.
			return -131;
		}
		const int n = sceIoRead(_f, buffer, bufferSamples * sizeof(Sample));
		const auto samplesRead = n / sizeof(Sample);
		if (samplesRead < bufferSamples) {
			// Underread, fill the rest of the buffer with zeros.
			std::fill(buffer + samplesRead, buffer + bufferSamples, Sample{});
		}
		_sampleIndex += samplesRead;
		return samplesRead;
	}

	float AudioFileWav::getTime() const {
		if (_numSamples) {
			return static_cast<float>(_sampleIndex) / _sampleRate;
		}
		return 0.0f;
	}

	bool AudioFileWav::rewind() {
		if (_numSamples) {
			_sampleIndex = 0;
			sceIoLseek(_f, 0, SEEK_SET);
			return true;
		}
		return false;
	}

	uint32_t AudioFileWav::numSamples() const {
		return _numSamples;
	}

	uint32_t AudioFileWav::sampleRate() const {
		return _sampleRate;
	}

	const std::string& AudioFileWav::path() const {
		return _path;
	}
}
