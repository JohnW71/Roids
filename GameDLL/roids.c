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

static void gameReload(struct gameState *state)
{
	--state->lives;
	shipReset();
	bulletsReset();
	asteroidsReset();
}

static void gameReset(struct gameState *state)
{
	state->score = 0;
	state->lives = 3;
	state->asteroids = 2;
	state->hud = true;
	state->fps = true;
	shipReset();
	bulletsReset();
	asteroidsReset();
}

static void shipReset(void)
{
	ship.verts = 3;
	ship.position.angle = 0.0f;
	ship.position.x = 0;
	ship.position.y = 0;
	ship.position.dx = 0;
	ship.position.dy = 0;
	ship.position.vectors[0].x = 0.0f;
	ship.position.vectors[0].y = -4.0f;
	ship.position.vectors[1].x = -4.0f;
	ship.position.vectors[1].y = 4.0f;
	ship.position.vectors[2].x = 4.0f;
	ship.position.vectors[2].y = 4.0f;
}

static void bulletsReset(void)
{
	for (int i = 0; i < MAX_BULLETS; ++i)
		bulletReset(i);
}

static void bulletReset(int i)
{
	bullets[i].alive = false;
	bullets[i].position.angle = 0;
	bullets[i].position.dx = 0;
	bullets[i].position.dy = 0;
}

static void asteroidsReset(void)
{
	for (int i = 0; i < MAX_ASTEROIDS; ++i)
		asteroidReset(i);
}

static void asteroidReset(int i)
{
	asteroids[i].alive = false;
	asteroids[i].size = 0.0f;
	asteroids[i].verts = 0;
	asteroids[i].position.angle = 0.0f;
	asteroids[i].position.x = 0;
	asteroids[i].position.y = 0;
	asteroids[i].position.dx = 0;
	asteroids[i].position.dy = 0;

	for (int v = 0; v < MAX_VERTS; ++v)
	{
		asteroids[i].position.vectors[v].x = 0.0f;
		asteroids[i].position.vectors[v].y = 0.0f;
	}
}

static short countAsteroids()
{
	short c = 0;
	for (int i = 0; i < MAX_ASTEROIDS; ++i)
		if (asteroids[i].alive)
			++c;
	return c;
}

static void createAsteroid(short parent, float dtForFrame)
{
	for (int child = parent + 1; child < MAX_ASTEROIDS; ++child)
	{
		short sizeVariance;

		if (!asteroids[child].alive)
		{
			if (asteroids[parent].size == ASTEROID_BIG)
			{
				asteroids[child].size = ASTEROID_MED;
				asteroids[child].verts = ASTEROID_MED_VERTS;
				sizeVariance = 5;
			}
			else if (asteroids[parent].size == ASTEROID_MED)
			{
				asteroids[child].size = ASTEROID_SMALL;
				asteroids[child].verts = ASTEROID_SMALL_VERTS;
				sizeVariance = 3;
			}
			asteroids[child].alive = true;
			asteroids[child].position.angle = (float)(rand() % 360);
			asteroids[child].position.x = asteroids[parent].position.x;
			asteroids[child].position.y = asteroids[parent].position.y;
			asteroids[child].position.dx = sinf(asteroids[child].position.angle) * ASTEROID_SPEED * dtForFrame;
			asteroids[child].position.dy = cosf(asteroids[child].position.angle) * ASTEROID_SPEED * dtForFrame;

			// generate vectors
			for (int v = 0; v < asteroids[child].verts; ++v)
			{
				short variance = rand() % sizeVariance;
				float vector = ((float)v / (float)asteroids[child].verts) * (2 * Pi32);
				asteroids[child].position.vectors[v].x = (float)asteroids[child].size * sinf(vector) + (float)variance;
				asteroids[child].position.vectors[v].y = (float)asteroids[child].size * cosf(vector) + (float)variance;
			}
			return;
		}
	}
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
	//if (MAX_COLS % 2 != 0)
	//	++result;
	return result;
}

