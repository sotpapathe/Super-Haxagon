#ifndef SUPER_HAXAGON_PLATFORM_WIN_HPP
#define SUPER_HAXAGON_PLATFORM_WIN_HPP
#define SFML_STATIC

#include <SFML/System.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <deque>
#include <Driver/Audio.hpp>


#include "Driver/Platform.hpp"

namespace SuperHaxagon {
	class Player;

	class PlatformWin : public Platform {
	public:
		PlatformWin();
		~PlatformWin() override;

		bool loop() override;
		double getDilation() override;

		std::string getPath(const std::string& partial) override;
		std::string getPathRom(const std::string& partial) override;
		std::unique_ptr<Audio> loadAudio(const std::string& path, Stream stream) override;
		std::unique_ptr<Font> loadFont(const std::string& path, int size) override;

		void playSFX(Audio& audio) override;
		void playBGM(Audio& audio) override;
		void stopBGM() override;

		Buttons getPressed() override;
		Point getScreenDim() const override;

		void screenBegin() override;
		void screenSwap() override;
		void screenFinalize() override;
		void drawRect(const Color& color, const Point& point, const Point& size) override;
		void drawTriangle(const Color& color, const std::array<Point, 3>& points) override;

		std::unique_ptr<Twist> getTwister() override;
		sf::RenderWindow& getWindow() const {return *window;}

	private:
		bool loaded = false;
		double delta = 0.0;
		sf::Clock clock;
		std::unique_ptr<sf::RenderWindow> window;
		std::deque<std::unique_ptr<Player>> sfx;
		std::unique_ptr<Player> bgm;
	};
}

#endif //SUPER_HAXAGON_PLATFORM_WIN_HPP
