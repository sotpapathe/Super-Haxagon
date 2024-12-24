#include "Driver/PSP/AudioLoaderPSP.hpp"

#include <istream>

namespace SuperHaxagon {
	AudioLoaderPSP::AudioLoaderPSP(const std::string& /* path */, Stream /* stream */) {
		// TODO
	}

	std::unique_ptr<AudioPlayer> AudioLoaderPSP::instantiate() {
		// TODO
		return std::make_unique<AudioPlayerPSP>();
	}
}
