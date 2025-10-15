// SPDX-FileCopyrightText: 2025-2026 Sotiris Papatheodorou
// SPDX-License-Identifier: GPL-3.0-or-later

#include "Driver/Platform.hpp"

#include "Driver/Font.hpp"
#include "Driver/Music.hpp"
#include "Driver/Screen.hpp"
#include "Driver/Sound.hpp"
#include "Driver/Tools/Configuration.hpp"
#include "Driver/Tools/Random.hpp"

#include <filesystem>
#include <pspaudio.h>
#include <pspaudiolib.h>
#include <psploadexec.h>
#include <psprtc.h>
#include <pspthreadman.h>
#include <psputils.h>

#include "CommonPSP.hpp"
#include "ControlsPSP.hpp"

// The PSP CPU is little endian and the sizes of basic types are the following:
static_assert(sizeof(short) == 2);
static_assert(sizeof(int) == 4);
static_assert(sizeof(long) == 4);
static_assert(sizeof(long long) == 8);
static_assert(sizeof(void*) == 4);

PSP_MODULE_INFO(APP_NAME, PSP_MODULE_USER, VERSION_MAJOR, VERSION_MINOR);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_USER | PSP_THREAD_ATTR_VFPU);

namespace SuperHaxagon {
	std::unique_ptr<Font> createFont(const Platform& platform, const std::string& path, int size);
	std::unique_ptr<Music> createMusic(const Platform& platform, const std::string& path);
	std::unique_ptr<Screen> createScreen(bool debug);
	std::unique_ptr<Sound> createSound(const Platform& platform, const std::string& path);

	struct Platform::PlatformImpl {
		PlatformImpl(bool debug) :
			dataDir("DATA"),
			userDir("USER"),
			debug(debug)
	       	{
			if (debug) {
				debugStream.open(userDir + "/LOG.TXT");
			}

			running = initCallbacks();
			screen = createScreen(debug);
			initControls();
			if (sceAudioChReserve(PSP_MUSIC_CHANNEL, PSP_NUM_AUDIO_SAMPLES, PSP_AUDIO_FORMAT_STEREO) < 0) {
				message(Dbg::FATAL, "platform", "error reserving music channel");
				shutdown();
			}

			message(Dbg::INFO, "platform", "initialized");
		}

		void shutdown() {
			sceAudioChRelease(PSP_MUSIC_CHANNEL);
			sceKernelExitGame();
			debugStream.close();
		}

		void message(const Dbg dbg, const std::string& where, const std::string& message) {
			if (!debug || !debugStream.good()) {
				return;
			}
			std::string format;
			switch (dbg) {
			case Dbg::FATAL:
				format = "[psp:fatal]";
				break;
			case Dbg::WARN:
				format = "[psp:warn]";
				break;
			default:
				format = "[psp:info]";
			}
			debugStream << format << " " << where << ": " << message << std::endl;
			pspDebugScreenPrintf("%s %s: %s\n", format.c_str(), where.c_str(), message.c_str());
		}

		inline static bool running;

		const std::string dataDir;
		const std::string userDir;
		const bool debug;
		std::ofstream debugStream;

		std::unique_ptr<Screen> screen;

		private:
		static int exitCallback(int, int, void*) {
			running = false;
			return 0;
		}

		static int callbackThread(SceSize, void*) {
			const int id = sceKernelCreateCallback("exitCallback", exitCallback, nullptr);
			if (id < 0 || sceKernelRegisterExitCallback(id) < 0 || sceKernelSleepThreadCB() < 0) {
				return -1;
			}
			return 0;
		}

		bool initCallbacks() {
			// XXX: Could the priority (0x11) be reduced (high value -> low priority)?
			// XXX: Could the stack size (0xFA0) be reduced?
			const SceUID id = sceKernelCreateThread(
					"callbackThread",
					callbackThread,
					0x11,
					0xFA0,
					PSP_THREAD_ATTR_USER,
					nullptr);
			return id >= 0 && sceKernelStartThread(id, 0, nullptr) == 0;
		}
	};

	Platform::Platform() : _impl(std::make_unique<PlatformImpl>(DEBUG_CONSOLE)) {}

	Platform::~Platform() = default;

	bool Platform::loop() {
		return _impl->running;
	}

	float Platform::getDilation() const {
		// There's no need for time dilation since the PSP runs at 60
		// FPS, same as the game.
		return 1.0f;
	}

	std::string Platform::getPath(const std::string& partial, const Location location) const {
		switch (location) {
		case Location::ROM:
			return _impl->dataDir + partial;
		case Location::USER:
			return _impl->userDir + partial;
		default:
			return "";
		}
	}

	std::unique_ptr<Font> Platform::loadFont(int size) const {
		return createFont(*this, getPath("/bump-it-up.ttf", Location::ROM), size);
	}

	std::unique_ptr<Sound> Platform::loadSound(const std::string& base) const {
		return createSound(*this, getPath(base, Location::ROM));
	}

	std::unique_ptr<Music> Platform::loadMusic(const std::string& base, Location location) const {
		return createMusic(*this, getPath(base, location));
	}

	Screen& Platform::getScreen() {
		return *_impl->screen;
	}

	std::string Platform::getButtonName(ButtonName buttonName) {
		return buttonNameString(buttonName);
	}

	Buttons Platform::getPressed() const {
		return pressedButtons();
	}

	void Platform::shutdown() {
		_impl->shutdown();
	}

	void Platform::message(const Dbg dbg, const std::string& where, const std::string& message) const {
		_impl->message(dbg, where, message);
	}

	Supports Platform::supports() {
		return Supports::SHADOWS;
	}
}
