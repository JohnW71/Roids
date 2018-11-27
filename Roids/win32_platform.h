#pragma once

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 920

void outs(char *);
void createBuffer(struct bufferInfo *, int, int);
void updateAndRender(struct GameMemory *, struct bufferInfo *);
void displayBuffer(struct bufferInfo *, HDC, int, int);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

struct bufferInfo
{
	BITMAPINFO info;
	void *memory;
	int width;
	int height;
	int pitch;
};