// convert coord into 2d row with 0,0 top left
static int offsetRow(int coord)
{
	int result = (MAX_ROWS / 2) + coord;
	//if (MAX_ROWS % 2 != 0)
	//	++result;
	return result;
}

// toroidal wrapping
static void wrapCoordinates(int xIn, int yIn, int *xOut, int *yOut)
{
	if (xIn < 0)
		*xOut = xIn + MAX_COLS;

	if (xIn > MAX_COLS)
		*xOut = xIn - MAX_COLS;

	if (yIn < 0)
		*yOut = yIn + MAX_ROWS;

	if (yIn > MAX_ROWS)
		*yOut = yIn - MAX_ROWS;
}

static void wrapModel(struct Position *position)
{
	if (position->x < 0.0f)
		position->x += MAX_COLS;

	if (position->x > (MAX_COLS / 2))
		position->x -= MAX_COLS;

	if (position->y < 0.0f)
		position->y += MAX_ROWS;

	if (position->y > (MAX_ROWS / 2))
		position->y -= MAX_ROWS;
}

// draw coloured square at specified position
static void blob(struct gameDisplayBuffer *buffer, int col, int row, uint32_t colour)
{
	wrapCoordinates(col, row, &col, &row);

	if (row <= -MAX_ROWS)
		row += MAX_ROWS-1;

	if (row >= MAX_ROWS)
		row = MAX_ROWS-1;

	uint32_t *pixel = (uint32_t *)buffer->memory;
	pixel += (row * WINDOW_WIDTH * ROW_HEIGHT) + (col * COL_WIDTH);

	for (int y = 0; y < ROW_HEIGHT; ++y)
	{
		for (int x = 0; x < COL_WIDTH; ++x)
			*pixel++ = colour;

		pixel += (WINDOW_WIDTH - COL_WIDTH);
	}
}

static bool collisionDetected(float circleX, float circleY, float radius, float pointX, float pointY)
{
	return sqrt((pointX - circleX) * (pointX - circleX) + (pointY - circleY) * (pointY - circleY)) < radius;
}

//static void drawDebugLines(struct gameDisplayBuffer *buffer)
//{
//	// draw red line all of row 0
//	uint32_t *dot = (uint32_t *)buffer->memory;
//	for (int i = 0; i < WINDOW_WIDTH; ++i)
//		*dot++ = RED;
//
//	// draw 5 rows of columns with gaps
//	int rows = 5;
//	int cols = 103;
//	for (int j = 0; j < rows; ++j)
//	{
//		int col_width = 5;
//		int gap = 5;
//
//		for (int i = 0; i < cols; ++i)
//		{
//			for (int k = 0; k < col_width; ++k)
//				*dot++ = WHITE;
//			for (int l = 0; l < gap; ++l)
//				dot++;
//		}
//		dot += (WINDOW_WIDTH - (cols * (gap + col_width)));
//	}
//
//	// blue line all width
//	for (int i = 0; i < WINDOW_WIDTH; ++i)
//		*dot++ = BLUE;
//
//	// draw 5 rows of columns with gaps
//	for (int j = 0; j < rows; ++j)
//	{
//		int col_width = 5;
//		int gap = 5;
//
//		for (int i = 0; i < cols; ++i)
//		{
//			for (int k = 0; k < col_width; ++k)
//				*dot++ = WHITE;
//			for (int l = 0; l < gap; ++l)
//				dot++;
//		}
//		dot += (WINDOW_WIDTH - (cols * (gap + col_width)));
//	}
//
//	// yellow line all width
//	for (int i = 0; i < WINDOW_WIDTH; ++i)
//		*dot++ = YELLOW;
//
//	// draw full diagonal line, single pixel
//	uint8_t *row = (uint8_t *)buffer->memory;
//	for (int y = 0; y < WINDOW_HEIGHT; ++y)
//	{
//		float col = (y / 600.0f) * WINDOW_WIDTH;
//		uint32_t *pixel = (uint32_t *)row;
//		pixel += ((uint32_t)col);
//		*pixel++ = GREEN;
//		row += buffer->pitch;
//	}
//}

