#include <iostream>
#include <cstring>

#include "audio.hpp"
#include "file.hpp"

#include "mmreg.h"


Audio::Audio(std::string fileName)
{
	audioStream = FileToU8Vec(fileName);
	Init();
}


Audio::Audio(std::vector<uint8_t> &audioData)
{
	audioStream = audioData;
	Init();
}


Audio::Audio(void *audioSrc, int64_t requestedBufferTime)
{
	audioSource = audioSrc;
	requestedDuration = requestedBufferTime * 4;
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


void Audio::Stream()
{
	while(flags != AUDCLNT_BUFFERFLAGS_SILENT)
	{
		hr = pAudioClient->GetCurrentPadding(&queuedFrames);
		TestHResult(hr, "GetCurrentPadding");

		numFramesAvailable = bufferFrameCount - queuedFrames;

		hr = pRenderClient->GetBuffer(numFramesAvailable, &pData);
		TestHResult(hr, "GetBuffer");
		LoadData();
		hr = pRenderClient->ReleaseBuffer(numFramesAvailable, flags);
		TestHResult(hr, "ReleaseBuffer");

		Sleep(sleepTime);
	}
	flags = 0;
}


void Audio::StreamSource()
{
	pAudioClient->GetCurrentPadding(&queuedFrames);
	while(queuedFrames > 734*3)
	{
		Sleep(1);
		pAudioClient->GetCurrentPadding(&queuedFrames);
	}

	pRenderClient->GetBuffer(734, &pData);
	std::memcpy(pData, audioSource, 734*2*4);
	pRenderClient->ReleaseBuffer(734, flags);
}


void Audio::LoadData()
{
	const uint32_t requestAmount = frameSize * numFramesAvailable;
	const uint32_t remainingAudio = audioStream.size() - cursor;
	const uint8_t *audioOffset = audioStream.data() + cursor;

	if(remainingAudio >= requestAmount)
	{
		std::memcpy(pData, audioOffset, requestAmount);
		cursor += requestAmount;
	}
	else
	{
		if(!remainingAudio)
		{
			flags = AUDCLNT_BUFFERFLAGS_SILENT;
			return;
		}

		std::memcpy(pData, audioOffset, remainingAudio);
		std::memset(pData + remainingAudio, 0, requestAmount - remainingAudio);
		cursor = audioStream.size();
	}
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

	hr = pAudioClient->GetBufferSize(&bufferFrameCount);
	TestHResult(hr, "GetBufferSize");

	int64_t actualDuration = double(referenceTimeSec) * bufferFrameCount / pwfx->nSamplesPerSec;
	sleepTime = double(actualDuration) / referenceTimeMSec / 2;

	// std::cout << "requested " << requestedDuration * 0.0001 << "ms, actual duration: " << actualDuration * 0.0001 << "ms\n";

	const IID IID_IAudioRenderClient = __uuidof(IAudioRenderClient);
	hr = pAudioClient->GetService(IID_IAudioRenderClient, (void**)&pRenderClient);
	TestHResult(hr, "GetService");
}


void Audio::SetFormat()
{
	//2 channels, 44100hz, 32bit(float)
	channels = pwfx->nChannels;
	sampleRate = pwfx->nSamplesPerSec;
	bitRate = pwfx->wBitsPerSample;
	frameSize = pwfx->nBlockAlign; // frameSize = channels * (bitRate / 8);

	bool floatPCM = false;
	if(pwfx->wFormatTag == WAVE_FORMAT_EXTENSIBLE)
	{
		WAVEFORMATEXTENSIBLE *pwfxEx = (WAVEFORMATEXTENSIBLE*)pwfx;
		if(pwfxEx->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT)
		{
			floatPCM = true;
		}
	}

	// std::cout << pwfx->wFormatTag << "\n"
			  // << pwfx->nAvgBytesPerSec << "\n"
			  // << pwfx->cbSize << "\n";

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
