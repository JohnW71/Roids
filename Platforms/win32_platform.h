#pragma once

#include "..\GameDLL\roids.h"

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

//struct win32gameCode
//{
//	HMODULE gameCodeDLL;
//	FILETIME DLLlastWriteTime;
//
//	// function pointers
//	game_UpdateAndRender *updateAndRender;
//	game_GetSoundSamples *getSoundSamples;
//
//	bool isValid;
//};

struct win32state
{
	uint64_t totalSize;
	void *gameMemoryBlock;
	char exeFilename[MAX_PATH];
	char *onePastLastSlash;
};
