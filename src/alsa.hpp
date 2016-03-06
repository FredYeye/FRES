#pragma once

#include <alsa/asoundlib.h>


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
		void Release();
		void TestReturn(int err, std::string functionName);

		snd_pcm_t *pcmHandle = 0;
		snd_pcm_hw_params_t *params = 0;
		void *audioSource;

		int err;
		uint32_t rate = 44100;
		uint32_t requestedDuration = 17000 * 4;
		uint32_t bufferSize;
		uint8_t channels = 2;
};