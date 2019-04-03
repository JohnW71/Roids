#pragma once

#include <stdint.h>
#include <stdbool.h>

//#define WINDOW_WIDTH 160
//#define WINDOW_HEIGHT 100
//#define WINDOW_WIDTH 960
//#define WINDOW_HEIGHT 540
#define WINDOW_WIDTH 1030
#define WINDOW_HEIGHT 600

#define COL_WIDTH 4
#define ROW_HEIGHT 4
#define MAX_COLS (WINDOW_WIDTH / COL_WIDTH)
#define MAX_ROWS (WINDOW_HEIGHT / ROW_HEIGHT)

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

// #define ASTEROID_MAX_COUNT 4
// #define ASTEROID_MAX_DIAMETER 64
// #define ASTEROID_MIN_DIAMETER 5
// #define ASTEROID_SIDES 6
// #define ASTEROID_SPEED_PPS 10

// #define BULLET_SPEED_PPS 10
// #define BULLET_MAX_COUNT 4

// #define SHIP_ROTATION_SPEED_PPS 10
// #define SHIP_MAX_SPEED_PPS 10
// #define SHIP_ACCELERATION_PPS 3
// #define SHIP_DECELERATION_PPS 2

// manual function definitions
#define GAME_UPDATE_AND_RENDER(name) void name(struct threadContext *thread, struct gameMemory *memory, struct gameInput *input, struct gameDisplayBuffer *buffer)
#define GAME_GET_SOUND_SAMPLES(name) void name(struct threadContext *thread, struct gameMemory *memory, struct gameSoundOutputBuffer *soundBuffer)

// function typedefs
typedef GAME_UPDATE_AND_RENDER(game_UpdateAndRender);
typedef GAME_GET_SOUND_SAMPLES(game_GetSoundSamples);

// int rockGridSize = ASTEROID_MAX_DIAMETER * ASTEROID_MAX_DIAMETER;

void outputSound(struct gameState *, struct gameSoundOutputBuffer *, int);
// void fillBuffer(struct gameDisplayBuffer *, int, int);
// void shipConfig(void);
// void renderPlayer(struct gameDisplayBuffer *, int, int);
int32_t roundFloatToInt32(float);
void drawRectangle(struct gameDisplayBuffer *, float, float, float, float, float, float, float);
void blob(struct gameDisplayBuffer *, int, int, int32_t);
void line(struct gameDisplayBuffer *, int, int, int, int, int32_t);
int offsetCol(int);
int offsetRow(int);

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
	float playerX;
	float playerY;
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
	float stickAverageX;
	float stickAverageY;

	union
	{
		struct gameButtonState buttons[12];
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

inline struct gameControllerInput *getController(struct gameInput *input, unsigned int controllerIndex)
{
	assert(controllerIndex < arrayCount(input->controllers));
	struct gameControllerInput *result = &input->controllers[controllerIndex];
	return result;
}

//struct Ship
//{
//	int col;
//	int row;
//	int facing;
//	int trajectory;
//	int speed;
//	int acceleration;
//	int lives;
//} ship;
