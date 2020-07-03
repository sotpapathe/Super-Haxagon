#include "Driver/Player3DS.h"

namespace SuperHaxagon {
	Player3DS::Player3DS(u8* data, u32 sampleRate, u32 dataSize, u16 channels, u16 bitsPerSample, u16 ndspFormat) :
			data(data),
			sampleRate(sampleRate),
			dataSize(dataSize),
			channels(channels),
			bitsPerSample(bitsPerSample),
			ndspFormat(ndspFormat),
			controller()
	{}

	Player3DS::~Player3DS() {
		Player3DS::stop();
		ndspChnWaveBufClear(channel);
	}

	void Player3DS::play() {
		stop();
		ndspChnReset(channel);
		ndspChnWaveBufClear(channel);
		ndspChnSetInterp(channel, NDSP_INTERP_LINEAR);
		ndspChnSetRate(channel, sampleRate);
		ndspChnSetFormat(channel, ndspFormat);
		controller.data_vaddr = (void*)data;
		controller.nsamples = dataSize / bitsPerSample;
		controller.looping = loop;
		controller.offset = 0;
		DSP_FlushDataCache(data, dataSize);
		ndspChnWaveBufAdd(channel, &controller);
	}

	void Player3DS::stop() {
		ndspChnReset(channel);
	}

	bool Player3DS::isDone() {
		return controller.status == NDSP_WBUF_DONE;
	}
}