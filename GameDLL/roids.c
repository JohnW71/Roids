#include "roids.h"

#include <math.h> // sinf

static void outputSound(struct gameState *state, struct gameSoundOutputBuffer *soundBuffer, int toneHz)
{
	//int16_t toneVolume = 3000;
	//int wavePeriod = soundBuffer->samplesPerSecond / toneHz;
	int16_t *sampleOut = soundBuffer->samples;

	for (int sampleIndex = 0; sampleIndex < soundBuffer->sampleCount; ++sampleIndex)
	{
		//float sineValue = sinf(state->tSine);
		//int16_t sampleValue = (int16_t)(sineValue * toneVolume);
		int16_t sampleValue = 0;
		*sampleOut++ = sampleValue;
		*sampleOut++ = sampleValue;

		//state->tSine += 2.0f * Pi32 * 1.0f / (float)wavePeriod;
		//if (state->tSine > 2.0f * Pi32)
		//	state->tSine -= 2.0f * Pi32;
	}
}

static void shipReset(void)
{
	ship.lives = 3;
	ship.verts = 3;
	ship.trajectory = 0.0f;
	ship.speed = 0.0f;
	ship.thrust = 0.0f;
	ship.position.angle = 0.0f;
	ship.position.coords[0][0] = 0.0f;
	ship.position.coords[0][1] = 0.0f;
	ship.position.coords[1][0] = -4.0f;
	ship.position.coords[1][1] = 8.0f;
	ship.position.coords[2][0] = 4.0f;
	ship.position.coords[2][1] = 8.0f;
}

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

//TODO not worth having this? only used to fill background now?
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

//#pragma comment(linker, "/export:gameGetSoundSamples")
GAME_GET_SOUND_SAMPLES(gameGetSoundSamples)
{
	struct gameState *state = (struct gameState *)memory->permanentStorage;
	outputSound(state, soundBuffer, 400);
}

// draw coloured line from start point to end point
static void line(struct gameDisplayBuffer *buffer, int startCol, int startRow, int endCol, int endRow, uint32_t colour)
{
	startCol = offsetCol(startCol);
	startRow = offsetRow(startRow);
	endCol = offsetCol(endCol);
	endRow = offsetRow(endRow);

	if (abs(endRow - startRow) < abs(endCol - startCol))
	{
		if (startCol > endCol)
			lineLow(buffer, endCol, endRow, startCol, startRow, colour);
		else
			lineLow(buffer, startCol, startRow, endCol, endRow, colour);
	}
	else
	{
		if (startRow > endRow)
			lineHigh(buffer, endCol, endRow, startCol, startRow, colour);
		else
			lineHigh(buffer, startCol, startRow, endCol, endRow, colour);
	}
}

// draw coloured line from start point to end point
static void lineHigh(struct gameDisplayBuffer *buffer, int startCol, int startRow, int endCol, int endRow, uint32_t colour)
{
	++endRow;
	int colGap = endCol - startCol;
	int rowGap = endRow - startRow;
	int step = 1;

	if (colGap < 0)
	{
		step = -1;
		colGap = -colGap;
	}

	int difference = 2 * colGap - rowGap;
	int col = startCol;

	for (int row = startRow; row < endRow; ++row)
	{
		blob(buffer, col, row, colour);

		if (difference > 0)
		{
			col = col + step;
			difference = difference - (2 * rowGap);
		}

		difference = difference + (2 * colGap);
	}
}

// draw coloured line from start point to end point
static void lineLow(struct gameDisplayBuffer *buffer, int startCol, int startRow, int endCol, int endRow, uint32_t colour)
{
	++endCol;
	int colGap = endCol - startCol;
	int rowGap = endRow - startRow;
	int step = 1;

	if (rowGap < 0)
	{
		step = -1;
		rowGap = -rowGap;
	}

	int difference = 2 * rowGap - colGap;
	int row = startRow;

	for (int col = startCol; col < endCol; ++col)
	{
		blob(buffer, col, row, colour);

		if (difference > 0)
		{
			row = row + step;
			difference = difference - (2 * colGap);
		}

		difference = difference + (2 * rowGap);
	}
}

// convert col number into 2d space value
static int offsetCol(int coord)
{
	int result = (MAX_COLS / 2) + coord;
	if (result >= MAX_COLS)
		result = MAX_COLS-1;
	if (result < 0)
		result = 0;
	return result;
}

// convert row number into 2d space value
static int offsetRow(int coord)
{
	int result = (MAX_ROWS / 2) + coord;
	if (result >= MAX_ROWS)
		result = MAX_ROWS-1;
	if (result < 0)
		result = 0;
	return result;
}

// draw coloured square at specified position
static void blob(struct gameDisplayBuffer *buffer, int col, int row, uint32_t colour)
{
	if (col >= MAX_COLS || row >= MAX_ROWS)
		return;

	uint32_t *pixel = (uint32_t *)buffer->memory;
	pixel += (row * WINDOW_WIDTH * ROW_HEIGHT) + (col * COL_WIDTH);

	for (int y = 0; y < ROW_HEIGHT; ++y)
	{
		for (int x = 0; x < COL_WIDTH; ++x)
			*pixel++ = colour;

		pixel += (WINDOW_WIDTH - COL_WIDTH);
	}
}

