#include "ui_sound.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")
#endif

static int click_count{};

//16 bit mono wav: a quick decaying tone, sounds like a soft tick
static std::vector<uint8_t> makeClick(const float volume)
{
	constexpr uint32_t rate{ 22050 };
	constexpr uint32_t samples{ rate * 35 / 1000 };
	constexpr float pi{ 3.14159265f };

	std::vector<uint8_t> wav(44 + samples * 2);

	auto put16 = [&wav](const size_t at, const uint16_t v) { wav[at] = v & 0xFF; wav[at + 1] = v >> 8; };
	auto put32 = [&wav](const size_t at, const uint32_t v) { for (size_t i{}; i < 4; i++) wav[at + i] = (v >> (i * 8)) & 0xFF; };

	memcpy(wav.data(), "RIFF", 4);
	put32(4, static_cast<uint32_t>(wav.size() - 8));
	memcpy(wav.data() + 8, "WAVEfmt ", 8);
	put32(16, 16);
	put16(20, 1); //pcm
	put16(22, 1); //mono
	put32(24, rate);
	put32(28, rate * 2);
	put16(32, 2);
	put16(34, 16);
	memcpy(wav.data() + 36, "data", 4);
	put32(40, samples * 2);

	for (uint32_t i{}; i < samples; i++)
	{
		const float t{ static_cast<float>(i) / rate };
		const float s{ std::sin(2.0f * pi * 1500.0f * t) * std::exp(-t * 140.0f) * std::clamp(volume, 0.0f, 1.0f) * 0.6f };

		put16(44 + i * 2, static_cast<uint16_t>(static_cast<int16_t>(s * 32767.0f)));
	}

	return wav;
}

void playClickSound(const float volume)
{
	//PlaySound reads the buffer while it plays, so it's only rebuilt (after stopping) when the volume changes
	static std::vector<uint8_t> wav{};
	static float wav_volume{ -1.0f };

	click_count++;

	if (volume <= 0.0f) {
		return;
	}

	if (volume != wav_volume)
	{
#ifdef _WIN32
		PlaySoundA(nullptr, nullptr, 0);
#endif

		wav = makeClick(volume);
		wav_volume = volume;
	}

#ifdef _WIN32
	PlaySoundA(reinterpret_cast<LPCSTR>(wav.data()), nullptr, SND_MEMORY | SND_ASYNC | SND_NODEFAULT);
#endif
}

int clickSoundCount()
{
	return click_count;
}
