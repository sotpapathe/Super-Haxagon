// SPDX-FileCopyrightText: 2025 Sotiris Papatheodorou
// SPDX-License-Identifier: GPL-3.0-or-later

#include <algorithm>

#include "AudioFileVorbisPSP.hpp"

namespace SuperHaxagon {
	AudioFileVorbis::AudioFileVorbis(const std::string& path) : _path(path) {
		if (ov_fopen(path.c_str(), &_vf) < 0) {
			return;
		}
		const vorbis_info* vi = ov_info(&_vf, -1);
		if (vi->channels != 2) {
			ov_clear(&_vf);
			return;
		}
		_numSamples = ov_pcm_total(&_vf, -1);
		_sampleRate = vi->rate;
	}

	AudioFileVorbis::~AudioFileVorbis() {
		if (_numSamples) {
			ov_clear(&_vf);
		}
	}

	long AudioFileVorbis::read(Sample* buffer, uint32_t bufferSamples) {
		if (!_numSamples) {
			return OV_EINVAL;
		}
		char* buf = reinterpret_cast<char*>(buffer);
		long bytesRemaining = bufferSamples * sizeof(Sample);
		while (bytesRemaining > 0) {
			const long r = ov_read(&_vf, buf, bytesRemaining,
				0, sizeof(int16_t), 1, nullptr);
			if (r < 0) {
				// Error.
				return r;
			}
			if (r == 0) {
				// EOF.
				const long samplesRemaining = bytesRemaining / sizeof(Sample);
				if (samplesRemaining < bufferSamples) {
					// We read some data into buffer before
					// EOF, fill the rest of the buffer
					// with zeros.
					std::fill(buffer + bufferSamples - samplesRemaining, buffer + bufferSamples, Sample{});
					return bufferSamples - samplesRemaining;
				}
				return r;
			}
			buf += r;
			bytesRemaining -= r;
		}
		return bufferSamples;
	}

	float AudioFileVorbis::getTime() const {
		if (_numSamples) {
			return ov_time_tell(const_cast<OggVorbis_File*>(&_vf));
		}
		return 0.0f;
	}

	bool AudioFileVorbis::rewind() {
		if (_numSamples) {
			return ov_raw_seek_lap(&_vf, 0) == 0;
		}
		return false;
	}

	uint32_t AudioFileVorbis::numSamples() const {
		return _numSamples;
	}

	uint32_t AudioFileVorbis::sampleRate() const {
		return _sampleRate;
	}

	const std::string& AudioFileVorbis::path() const {
		return _path;
	}
}
