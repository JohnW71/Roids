#include <math.h> // sinf
#include <stdint.h>
#include <stdbool.h>
#include "roids.h"

static void outputSound(struct gameState *state, struct gameSoundOutputBuffer *soundBuffer, int toneHz)
{
	int16_t toneVolume = 3000;
	int wavePeriod = soundBuffer->samplesPerSecond / toneHz;
	int16_t *sampleOut = soundBuffer->samples;

	for (int sampleIndex = 0; sampleIndex < soundBuffer->sampleCount; ++sampleIndex)
	{
		// float sineValue = sinf(state->tSine);
		// int16_t sampleValue = (int16_t)(sineValue * toneVolume);
		int16_t sampleValue = 0;
		*sampleOut++ = sampleValue;
		*sampleOut++ = sampleValue;

		// state->tSine += 2.0f * Pi32 * 1.0f / (float)wavePeriod;
		// if (state->tSine > 2.0f * Pi32)
		// 	state->tSine -= 2.0f * Pi32;
	}
}

// static void fillBuffer(struct gameDisplayBuffer *buffer, int blueOffset, int greenOffset)
// {
// 	uint8_t *row = (uint8_t *)buffer->memory;

// 	for (int y = 0; y < buffer->height; ++y)
// 	{
// 		uint32_t *pixel = (uint32_t *)row;

// 		for (int x = 0; x < buffer->width; ++x)
// 		{
// 			uint8_t blue = (uint8_t)(x + blueOffset);
// 			uint8_t green = (uint8_t)(y + greenOffset);
// 			*pixel++ = ((green << 16) | blue);
// 		}

// 		row += buffer->pitch;
// 	}
// }

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

// static void renderPlayer(struct gameDisplayBuffer *buffer, int playerX, int playerY)
// {
// 	uint8_t *endOfBuffer = (uint8_t *)buffer->memory + (uint8_t)buffer->pitch * (uint8_t)buffer->height;
// 	uint32_t colour = 0xFFFFFFFF;
// 	int top = playerY;
// 	int bottom = playerY + 10;

// 	for (int x = playerX; x < playerX + 10; ++x)
// 	{
// 		uint8_t *pixel = ((uint8_t *)buffer->memory + x * buffer->bytesPerPixel + top * buffer->pitch);

// 		for (int y = top; y < bottom; ++y)
// 		{
// 			if ((pixel >= (uint8_t *)buffer->memory) && ((pixel + (uint8_t)4) <= endOfBuffer))
// 				*(uint32_t *)pixel = colour;
// 		}

// 		pixel += buffer->pitch;
// 	}
// }

inline int32_t roundFloatToInt32(float f)
{
	int32_t result = (int32_t)(f + 0.5f);
	return result;
}

inline int32_t roundFloatToUInt32(float f)
{
	uint32_t result = (uint32_t)(f + 0.5f);
	return result;
}

// inline int32_t truncateFloatToInt32(float f)
// {
// 	int32_t result = (int32_t)f;
// 	return result;
// }

static void drawRectangle(struct gameDisplayBuffer *buffer,
						  float fMinX, float fMinY,
						  float fMaxX, float fMaxY,
						  float R, float G, float B)
{
	int32_t minX = roundFloatToInt32(fMinX);
	int32_t minY = roundFloatToInt32(fMinY);
	int32_t maxX = roundFloatToInt32(fMaxX);
	int32_t maxY = roundFloatToInt32(fMaxY);

	if (minX < 0)
		minX = 0;
	if (minY < 0)
		minY = 0;
	if (maxX > buffer->width)
		maxX = buffer->width;
	if (maxY > buffer->height)
		maxY = buffer->height;

	uint32_t colour = ((roundFloatToUInt32(R * 255.0f) << 16) |
					   (roundFloatToUInt32(G * 255.0f) << 8) |
					   (roundFloatToUInt32(B * 255.0f) << 0));

	uint8_t *row = ((uint8_t *)buffer->memory + minX * buffer->bytesPerPixel + minY * buffer->pitch);
	for (int y = minY; y < maxY; ++y)
	{
		uint32_t *pixel = (uint32_t *)row;
		for (int x = minX; x < maxX; ++x)
			*pixel++ = colour;

		row += buffer->pitch;
	}
}

