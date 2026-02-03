// SPDX-FileCopyrightText: 2025 Sotiris Papatheodorou
// SPDX-License-Identifier: GPL-3.0-or-later

#include "Driver/Music.hpp"

#include "Driver/Platform.hpp"

#include <algorithm>
#include <pspaudio_kernel.h>
#include <pspaudiolib.h>
#include <sstream>
#include <string>

#include "AudioFilePSP.hpp"

namespace SuperHaxagon {
	struct Music::MusicImpl {
		MusicImpl(const Platform& platform, const std::string& path) : _af(createAudioFile(platform, path)) {
			if (!_af || _af->sampleRate() != PSP_AUDIO_FREQ_44K) {
				// This is not a fatal error since the game
				// first looks for user-supplied audio files
				// and falls back to built-in files.
				return;
			}
			loaded = true;
			pspAudioSetChannelCallback(PSP_MUSIC_CHANNEL, callback, this);

			std::stringstream s;
			s << "playing \"" << _af->path() << "\", " << _af->numSamples() << " samples, " << _af->sampleRate() << " Hz";
			platform.message(Dbg::INFO, "music", s.str());
		}

		~MusicImpl() {
			if (loaded) {
				// Only reset the callback if it was set by this instance.
				pspAudioCallback_t _;
				void *data;
				pspAudioGetChannelCallback(PSP_MUSIC_CHANNEL, &_, &data);
				if (data == this) {
					pspAudioSetChannelCallback(PSP_MUSIC_CHANNEL, nullptr, nullptr);
				}
			}
		}

		float getTime() {
			if (loaded) {
				return _af->getTime();
			}
			return 0.0f;
		}

		// Always use channel 0 for music since there's at most one
		// music track playing at any given time.
		static constexpr int PSP_MUSIC_CHANNEL = 0;

		// The PSP's CPU has a single core so using volatile instead of
		// std::atomic should be safe enough.
		// TODO: use semaphores/events?
		volatile bool done = false;
		volatile bool loop = false;
		volatile bool playing = false;
		bool loaded = false;

		private:
		// The PSP audio callback must be a free function. We pass the
		// pointer to the current MusicImpl instance as additional
		// data to allow calling its audioCallback() member function.
		static void callback(void* buf, unsigned numSamples, void* data) {
			reinterpret_cast<MusicImpl*>(data)->
				audioCallback(reinterpret_cast<Sample*>(buf), numSamples);
		}

		std::unique_ptr<AudioFile> _af;

		void audioCallback(Sample* buf, int numSamples) {
			if (!playing) {
				std::fill(buf, buf + numSamples, Sample{});
				return;
			}
			const long r = _af->read(buf, numSamples);
			if (r == 0 && loop) {
				// EOF reached, start from the beginning.
				if (!_af->rewind()) {
					// Read error.
					done = true;
				}
			} else if (r <= 0) {
				// EOF or error.
				done = true;
			}
		}
	};

	Music::Music(std::unique_ptr<Music::MusicImpl> impl) : _impl(std::move(impl)) {}

	Music::~Music() = default;

	// Track looping is handled in audioCallback().
	void Music::update() const {}

	void Music::setLoop(const bool loop) const {
		_impl->loop = loop;
	}

	void Music::play() const {
		_impl->playing = true;
	}

	void Music::pause() const {
		_impl->playing = false;
	}

	bool Music::isDone() const {
		return _impl->done;
	}

	float Music::getTime() const {
		return _impl->getTime();
	}

	std::unique_ptr<Music> createMusic(const Platform& platform, const std::string& path) {
		auto data = std::make_unique<Music::MusicImpl>(platform, path);
		if (!data->loaded) return nullptr;
		return std::make_unique<Music>(std::move(data));
	}
}
