// SPDX-FileCopyrightText: 2025 Sotiris Papatheodorou
// SPDX-License-Identifier: GPL-3.0-or-later

#include "AudioBufferPSP.hpp"

namespace SuperHaxagon {
	AudioBuffer::AudioBuffer(const std::string& path_) {
		auto f = createAudioFile(path_);
		if (!f || f->numSamples() == 0) {
			return;
		}
		path = f->path();
		try {
			samples.resize(roundSamples(f->numSamples()));
		} catch (const std::bad_alloc&) {
			return;
		}
		if (f->read(samples.data(), f->numSamples()) <= 0) {
			samples.clear();
			samples.shrink_to_fit();
		}
		// The remaining (samples.size() - f->numSamples()) elements
		// are initialized to 0 by the call to resize().
	}
}