static void drawDigits(struct gameDisplayBuffer *buffer, short col, short row, float v, uint32_t colour)
{
	if (v < 0)
		colour = RED;

	int value = abs((int)v);
	int thousands = (int)value / 1000;
	int hundreds = (int)(value % 1000) / 100;
	int tens = ((int)(value % 1000) % 100) / 10;
	int ones = ((int)(value % 1000) % 100) % 10;

	drawDigit(buffer, col, row, thousands, colour);
	drawDigit(buffer, col + 4, row, hundreds, colour);
	drawDigit(buffer, col + 8, row, tens, colour);
	drawDigit(buffer, col + 12, row, ones, colour);
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
		memory->isInitialized = true;
		srand((unsigned int)time(NULL));
		//state->toneHz = 512;
		//state->tSine = 0.0f;
		gameReset(state);
	}

	for (int controllerIndex = 0; controllerIndex < arrayCount(input->controllers); ++controllerIndex)
	{
		struct gameControllerInput *controller = getController(input, controllerIndex);
		if (controller->isAnalog)
		{
			//ship.position.rotation += (int)(4.0f * controller->stickAverageX);
		}
		else
		{
			// update ship direction
			if (controller->moveLeft.endedDown || controller->actionLeft.endedDown)
				ship.position.angle -= TURN_SPEED * input->dtForFrame;
			if (controller->moveRight.endedDown || controller->actionRight.endedDown)
				ship.position.angle += TURN_SPEED * input->dtForFrame;

			// thrust
			if (controller->actionUp.endedDown)
			{
				// acceleration changes velocity over time
				ship.position.dx += sinf(ship.position.angle) * MOVE_SPEED * input->dtForFrame;
				ship.position.dy += -cosf(ship.position.angle) * MOVE_SPEED * input->dtForFrame;
			}

			// reset
			if (controller->reset.endedDown)
			{
				gameReset(state);
				controller->reset.endedDown = false;
			}

			// shoot
			if (controller->back.endedDown)
			{
				for (int i = 0; i < MAX_BULLETS; ++i)
				{
					if (!bullets[i].alive)
					{
						bullets[i].alive = true;
						bullets[i].position.angle = ship.position.angle;
						bullets[i].position.dx = ship.position.x;
						bullets[i].position.dy = ship.position.y;
						break;
					}
				}
				controller->back.endedDown = false;
			}

			// hud
			if (controller->hud.endedDown)
			{
				if (state->hud)
					state->hud = false;
				else
					state->hud = true;
				controller->hud.endedDown = false;
			}

			// fps
			if (controller->fps.endedDown)
			{
				if (state->fps)
					state->fps = false;
				else
					state->fps = true;
				controller->fps.endedDown = false;
			}
		}
	}

	// clear background
	uint32_t *dot = (uint32_t *)buffer->memory;
	for (int i = 0; i < WINDOW_HEIGHT; ++i)
		for (int j = 0; j < WINDOW_WIDTH; ++j)
			*dot++ = BLACK;

	//drawDebugLines(buffer);

	// game over
	if (state->lives == 0)
	{
		gameReset(state);
	}

	// ship velocity changes position over time
	ship.position.x += ship.position.dx * input->dtForFrame;
	ship.position.y += ship.position.dy * input->dtForFrame;
	
	// re-wrap co-ordinates to prevent ever larger positions
	wrapModel(&ship.position);

	// update bullets
	for (int i = 0; i < MAX_BULLETS; ++i)
		if (bullets[i].alive)
		{
			bullets[i].position.dx += sinf(bullets[i].position.angle) * BULLET_SPEED * input->dtForFrame;
			bullets[i].position.dy += -cosf(bullets[i].position.angle) * BULLET_SPEED * input->dtForFrame;

			// if off screen remove it
			if (offsetCol((int)bullets[i].position.dx) < 0 || offsetCol((int)bullets[i].position.dx) > MAX_COLS ||
				offsetRow((int)bullets[i].position.dy) < 0 || offsetRow((int)bullets[i].position.dy) > MAX_ROWS)
				bulletReset(i);
		}

	// create new level asteroids
	if (countAsteroids() == 0)
	{
		for (int i = 0; i < state->asteroids; ++i)
		{
			asteroids[i].alive = true;
			asteroids[i].size = ASTEROID_BIG;
			asteroids[i].verts = ASTEROID_BIG_VERTS;
			asteroids[i].position.angle = (float)(rand() % 360);
			asteroids[i].position.x = (float)(rand() % MAX_COLS) - (MAX_COLS / 2);
			asteroids[i].position.y = (float)(rand() % MAX_ROWS) - (MAX_ROWS / 2);

			// prevent starting on top of ship
			while (collisionDetected(asteroids[i].position.x, asteroids[i].position.y,
				asteroids[i].size*3,
				ship.position.x, ship.position.y))
			{
				asteroids[i].position.x = (float)(rand() % MAX_COLS) - (MAX_COLS / 2);
				asteroids[i].position.y = (float)(rand() % MAX_ROWS) - (MAX_ROWS / 2);
			}

			asteroids[i].position.dx = sinf(asteroids[i].position.angle) * ASTEROID_SPEED * input->dtForFrame;
			asteroids[i].position.dy = cosf(asteroids[i].position.angle) * ASTEROID_SPEED * input->dtForFrame;

			for (int v = 0; v < asteroids[i].verts; ++v)
			{
				int variance = rand() % 7;
				float vector = ((float)v / (float)asteroids[i].verts) * (2 * Pi32);
				asteroids[i].position.vectors[v].x = (float)asteroids[i].size * sinf(vector) + (float)variance;
				asteroids[i].position.vectors[v].y = (float)asteroids[i].size * cosf(vector) + (float)variance;
			}
		}

		if (state->asteroids < MAX_NEW_ASTEROIDS)
			++state->asteroids;
	}

	// update asteroids
	for (int i = 0; i < MAX_ASTEROIDS; ++i)
		if (asteroids[i].alive)
		{
			// check for ship collision
			if (collisionDetected(asteroids[i].position.x, asteroids[i].position.y, 
				asteroids[i].size,
				ship.position.x, ship.position.y))
			{
				gameReload(state);
			}

			// check for bullet collision
			for (int b = 0; b < MAX_BULLETS; ++b)
				if (bullets[b].alive)
					if (collisionDetected(asteroids[i].position.x, asteroids[i].position.y,
						(asteroids[i].size / 100.0f) * 95.0f,
						bullets[b].position.dx, bullets[b].position.dy))
					{
						asteroids[i].alive = false;
						bullets[b].alive = false;
						++state->score;

						if (asteroids[i].size == ASTEROID_SMALL)
							break;

						// make 2 smaller asteroids
						createAsteroid(i, input->dtForFrame);
						createAsteroid(i, input->dtForFrame);
						break;
					}

			if (!asteroids[i].alive)
				continue;

			asteroids[i].position.angle += ASTEROID_ROTATION * input->dtForFrame;
			asteroids[i].position.x += asteroids[i].position.dx;
			asteroids[i].position.y += asteroids[i].position.dy;

			// re-wrap co-ordinates to prevent ever larger positions
			wrapModel(&asteroids[i].position);
		}

	// draw ship
	drawFrame(buffer, state, &ship.position, ship.verts, SHIP_SCALE, WHITE);

	// draw asteroids
	for (int i = 0; i < MAX_ASTEROIDS; ++i)
		if (asteroids[i].alive)
			drawFrame(buffer, state, &asteroids[i].position, asteroids[i].verts, ASTEROID_SCALE, WHITE);

	// draw bullets
	for (int i = 0; i < MAX_BULLETS; ++i)
		if (bullets[i].alive)
			blob(buffer, offsetCol((int)bullets[i].position.dx), offsetRow((int)bullets[i].position.dy), WHITE);

	// draw data info
	drawDigits(buffer, 1, 1, state->score, WHITE);
	drawDigit(buffer, 1, 7, state->lives, YELLOW);

	if (state->fps)
	{
		// fps
		drawDigits(buffer, MAX_COLS - 15, 1, FPS, BLUE);
	}

	if (state->hud)
	{
		// angle
		drawDigits(buffer, offsetCol((int)ship.position.x) + 10, offsetRow((int)ship.position.y), ship.position.angle, BLUE);

		// position
		drawDigits(buffer, offsetCol((int)ship.position.x) + 10, offsetRow((int)ship.position.y) + 6, ship.position.x, CYAN);
		drawDigits(buffer, offsetCol((int)ship.position.x) + 28, offsetRow((int)ship.position.y) + 6, ship.position.y, CYAN);

		// thrust
		drawDigits(buffer, offsetCol((int)ship.position.x) + 10, offsetRow((int)ship.position.y) + 12, ship.position.dx, ORANGE);
		drawDigits(buffer, offsetCol((int)ship.position.x) + 28, offsetRow((int)ship.position.y) + 12, ship.position.dy, ORANGE);

		// vec1
		drawDigits(buffer, offsetCol((int)ship.position.x) + 10, offsetRow((int)ship.position.y) + 18, ship.position.vectors[0].x, YELLOW);
		drawDigits(buffer, offsetCol((int)ship.position.x) + 28, offsetRow((int)ship.position.y) + 18, ship.position.vectors[0].y, YELLOW);

		// vec2
		drawDigits(buffer, offsetCol((int)ship.position.x) + 48, offsetRow((int)ship.position.y) + 18, ship.position.vectors[1].x, YELLOW);
		drawDigits(buffer, offsetCol((int)ship.position.x) + 66, offsetRow((int)ship.position.y) + 18, ship.position.vectors[1].y, YELLOW);

		// vec3
		drawDigits(buffer, offsetCol((int)ship.position.x) + 86, offsetRow((int)ship.position.y) + 18, ship.position.vectors[2].x, YELLOW);
		drawDigits(buffer, offsetCol((int)ship.position.x) + 104, offsetRow((int)ship.position.y) + 18, ship.position.vectors[2].y, YELLOW);

		// grid size
		drawDigits(buffer, 1, 13, MAX_COLS, BLUE);
		drawDigits(buffer, 19, 13, MAX_ROWS, BLUE);

		// asteroid count
		drawDigits(buffer, 1, 19, countAsteroids(), BLUE);

		// bullet count
		int bulletCount = 0;
		for (int i = 0; i < MAX_BULLETS; ++i)
			if (bullets[i].alive)
				++bulletCount;
		drawDigits(buffer, 1, 25, (float)bulletCount, RED);

		// bullets
		int rows = 31;
		for (int i = 0; i < MAX_BULLETS; ++i)
			if (bullets[i].alive)
			{
				drawDigits(buffer, 1, rows, bullets[i].position.dx, RED);
				drawDigits(buffer, 19, rows, bullets[i].position.dy, RED);
				rows += 6;
			}
	}

	// slow down gradually, temporary?
	//if (ship.position.dx > 0.0f)
	//	ship.position.dx -= SLOW_SPEED * input->dtForFrame;
	//else
	//	ship.position.dx += SLOW_SPEED * input->dtForFrame;

	//if (ship.position.dy > 0.0f)
	//	ship.position.dy -= SLOW_SPEED * input->dtForFrame;
	//else
	//	ship.position.dy += SLOW_SPEED * input->dtForFrame;
}

