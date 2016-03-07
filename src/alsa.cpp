#include <iostream>
#include <chrono>
#include <thread>

#include "alsa.hpp"


Audio::Audio(void *audioSrc, bool moreAccurateFramerate)
{
	audioSource = audioSrc;
	Init();
}


Audio::~Audio()
{
	Release();
}


void Audio::StartAudio()
{
	err = snd_pcm_pause(pcmHandle, 0);
	TestReturn(err, "snd_pcm_pause");
}


void Audio::StopAudio()
{
	err = snd_pcm_pause(pcmHandle, 1);
	TestReturn(err, "snd_pcm_pause");
}


void Audio::StreamSource()
{
	uint32_t queuedFrames = bufferSize - snd_pcm_avail(pcmHandle);
	while(queuedFrames > 735)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
		queuedFrames = bufferSize - snd_pcm_avail(pcmHandle);
	}

	err = snd_pcm_writei(pcmHandle, audioSource, 735);
	TestReturn(err, "snd_pcm_writei");
}


void Audio::Init()
{
	err = snd_pcm_open(&pcmHandle, "default", SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK);
	TestReturn(err, "snd_pcm_open");

	err = snd_pcm_hw_params_malloc(&params);
	TestReturn(err, "snd_pcm_hw_params_malloc");

	err = snd_pcm_hw_params_any(pcmHandle, params);
	TestReturn(err, "snd_pcm_hw_params_any");

	err = snd_pcm_hw_params_set_access(pcmHandle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
	TestReturn(err, "snd_pcm_hw_params_set_access");

	err = snd_pcm_hw_params_set_format(pcmHandle, params, SND_PCM_FORMAT_FLOAT_LE);
	TestReturn(err, "snd_pcm_hw_params_set_format");

	err = snd_pcm_hw_params_set_channels(pcmHandle, params, channels);
	TestReturn(err, "snd_pcm_hw_params_set_channels");

	err = snd_pcm_hw_params_set_rate_near(pcmHandle, params, &rate, 0);
	TestReturn(err, "snd_pcm_hw_params_set_rate_near");

	err = snd_pcm_hw_params_set_buffer_time_near(pcmHandle, params, &requestedDuration, 0);
	TestReturn(err, "snd_pcm_hw_params_set_buffer_time_near");

	err = snd_pcm_hw_params(pcmHandle, params);
	TestReturn(err, "snd_pcm_hw_params");

	err = snd_pcm_hw_params_get_buffer_size(params, &bufferSize);
	TestReturn(err, "snd_pcm_hw_params_get_buffer_size");

	std::cout << "buffersize in frames: " << bufferSize << "\n";
	if(bufferSize < 735*2)
	{
		std::cout << "HMM buffer too small";
		exit(0);
	}

	snd_pcm_hw_params_free(params);

	err = snd_pcm_prepare(pcmHandle);
	TestReturn(err, "snd_pcm_prepare");

	err = snd_pcm_pause(pcmHandle, 1);
	TestReturn(err, "snd_pcm_pause");
}


void Audio::Release()
{
	if(pcmHandle)
	{
		snd_pcm_close(pcmHandle);
	}
	if(params)
	{
		snd_pcm_hw_params_free(params);
	}
}


void Audio::TestReturn(int err, std::string functionName)
{
	if(err < 0)
	{
		std::cout << functionName << " failed: " << snd_strerror(err) << "\n";
		Release();
		exit(0);
	}
}
