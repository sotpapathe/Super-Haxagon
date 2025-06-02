// SPDX-FileCopyrightText: 2025 Sotiris Papatheodorou
// SPDX-License-Identifier: GPL-3.0-or-later

#include "Driver/Screen.hpp"

#include "Driver/Platform.hpp"

#include <array>
#include <pspdebug.h>
#include <pspdisplay.h>
#include <pspgu.h>
#include <psprtc.h>
#include <psputils.h>

#include "ColorPSP.hpp"
#include "CommonPSP.hpp"

namespace SuperHaxagon {
	struct Screen::ScreenImpl {
		ScreenImpl(bool debug) : debug(debug) {
			// The framebuffer width must be a power of two.
			constexpr unsigned fb_width = 512;
			constexpr unsigned fb_height = height;
			drawBuf = guGetStaticVramBuffer(fb_width, fb_height, PSP_FB_FMT);
			void* disp_buf = guGetStaticVramBuffer(fb_width, fb_height, PSP_FB_FMT);

			sceGuInit();
			sceGuStart(GU_DIRECT, guList);
			sceGuDrawBuffer(PSP_FB_FMT, reinterpret_cast<void*>(drawBuf), fb_width);
			sceGuDispBuffer(width, height, reinterpret_cast<void*>(disp_buf), fb_width);
			// There's no need for a depth buffer in this game.
			sceGuDepthBuffer(nullptr, 0);

			// The PSP uses a 4096x4096 virtual canvas for rendering.
			constexpr unsigned virt_width = 4096;
			constexpr unsigned virt_height = 4096;
			sceGuOffset((virt_width - width) / 2, (virt_height - height) / 2);
			sceGuViewport(virt_width / 2, virt_height / 2, width, height);

			sceGuScissor(0, 0, width, height);
			sceGuEnable(GU_SCISSOR_TEST);

			sceGuShadeModel(GU_FLAT);

			sceGuDisable(GU_ALPHA_TEST);
			sceGuDisable(GU_CULL_FACE);
			sceGuDisable(GU_DEPTH_TEST);

			// Setup font rendering from textures. Only enable
			// GU_TEXTURE_2D when drawing textures and disable it
			// afterwards.
			sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
			sceGuEnable(GU_BLEND);
			sceGuTexMode(GU_PSM_T8, 0, 0, GU_FALSE);
			sceGuTexFunc(GU_TFX_MODULATE, GU_TCC_RGBA);
			sceGuTexFilter(GU_NEAREST, GU_NEAREST);
			sceGuTexWrap(GU_CLAMP, GU_CLAMP);
			sceGuClutMode(PSP_FB_FMT, 0, 0xFF, 0);
			sceGuClutLoad(fontClut.size() / 8, fontClut.data());

			if (debug) {
				pspDebugScreenInitEx(drawBuf, PSP_FB_FMT, 1);
			}

			sceGuFinish();
			sceGuSync(GU_SYNC_FINISH, GU_SYNC_WHAT_DONE);
			sceDisplayWaitVblankStart();
			sceGuDisplay(GU_TRUE);
		}

		~ScreenImpl() {
			sceGuDisplay(GU_FALSE);
			sceGuTerm();
		}

		void screenBegin() {
			sceGuStart(GU_DIRECT, guList);
			clear(COLOR_BLACK);
			sceKernelDcacheWritebackInvalidateAll();

			if (debug) {
				pspDebugScreenSetOffset(reinterpret_cast<int>(drawBuf));
				pspDebugScreenSetXY(0, 0);
				sceRtcGetCurrentTick(&dbgStartTime);
			}
		}

		void screenFinalize() {
			// Show the percentage of the frame time that is used.
			if (debug) {
				static constexpr float budgetMs = 1000.0f / 60.0f;
				static const float ticksPerMs = sceRtcGetTickResolution() / 1000;
				uint64_t endTime;
				sceRtcGetCurrentTick(&endTime);
				const float frameMs = (endTime - dbgStartTime) / ticksPerMs;
				pspDebugScreenPrintf("%.3f%%", frameMs / budgetMs * 100.0f);
			}

			sceGuFinish();
			sceGuSync(GU_SYNC_FINISH, GU_SYNC_WHAT_DONE);
			sceDisplayWaitVblankStart();
			drawBuf = sceGuSwapBuffers();
		}

		void drawPoly(const Color& color, const std::vector<Vec2f>& points) {
			Vertex* v = reinterpret_cast<Vertex*>(
				sceGuGetMemory(points.size() * sizeof(Vertex)));
			for (size_t i = 0; i < points.size(); i++) {
				v[i].x = points[i].x;
				v[i].y = points[i].y;
				v[i].z = 0.0f;
			}
			sceGuColor(packColor<uint32_t>(color));
			sceGuDrawArray(GU_TRIANGLE_FAN,
				GU_VERTEX_32BITF | GU_TRANSFORM_2D,
				points.size(),
				nullptr,
				v);
		}

		void clear(const Color& color) {
			sceGuClearColor(packColor<uint32_t>(color));
			sceGuClear(GU_COLOR_BUFFER_BIT);
		}

		Vec2f getScreenDim() {
			return {width, height};
		}

		static constexpr unsigned width = 480;
		static constexpr unsigned height = 272;
		alignas(16) static constexpr std::array<ColorPSP, 256> fontClut = generateFontClut();

		void* drawBuf = nullptr;
		alignas(16) uint8_t guList[0x40000]; // 256 kB

		const bool debug;
		uint64_t dbgStartTime = 0;

		private:
		struct Vertex {
			float x;
			float y;
			float z;
		};
	};

	Screen::Screen(std::unique_ptr<ScreenImpl> impl) : _impl(std::move(impl)) {}

	Screen::~Screen() = default;

	Vec2f Screen::getScreenDim() const {
		return _impl->getScreenDim();
	}

	void Screen::screenBegin() const {
		return _impl->screenBegin();
	}

	// Nothing to do, the PSP has only one screen.
	void Screen::screenSwitch() const {}

	void Screen::screenFinalize() const {
		_impl->screenFinalize();
	}

	void Screen::drawPoly(const Color& color, const std::vector<Vec2f>& points) const {
		_impl->drawPoly(color, points);
	}

	void Screen::clear(const Color& color) const {
		_impl->clear(color);
	}

	std::unique_ptr<Screen> createScreen(bool debug) {
		return std::make_unique<Screen>(std::make_unique<Screen::ScreenImpl>(debug));
	}
}
