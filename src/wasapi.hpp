#pragma once

#include <cstdint>
#include <string>

#include "audioclient.h"
#include "mmdeviceapi.h"


class Audio
{
	public:
		Audio(const void *audioSrc, bool moreAccurateFramerate);
		~Audio();
		void StartAudio();
		void StopAudio();
		void StreamSource();

	private:
		void Init();
		void SetFormat();
		void Release();
		void TestHResult(HRESULT hr, std::string functionName);

		HRESULT hr;

		IMMDeviceEnumerator *pEnumerator = 0;
		IMMDevice *pDevice = 0;
		IAudioClient *pAudioClient = 0;
		IAudioRenderClient *pRenderClient = 0;
		WAVEFORMATEX *pwfx = 0;

		int64_t requestedDuration; //1 = 100ns (REFERENCE_TIME)

		const void *audioSource;
		uint16_t frameAmount;
};
