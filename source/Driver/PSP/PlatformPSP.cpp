#include "Driver/PSP/PlatformPSP.hpp"

#include "Core/Twist.hpp"
#include "Driver/PSP/AudioLoaderPSP.hpp"
#include "Driver/PSP/AudioPlayerPSP.hpp"
#include "Driver/PSP/FontPSP.hpp"

#include <assert.h>
#include <filesystem>
#include <pspctrl.h>
#include <pspdebug.h>
#include <pspdisplay.h>
#include <pspgu.h>
#include <psploadexec.h>
#include <psprtc.h>
#include <pspthreadman.h>
#include <psputils.h>
#include <string.h>

PSP_MODULE_INFO("SuperHaxagon", PSP_MODULE_USER, VERSION_MAJOR, VERSION_MINOR);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER);

struct Vertex {
	float x;
	float y;
	float z;
};

uint32_t packColor(const SuperHaxagon::Color color) {
	// The PSP CPU is little-endian and colors are packed in the ABGR
	// order.
	return (static_cast<uint32_t>(color.a) << 24) +
		(static_cast<uint32_t>(color.b) << 16) +
		(static_cast<uint32_t>(color.g) << 8) +
		static_cast<uint32_t>(color.r);
}

alignas(16) static unsigned gu_list[262144];



namespace SuperHaxagon {
	void* const PlatformPSP::_scratchpad = reinterpret_cast<void*>(0x00010000);

	bool PlatformPSP::_running = true;

	// TODO: Use more descriptive paths than romfs/sdmc?
	PlatformPSP::PlatformPSP(const Dbg dbg, int, char** argv) :
		Platform(dbg),
		_game_dir(std::filesystem::path(argv[0]).parent_path()),
		_rom_dir(_game_dir + "/romfs"),
		_user_dir(_game_dir + "/sdmc"),
		_ticks_per_sec(sceRtcGetTickResolution())
	{
		std::filesystem::create_directories(_user_dir);
		_loaded = initCallbacks() && initVideo() && initAudio() &&
			initController();
	}

	bool PlatformPSP::loop() {
		uint64_t curr;
		sceRtcGetCurrentTick(&curr);
		_delta = (curr - _last) / _ticks_per_sec;
		_last = curr;
		return _loaded && _running;
	}

	float PlatformPSP::getDilation() {
		// The game is designed to run at 60 FPS, compute the time
		// dilation value based on the actual framerate.
		return 60.0f * _delta;
	}

	std::string PlatformPSP::getPath(const std::string& partial, const Location location) {
		switch (location) {
		case Location::ROM:
			return _rom_dir + partial;
		case Location::USER:
			return _user_dir + partial;
		default:
			return "";
		}
	}

	std::unique_ptr<AudioLoader> PlatformPSP::loadAudio(const std::string& partial, const Stream stream, const Location location) {
		return std::make_unique<AudioLoaderPSP>(getPath(partial, location), stream);
	}

	std::unique_ptr<Font> PlatformPSP::loadFont(const std::string& partial, int size) {
		return std::make_unique<FontPSP>(getPath(partial, Location::ROM), size);
	}

	void PlatformPSP::playSFX(AudioLoader& /* audio */) {
		// TODO: Play SFX.
	}

	void PlatformPSP::playBGM(AudioLoader& /* audio */) {
		// TODO: Play BGM.
	}

	std::string PlatformPSP::getButtonName(const Buttons& button) {
		if (button.select) return "CROSS";
		if (button.back) return "CIRCLE";
		if (button.quit) return "SELECT";
		if (button.left) return "LEFT";
		if (button.right) return "RIGHT";
		return "?";
	}

	Buttons PlatformPSP::getPressed() {
		SceCtrlData pad;
		sceCtrlPeekBufferPositive(&pad, 1);
		Buttons buttons;
		buttons.select = static_cast<bool>(pad.Buttons & PSP_CTRL_CROSS);
		buttons.back = static_cast<bool>(pad.Buttons & PSP_CTRL_CIRCLE);
		buttons.quit = static_cast<bool>(pad.Buttons & PSP_CTRL_SELECT);
		buttons.left = static_cast<bool>(pad.Buttons & PSP_CTRL_LEFT);
		buttons.right = static_cast<bool>(pad.Buttons & PSP_CTRL_RIGHT);
		return buttons;
	}

	Point PlatformPSP::getScreenDim() const {
		return {_width, _height};
	}

	void PlatformPSP::screenBegin() {
		sceGuStart(GU_DIRECT, gu_list);
		sceGuClearColor(0xFF000000);
		sceGuClear(GU_COLOR_BUFFER_BIT);

		pspDebugScreenSetOffset(reinterpret_cast<int>(_draw_buf));
		pspDebugScreenSetXY(0, 0);
	}

	void PlatformPSP::screenFinalize() {
		pspDebugScreenPrintf("%.2f FPS", 1.0f / _delta);

		sceGuFinish();
		sceGuSync(GU_SYNC_FINISH, GU_SYNC_WHAT_DONE);
		sceDisplayWaitVblankStart();
		_draw_buf = sceGuSwapBuffers();
	}

