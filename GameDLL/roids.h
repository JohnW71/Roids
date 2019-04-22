#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>	// exit(), rand(), malloc()
#include <time.h>	// time()

//#define WINDOW_WIDTH 160
//#define WINDOW_HEIGHT 100
//#define WINDOW_WIDTH 960
//#define WINDOW_HEIGHT 540
#define WINDOW_WIDTH 1030
#define WINDOW_HEIGHT 600

#define SHIP_SCALE 1.2f
#define TURN_SPEED 4.0f
#define MOVE_SPEED 20.0f
#define SLOW_SPEED 1.0f
#define BULLET_SPEED 40.0f
#define ASTEROID_SCALE 1.0f
#define ASTEROID_SPEED 20.0f
#define ASTEROID_ROTATION 1.0f
#define ASTEROID_BIG_VERTS 20
#define ASTEROID_MED_VERTS 12
#define ASTEROID_SMALL_VERTS 8
#define ASTEROID_BIG 30.0f
#define ASTEROID_MED 15.0f
#define ASTEROID_SMALL 7.5f
#define COL_WIDTH 3
#define ROW_HEIGHT 3
#define MAX_COLS (WINDOW_WIDTH / COL_WIDTH)
#define MAX_ROWS (WINDOW_HEIGHT / ROW_HEIGHT)
#define MAX_VERTS 20
#define MAX_BULLETS 8
#define MAX_NEW_ASTEROIDS 8
#define MAX_ASTEROIDS MAX_NEW_ASTEROIDS*7

#define Pi32 3.14159265359f
#define assert(expression) if(!(expression)) {*(int *)0 = 0;}
#define arrayCount(array) (sizeof(array) / sizeof((array)[0]))

#define kilobytes(value) ((value)*1024LL)
#define megabytes(value) (kilobytes(value)*1024LL)
#define gigabytes(value) (megabytes(value)*1024LL)
#define terabytes(value) (gigabytes(value)*1024LL)

// AA RR GG BB
#define RED 0x00FF0000
#define GREEN 0x0000FF00
#define BLUE 0x000000FF
#define CYAN 0x0000FFFF
#define MAGENTA 0x00FF00FF
#define YELLOW 0x00FFFF00
#define WHITE 0x00FFFFFF
#define BLACK 0x00000000
#define ORANGE 0x00FFA500

// manual function definitions
#define GAME_UPDATE_AND_RENDER(name) void name(struct threadContext *thread, struct gameMemory *memory, struct gameInput *input, struct gameDisplayBuffer *buffer, float FPS)
#define GAME_GET_SOUND_SAMPLES(name) void name(struct threadContext *thread, struct gameMemory *memory, struct gameSoundOutputBuffer *soundBuffer)

// function typedefs
typedef GAME_UPDATE_AND_RENDER(game_UpdateAndRender);
typedef GAME_GET_SOUND_SAMPLES(game_GetSoundSamples);

int offsetCol(int);
int offsetRow(int);
short countAsteroids(void);
bool collisionDetected(float, float, float, float, float);
void outputSound(struct gameState *, struct gameSoundOutputBuffer *, int);
void gameReload(struct gameState *);
void gameReset(struct gameState *);
void shipReset(void);
void bulletReset(int);
void bulletsReset(void);
void asteroidReset(int);
void asteroidsReset(void);
void line(struct gameDisplayBuffer *, int, int, int, int, uint32_t);
void lineLow(struct gameDisplayBuffer *, int, int, int, int, uint32_t);
void lineHigh(struct gameDisplayBuffer *, int, int, int, int, uint32_t);
void blob(struct gameDisplayBuffer *, int, int, uint32_t);
void drawFrame(struct gameDisplayBuffer *, struct gameState *, struct Position *, short, float, uint32_t);
void wrapCoordinates(int, int, int *, int *);
void wrapModel(struct Position *);
void drawDigit(struct gameDisplayBuffer *, short, short, short, uint32_t);
void drawDigits(struct gameDisplayBuffer *, short, short, float, uint32_t);
void createAsteroid(short, float);

struct threadContext
{
	int placeholder;
};

struct gameDisplayBuffer
{
	void *memory;
	int width;
	int height;
	int pitch;
	int bytesPerPixel;
};

struct gameSoundOutputBuffer
{
	int samplesPerSecond;
	int sampleCount;
	int16_t *samples;
};

struct gameState
{
	short lives;
	short score;
	short asteroids;
	bool hud;
	bool fps;
};

struct gameMemory
{
	bool isInitialized;
	uint64_t permanentStorageSize;
	uint64_t transientStorageSize;
	void *permanentStorage;
	void *transientStorage;
};

struct gameButtonState
{
	int halfTransitionCount;
	bool endedDown;
};

struct gameControllerInput
{
	bool isConnected;
	bool isAnalog;
	//float stickAverageX;
	//float stickAverageY;

	union
	{
		struct gameButtonState buttons[15];
		struct
		{
			struct gameButtonState moveUp;
			struct gameButtonState moveDown;
			struct gameButtonState moveLeft;
			struct gameButtonState moveRight;

			struct gameButtonState actionUp;
			struct gameButtonState actionDown;
			struct gameButtonState actionLeft;
			struct gameButtonState actionRight;

			struct gameButtonState leftShoulder;
			struct gameButtonState rightShoulder;

			struct gameButtonState fps;
			struct gameButtonState hud;
			struct gameButtonState reset;

			struct gameButtonState back;
			struct gameButtonState start;
			//
			struct gameButtonState terminator;
		};
	};
};

struct gameInput
{
	struct gameButtonState mouseButtons[5];
	int32_t mouseX, mouseY, mouseZ;
	float dtForFrame;
	struct gameControllerInput controllers[5];
};

struct gameControllerInput *getController(struct gameInput *input, unsigned int controllerIndex)
{
	assert(controllerIndex < arrayCount(input->controllers));
	struct gameControllerInput *result = &input->controllers[controllerIndex];
	return result;
}

struct Vector
{
	float x;
	float y;
};

struct Position
{
	float dx;
	float dy;
	float x;
	float y;
	float angle;
	struct Vector vectors[MAX_VERTS];
};

struct Ship
{
	short verts;
	struct Position position;
} ship;

struct Asteroid
{
	bool alive;
	float size;
	short verts;
	struct Position position;
} asteroids[MAX_ASTEROIDS];

struct Bullet
{
	bool alive;
	struct Position position;
} bullets[MAX_BULLETS];
