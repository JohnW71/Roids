#pragma once

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 920

void outs(char *);
void createBuffer(struct displayBuffer *, int, int);
void displayBuffer(struct displayBuffer *, HDC, int, int);
void loadXInput(void);
void initDSound(HWND, int32_t, int32_t);
void fillSoundBuffer(struct soundOutput *, DWORD, DWORD, struct gameSoundOutputBuffer *);
void clearSoundBuffer(struct soundOutput *);
void updateAndRender(struct gameMemory *, struct gameDisplayBuffer *, struct gameSoundOutputBuffer *, int);

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

struct displayBuffer
{
	BITMAPINFO info;
	void *memory;
	int width;
	int height;
	int pitch;
};

struct soundOutput
{
	int samplesPerSecond;
	uint32_t runningSampleIndex;
	int bytesPerSample;
	int secondaryBufferSize;
	float tSine;
	int latencySampleCount;
};
