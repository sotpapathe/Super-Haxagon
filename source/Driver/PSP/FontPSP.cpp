#include "Driver/PSP/FontPSP.hpp"

#include "Core/Structs.hpp"
#include "Driver/PSP/CommonPSP.hpp"

#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_STATIC
#include <stb_truetype.h>

#include <cmath>
#include <filesystem>
#include <fstream>
#include <memory>
#include <pspgu.h>

// TODO: https://pspdev.github.io/basic_programs.html

namespace SuperHaxagon {
	FontPSP::FontPSP(const std::string& path, const float size) :
		_size(size),
		_tex_dim(texDim(size))
	{
		// Read the font into a temporary buffer.
		const std::string filename = path + ".ttf";
		std::error_code _;
		const auto file_size = std::filesystem::file_size(filename, _);
		if (file_size <= 0) {
			return;
		}
		std::unique_ptr<uint8_t[]> buf(new uint8_t[file_size]);
		std::ifstream f(filename);
		f.read(reinterpret_cast<char*>(buf.get()), file_size);
		if (!f.good()) {
			return;
		}
		// Bake the printable ASCII glyphs of the font on a texture
		// stored in VRAM.
		_tex = guGetStaticVramTexture(_tex_dim, _tex_dim, GU_PSM_8888);
		stbtt_BakeFontBitmap(
				buf.get(),
				0,
				_size,
				reinterpret_cast<unsigned char*>(_tex),
				_tex_dim,
				_tex_dim,
				_first_char,
				_num_chars,
				_chars);
	}

	float FontPSP::getHeight() const {
		return _size;
	}

	float FontPSP::getWidth(const std::string& text) const {
		float width = 0.0f;
		for (char ch : text) {
			// TODO: scale?
			width += _chars[ch - _first_char].xadvance;
		}
		return width;
	}

	void FontPSP::draw(const Color& color, const Point& position, const Alignment /* alignment */, const std::string& text) {
		// TODO: handle alignment
		// TODO: Swizzle at construction?
		sceGuTexMode(GU_PSM_8888, 0, 0, GU_FALSE);
		sceGuTexFunc(GU_TFX_DECAL, GU_TCC_RGBA);
		sceGuTexImage(0, _tex_dim, _tex_dim, _tex_dim, _tex);
		sceGuTexFilter(GU_NEAREST, GU_NEAREST);
		sceGuEnable(GU_TEXTURE_2D);
		const uint32_t c = packColor(color);
		float x = position.x;
		float y = position.y + _size;
		for (char ch : text) {
			const int i = ch - _first_char;
			stbtt_aligned_quad q;
			stbtt_GetBakedQuad(_chars + i,
					_tex_dim,
					_tex_dim,
					i,
					&x,
					&y,
					&q,
					1);
			Vertex* const v = reinterpret_cast<Vertex*>(sceGuGetMemory(2 * sizeof(Vertex)));
			v[0] = { q.s0, q.t0, c, q.x0, q.y0, 0.0f };
			v[1] = { q.s1, q.t1, c, q.x1, q.y1, 0.0f };
			sceGuDrawArray(GU_SPRITES, GU_TEXTURE_32BITF | GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_2D, 2, nullptr, v);
		}
		sceGuDisable(GU_TEXTURE_2D);
	}

	unsigned FontPSP::texDim(float font_size) {
		font_size = std::ceil(font_size);
		const float num_px = font_size * font_size * _num_chars;
		const float dim = std::sqrt(num_px);
		// Return the smallest power of 2 not smaller than dim.
		const unsigned p = std::ceil(std::log2(dim));
		return 1u << p;
	}
}
