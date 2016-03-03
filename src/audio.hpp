#pragma once

#include <vector>
#include <cstdint>
#include <string>

#include "audioclient.h"
#include "mmdeviceapi.h"


class Audio
{
	public:
		Audio(void *audioSrc, bool moreAccurateFramerate);
		~Audio();
		void StartAudio();
		void StopAudio();
		void StreamSource();

	private:
		void Init();
		void SetFormat();
		void Release();
		void TestHResult(HRESULT hr, std::string functionName);

		std::vector<uint8_t> audioStream;
		uint32_t cursor = 0;
		uint8_t channels;
		uint32_t sampleRate;
		uint8_t bitRate;

		uint8_t frameSize;

		HRESULT hr;

		IMMDeviceEnumerator *pEnumerator = 0;
		IMMDevice *pDevice = 0;
		IAudioClient *pAudioClient = 0;
		IAudioRenderClient *pRenderClient = 0;
		WAVEFORMATEX *pwfx = 0;

		uint8_t *pData;
		uint32_t flags = 0;

		const uint32_t referenceTimeSec = 10000000;
		const uint32_t referenceTimeMSec = 10000;
		int64_t requestedDuration = referenceTimeSec; //1 = 100ns (REFERENCE_TIME)

		uint32_t bufferFrameCount;
		uint32_t queuedFrames;
		uint32_t numFramesAvailable;
		uint32_t sleepTime;

		void *audioSource;
		uint16_t frameAmount;
};
