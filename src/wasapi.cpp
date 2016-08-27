#include <iostream>
#include <cstring>

#include "wasapi.hpp"

#include "mmreg.h"


Audio::Audio(void *audioSrc, bool moreAccurateFramerate)
{
	if(moreAccurateFramerate)
	{
		// for 60.0817fps, nearest whole step with 44100hz to 60.0988 (actual nes framerate)
		requestedDuration = 166440 * 2;
		frameAmount = 734;
	}
	else
	{
		// for 60fps (convenient framerate)
		requestedDuration = 166667 * 2;
		frameAmount = 735;
	}

	audioSource = audioSrc;
	Init();
}


Audio::~Audio()
{
	Release();
}


void Audio::StartAudio()
{
	hr = pAudioClient->Start();
	TestHResult(hr, "Start");
}


void Audio::StopAudio()
{
	hr = pAudioClient->Stop();
	TestHResult(hr, "Stop");
}


void Audio::StreamSource()
{
	uint32_t queuedFrames;
	pAudioClient->GetCurrentPadding(&queuedFrames);
	while(queuedFrames > frameAmount)
	{
		Sleep(1);
		pAudioClient->GetCurrentPadding(&queuedFrames);
	}

	uint8_t *pData;
	uint32_t flags = 0;
	pRenderClient->GetBuffer(frameAmount, &pData);
	std::memcpy(pData, audioSource, frameAmount * 2 * 4);
	pRenderClient->ReleaseBuffer(frameAmount, flags);
}


void Audio::Init()
{
	hr = CoInitializeEx(0, COINIT_MULTITHREADED);
	TestHResult(hr, "CoInitializeEx");

	const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
	const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
	hr = CoCreateInstance(CLSID_MMDeviceEnumerator, 0, CLSCTX_ALL, IID_IMMDeviceEnumerator, (void**)&pEnumerator);
	TestHResult(hr, "CoCreateInstance");

	hr = pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice);
	TestHResult(hr, "GetDefaultAudioEndpoint");

	const IID IID_IAudioClient = __uuidof(IAudioClient);
	hr = pDevice->Activate(IID_IAudioClient, CLSCTX_ALL, 0, (void**)&pAudioClient);
	TestHResult(hr, "Activate");

	hr = pAudioClient->GetMixFormat(&pwfx);
	TestHResult(hr, "GetMixFormat");

	hr = pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, 0, requestedDuration, 0, pwfx, 0);
	TestHResult(hr, "Initialize");

	SetFormat();

	uint32_t bufferFrameCount;
	hr = pAudioClient->GetBufferSize(&bufferFrameCount);
	TestHResult(hr, "GetBufferSize");

	if(bufferFrameCount != frameAmount*2)
	{
		int64_t actualDuration = double(referenceTimeSec) * bufferFrameCount / pwfx->nSamplesPerSec;
		std::cout << "requested " << frameAmount*2 << " frames (" << requestedDuration * 0.0001 << " ms), got "
				  << bufferFrameCount << " frames (" << actualDuration * 0.0001 << " ms)";
	}
	// uint32_t sleepTime = double(actualDuration) / referenceTimeMSec / 2;

	const IID IID_IAudioRenderClient = __uuidof(IAudioRenderClient);
	hr = pAudioClient->GetService(IID_IAudioRenderClient, (void**)&pRenderClient);
	TestHResult(hr, "GetService");
}


void Audio::SetFormat()
{
	//2 channels, 44100hz, 32bit(float)
	uint8_t channels = pwfx->nChannels;
	uint32_t sampleRate = pwfx->nSamplesPerSec;
	uint8_t bitRate = pwfx->wBitsPerSample;
	uint8_t frameSize = pwfx->nBlockAlign; // frameSize = channels * (bitRate / 8);

	bool floatPCM = false;
	if(pwfx->wFormatTag == WAVE_FORMAT_EXTENSIBLE)
	{
		WAVEFORMATEXTENSIBLE *pwfxEx = (WAVEFORMATEXTENSIBLE*)pwfx;
		if(pwfxEx->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT)
		{
			floatPCM = true;
		}
	}

	if(channels != 2 || sampleRate != 44100 || bitRate != 32 || floatPCM != true)
	{
		std::cout << "shiiet i need to be fixed to play on that\n";
	}
}


void Audio::Release()
{
	CoTaskMemFree(pwfx);
	if(pEnumerator != 0)
	{
		pEnumerator->Release();
		pEnumerator = 0;
	}
	if(pDevice != 0)
	{
		pDevice->Release();
		pDevice = 0;
	}
	if(pAudioClient != 0)
	{
		pAudioClient->Release();
		pAudioClient = 0;
	}
	if(pRenderClient != 0)
	{
		pRenderClient->Release();
		pRenderClient = 0;
	}
}


void Audio::TestHResult(HRESULT hr, std::string functionName)
{
	if(hr < 0)
	{
		std::cout << std::hex << std::uppercase << functionName << " failed. HRESULT = 0x" << hr << "\n";
		if(functionName == "GetBuffer" && hr == AUDCLNT_E_BUFFER_TOO_LARGE)
		{
			std::cout << "Requested more frames than frames available\n";
		}

		Release();
		exit(0);
	}
}
