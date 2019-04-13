#include "roids.h"

#include <math.h>

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

//#pragma comment(linker, "/export:gameGetSoundSamples")
GAME_GET_SOUND_SAMPLES(gameGetSoundSamples)
{
	struct gameState *state = (struct gameState *)memory->permanentStorage;
	outputSound(state, soundBuffer, 400);
}

static void shipReset(void)
{
	ship.verts = 3;
	ship.position.angle = 0.0f;
	ship.position.x = 0;
	ship.position.y = 0;
	ship.position.dx = 0;
	ship.position.dy = 0;
	ship.position.coords[0].x = 0.0f;
	ship.position.coords[0].y = -4.0f;
	ship.position.coords[1].x = -4.0f;
	ship.position.coords[1].y = 4.0f;
	ship.position.coords[2].x = 4.0f;
	ship.position.coords[2].y = 4.0f;
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

// convert coord into 2d column with 0,0 top left
static int offsetCol(int coord)
{
	int result = (MAX_COLS / 2) + coord;
	//if (result >= MAX_COLS)
	//	result = MAX_COLS - 1;
	//if (result < 0)
	//	result = 0;
	return result;
}

// convert coord into 2d row with 0,0 top left
static int offsetRow(int coord)
{
	int result = (MAX_ROWS / 2) + coord;
	//if (result >= MAX_ROWS)
	//	result = MAX_ROWS - 1;
	//if (result < 0)
	//	result = 0;
	return result;
}

// toroidal wrapping
//static void wrapCoordinates(float xIn, float yIn, float *xOut, float *yOut)
//{
//	if (xIn < 0.0f)
//		*xOut = xIn + MAX_COLS;
//
//	if (xIn >= MAX_COLS)
//		*xOut = xIn - MAX_COLS;
//
//	if (yIn < 0.0f)
//		*yOut = yIn + MAX_ROWS;
//
//	if (yIn >= MAX_ROWS)
//		*yOut = yIn - MAX_ROWS;
//}

//static void wrapCoordinates(float xIn, float yIn, float *xOut, float *yOut)
//{
//	if (xIn < 0.0f)
//		*xOut = xIn + WINDOW_WIDTH;
//
//	if (xIn >= WINDOW_WIDTH)
//		*xOut = xIn - WINDOW_WIDTH;
//
//	if (yIn < 0.0f)
//		*yOut = yIn + WINDOW_HEIGHT;
//
//	if (yIn >= WINDOW_HEIGHT)
//		*yOut = yIn - WINDOW_HEIGHT;
//}

//static void wrapCoordinates(int xIn, int yIn, int *xOut, int *yOut)
//{
//	if (xIn < 0)
//		*xOut = xIn + MAX_COLS;
//
//	if (xIn >= MAX_COLS)
//		*xOut = xIn - MAX_COLS;
//
//	if (yIn < 0)
//		*yOut = yIn + MAX_ROWS;
//
//	if (yIn >= MAX_ROWS)
//		*yOut = yIn - MAX_ROWS;
//}

// draw coloured square at specified position
static void blob(struct gameDisplayBuffer *buffer, int col, int row, uint32_t colour)
{
	assert(col < MAX_COLS);
	assert(row < MAX_ROWS);

	if (col >= MAX_COLS || row >= MAX_ROWS)
		return;

	//wrapCoordinates((float)col, (float)row, (float *)&col, (float *)&row);

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

static void drawDigits(struct gameDisplayBuffer *buffer, short col, short row, float v, uint32_t colour)
{
	if (v < 0)
		colour = RED;

	int value = abs((int)v);
	int hundreds = (int)value / 100;
	int tens = ((int)value % 100) / 10;
	int ones = ((int)value % 100) % 10;

	drawDigit(buffer, col, row, hundreds, colour);
	drawDigit(buffer, col + 4, row, tens, colour);
	drawDigit(buffer, col + 8, row, ones, colour);
}

static void drawDigit(struct gameDisplayBuffer *buffer, short col, short row, short digit, uint32_t colour)
{
	switch (digit)
	{
		case 0:
			blob(buffer, col, row + 0, colour); blob(buffer, col + 1, row + 0, colour); blob(buffer, col + 2, row + 0, colour);
			blob(buffer, col, row + 1, colour);											blob(buffer, col + 2, row + 1, colour);
			blob(buffer, col, row + 2, colour);											blob(buffer, col + 2, row + 2, colour);
			blob(buffer, col, row + 3, colour);											blob(buffer, col + 2, row + 3, colour);
			blob(buffer, col, row + 4, colour); blob(buffer, col + 1, row + 4, colour); blob(buffer, col + 2, row + 4, colour);
			break;
		case 1:
			blob(buffer, col + 1, row + 0, colour);
			blob(buffer, col + 1, row + 1, colour);
			blob(buffer, col + 1, row + 2, colour);
			blob(buffer, col + 1, row + 3, colour);
			blob(buffer, col + 1, row + 4, colour);
			break;
		case 2:
			blob(buffer, col, row + 0, colour); blob(buffer, col + 1, row + 0, colour); blob(buffer, col + 2, row + 0, colour);
			blob(buffer, col + 2, row + 1, colour);
			blob(buffer, col, row + 2, colour); blob(buffer, col + 1, row + 2, colour); blob(buffer, col + 2, row + 2, colour);
			blob(buffer, col, row + 3, colour);
			blob(buffer, col, row + 4, colour); blob(buffer, col + 1, row + 4, colour); blob(buffer, col + 2, row + 4, colour);
			break;
		case 3:
			blob(buffer, col, row + 0, colour); blob(buffer, col + 1, row + 0, colour); blob(buffer, col + 2, row + 0, colour);
			blob(buffer, col + 2, row + 1, colour);
			blob(buffer, col + 1, row + 2, colour); blob(buffer, col + 2, row + 2, colour);
			blob(buffer, col + 2, row + 3, colour);
			blob(buffer, col, row + 4, colour); blob(buffer, col + 1, row + 4, colour); blob(buffer, col + 2, row + 4, colour);
			break;
		case 4:
			blob(buffer, col, row + 0, colour);											blob(buffer, col + 2, row + 0, colour);
			blob(buffer, col, row + 1, colour);											blob(buffer, col + 2, row + 1, colour);
			blob(buffer, col, row + 2, colour); blob(buffer, col + 1, row + 2, colour); blob(buffer, col + 2, row + 2, colour);
			blob(buffer, col + 2, row + 3, colour);
			blob(buffer, col + 2, row + 4, colour);
			break;
		case 5:
			blob(buffer, col, row + 0, colour); blob(buffer, col + 1, row + 0, colour); blob(buffer, col + 2, row + 0, colour);
			blob(buffer, col, row + 1, colour);
			blob(buffer, col, row + 2, colour); blob(buffer, col + 1, row + 2, colour); blob(buffer, col + 2, row + 2, colour);
			blob(buffer, col + 2, row + 3, colour);
			blob(buffer, col, row + 4, colour); blob(buffer, col + 1, row + 4, colour); blob(buffer, col + 2, row + 4, colour);
			break;
		case 6:
			blob(buffer, col, row + 0, colour);
			blob(buffer, col, row + 1, colour);
			blob(buffer, col, row + 2, colour); blob(buffer, col + 1, row + 2, colour); blob(buffer, col + 2, row + 2, colour);
			blob(buffer, col, row + 3, colour);											blob(buffer, col + 2, row + 3, colour);
			blob(buffer, col, row + 4, colour); blob(buffer, col + 1, row + 4, colour); blob(buffer, col + 2, row + 4, colour);
			break;
		case 7:
			blob(buffer, col, row + 0, colour); blob(buffer, col + 1, row + 0, colour); blob(buffer, col + 2, row + 0, colour);
			blob(buffer, col + 2, row + 1, colour);
			blob(buffer, col + 2, row + 2, colour);
			blob(buffer, col + 2, row + 3, colour);
			blob(buffer, col + 2, row + 4, colour);
			break;
		case 8:
			blob(buffer, col, row + 0, colour); blob(buffer, col + 1, row + 0, colour); blob(buffer, col + 2, row + 0, colour);
			blob(buffer, col, row + 1, colour);											blob(buffer, col + 2, row + 1, colour);
			blob(buffer, col, row + 2, colour); blob(buffer, col + 1, row + 2, colour); blob(buffer, col + 2, row + 2, colour);
			blob(buffer, col, row + 3, colour);											blob(buffer, col + 2, row + 3, colour);
			blob(buffer, col, row + 4, colour); blob(buffer, col + 1, row + 4, colour); blob(buffer, col + 2, row + 4, colour);
			break;
		case 9:
			blob(buffer, col, row + 0, colour); blob(buffer, col + 1, row + 0, colour); blob(buffer, col + 2, row + 0, colour);
			blob(buffer, col, row + 1, colour);											blob(buffer, col + 2, row + 1, colour);
			blob(buffer, col, row + 2, colour); blob(buffer, col + 1, row + 2, colour); blob(buffer, col + 2, row + 2, colour);
			blob(buffer, col + 2, row + 3, colour);
			blob(buffer, col + 2, row + 4, colour);
			break;
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
		state->score = 999;
		state->lives = 3;
		state->hud = true;
		memory->isInitialized = true;
		shipReset();
	}

	for (int controllerIndex = 0; controllerIndex < arrayCount(input->controllers); ++controllerIndex)
	{
		struct gameControllerInput *controller = getController(input, controllerIndex);
		if (controller->isAnalog)
		{
			// analog movement
			//ship.position.rotation += (int)(4.0f * controller->stickAverageX);
		}
		else
		{
			// digital movement
			float turnSpeed = 2.0f;
			float moveSpeed = 20.0f;

			// update ship direction
			if (controller->moveLeft.endedDown)
				ship.position.angle -= turnSpeed * input->dtForFrame;
			if (controller->moveRight.endedDown)
				ship.position.angle += turnSpeed * input->dtForFrame;

			//TODO test toroidal mapping first
			//TODO get thrust working then remove these
			if (controller->actionUp.endedDown)
			{
				ship.position.coords[0].y -= moveSpeed * input->dtForFrame;
				ship.position.coords[1].y -= moveSpeed * input->dtForFrame;
				ship.position.coords[2].y -= moveSpeed * input->dtForFrame;

				// acceleration changes velocity over time
				//ship.position.dx += sinf(ship.position.angle) * moveSpeed * input->dtForFrame;
				//ship.position.dy += -cosf(ship.position.angle) * moveSpeed * input->dtForFrame;
			}
			if (controller->actionDown.endedDown)
			{
				ship.position.coords[0].y += moveSpeed * input->dtForFrame;
				ship.position.coords[1].y += moveSpeed * input->dtForFrame;
				ship.position.coords[2].y += moveSpeed * input->dtForFrame;
			}
			if (controller->actionLeft.endedDown)
			{
				ship.position.coords[0].x -= moveSpeed * input->dtForFrame;
				ship.position.coords[1].x -= moveSpeed * input->dtForFrame;
				ship.position.coords[2].x -= moveSpeed * input->dtForFrame;
			}
			if (controller->actionRight.endedDown)
			{
				ship.position.coords[0].x += moveSpeed * input->dtForFrame;
				ship.position.coords[1].x += moveSpeed * input->dtForFrame;
				ship.position.coords[2].x += moveSpeed * input->dtForFrame;
			}
			if (controller->back.endedDown)
			{
				shipReset();
				controller->back.endedDown = false;
			}
			if (controller->hud.endedDown)
			{
				if (state->hud)
					state->hud = false;
				else
					state->hud = true;
				controller->hud.endedDown = false;
			}
		}
	}

	// clear background
	uint32_t *dot = (uint32_t *)buffer->memory;
	for (int i = 0; i < WINDOW_HEIGHT; ++i)
		for (int j = 0; j < WINDOW_WIDTH; ++j)
			*dot++ = BLACK;

	//drawDebugLines(buffer);

	//if (state->lives == 0)
	//{
	//	state->score = 0;
	//	shipReset();
	//}

	// velocity changes position over time
	ship.position.x += ship.position.dx * input->dtForFrame;
	ship.position.y += ship.position.dy * input->dtForFrame;

	// draw models
	drawFrame(buffer, state, &ship.position, ship.verts, 1.5f, WHITE);
	//drawFrame(buffer, state, &asteroid.position, asteroid.verts, 3.0, WHITE);

	// draw data info
	drawDigits(buffer, 1, 1, state->score, WHITE);
	drawDigit(buffer, 1, 7, state->lives, YELLOW);

	if (state->hud)
	{
		//FPS
		drawDigits(buffer, 230, 1, FPS, BLUE);

		//angle
		drawDigits(buffer, 1, 15, ship.position.angle, BLUE);

		//position
		drawDigits(buffer, 1, 22, ship.position.x, CYAN);
		drawDigits(buffer, 15, 22, ship.position.y, CYAN);

		//thrust
		drawDigits(buffer, 1, 28, ship.position.dx, CYAN);
		drawDigits(buffer, 15, 28, ship.position.dy, CYAN);

		//v1
		drawDigits(buffer, 1, 35, ship.position.coords[0].x, YELLOW);
		drawDigits(buffer, 15, 35, ship.position.coords[0].y, YELLOW);

		//v2
		drawDigits(buffer, 1, 41, ship.position.coords[1].x, YELLOW);
		drawDigits(buffer, 15, 41, ship.position.coords[1].y, YELLOW);

		//v3
		drawDigits(buffer, 1, 47, ship.position.coords[2].x, YELLOW);
		drawDigits(buffer, 15, 47, ship.position.coords[2].y, YELLOW);

		//size
		drawDigits(buffer, 1, 54, MAX_COLS, ORANGE);
		drawDigits(buffer, 15, 54, MAX_ROWS, ORANGE);
	}

	// slow down gradually, temporary!
	//float slowdown = 1.0f;

	//if (ship.position.dx > 0)
	//	ship.position.dx -= slowdown * input->dtForFrame;
	//else
	//	ship.position.dx += slowdown * input->dtForFrame;

	//if (ship.position.dy > 0)
	//	ship.position.dy -= slowdown * input->dtForFrame;
	//else
	//	ship.position.dy += slowdown * input->dtForFrame;
}

// draw all vertices to form a frame
static void drawFrame(struct gameDisplayBuffer *buffer, struct gameState *state, struct Position *position, short verts, float scale, uint32_t colour)
{
	assert(verts <= MAX_VERTS);

	float new_coords[MAX_VERTS][2];

	// update rotation
	float r = position->angle;
	for (int i = 0; i < verts; ++i)
	{
		float x = position->coords[i].x;
		float y = position->coords[i].y;
		new_coords[i][0] = x * cosf(r) - y * sinf(r);
		new_coords[i][1] = x * sinf(r) + y * cosf(r);
	}

	//rotated coords
	if (state->hud)
	{
		drawDigits(buffer, 1, 61, new_coords[0][0], GREEN);
		drawDigits(buffer, 15, 61, new_coords[0][1], GREEN);
		drawDigits(buffer, 35, 61, new_coords[1][0], GREEN);
		drawDigits(buffer, 49, 61, new_coords[1][1], GREEN);
		drawDigits(buffer, 70, 61, new_coords[2][0], GREEN);
		drawDigits(buffer, 84, 61, new_coords[2][1], GREEN);
	}

	// update scale
	for (int i = 0; i < verts; ++i)
	{
		new_coords[i][0] = roundf(new_coords[i][0] * scale);
		new_coords[i][1] = roundf(new_coords[i][1] * scale);
		//new_coords[i][0] *= scale;
		//new_coords[i][1] *= scale;
	}

	//scaled coords
	if (state->hud)
	{
		drawDigits(buffer, 1, 67, new_coords[0][0], CYAN);
		drawDigits(buffer, 15, 67, new_coords[0][1], CYAN);
		drawDigits(buffer, 35, 67, new_coords[1][0], CYAN);
		drawDigits(buffer, 49, 67, new_coords[1][1], CYAN);
		drawDigits(buffer, 70, 67, new_coords[2][0], CYAN);
		drawDigits(buffer, 84, 67, new_coords[2][1], CYAN);
	}

	// translate co-ordinates
	for (int i = 0; i < verts; ++i)
	{
		new_coords[i][0] += roundf(position->x);
		new_coords[i][1] += roundf(position->y);
		//new_coords[i][0] += position->x;
		//new_coords[i][1] += position->y;
	}

	//translated coords
	if (state->hud)
	{
		drawDigits(buffer, 1, 73, new_coords[0][0], ORANGE);
		drawDigits(buffer, 15, 73, new_coords[0][1], ORANGE);
		drawDigits(buffer, 35, 73, new_coords[1][0], ORANGE);
		drawDigits(buffer, 49, 73, new_coords[1][1], ORANGE);
		drawDigits(buffer, 70, 73, new_coords[2][0], ORANGE);
		drawDigits(buffer, 84, 73, new_coords[2][1], ORANGE);
	}

	uint32_t original = colour;

	// draw vertex lines
	--verts;
	for (int i = 0; i < verts; ++i)
	{
		if (i == 1)
			colour = BLUE;
		else
			colour = original;

		line(buffer,
			(int)new_coords[i][0], (int)new_coords[i][1],
			(int)new_coords[i + 1][0], (int)new_coords[i + 1][1],
			colour);
	}

	// draw last vertex line
	line(buffer,
		(int)new_coords[verts][0], (int)new_coords[verts][1],
		(int)new_coords[0][0], (int)new_coords[0][1],
		WHITE);
}
