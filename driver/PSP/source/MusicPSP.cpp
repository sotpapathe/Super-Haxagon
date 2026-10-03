// SPDX-FileCopyrightText: 2025 Sotiris Papatheodorou
// SPDX-License-Identifier: GPL-3.0-or-later

#include "Driver/Music.hpp"

#include "Driver/Platform.hpp"

namespace SuperHaxagon {
	struct Music::MusicImpl {
	};

	Music::Music(std::unique_ptr<Music::MusicImpl> impl) : _impl(std::move(impl)) {}

	Music::~Music() = default;

	void Music::update() const {}

	void Music::setLoop(const bool loop) const {}

	void Music::play() const {}

	void Music::pause() const {}

	bool Music::isDone() const {
		return true;
	}

	float Music::getTime() const {
		return 0.0f;
	}

	std::unique_ptr<Music> createMusic(const Platform& platform, const std::string& path) {
		return nullptr;
	}
}
