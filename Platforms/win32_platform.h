#pragma once

#include "..\GameDLL\roids.h"

int stringLength(char *);
void outs(char *);
void createBuffer(struct win32displayBuffer *, int, int);
void displayBuffer(struct win32displayBuffer *, HDC);
void loadXInput(void);
void initDSound(HWND, int32_t, int32_t);
void fillSoundBuffer(struct win32soundOutput *, DWORD, DWORD, struct gameSoundOutputBuffer *);
void clearSoundBuffer(struct win32soundOutput *);
void catStrings(size_t, char *, size_t, char *, size_t, char *);
void buildDLLPathFilename(struct win32state *, char *, int, char *);
void getExeFilename(struct win32state *);
void unloadGameCode(struct win32gameCode *);
void processKeyboardMessage(struct gameButtonState *, bool);
void processXinputDigitalButton(DWORD, struct gameButtonState *, DWORD, struct gameButtonState *);
void processPendingMessages(struct win32state *, struct gameControllerInput *);
float processXinputStickValue(SHORT, SHORT);
struct win32gameCode loadGameCode(char *, char *);
inline FILETIME getLastWriteTime(char *);
inline LARGE_INTEGER getWallClock(void);
inline float getSecondsElapsed(LARGE_INTEGER, LARGE_INTEGER, int64_t);

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

struct win32displayBuffer
{
	// pixels are always 32-bits wide, Memory Order BB GG RR XX
	BITMAPINFO info;
	void *memory;
	int width;
	int height;
	int pitch;
	int bytesPerPixel;
};

struct win32soundOutput
{
	int samplesPerSecond;
	uint32_t runningSampleIndex;
	int bytesPerSample;
	DWORD secondaryBufferSize;
	DWORD safetyBytes;
	// float tSine;
};

struct win32debugTimeMarker
{
	DWORD outputPlayCursor;
	DWORD outputWriteCursor;
	DWORD outputLocation;
	DWORD outputByteCount;
	DWORD expectedFlipPlayCursor;
	DWORD flipPlayCursor;
	DWORD flipWriteCursor;
};

struct win32gameCode
{
	HMODULE gameCodeDLL;
	FILETIME DLLlastWriteTime;

	// function pointers
	game_UpdateAndRender *updateAndRender;
	game_GetSoundSamples *getSoundSamples;

	bool isValid;
};

struct win32state
{
	uint64_t totalSize;
	void *gameMemoryBlock;
	char exeFilename[MAX_PATH];
	char *onePastLastSlash;
};