	void PlatformPSP::drawPoly(const Color& color, const std::vector<Point>& points) {
		assert(sizeof(Vertex) * points.size() < _scratchpad_size);
		Vertex* v = reinterpret_cast<Vertex*>(_scratchpad);
		for (size_t i = 0; i < points.size(); i++) {
			v[i].x = points[i].x;
			v[i].y = points[i].y;
			v[i].z = 0.0f;
		}
		sceGuColor(packColor(color));
		sceGuDrawArray(GU_TRIANGLE_FAN,
			GU_VERTEX_32BITF | GU_TRANSFORM_2D,
			points.size(),
			nullptr,
			v);
	}

	std::unique_ptr<Twist> PlatformPSP::getTwister() {
		// Not great but probably good enough.
		uint64_t tick;
		sceRtcGetCurrentTick(&tick);
		return std::make_unique<Twist>(
			std::unique_ptr<std::seed_seq>(new std::seed_seq{tick})
		);
	}

	void PlatformPSP::shutdown() {
		sceGuTerm();
		sceKernelExitGame();
		clearScratchpad();
	}

	void PlatformPSP::message(const Dbg dbg, const std::string& where, const std::string& message) {
		std::string format;
		switch (dbg) {
		case Dbg::WARN:
			format = "[psp:warn]";
			break;
		case Dbg::FATAL:
			format = "[psp:fatal]";
			break;
		default:
			format = "[psp:info]";
		}
		pspDebugScreenPrintf("%s %s: %s\n", format.c_str(), where.c_str(), message.c_str());
	}

	Supports PlatformPSP::supports() {
		// TODO: Enabling FILESYSTEM crashes the PSP.
		//return Supports::FILESYSTEM | Supports::SHADOWS;
		return Supports::SHADOWS;
	}

	int PlatformPSP::exitCallback(int, int, void*) {
		_running = false;
		return 0;
	}

	int PlatformPSP::callbackThread(SceSize, void*) {
		// XXX: Does returning -1 indicate error in this context?
		int id = sceKernelCreateCallback("exitCallback", exitCallback, nullptr);
		if (id < 0) {
			return -1;
		}
		if (sceKernelRegisterExitCallback(id) < 0) {
			return -1;
		}
		if (sceKernelSleepThreadCB() < 0) {
			return -1;
		}
		return 0;
	}

	void PlatformPSP::clearScratchpad() {
		memset(_scratchpad, 0, _scratchpad_size);
	}

	bool PlatformPSP::initCallbacks() {
		// XXX: Could the priority (0x11) be reduced (high value -> low priority)?
		// XXX: Could the stack size (0xFA0) be reduced?
		SceUID id = sceKernelCreateThread("callbackThread", callbackThread, 0x11, 0xFA0, PSP_THREAD_ATTR_USER, nullptr);
		if (id < 0) {
			return false;
		}
		return sceKernelStartThread(id, 0, 0) == 0;
	}

	bool PlatformPSP::initVideo() {
		/* The framebuffer width must be a power of two. */
		constexpr unsigned fb_width = 512;
		constexpr unsigned fb_fmt = GU_PSM_8888;
		_draw_buf = guGetStaticVramBuffer(fb_width, _height, fb_fmt);
		void* disp_buf = guGetStaticVramBuffer(fb_width, _height, fb_fmt);
		void* z_buf = guGetStaticVramBuffer(fb_width, _height, GU_PSM_4444);

		pspDebugScreenInit();
		sceGuInit();
		sceGuStart(GU_DIRECT, gu_list);
		sceGuDrawBuffer(fb_fmt, reinterpret_cast<void*>(_draw_buf), fb_width);
		sceGuDispBuffer(_width, _height, reinterpret_cast<void*>(disp_buf), fb_width);
		sceGuDepthBuffer(reinterpret_cast<void*>(z_buf), fb_width);

		/* The PSP uses a 4096x4096 virtual canvas for rendering. */
		constexpr unsigned virt_width = 4096;
		constexpr unsigned virt_height = 4096;
		sceGuOffset((virt_width - _width) / 2, (virt_height - _height) / 2);
		sceGuViewport(virt_width / 2, virt_height / 2, _width, _height);

		sceGuScissor(0, 0, _width, _height);
		sceGuEnable(GU_SCISSOR_TEST);

		sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
		sceGuEnable(GU_BLEND);

		sceGuShadeModel(GU_FLAT);

		sceGuDisable(GU_ALPHA_TEST);
		sceGuDisable(GU_CULL_FACE);
		sceGuDisable(GU_DEPTH_TEST);
		sceGuDisable(GU_TEXTURE_2D);

		sceGuFinish();
		sceGuSync(GU_SYNC_FINISH, GU_SYNC_WHAT_DONE);
		sceDisplayWaitVblankStart();
		sceGuDisplay(GU_TRUE);
		return true;
	}

	bool PlatformPSP::initAudio() {
		// TODO: Initialize audio.
		return true;
	}

	bool PlatformPSP::initController() {
		sceCtrlSetSamplingCycle(0);
		/* The analog stick isn't used. */
		sceCtrlSetSamplingMode(PSP_CTRL_MODE_DIGITAL);
		return true;
	}
}



/* The following are true for the PSP. */
static_assert(sizeof(void*) == 4);
static_assert(sizeof(uintptr_t) == 4);
static_assert(sizeof(short) == 2);
static_assert(sizeof(int) == 4);
static_assert(sizeof(long) == 4);
static_assert(sizeof(long long) == 8);
