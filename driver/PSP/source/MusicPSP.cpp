// SPDX-FileCopyrightText: 2025 Sotiris Papatheodorou
// SPDX-License-Identifier: GPL-3.0-or-later

#include "Driver/Music.hpp"

#include "Driver/Platform.hpp"

#include <pspaudio.h>
#include <pspiofilemgr.h>
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
			// TODO: how? SceMp3???
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
			int status = 0;
			int samplesPlayed = 0;

			alignas(64) SceUChar8 mp3Buf[16 * 1024];
			alignas(64) SceUChar8 pcmBuf[16 * (1152 / 2)];
			SceMp3InitArg mp3Init;
			mp3Init.mp3StreamStart = 0;
			mp3Init.mp3StreamEnd = sceIoLseek32(fd, 0, PSP_SEEK_END);
			mp3Init.mp3Buf = mp3Buf;
			mp3Init.mp3BufSize = sizeof(mp3Buf);
			mp3Init.pcmBuf = pcmBuf;
			mp3Init.pcmBufSize = sizeof(pcmBuf);
			const SceInt32 handle = sceMp3ReserveMp3Handle(&mp3Init);
			if (handle < 0) {
				status = -1;
				goto cleanup1;
			}
			if (fillbuf(fd, handle) < 0 || sceMp3Init(handle) < 0) {
				status = -1;
				goto cleanup2;
			}
			while (1) { // TODO: stop condition
				// TODO: check for paused
				if (sceMp3CheckStreamDataNeeded(handle) > 0) {
					if (fillbuf(fd, handle) < 0) {
						status = -1;
						goto cleanup2;
					}
				}
				// TODO: retry Decode in case looping is needed
				SceShort16* samples;
				SceInt32 bytesDecoded = sceMp3Decode(handle, &samples);
				if (bytesDecoded < 0) {
					status = -1;
					goto cleanup2;
				}
				if (bytesDecoded == 0) {
					sceMp3ResetPlayPosition(handle);
					samplesPlayed = 0;
					// TODO: paused = true;
					// TODO: handle looping?
				}
				// TODO: play
				samplesPlayed += sceAudioSRCOutputBlocking(PSP_AUDIO_VOLUME_MAX, samples);
			}
cleanup2:
			sceMp3ReleaseMp3Handle(handle);
cleanup1:
			sceIoClose(fd);
			return 0;
		}

		static int fillbuf(SceUID fd, SceInt32 handle)
		{
			SceUChar8* dst;
			SceInt32 toWrite;
			SceInt32 srcPos;
			if (sceMp3GetInfoToAddStreamData(handle, &dst, &toWrite, &srcPos) < 0
					|| sceIoLseek32(fd, srcPos, PSP_SEEK_SET) < 0) {
				return -1;
			}
			int read = sceIoRead(fd, dst, toWrite);
			if (read < 0) {
				return -1;
			}
			if (read == 0) {
				return 0;
			}
			if (sceMp3NotifyAddStreamData(handle, read) < 0) {
				return -1;
			}
			return srcPos > 0;
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
