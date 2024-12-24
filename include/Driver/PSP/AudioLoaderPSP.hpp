#ifndef SUPER_HAXAGON_AUDIO_LOADER_PSP_HPP
#define SUPER_HAXAGON_AUDIO_LOADER_PSP_HPP

#include "Core/AudioLoader.hpp"
#include "Driver/PSP/AudioPlayerPSP.hpp"

namespace SuperHaxagon {
	enum class Location;
	class Platform;

	class AudioLoaderPSP : public AudioLoader {
	public:
		AudioLoaderPSP(const std::string& path, Stream stream);
		~AudioLoaderPSP() override = default;

		std::unique_ptr<AudioPlayer> instantiate() override;

	private:
	};
}

#endif //SUPER_HAXAGON_AUDIO_LOADER_PSP_HPP
