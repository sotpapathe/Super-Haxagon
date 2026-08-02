// SPDX-FileCopyrightText: 2025 Sotiris Papatheodorou
// SPDX-License-Identifier: GPL-3.0-or-later

#define STBTT_STATIC
#define STB_RECT_PACK_IMPLEMENTATION
#define STB_TRUETYPE_IMPLEMENTATION

#include "Driver/Font.hpp"

#include "Driver/Platform.hpp"

#include <algorithm>
#include <array>
#include <filesystem>
#include <pspgu.h>
#include <psploadexec.h>
#include <sstream>
#include <stb_rect_pack.h>
#include <stb_truetype.h>

#include "ColorPSP.hpp"
#include "CommonPSP.hpp"
#include "PromptFontPSP.hpp"

namespace SuperHaxagon {
	struct Font::FontImpl {
		FontImpl(const Platform& platform, const std::string& path, int size_) :
				size(adjustFontSize(size_)),
				_texWidth(gePowerOf2(size * _numGlyphsPerRow)),
				_texHeight(_texWidth),
				_tex(reinterpret_cast<uint8_t*>(guGetStaticVramTexture(_texWidth, _texHeight, GU_PSM_T8)))
	       	{
			stbtt_pack_context spc;
			if (!stbtt_PackBegin(&spc, _tex, _texWidth, _texHeight, 0, _padding, nullptr)) {
				platform.message(Dbg::FATAL, "font", "error initiating font packing");
				sceKernelExitGame();
			}
			const float sizef = size;

			auto buf = readFile(path);
 			if (buf.empty()) {
				platform.message(Dbg::FATAL, "font", "error reading file " + path);
				sceKernelExitGame();
			}
			// Skip ASCII glyphs that are not present in the Bump IT UP font.
			std::array<stbtt_pack_range, 7> asciiRanges = {{
				{sizef,   1, nullptr, 1,             _glyphs}, // Glyph for invalid characters.
				{sizef, ' ', nullptr, ';' - ' ' + 1, _glyphs + ' '},
				{sizef, '=', nullptr, 1,             _glyphs + '='},
				{sizef, '?', nullptr, 'Z' - '?' + 1, _glyphs + '?'},
				{sizef, '_', nullptr, 1,             _glyphs + '_'},
				{sizef, 'a', nullptr, 'z' - 'a' + 1, _glyphs + 'a'},
				{sizef, '|', nullptr, 1,             _glyphs + '|'},
			}};
			if (!stbtt_PackFontRanges(&spc, buf.data(), 0, asciiRanges.data(), asciiRanges.size())) {
				platform.message(Dbg::FATAL, "font", "invalid font data in " + path);
				sceKernelExitGame();
			}

			const std::string pathPSP = std::filesystem::path(path).parent_path() / "psp_PromptFont.ttf";
			buf = readFile(pathPSP);
 			if (buf.empty()) {
				platform.message(Dbg::FATAL, "font", "error reading file " + pathPSP);
				sceKernelExitGame();
			}
			// Map PSP button glyphs to the ASCII control
			// characters, starting from 1 (SOH) in the order that
			// they appear in this array. Only pack the actually
			// used glyphs to allow using smaller textures. Use a
			// larger font size to make the glyphs more legible.
			std::array<stbtt_pack_range, 5> pspRanges = {{
				{sizef + 8,  CPPlaystation, nullptr, 1, _glyphs + 1},
				{sizef + 5,         CPLeft, nullptr, 1, _glyphs + 2},
				{sizef + 5,        CPRight, nullptr, 1, _glyphs + 3},
				{sizef + 8,       CPCircle, nullptr, 2, _glyphs + 4},
				{sizef + 15, CPLeftTrigger, nullptr, 2, _glyphs + 6},
			}};
			if (!stbtt_PackFontRanges(&spc, buf.data(), 0, pspRanges.data(), pspRanges.size())) {
				platform.message(Dbg::FATAL, "font", "invalid font data in " + pathPSP);
				sceKernelExitGame();
			}
			// Move the D-pad left/right glyphs slightly higher
			// compared to the baseline for better alignment.
			_glyphs[2].yoff  -= 3;
			_glyphs[2].yoff2 -= 3;
			_glyphs[3].yoff  -= 2;
			_glyphs[3].yoff2 -= 2;

			stbtt_PackEnd(&spc);

			// Set all unused elements of _glyphs to describe the invalid glyph.
			std::replace_if(
				_glyphs + 1,
				_glyphs + _numGlyphs,
				[](stbtt_packedchar p) { return p.xadvance == 0; },
				_glyphs[0]);

			std::stringstream s;
			s << "baked \"" << path << "\" at " << size << "px on a "
				<< _texWidth << "x" << _texHeight << " texture ("
				<< _texWidth * _texHeight * sizeof(uint8_t) / 1024 << " kB)";
			platform.message(Dbg::INFO, "font", s.str());
		}

