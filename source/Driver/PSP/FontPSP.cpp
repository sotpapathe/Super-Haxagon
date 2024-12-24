#include "Driver/PSP/FontPSP.hpp"

namespace SuperHaxagon {
	FontPSP::FontPSP(const std::string& /* path */, const float size) :
		_scale(1),
		_size(size)
	{
		// TODO: Load font. Either load pre-rasterized version into
		// VRAM or load TTF font and rasterize in VRAM.
	}

	float FontPSP::getHeight() const {
		// TODO: Compute font height.
		return _size * _scale;
	}

	float FontPSP::getWidth(const std::string& /* text */) const {
		// TODO: Implement FontPSP::getWidth().
		return 20.0f;
	}

	void FontPSP::draw(const Color& /* color */, const Point& /* position */, const Alignment /* alignment */, const std::string& /* text */) {
		// TODO: Implement FontPSP::draw().
	}
}
