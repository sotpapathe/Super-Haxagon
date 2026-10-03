// SPDX-FileCopyrightText: 2025 Sotiris Papatheodorou
// SPDX-License-Identifier: GPL-3.0-or-later

#include "Driver/Sound.hpp"

#include "Driver/Platform.hpp"

namespace SuperHaxagon {
	struct Sound::SoundImpl {
	};

	Sound::Sound(std::unique_ptr<SoundImpl> impl) : _impl(std::move(impl)) {}

	Sound::~Sound() = default;

	void Sound::play() const {}

	std::unique_ptr<Sound> createSound(const Platform& platform, const std::string& path) {
		return nullptr;
	}
}
