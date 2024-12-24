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

// The following are true for the PSP.
static_assert(sizeof(void*) == 4);
static_assert(sizeof(uintptr_t) == 4);
static_assert(sizeof(short) == 2);
static_assert(sizeof(int) == 4);
static_assert(sizeof(long) == 4);
static_assert(sizeof(long long) == 8);



PSP_MODULE_INFO("SuperHaxagon", PSP_MODULE_USER, VERSION_MAJOR, VERSION_MINOR);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);



namespace SuperHaxagon {
	bool PlatformPSP::_running = true;

	// TODO: Use more descriptive paths than romfs/sdmc?
	PlatformPSP::PlatformPSP(const Dbg dbg, int, char** argv) :
		Platform(dbg),
		_game_dir(std::filesystem::path(argv[0]).parent_path()),
		_rom_dir(_game_dir + "/romfs"),
		_user_dir(_game_dir + "/sdmc")
	{
		std::filesystem::create_directories(_user_dir);
		_running = initCallbacks() && initVideo() && initAudio() &&
			initController();
	}

	bool PlatformPSP::loop() {
		return _running;
	}

	float PlatformPSP::getDilation() {
		// There's no need for time dilation since the PSP runs at 60
		// FPS, the same framerate the game was designed to run at.
		return 1.0f;
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
		// There's no quit button on the PSP as the convention is to
		// quit using the PlayStation button.
		if (button.select) return "X";
		if (button.back) return "O";
		if (button.left) return "L";
		if (button.right) return "R";
		return "?";
	}

	Buttons PlatformPSP::getPressed() {
		// There's no quit button on the PSP as the convention is to
		// quit using the PlayStation button.
		SceCtrlData pad;
		sceCtrlPeekBufferPositive(&pad, 1);
		Buttons buttons;
		buttons.select = static_cast<bool>(pad.Buttons & PSP_CTRL_CROSS);
		buttons.back = static_cast<bool>(pad.Buttons & PSP_CTRL_CIRCLE);
		buttons.left = static_cast<bool>(pad.Buttons & PSP_CTRL_LTRIGGER)
			|| static_cast<bool>(pad.Buttons & PSP_CTRL_LEFT);
		buttons.right = static_cast<bool>(pad.Buttons & PSP_CTRL_RTRIGGER)
			|| static_cast<bool>(pad.Buttons & PSP_CTRL_RIGHT);
		return buttons;
	}

	Point PlatformPSP::getScreenDim() const {
		return {_width, _height};
	}

	void PlatformPSP::screenBegin() {
		sceGuStart(GU_DIRECT, _gu_list);
		sceGuClearColor(0xFF000000);
		sceGuClear(GU_COLOR_BUFFER_BIT);

		pspDebugScreenSetOffset(reinterpret_cast<int>(_draw_buf));
		pspDebugScreenSetXY(0, 0);
		sceRtcGetCurrentTick(&_dbg_t_start);
	}

	void PlatformPSP::screenFinalize() {
		// Show the percentage of the frame time at 60 FPS that is used.
		static constexpr float budget_ms = 1000.0f / 60.0f;
		static const float ticks_per_ms = sceRtcGetTickResolution() / 1000;
		if (_dbg <= Dbg::INFO) {
			uint64_t t_end;
			sceRtcGetCurrentTick(&t_end);
			const float frame_ms = (t_end - _dbg_t_start) / ticks_per_ms;
			pspDebugScreenPrintf("%.3f%%", frame_ms / budget_ms * 100.0f);
		}

		sceGuFinish();
		sceGuSync(GU_SYNC_FINISH, GU_SYNC_WHAT_DONE);
		sceDisplayWaitVblankStart();
		_draw_buf = sceGuSwapBuffers();
	}

	void PlatformPSP::drawPoly(const Color& color, const std::vector<Point>& points) {
		struct Vertex {
			float x;
			float y;
			float z;
		};

		Vertex* const v = reinterpret_cast<Vertex*>(sceGuGetMemory(points.size() * sizeof(Vertex)));
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
		sceGuDisplay(GU_FALSE);
		sceGuTerm();
		sceKernelExitGame();
	}

	void PlatformPSP::message(const Dbg dbg, const std::string& where, const std::string& message) {
		if (dbg < _dbg) {
			return;
		}
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
		const int id = sceKernelCreateCallback("exitCallback", exitCallback, nullptr);
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

	uint32_t PlatformPSP::packColor(const Color color) {
		// The PSP CPU is little-endian and colors are packed in the
		// ABGR order.
		return (static_cast<uint32_t>(color.a) << 24) +
			(static_cast<uint32_t>(color.b) << 16) +
			(static_cast<uint32_t>(color.g) << 8) +
			static_cast<uint32_t>(color.r);
	}

	bool PlatformPSP::initCallbacks() {
		// XXX: Could the priority (0x11) be reduced (high value -> low priority)?
		// XXX: Could the stack size (0xFA0) be reduced?
		const SceUID id = sceKernelCreateThread(
				"callbackThread",
				callbackThread,
				0x11,
				0xFA0,
				PSP_THREAD_ATTR_USER,
				nullptr);
		if (id < 0) {
			return false;
		}
		return sceKernelStartThread(id, 0, 0) == 0;
	}

	bool PlatformPSP::initVideo() {
		/* The framebuffer width must be a power of two. */
		static constexpr unsigned fb_width = 512;
		static constexpr unsigned fb_height = _height;
		static constexpr unsigned fb_fmt = GU_PSM_8888;
		_draw_buf = guGetStaticVramBuffer(fb_width, fb_height, fb_fmt);
		void* disp_buf = guGetStaticVramBuffer(fb_width, fb_height, fb_fmt);

		pspDebugScreenInit();
		sceGuInit();
		sceGuStart(GU_DIRECT, _gu_list);
		sceGuDrawBuffer(fb_fmt, reinterpret_cast<void*>(_draw_buf), fb_width);
		sceGuDispBuffer(_width, _height, reinterpret_cast<void*>(disp_buf), fb_width);
		/* There's no need for a depth buffer in this game. */
		sceGuDepthBuffer(0, 0);

		/* The PSP uses a 4096x4096 virtual canvas for rendering. */
		constexpr unsigned virt_width = 4096;
		constexpr unsigned virt_height = 4096;
		sceGuOffset((virt_width - _width) / 2, (virt_height - _height) / 2);
		sceGuViewport(virt_width / 2, virt_height / 2, _width, _height);

		sceGuScissor(0, 0, _width, _height);
		sceGuEnable(GU_SCISSOR_TEST);

		sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
		sceGuEnable(GU_BLEND);

		// TODO: double-check, vars for buffer dims
		//sceGuTexMode(GU_PSM_8888, 0, 0, GU_FALSE);
		////sceGuTexImage(0, 256, 128, 256, font_tex); // TODO: call only before drawing text?
		//sceGuTexFunc(GU_TFX_MODULATE, GU_TCC_RGBA);
		//sceGuTexFilter(GU_NEAREST, GU_NEAREST);
		//sceGuEnable(GU_TEXTURE_2D);
		sceGuDisable(GU_TEXTURE_2D);

		sceGuShadeModel(GU_FLAT);

		sceGuDisable(GU_ALPHA_TEST);
		sceGuDisable(GU_CULL_FACE);
		sceGuDisable(GU_DEPTH_TEST);

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
