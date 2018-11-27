#pragma once

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 920

void outs(char *);
void createBuffer(struct bufferInfo *, int, int);
void updateAndRender(struct gameMemory *, struct bufferInfo *);
void displayBuffer(struct bufferInfo *, HDC, int, int);
void loadXInput(void);

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
