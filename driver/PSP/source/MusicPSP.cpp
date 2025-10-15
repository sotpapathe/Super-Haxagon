// SPDX-FileCopyrightText: 2025 Sotiris Papatheodorou
// SPDX-License-Identifier: GPL-3.0-or-later

#include "Driver/Music.hpp"

#include "Driver/Platform.hpp"

#include <algorithm>
#include <cassert>
#include <pspaudio.h>
#include <pspaudio_kernel.h>
#include <pspaudiolib.h>
#include <pspthreadman.h>
#include <sstream>
#include <string>

#include "AudioFilePSP.hpp"
#include "CommonPSP.hpp"

namespace SuperHaxagon {
	struct Music::MusicImpl {
		MusicImpl(const Platform& platform, const std::string& path) : path(path), af(createAudioFile(path)) {
			if (!af || af->sampleRate() != PSP_AUDIO_FREQ_44K) {
				// This is not a fatal error since the game
				// first looks for user-supplied audio files
				// and falls back to built-in files.
				return;
			}
			loaded = true;

			std::stringstream s;
			s << "playing \"" << af->path() << "\", " << af->numSamples() << " samples, " << af->sampleRate() << " Hz";
			platform.message(Dbg::INFO, "music", s.str());
		}

		~MusicImpl() {
			if (threadId >= 0) {
				SceKernelThreadRunStatus status;
				sceKernelReferThreadRunStatus(threadId, &status);
				if (status.status == PSP_THREAD_RUNNING) {
					sceKernelTerminateDeleteThread(threadId);
				}
			}
		}

		float getTime() {
			if (loaded) {
				return af->getTime();
			}
			return 0.0f;
		}

		void startThread() {
			if (!loaded) {
				return;
			}
			if (threadId >= 0) {
				SceKernelThreadRunStatus status;
				sceKernelReferThreadRunStatus(threadId, &status);
				if (status.status == PSP_THREAD_RUNNING) {
					return;
				}
			}
			// No playback thread running, create a new one.
			threadId = sceKernelCreateThread(
					path.c_str(),
					soundThread,
					0x11,
					0xFA0,
					PSP_THREAD_ATTR_USER,
					nullptr);
			if (threadId < 0) {
				return;
			}
			sceKernelStartThread(threadId, sizeof(MusicImpl), const_cast<MusicImpl*>(this));
		}

		const std::string path;
		std::unique_ptr<AudioFile> af;
		SceUID threadId = -1;
		// The PSP's CPU has a single core so using volatile instead of
		// std::atomic should be safe enough.
		volatile bool done = false;
		volatile bool loop = false;
		volatile bool playing = false;
		bool loaded = false;

		private:
		static int soundThread(SceSize argpSize, void *argp) {
			assert(argpSize == sizeof(MusicImpl));
			assert(argp);
			auto& impl = *reinterpret_cast<MusicImpl*>(argp);
			impl.done = false;
			impl.playing = true;
			Sample buf[PSP_NUM_AUDIO_SAMPLES];
			while (!impl.done) {
				if (impl.playing) {
					const long r = impl.af->read(buf, PSP_NUM_AUDIO_SAMPLES);
					if (r == 0 && impl.loop) {
						// EOF reached, start from the beginning.
						if (!impl.af->rewind()) {
							// Read error.
							impl.done = true;
						}
					} else if (r <= 0) {
						// EOF or error.
						impl.done = true;
					}
					if (impl.done) {
						std::fill(buf, buf + PSP_NUM_AUDIO_SAMPLES, Sample{});
					}
				} else {
					std::fill(buf, buf + PSP_NUM_AUDIO_SAMPLES, Sample{});
 				}
				// XXX: still have cracking as with sceAudioOutputBlocking()
				//while (sceAudioGetChannelRestLen(PSP_MUSIC_CHANNEL) > 0) {
				//	// TODO: sleep (PSP_NUM_AUDIO_SAMPLES / PSP_AUDIO_FREQ_44K / 10) or so
				//}
				//sceAudioOutput(PSP_MUSIC_CHANNEL, PSP_AUDIO_VOLUME_MAX, buf);
				sceAudioOutputBlocking(PSP_MUSIC_CHANNEL, PSP_AUDIO_VOLUME_MAX, buf);
 			}
			impl.threadId = -1;
			return sceKernelExitDeleteThread(0);
 		}
	};

	Music::Music(std::unique_ptr<Music::MusicImpl> impl) : _impl(std::move(impl)) {}

	Music::~Music() = default;

	// Track looping is handled in soundThread().
	void Music::update() const {}

	void Music::setLoop(const bool loop) const {
		_impl->loop = loop;
	}

	void Music::play() const {
		_impl->startThread();
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
