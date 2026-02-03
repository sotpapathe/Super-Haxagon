// SPDX-FileCopyrightText: 2025 Sotiris Papatheodorou
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SUPER_HAXAGON_PSP_AUDIO_FILE_PSP_HPP
#define SUPER_HAXAGON_PSP_AUDIO_FILE_PSP_HPP

#include "Driver/Platform.hpp"

#include <memory>
#include <stdint.h>
#include <string>

namespace SuperHaxagon {
	// A 2 channel 16-bit PCM sample, as required by the PSP.
	struct Sample {
		int16_t left = 0;
		int16_t right = 0;
	};

	// Return samples rounded-up to a multiple of PSP_NUM_AUDIO_SAMPLES.
	uint32_t roundSamples(uint32_t samples);

	// Interface for reading 44.1 kHz 2 channel 16-bit PCM audio data.
	struct AudioFile {
		// Read bufferSamples audio samples into buffer. If fewer than
		// bufferSamples but more than 0 samples remain, the rest of
		// the buffer will be filled with zeros. Returns the number of
		// samples read (excluding padding samples), or 0 on EOF, or a
		// negative value on error.
		virtual long read(Sample* buffer, uint32_t bufferSamples) = 0;

		// Return the duration of audio in seconds that has been read
		// from the beginning of the file.
		virtual float getTime() const = 0;

		// Rewind the file so that read() will start reading from the
		// beginning. Returns whether the operation succeeded.
		virtual bool rewind() = 0;

		// Return the number of audio samples contained in the file, or
		// 0 if the file couldn't be read or is invalid.
		virtual uint32_t numSamples() const = 0;

		// Return the sample rate in Hz.
		virtual uint32_t sampleRate() const = 0;

		// Return the path to the file being read.
		virtual const std::string& path() const = 0;
	};

	// Attempt to read the audio file whose filename is pathNoExt
	// concatenated with one of the following file extensions, in order:
	// * ".ogg", for Vorbis audio
	// * ".wav", for WAVE audio
	// The function returns on the first file that is successfully read. If
	// no file is successfully read it returns an uninitialized
	// std::unique_ptr.
	std::unique_ptr<AudioFile> createAudioFile(const Platform& platform, const std::string& pathNoExt);
}

#endif // SUPER_HAXAGON_PSP_AUDIO_FILE_PSP_HPP
