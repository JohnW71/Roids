#pragma once

#include <stdint.h>
#include <stdbool.h>

#define Pi32 3.14159265359f

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

// int rockGridSize = ASTEROID_MAX_DIAMETER * ASTEROID_MAX_DIAMETER;

void outs(char *);
//void shipConfig(void);
void outputSound(struct gameSoundOutputBuffer *, int);
void fillBuffer(struct gameDisplayBuffer *, int);

struct gameDisplayBuffer
{
	void *memory;
	int width;
	int height;
	int pitch;
};

struct gameSoundOutputBuffer
{
	int samplesPerSecond;
	int sampleCount;
	int16_t *samples;
};

struct gameState
{
	int toneHz;
};

struct gameMemory
{
	bool isInitialized;
	uint64_t storageSize;
	void *storage;
};

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