//extern "C" GAME_GET_SOUND_SAMPLES(gameGetSoundSamples)
//void gameGetSoundSamples(struct threadContext *thread, struct gameMemory *memory, struct gameSoundOutputBuffer *soundBuffer)
//#pragma comment(linker, "/export:gameGetSoundSamples")
GAME_GET_SOUND_SAMPLES(gameGetSoundSamples)
{
	struct gameState *state = (struct gameState *)memory->permanentStorage;
	outputSound(state, soundBuffer, 400);
}

//extern "C" GAME_UPDATE_AND_RENDER(gameUpdateAndRender)
//void gameUpdateAndRender(struct threadContext *thread, struct gameMemory *memory, struct gameInput *input, struct gameDisplayBuffer *buffer)
//#pragma comment(linker, "/export:gameUpdateAndRender")
GAME_UPDATE_AND_RENDER(gameUpdateAndRender)
{
	// check that difference between addresses of first & last buttons matches intended count
	assert((&input->controllers[0].terminator - &input->controllers[0].buttons[0]) == (arrayCount(input->controllers[0].buttons)));

	// check state fits in available size
	assert(sizeof(struct gameState) <= memory->permanentStorageSize);

	struct gameState *state = (struct gameState *)memory->permanentStorage;
	if (!memory->isInitialized)
	{
		// state->toneHz = 512; //1000
		// state->tSine = 0.0f;
		state->playerX = 150;
		state->playerY = 150;
		memory->isInitialized = true;
	}

	for (int controllerIndex = 0; controllerIndex < arrayCount(input->controllers); ++controllerIndex)
	{
		struct gameControllerInput *controller = getController(input, controllerIndex);
		if (controller->isAnalog)
		{
			// analog movement tuning
			// state->blueOffset += (int)(4.0f * controller->stickAverageX);
			// state->toneHz = 512 + (int)(128.0f * controller->stickAverageY);
		}
		else
		{
			// digital movement tuning
			float dPlayerX = 0.0f; // pixels/second
			float dPlayerY = 0.0f; // pixels/second

			if (controller->moveUp.endedDown)
				dPlayerY = -1.0f;
			if (controller->moveDown.endedDown)
				dPlayerY = 1.0f;
			if (controller->moveLeft.endedDown)
				dPlayerX = -1.0f;
			if (controller->moveRight.endedDown)
				dPlayerX = 1.0f;

			dPlayerX *= 64.0f;
			dPlayerY *= 64.0f;

			float newPlayerX = state->playerX + input->dtForFrame*dPlayerX;
			float newPlayerY = state->playerY + input->dtForFrame*dPlayerY;

			// should check for valid movement here
			state->playerX = newPlayerX;
			state->playerY = newPlayerY;
		}
	}

	//drawRectangle(buffer, 0.0f, 0.0f, (float)buffer->width, (float)buffer->height, 0x00FF00FF);
	//drawRectangle(buffer, 10.0f, 10.0f, 40.0f, 40.0f, 0x0000FFFF);

	uint32_t tileMap[9][17] =
	{
		{1, 1, 1, 1,  1, 1, 1, 1,  0, 1, 1, 1,  1, 1, 1, 1, 1},
		{1, 1, 0, 0,  0, 1, 0, 0,  0, 0, 0, 0,  0, 1, 0, 0, 1},
		{1, 1, 0, 0,  0, 0, 0, 0,  1, 0, 0, 0,  0, 1, 1, 0, 1},
		{1, 0, 0, 0,  0, 0, 0, 0,  1, 1, 0, 0,  0, 0, 0, 0, 1},
		{0, 0, 0, 0,  0, 1, 0, 0,  1, 1, 0, 0,  0, 0, 0, 0, 0},
		{1, 1, 0, 0,  0, 1, 0, 0,  1, 0, 0, 0,  0, 1, 1, 0, 1},
		{1, 0, 0, 0,  0, 1, 0, 0,  1, 0, 0, 0,  1, 1, 0, 0, 1},
		{1, 1, 1, 1,  1, 1, 0, 0,  0, 0, 0, 0,  0, 1, 0, 0, 1},
		{1, 1, 1, 1,  1, 1, 1, 1,  0, 1, 1, 1,  1, 1, 1, 1, 1},
	};

	float upperLeftX = -30;
	float upperLeftY = 0;
	float tileWidth = 60;
	float tileHeight = 60;

	// set background
	drawRectangle(buffer, 0.0f, 0.0f, (float)buffer->width, (float)buffer->height, 0.7f, 0.7f, 0.7f);

	// draw tilemap
	//for (int row = 0; row < 9; ++row)
	//{
	//	for (int col = 0; col < 17; ++col)
	//	{
	//		uint32_t tileID = tileMap[row][col];
	//		float grey = 0.5f;
	//		if (tileID == 1)
	//			grey = 1.0f;

	//		float minX = upperLeftX + ((float)col) * tileWidth;
	//		float minY = upperLeftY + ((float)row) * tileHeight;
	//		float maxX = minX + tileWidth;
	//		float maxY = minY + tileHeight;
	//		drawRectangle(buffer, minX, minY, maxX, maxY, grey, grey, grey);
	//	}
	//}

	// draw player
	float playerR = 1.0f;
	float playerG = 0.0f;
	float playerB = 0.0f;
	float playerWidth = 0.75f * tileWidth;
	float playerHeight = tileHeight;
	float playerLeft = state->playerX - 0.5f * playerWidth;
	float playerTop = state->playerY - playerHeight;

	drawRectangle(buffer, playerLeft, playerTop,
				  playerLeft + playerWidth,
				  playerTop + playerHeight,
				  playerR, playerG, playerB);

	// all of row 0
	//int32_t *dot = (int32_t *)buffer->memory;
	//for (int i = 0; i < WINDOW_WIDTH; ++i)
	//	*dot++  = BLUE;

	int32_t *dot = (int32_t *)buffer->memory;

	int rows = 5;
	int cols = 103;

	for (int j = 0; j < rows; ++j)
	{
		int col_width = 5;
		int gap = 5;

		for (int i = 0; i < cols; ++i)
		{
			for (int i = 0; i < col_width; ++i)
				*dot++ = WHITE;
			for (int i = 0; i < gap; ++i)
				*dot++;
		}
		dot += (WINDOW_WIDTH - (cols * (gap + col_width)));
	}

	// blue line
	for (int i = 0; i < WINDOW_WIDTH; ++i)
		*dot++ = BLUE;
	//dot += (WINDOW_WIDTH - 100);

	for (int j = 0; j < rows; ++j)
	{
		int col_width = 5;
		int gap = 5;

		for (int i = 0; i < cols; ++i)
		{
			for (int i = 0; i < col_width; ++i)
				*dot++ = WHITE;
			for (int i = 0; i < gap; ++i)
				*dot++;
		}
		dot += (WINDOW_WIDTH - (cols * (gap + col_width)));
	}



	// blue line
	for (int i = 0; i < WINDOW_WIDTH; ++i)
		*dot++ = BLUE;
	//dot += (WINDOW_WIDTH - 100);

	// diagonal
	float col = 0.0f;
	uint8_t *row = (uint8_t *)buffer->memory;
	for (int y = 0; y < WINDOW_HEIGHT; ++y)
	{
		col = (y / 600.0f) * WINDOW_WIDTH;
		uint32_t *pixel = (uint32_t *)row;
		pixel += ((uint32_t) col);
		*pixel++ = GREEN;
		row += buffer->pitch;
	}

	blob(buffer, 0, 0, RED);
	blob(buffer, 10, 10, GREEN);
	blob(buffer, 20, 20, BLUE);
	blob(buffer, 30, 30, WHITE);
	blob(buffer, 40, 40, BLACK);
	blob(buffer, (WINDOW_WIDTH / 2) / COL_WIDTH, (WINDOW_HEIGHT / 2) / ROW_HEIGHT, MAGENTA);

	for (int i = 60; i < 100; ++i)
		blob(buffer, i, i, YELLOW);
	for (int i = 100; i < 200; ++i)
		blob(buffer, i, 25, CYAN);
}

static void blob(struct gameDisplayBuffer *buffer, int col, int row, int32_t colour)
{
	if (col >= MAX_COLS || row >= MAX_ROWS)
		return;

	int32_t *pixel = (int32_t *)buffer->memory;
	pixel += (row * WINDOW_WIDTH * ROW_HEIGHT) + (col * COL_WIDTH);

	for (int y = 0; y < ROW_HEIGHT; ++y)
	{
		for (int x = 0; x < COL_WIDTH; ++x)
			*pixel++ = colour;

		pixel += (WINDOW_WIDTH - COL_WIDTH);
	}
}
