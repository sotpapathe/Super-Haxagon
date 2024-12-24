#ifndef SUPER_HAXAGON_PLATFORM_PSP_HPP
#define SUPER_HAXAGON_PLATFORM_PSP_HPP

#include "Core/Platform.hpp"

#include <pspkerneltypes.h>

#define VERSION_MAJOR 1
#define VERSION_MINOR 0

namespace SuperHaxagon {
	class PlatformPSP : public Platform {
	public:
		explicit PlatformPSP(Dbg dbg, int argc, char** argv);
		PlatformPSP(PlatformPSP&) = delete;
		~PlatformPSP() override = default;

		bool loop() override;
		float getDilation() override;

		std::string getPath(const std::string& partial, Location location) override;
		std::unique_ptr<AudioLoader> loadAudio(const std::string& partial, Stream stream, Location location) override;
		std::unique_ptr<Font> loadFont(const std::string& partial, int size) override;

		void playSFX(AudioLoader& audio) override;
		void playBGM(AudioLoader& audio) override;

		std::string getButtonName(const Buttons& button) override;
		Buttons getPressed() override;
		Point getScreenDim() const override;

		void screenBegin() override;
		void screenFinalize() override;
		void drawPoly(const Color& color, const std::vector<Point>& points) override;

		std::unique_ptr<Twist> getTwister() override;

		void shutdown() override;
		void message(Dbg dbg, const std::string& where, const std::string& message) override;
		Supports supports() override;

	private:
		/* Resolution of the PSP display in pixels. */
		static constexpr unsigned _width = 480;
		static constexpr unsigned _height = 272;
		/* The 16 KB scratchpad SRAM. Its start is 16-byte aligned. */
		static void* const _scratchpad;
		static constexpr unsigned _scratchpad_size = 0x00003fff;

		static bool _running;

		static int exitCallback(int, int, void*);
		static int callbackThread(SceSize, void*);
		static void clearScratchpad();

		const std::string _game_dir;
		const std::string _rom_dir;
		const std::string _user_dir;
		const float _ticks_per_sec;
		void* _draw_buf = nullptr;
		bool _loaded = false;
		uint64_t _last = 0;
		float _delta = 0.0f;

		bool initCallbacks();
		bool initVideo();
		bool initAudio();
		bool initController();
	};
}

#endif //SUPER_HAXAGON_PLATFORM_PSP_HPP
