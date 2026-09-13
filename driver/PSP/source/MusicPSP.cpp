// SPDX-FileCopyrightText: 2025 Sotiris Papatheodorou
// SPDX-License-Identifier: GPL-3.0-or-later

#include "Driver/Music.hpp"

#include "Driver/Platform.hpp"

#include <pspmp3.h>
#include <pspthreadman.h>
#include <sstream>
#include <string>

#include "CommonPSP.hpp"

namespace SuperHaxagon {
	struct Music::MusicImpl {
		MusicImpl(const Platform& platform, const std::string& path) {
			threadId = sceKernelCreateThread(
					"musicThread",
					musicCallback,
					PSP_THREAD_USER_MAX_PRIORITY,
					64 * 1024,
					PSP_THREAD_ATTR_USER,
					nullptr);
			if (threadId < 0 || sceKernelStartThread(threadId, sizeof(char*), const_cast<char*>(path.c_str())) < 0) {
				platform.message(Dbg::WARN, "music", "error creating music thread");
				return;
			}
			// TODO: wait for thread to initialize

			std::stringstream s;
			s << "playing \"" << path << "\"";
			platform.message(Dbg::INFO, "music", s.str());
		}

		~MusicImpl() {
			if (threadId >= 0) {
				// TODO: stop thread
			}
		}

		float getTime() {
			// TODO
			return 0.0f;
		}

		SceUID threadId = -1;

		private:
		static int musicCallback(SceSize dataSize, void* data) {
			if (dataSize != sizeof(char*) || !data) {
				return -1;
			}
			const char* const path = reinterpret_cast<const char*>(data);
			const SceUID fd = sceIoOpen(path, PSP_O_RDONLY, 0777);
			if (fd < 0) {
				return -1;
			}

			alignas(64) char mp3Buf[16 * 1024];
			alignas(64) char pcmBuf[16 * (1152 / 2)];
			SceMp3InitArg mp3Init;
			mp3Init.mp3StreamStart = 0;
			mp3Init.mp3StreamEnd = sceIoLseek32(fd, 0, PSP_SEEK_END);
			mp3Init.mp3Buf = mp3Buf;
			mp3Init.mp3BufSize = sizeof(mp3Buf);
			mp3Init.pcmBuf = pcmBuf;
			mp3Init.pcmBufSize = sizeof(pcmBuf);
			const SceInt32 handle = sceMp3ReserveMp3Handle(&mp3Init);
			if (handle < 0) {
				// TODO: cleanup
				return -1;
			}
			// TODO: fill buf
			if (sceMp3Init(handle) < 0) {
				// TODO: cleanup
				return -1;
			}
			// TODO: decode and play
			// sceMp3ReleaseMp3Handle(handle) == 0
			return 0;
		}

		static int fillbuf(SceUID fd, SceInt32 handle)
		{
			return 0;
		}
	};

	Music::Music(std::unique_ptr<Music::MusicImpl> impl) : _impl(std::move(impl)) {}

	Music::~Music() = default;

	// Track looping is handled in audioCallback().
	void Music::update() const {}

	void Music::setLoop(const bool loop) const {
		// TODO: notify thread
	}

	void Music::play() const {
		// TODO: notify thread
	}

	void Music::pause() const {
		// TODO: notify thread
	}

	bool Music::isDone() const {
		return true; // TODO
	}

	float Music::getTime() const {
		return _impl->getTime();
	}

	std::unique_ptr<Music> createMusic(const Platform& platform, const std::string& path) {
		auto data = std::make_unique<Music::MusicImpl>(platform, path);
		//if (!data->loaded) return nullptr; // TODO
		return std::make_unique<Music>(std::move(data));
	}
}
