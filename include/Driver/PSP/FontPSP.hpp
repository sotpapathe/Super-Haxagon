#ifndef SUPER_HAXAGON_FONT_PSP_HPP
#define SUPER_HAXAGON_FONT_PSP_HPP

#include "Core/Font.hpp"

namespace SuperHaxagon {
	class PlatformPSP;

	class FontPSP : public Font {
	public:
		FontPSP(const std::string& path, float size);
		~FontPSP() override = default;

		void setScale(float) override {};
		float getHeight() const override;
		float getWidth(const std::string& text) const override;
		void draw(const Color& color, const Point& position, Alignment alignment, const std::string& text) override;

	private:
		float _scale;
		float _size;
	};
}

#endif //SUPER_HAXAGON_FONT_PSP_HPP
