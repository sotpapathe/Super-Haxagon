// SPDX-FileCopyrightText: 2025 Sotiris Papatheodorou
// SPDX-License-Identifier: GPL-3.0-or-later

#include "AudioBufferPSP.hpp"

#include <sstream>

namespace SuperHaxagon {
	AudioBuffer::AudioBuffer(const Platform& platform, const std::string& path_) {
		auto f = createAudioFile(platform, path_);
		if (!f || f->numSamples() == 0) {
			std::stringstream s;
			s << "error loading \"" << path << "\", 0 samples";
			platform.message(Dbg::WARN, "sound", s.str());
			f.reset();
			return;
		}
		path = f->path();
		try {
			samples.resize(roundSamples(f->numSamples()));
		} catch (const std::bad_alloc&) {
			std::stringstream s;
			s << "error loading \"" << path << "\", bad_alloc";
			platform.message(Dbg::WARN, "sound", s.str());
			f.reset();
			return;
		}
		if (f->read(samples.data(), f->numSamples()) <= 0) {
			std::stringstream s;
			s << "error loading \"" << path << "\", error reading";
			platform.message(Dbg::WARN, "sound", s.str());
			samples.clear();
			samples.shrink_to_fit();
		}
		// The remaining (samples.size() - f->numSamples()) elements
		// are initialized to 0 by the call to resize().
		f.reset();
	}
}
