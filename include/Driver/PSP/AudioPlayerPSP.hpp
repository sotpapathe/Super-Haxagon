#ifndef SUPER_HAXAGON_AUDIO_PLAYER_PSP_HPP
#define SUPER_HAXAGON_AUDIO_PLAYER_PSP_HPP

#include "Core/AudioPlayer.hpp"

namespace SuperHaxagon {
	class AudioPlayerPSP : public AudioPlayer {
	public:
		AudioPlayerPSP();
		~AudioPlayerPSP() override = default;

		void setChannel(int) override {}
		void setLoop(bool) override {}

		void play() override;
		void pause() override;
		bool isDone() const override;
		float getTime() const override;

	private:
	};
}


#endif //SUPER_HAXAGON_AUDIO_PLAYER_PSP_HPP
