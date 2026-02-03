// SPDX-FileCopyrightText: 2025 Sotiris Papatheodorou
// SPDX-License-Identifier: GPL-3.0-or-later

#include "Driver/Sound.hpp"

#include "Driver/Platform.hpp"

#include <cassert>
#include <pspaudio.h>
#include <pspaudiolib.h>
#include <pspthreadman.h>
#include <sstream>
#include <string>

#include "AudioBufferPSP.hpp"

namespace SuperHaxagon {
	struct Sound::SoundImpl {
		SoundImpl(const Platform& platform, const std::string& path) : buffer(platform, path) {
			if (buffer.samples.empty()) {
				std::stringstream s;
				s << "error loading \"" << path << "\", expected 2 channel, 16-bit PCM, 44.1 kHz audio";
				platform.message(Dbg::WARN, "sound", s.str());
				return;
			}
			std::stringstream s;
			s << "loaded \"" << buffer.path << "\", " << buffer.samples.size()
				<< " samples (" << buffer.samples.size() * sizeof(Sample) / 1024 << " kB)";
			s << " [" << buffer.samples.data() << " - " << buffer.samples.data() + buffer.samples.size() << "]";
			platform.message(Dbg::INFO, "sound", s.str());
		}

		~SoundImpl() {
			if (threadId >= 0) {
				sceKernelWaitThreadEnd(threadId, nullptr);
			}
		}

		void play() {
			if (buffer.samples.empty()) {
				return;
			}
			threadId = sceKernelCreateThread(
					buffer.path.c_str(),
					soundThread,
					0x11,
					0xFA0,
					PSP_THREAD_ATTR_USER,
					nullptr);
			if (threadId < 0) {
				return;
			}
			sceKernelStartThread(threadId, sizeof(SoundImpl), this);
		}

		const AudioBuffer buffer;
		SceUID threadId = -1;

		private:
		static int soundThread(SceSize argpSize, void *argp) {
			assert(argpSize == sizeof(SoundImpl));
			assert(argp);
			auto& impl = *reinterpret_cast<SoundImpl*>(argp);
			assert(!impl.buffer.samples.empty());
			const int channel = sceAudioChReserve(PSP_AUDIO_NEXT_CHANNEL, PSP_NUM_AUDIO_SAMPLES, PSP_AUDIO_FORMAT_STEREO);
			if (channel < 0) {
				return sceKernelExitDeleteThread(1);
			}
			const Sample* buf = impl.buffer.samples.data();
			int samplesRemaining = impl.buffer.samples.size();
			while (samplesRemaining > 0) {
				sceAudioOutputBlocking(channel, PSP_AUDIO_VOLUME_MAX, const_cast<Sample*>(buf));
				buf += PSP_NUM_AUDIO_SAMPLES;
				samplesRemaining -= PSP_NUM_AUDIO_SAMPLES;
			}
			impl.threadId = -1;
			sceAudioChRelease(channel);
			return sceKernelExitDeleteThread(0);
		}
	};

	Sound::Sound(std::unique_ptr<SoundImpl> impl) : _impl(std::move(impl)) {}

	Sound::~Sound() = default;

	void Sound::play() const {
		_impl->play();
	}

	std::unique_ptr<Sound> createSound(const Platform& platform, const std::string& path) {
		auto data = std::make_unique<Sound::SoundImpl>(platform, path);
		if (data->buffer.samples.empty()) return nullptr;
		return std::make_unique<Sound>(std::move(data));
	}
}