// draw all vertices to form a frame
static void drawFrame(struct gameDisplayBuffer *buffer, struct gameState *state, struct Position *position, short verts, float scale, uint32_t colour)
{
	assert(verts <= MAX_VERTS);

	float new_vectors[MAX_VERTS][2];

	// update rotation
	float r = position->angle;
	for (int i = 0; i < verts; ++i)
	{
		float x = position->vectors[i].x;
		float y = position->vectors[i].y;
		new_vectors[i][0] = x * cosf(r) - y * sinf(r);
		new_vectors[i][1] = x * sinf(r) + y * cosf(r);
	}

	// rotated vectors
	if (state->hud && verts == 3)
	{
		drawDigits(buffer, offsetCol((int)ship.position.x) + 10, offsetRow((int)ship.position.y) + 24, new_vectors[0][0], GREEN);
		drawDigits(buffer, offsetCol((int)ship.position.x) + 28, offsetRow((int)ship.position.y) + 24, new_vectors[0][1], GREEN);
		drawDigits(buffer, offsetCol((int)ship.position.x) + 48, offsetRow((int)ship.position.y) + 24, new_vectors[1][0], GREEN);
		drawDigits(buffer, offsetCol((int)ship.position.x) + 66, offsetRow((int)ship.position.y) + 24, new_vectors[1][1], GREEN);
		drawDigits(buffer, offsetCol((int)ship.position.x) + 86, offsetRow((int)ship.position.y) + 24, new_vectors[2][0], GREEN);
		drawDigits(buffer, offsetCol((int)ship.position.x) + 104, offsetRow((int)ship.position.y) + 24, new_vectors[2][1], GREEN);
	}

	// update scale
	for (int i = 0; i < verts; ++i)
	{
		new_vectors[i][0] = roundf(new_vectors[i][0] * scale);
		new_vectors[i][1] = roundf(new_vectors[i][1] * scale);
	}

	// scaled vectors
	if (state->hud && verts == 3)
	{
		drawDigits(buffer, offsetCol((int)ship.position.x) + 10, offsetRow((int)ship.position.y) + 30, new_vectors[0][0], CYAN);
		drawDigits(buffer, offsetCol((int)ship.position.x) + 28, offsetRow((int)ship.position.y) + 30, new_vectors[0][1], CYAN);
		drawDigits(buffer, offsetCol((int)ship.position.x) + 48, offsetRow((int)ship.position.y) + 30, new_vectors[1][0], CYAN);
		drawDigits(buffer, offsetCol((int)ship.position.x) + 66, offsetRow((int)ship.position.y) + 30, new_vectors[1][1], CYAN);
		drawDigits(buffer, offsetCol((int)ship.position.x) + 86, offsetRow((int)ship.position.y) + 30, new_vectors[2][0], CYAN);
		drawDigits(buffer, offsetCol((int)ship.position.x) + 104, offsetRow((int)ship.position.y) + 30, new_vectors[2][1], CYAN);
	}

	// translate co-ordinates
	for (int i = 0; i < verts; ++i)
	{
		new_vectors[i][0] += roundf(position->x);
		new_vectors[i][1] += roundf(position->y);
	}

	// translated vectors
	if (state->hud && verts == 3)
	{
		drawDigits(buffer, offsetCol((int)ship.position.x) + 10, offsetRow((int)ship.position.y) + 36, new_vectors[0][0], ORANGE);
		drawDigits(buffer, offsetCol((int)ship.position.x) + 28, offsetRow((int)ship.position.y) + 36, new_vectors[0][1], ORANGE);
		drawDigits(buffer, offsetCol((int)ship.position.x) + 48, offsetRow((int)ship.position.y) + 36, new_vectors[1][0], ORANGE);
		drawDigits(buffer, offsetCol((int)ship.position.x) + 66, offsetRow((int)ship.position.y) + 36, new_vectors[1][1], ORANGE);
		drawDigits(buffer, offsetCol((int)ship.position.x) + 86, offsetRow((int)ship.position.y) + 36, new_vectors[2][0], ORANGE);
		drawDigits(buffer, offsetCol((int)ship.position.x) + 104, offsetRow((int)ship.position.y) + 36, new_vectors[2][1], ORANGE);
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
			(int)new_vectors[i][0], (int)new_vectors[i][1],
			(int)new_vectors[i + 1][0], (int)new_vectors[i + 1][1],
			colour);
	}

	// draw last vertex line
	line(buffer,
		(int)new_vectors[verts][0], (int)new_vectors[verts][1],
		(int)new_vectors[0][0], (int)new_vectors[0][1],
		WHITE);
}
