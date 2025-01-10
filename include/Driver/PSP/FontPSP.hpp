#ifndef SUPER_HAXAGON_FONT_PSP_HPP
#define SUPER_HAXAGON_FONT_PSP_HPP

#include "Core/Font.hpp"

#include <stb_truetype.h>
#include <stdint.h>

namespace SuperHaxagon {
	class PlatformPSP;

	class FontPSP : public Font {
	public:
		FontPSP(const std::string& path, float size);
		~FontPSP() override = default;

		// There's no need for arbitrary font scaling on the PSP.
		void setScale(float) override {};
		float getHeight() const override;
		float getWidth(const std::string& text) const override;
		void draw(const Color& color, const Point& position, Alignment alignment, const std::string& text) override;

	private:
		struct Vertex {
			float u;
			float v;
			uint32_t c;
			float x;
			float y;
			float z;
		};

		static constexpr unsigned _first_char = ' ';
		// The number of printable ASCII characters.
		static constexpr unsigned _num_chars = '~' - _first_char + 1;

		static unsigned texDim(float font_size);

		const float _size;
		const unsigned _tex_dim;
		void* _tex = nullptr;
		stbtt_bakedchar _chars[_num_chars];
	};
}

#endif //SUPER_HAXAGON_FONT_PSP_HPP
