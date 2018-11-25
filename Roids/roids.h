#pragma once

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 920

#define PI 3.14259

#define ASTEROID_MAX_COUNT 4
#define ASTEROID_MAX_DIAMETER 64
#define ASTEROID_MIN_DIAMETER 5
#define ASTEROID_SIDES 6
#define ASTEROID_SPEED_PPS 10

#define BULLET_SPEED_PPS 10

#define SHIP_ROTATION_SPEED_PPS 10
#define SHIP_MAX_SPEED_PPS 10
#define SHIP_ACCELERATION_PPS 3
#define SHIP_DECELERATION_PPS 2

const int rockGridSize = ASTEROID_MAX_DIAMETER * ASTEROID_MAX_DIAMETER;

void outs(char *);
//void shipConfig(void);
void createBuffer(struct bufferInfo *, int, int);
void fillBuffer(struct bufferInfo *);
void displayBuffer(struct bufferInfo *, HDC, int, int);

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

//struct Ship
//{
//	int x;
//	int y;
//	int facing;
//	int trajectory;
//	int speed;
//	int acceleration;
//	int lives;
//} ship;

struct bufferInfo
{
	BITMAPINFO info;
	void *memory;
	int width;
	int height;
	int pitch;
} backBuffer;