		float getWidth(const std::string& text) const {
			float width = 0.0f;
			for (char ch : text) {
				width += _glyphs[ch].xadvance;
			}
			return width;
		}

		void draw(const Color& color, const Vec2f& position, Alignment alignment, const std::string& text) const {
			float alignment_offset;
			switch (alignment) {
				case Alignment::CENTER:
					alignment_offset = -getWidth(text) / 2.0f;
					break;
				case Alignment::RIGHT:
					alignment_offset = -getWidth(text);
					break;
				default:
					alignment_offset = 0.0f;
			}
			const ColorPSP c = packColorPSP(color);
			float x = position.x + alignment_offset;
			float y = position.y + size;

			const size_t num_verts = 2 * text.size();
			auto* const v = reinterpret_cast<Vertex*>(sceGuGetMemory(num_verts * sizeof(Vertex)));
			for (size_t i = 0; i < text.size(); i++) {
				const char ch = text[i];
				const int charIndex = ch < _numGlyphs ? ch : 0;
				stbtt_aligned_quad q;
				stbtt_GetPackedQuad(_glyphs, _texWidth, _texHeight, charIndex, &x, &y, &q, 1);
				v[2 * i + 0] = { q.s0 * _texWidth, q.t0 * _texHeight, c, q.x0, q.y0, 0.0f };
				v[2 * i + 1] = { q.s1 * _texWidth, q.t1 * _texHeight, c, q.x1, q.y1, 0.0f };
			}
			// Batch draws to prevent flickering.
			sceGuEnable(GU_TEXTURE_2D);
			sceGuTexImage(0, _texWidth, _texHeight, _texWidth, _tex);
			sceGuDrawArray(GU_SPRITES,
				GU_TEXTURE_32BITF | PSP_GU_COLOR | GU_VERTEX_32BITF | GU_TRANSFORM_2D,
				num_verts,
				nullptr,
				v);
			sceGuDisable(GU_TEXTURE_2D);
		}

		const int size;

		private:
		struct Vertex {
			float s;
			float t;
			ColorPSP c;
			float x;
			float y;
			float z;
		};

		// All ASCII characters except DEL.
		static constexpr int _numGlyphs = '~' + 1;
		// Rough estimate of the number of glyphs at each texture row.
		static constexpr int _numGlyphsPerRow = 8;
		// Padding between glyphs in pixels. Not necessary since we're
		// not doing any interpolation but makes the textures easier to
		// debug and we don't gain any space by removing it.
		static constexpr int _padding = 1;

		// Texture dimensions must be powers of 2 on the PSP.
		const int _texWidth;
		const int _texHeight;
		uint8_t* const _tex;
		stbtt_packedchar _glyphs[_numGlyphs];

		// The Bump IT UP font looks better on heights that are
		// multiples of 10 pixels when baking using stb_truetype.h.
		// Multiples of 5 pixels look good enough, while allowing being
		// closer to the desired font size.
		static int adjustFontSize(int size) {
			// 32px -> 30px
			// 16px -> 15px
			return (size + 5/2) / 5 * 5;
		}
	};

	Font::Font(std::unique_ptr<Font::FontImpl> impl) : _impl(std::move(impl)) {}

	Font::~Font() = default;

	// There's no arbitrary font scaling on the PSP.
	void Font::setScale(float) {}

	float Font::getHeight() const {
		return _impl->size;
	}

	float Font::getWidth(const std::string& text) const {
		return _impl->getWidth(text);
	}

	void Font::draw(const Color& color, const Vec2f& position, Alignment alignment, const std::string& text) const {
		_impl->draw(color, position, alignment, text);
	}

	std::unique_ptr<Font> createFont(const Platform& platform, const std::string& path, int size) {
		return std::make_unique<Font>(std::make_unique<Font::FontImpl>(platform, path, size));
	}
}
