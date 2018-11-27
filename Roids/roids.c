#include "roids.h"

static void fillBuffer(struct bufferInfo *buffer)
{
	if (debugging)
		outs("fillBuffer()");

	uint8_t *row = (uint8_t *)buffer->memory;

	for (int y = 0; y < buffer->height; ++y)
	{
		uint32_t *pixel = (uint32_t *)row;

		for (int x = 0; x < buffer->width; ++x)
			*pixel++ = (uint32_t)(255.0f * ((float)x / (float)buffer->width)) + y;

		row += buffer->pitch;
	}
}

void updateAndRender(struct gameMemory *memory, struct bufferInfo *buffer)
{
	if (debugging)
		outs("updateAndRender()");

	if (!memory.isInitialized)
	{
		outs("memory no longer initialized somehow");
	}

	fillBuffer(&buffer);
}

// static void shipConfig(void)
// {
// 	if (debugging)
// 		outs("shipConfig()");

// 	ship.col = (GetSystemMetrics(SM_CXSCREEN) - WINDOW_WIDTH) / 2;
// 	ship.row = (GetSystemMetrics(SM_CYSCREEN) - WINDOW_HEIGHT) / 2;
// 	ship.facing = 0;
// 	ship.trajectory = 0;
// 	ship.speed = 0;
// 	ship.acceleration = 0;
// 	ship.lives = 3;
// 	outs("Ship created");
// }
