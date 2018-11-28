#include <math.h> // sinf
#include <stdint.h>
#include <stdbool.h>
#include "roids.h"

void updateAndRender(struct gameMemory *memory, 
					struct gameDisplayBuffer *buffer,
					struct gameSoundOutputBuffer *soundBuffer,
					int yOffset)
{
	if (!memory->isInitialized)
	{
		outs("memory no longer initialized somehow");
		return;
	}

	struct gameState *state = (struct gameState *)memory;
	state->toneHz = 1000;
	outputSound(soundBuffer, state->toneHz);
	fillBuffer(buffer, yOffset);
}

static void outputSound(struct gameSoundOutputBuffer *soundBuffer, int toneHz)
{
	static float tSine;
	int16_t toneVolume = 3000;
	int wavePeriod = soundBuffer->samplesPerSecond / toneHz;
	int16_t *sampleOut = soundBuffer->samples;

	for (int sampleIndex = 0; sampleIndex < soundBuffer->sampleCount; ++sampleIndex)
	{
		float sineValue = sinf(tSine);
		int16_t sampleValue = (int16_t)(sineValue * toneVolume);
		*sampleOut++ = sampleValue;
		*sampleOut++ = sampleValue;

		tSine += 2.0f * Pi32 * 1.0f / (float)wavePeriod;
	}
}

static void fillBuffer(struct gameDisplayBuffer *buffer, int yOffset)
{
	uint8_t *row = (uint8_t *)buffer->memory;

	for (int y = 0; y < buffer->height; ++y)
	{
		uint32_t *pixel = (uint32_t *)row;

		for (int x = 0; x < buffer->width; ++x)
			*pixel++ = (uint32_t)(255.0f * ((float)x / (float)buffer->width)) + yOffset;// y;

		row += buffer->pitch;
	}
}

// static void shipConfig(void)
// {
// 	ship.col = (GetSystemMetrics(SM_CXSCREEN) - WINDOW_WIDTH) / 2;
// 	ship.row = (GetSystemMetrics(SM_CYSCREEN) - WINDOW_HEIGHT) / 2;
// 	ship.facing = 0;
// 	ship.trajectory = 0;
// 	ship.speed = 0;
// 	ship.acceleration = 0;
// 	ship.lives = 3;
// 	outs("Ship created");
// }