static void drawDebugLines(struct gameDisplayBuffer *buffer)
{
	// draw red line all of row 0
	uint32_t *dot = (uint32_t *)buffer->memory;
	for (int i = 0; i < WINDOW_WIDTH; ++i)
		*dot++ = RED;

	// draw 5 rows of columns with gaps
	int rows = 5;
	int cols = 103;
	for (int j = 0; j < rows; ++j)
	{
		int col_width = 5;
		int gap = 5;

		for (int i = 0; i < cols; ++i)
		{
			for (int k = 0; k < col_width; ++k)
				*dot++ = WHITE;
			for (int l = 0; l < gap; ++l)
				dot++;
		}
		dot += (WINDOW_WIDTH - (cols * (gap + col_width)));
	}

	// blue line all width
	for (int i = 0; i < WINDOW_WIDTH; ++i)
		*dot++ = BLUE;

	// draw 5 rows of columns with gaps
	for (int j = 0; j < rows; ++j)
	{
		int col_width = 5;
		int gap = 5;

		for (int i = 0; i < cols; ++i)
		{
			for (int k = 0; k < col_width; ++k)
				*dot++ = WHITE;
			for (int l = 0; l < gap; ++l)
				dot++;
		}
		dot += (WINDOW_WIDTH - (cols * (gap + col_width)));
	}

	// yellow line all width
	for (int i = 0; i < WINDOW_WIDTH; ++i)
		*dot++ = YELLOW;

	// draw full diagonal line, single pixel
	uint8_t *row = (uint8_t *)buffer->memory;
	for (int y = 0; y < WINDOW_HEIGHT; ++y)
	{
		float col = (y / 600.0f) * WINDOW_WIDTH;
		uint32_t *pixel = (uint32_t *)row;
		pixel += ((uint32_t)col);
		*pixel++ = GREEN;
		row += buffer->pitch;
	}
}

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
		//state->toneHz = 512; //1000
		//state->tSine = 0.0f;
		memory->isInitialized = true;
		shipReset();
	}

	for (int controllerIndex = 0; controllerIndex < arrayCount(input->controllers); ++controllerIndex)
	{
		struct gameControllerInput *controller = getController(input, controllerIndex);
		if (controller->isAnalog)
		{
			// analog movement
			//state->blueOffset += (int)(4.0f * controller->stickAverageX);
			//state->toneHz = 512 + (int)(128.0f * controller->stickAverageY);

			//ship.position.rotation += (int)(4.0f * controller->stickAverageX);
		}
		else
		{
			// digital movement

			// update ship direction
			float turnSpeed = 2.0f;
			if (controller->moveLeft.endedDown)
				ship.position.angle -= turnSpeed * input->dtForFrame;
			if (controller->moveRight.endedDown)
				ship.position.angle += turnSpeed * input->dtForFrame;

			//TODO handle wrapping off edge of screen
			//TODO movement should only be thrust in facing direction
			float moveSpeed = 20.0f;
			if (controller->actionUp.endedDown)
			{
				ship.position.coords[0][1] -= moveSpeed * input->dtForFrame;
				ship.position.coords[1][1] -= moveSpeed * input->dtForFrame;
				ship.position.coords[2][1] -= moveSpeed * input->dtForFrame;
			}
			if (controller->actionDown.endedDown)
			{
				ship.position.coords[0][1] += moveSpeed * input->dtForFrame;
				ship.position.coords[1][1] += moveSpeed * input->dtForFrame;
				ship.position.coords[2][1] += moveSpeed * input->dtForFrame;
			}
			if (controller->actionLeft.endedDown)
			{
				ship.position.coords[0][0] -= moveSpeed * input->dtForFrame;
				ship.position.coords[1][0] -= moveSpeed * input->dtForFrame;
				ship.position.coords[2][0] -= moveSpeed * input->dtForFrame;
			}
			if (controller->actionRight.endedDown)
			{
				ship.position.coords[0][0] += moveSpeed * input->dtForFrame;
				ship.position.coords[1][0] += moveSpeed * input->dtForFrame;
				ship.position.coords[2][0] += moveSpeed * input->dtForFrame;
			}
			if (controller->back.endedDown)
			{
				shipReset();
			}
		}
	}

	// set background
	//TODO replace this with manual fill instead?
	drawRectangle(buffer, 0.0f, 0.0f, (float)buffer->width, (float)buffer->height, 0.7f, 0.7f, 0.7f);

	//drawDebugLines(buffer);

	if (ship.lives == 0)
		shipReset();

	// draw ship model
	drawFrame(buffer, &ship.position, ship.verts, 1.0, BLUE);

	//TODO not configured yet
	//drawFrame(buffer, &asteroid.position, asteroid.verts, 3.0, WHITE);
}

// draw all vertices to form a frame
//TODO add a scale parameter?
//TODO there is a bug drawing the lines at centre position
static void drawFrame(struct gameDisplayBuffer *buffer, struct Position *position, short verts, float scale, uint32_t colour)
{
	assert(verts <= MAX_VERTS);

	float new_coords[MAX_VERTS][2];

	// update rotation
	float r = position->angle;
	for (int i = 0; i < verts; ++i)
	{
		float x = position->coords[i][0];
		float y = position->coords[i][1];
		new_coords[i][0] = x * cosf(r) - y * sinf(r);
		new_coords[i][1] = x * sinf(r) + y * cosf(r);
	}

	// update scale
	for (int i = 0; i < verts; ++i)
	{
		new_coords[i][0] *= scale;
		new_coords[i][1] *= scale;
	}

	// draw all vertex lines
	--verts;
	for (int i = 0; i < verts; ++i)
		line(buffer, 
			roundFloatToInt32(new_coords[i][0]), roundFloatToInt32(new_coords[i][1]),
			roundFloatToInt32(new_coords[i+1][0]), roundFloatToInt32(new_coords[i+1][1]),
			colour);

	// draw last vertex line
	line(buffer, 
		roundFloatToInt32(new_coords[verts][0]), roundFloatToInt32(new_coords[verts][1]),
		roundFloatToInt32(new_coords[0][0]), roundFloatToInt32(new_coords[0][1]),
		BLACK);
}
