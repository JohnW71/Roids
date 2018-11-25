#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <stdio.h> // sprintf
#include <stdlib.h>	// exit(), rand(), malloc()
#include <time.h>	// time()
#include <stdbool.h>
#include <stdint.h>
#include "roids.h"

bool running = true;
char logFile[] = "log.txt";

int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance,
					_In_ LPWSTR lpCmdLine, _In_ int nShowCmd)
{
	LARGE_INTEGER perfCountFrequencyResult;
	QueryPerformanceFrequency(&perfCountFrequencyResult);
	int64_t perfCountFrequency = perfCountFrequencyResult.QuadPart;

	FILE *lf = fopen(logFile, "w");
	if (lf == NULL)
		MessageBox(NULL, "Can't open log file", "Error", MB_ICONEXCLAMATION | MB_OK);
	else
		fclose(lf);

	WNDCLASSEX wc = {0};

	createBuffer(&backBuffer, WINDOW_WIDTH, WINDOW_HEIGHT);

	wc.cbSize = sizeof(WNDCLASSEX);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	//wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	//wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
	wc.hInstance = hInstance;
	wc.lpfnWndProc = WndProc;
	wc.lpszClassName = "Roids";
	wc.style = CS_HREDRAW | CS_VREDRAW;

	if (!RegisterClassEx(&wc))
	{
		MessageBox(NULL, "Window registration failed", "Window registration failed", MB_ICONEXCLAMATION | MB_OK);
		return 0;
	}

	HWND hwnd = CreateWindowEx(WS_EX_OVERLAPPEDWINDOW,
								wc.lpszClassName,
								"Roids",
								WS_BORDER | WS_CAPTION | WS_VISIBLE,
								CW_USEDEFAULT, CW_USEDEFAULT,
								WINDOW_WIDTH, WINDOW_HEIGHT,
								NULL, NULL,	hInstance, NULL);

	if (!hwnd)
	{
		MessageBox(NULL, "Window creation failed", "Window creation failed", MB_ICONEXCLAMATION | MB_OK);
		return 0;
	}

	MSG msg = {0};
	LARGE_INTEGER lastCounter;
	QueryPerformanceCounter(&lastCounter);

	uint64_t lastCycleCount = __rdtsc();

	while (running)
	{
		while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
				running = false;

			TranslateMessage(&msg);
			DispatchMessageA(&msg);
		}

		HDC deviceContext = GetDC(hwnd);
		fillBuffer(&backBuffer);
		displayBuffer(&backBuffer, deviceContext, WINDOW_WIDTH, WINDOW_HEIGHT);
		ReleaseDC(hwnd, deviceContext);

		uint64_t endCycleCount = __rdtsc();
		uint64_t cyclesElapsed = endCycleCount - lastCycleCount;

		LARGE_INTEGER endCounter;
		QueryPerformanceCounter(&endCounter);
		int64_t counterElapsed = endCounter.QuadPart - lastCounter.QuadPart;
		float msPerFrame = ((1000.0f * (float)counterElapsed) / (float)perfCountFrequency);
		float FPS = (float)perfCountFrequency / (float)counterElapsed;
		float MCPF = (float)cyclesElapsed / (1000.0f * 1000.0f);

		char buf[256];
		sprintf(buf, "%.02fms, %.02f fps, %.02f megacycles per frame\n", msPerFrame, FPS, MCPF);
		OutputDebugString(buf);

		lastCounter = endCounter;
		lastCycleCount = endCycleCount;
	}

	return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	int screenWidth = GetSystemMetrics(SM_CXSCREEN);
	int screenHeight = GetSystemMetrics(SM_CYSCREEN);

	switch (msg)
	{
		case WM_CREATE:
			SetWindowPos(hwnd, HWND_TOP, (screenWidth - WINDOW_WIDTH) / 2, (screenHeight - WINDOW_HEIGHT) / 2, 0, 0, SWP_NOSIZE);
			break;
		case WM_PAINT:
		{
			PAINTSTRUCT paint;
			HDC deviceContext = BeginPaint(hwnd, &paint);
			displayBuffer(&backBuffer, deviceContext, WINDOW_WIDTH, WINDOW_HEIGHT);
			EndPaint(hwnd, &paint);
			break;
		}
		case WM_KEYUP:
			switch (wParam)
			{
				case VK_ESCAPE:
					DestroyWindow(hwnd);
					break;
			}
			break;
		case WM_DESTROY:
			running = false;
			PostQuitMessage(0);
			break;
		//case WM_SETCURSOR:
		//	SetCursor(0);
		//break;
	}
	return DefWindowProc(hwnd, msg, wParam, lParam);
}

void outs(char *s)
{
	FILE *lf = fopen(logFile, "a");
	if (lf == NULL)
	{
		MessageBox(NULL, "Can't open log file", "Error", MB_ICONEXCLAMATION | MB_OK);
		return;
	}

	fprintf(lf, "%s\n", s);
	fclose(lf);
}

//void shipConfig(void)
//{
//	ship.x = (GetSystemMetrics(SM_CXSCREEN) - WINDOW_WIDTH) / 2;
//	ship.y = (GetSystemMetrics(SM_CYSCREEN) - WINDOW_HEIGHT) / 2;
//	ship.facing = 0;
//	ship.trajectory = 0;
//	ship.speed = 0;
//	ship.acceleration = 0;
//	ship.lives = 3;
//	outs("Ship created");
//}

void createBuffer(struct bufferInfo *buffer, int width, int height)
{
	if (buffer->memory)
		VirtualFree(buffer->memory, 0, MEM_RELEASE);

	buffer->width = width;
	buffer->height = height;
	int bytesPerPixel = 4;

	buffer->info.bmiHeader.biSize = sizeof(buffer->info.bmiHeader);
	buffer->info.bmiHeader.biWidth = buffer->width;
	buffer->info.bmiHeader.biHeight = -buffer->height;
	buffer->info.bmiHeader.biPlanes = 1;
	buffer->info.bmiHeader.biBitCount = 32;
	buffer->info.bmiHeader.biCompression = BI_RGB;

	int bitmapMemorySize = (width * height) * bytesPerPixel;
	buffer->memory = VirtualAlloc(0, bitmapMemorySize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	buffer->pitch = width * bytesPerPixel;
}

void fillBuffer(struct bufferInfo *buffer)
{
	uint8_t *row = (uint8_t *)buffer->memory;

	for (int y = 0; y < buffer->height; ++y)
	{
		uint32_t *pixel = (uint32_t *)row;

		for (int x = 0; x < buffer->width; ++x)
			*pixel++ = (uint32_t)(255.0f * ((float)x / (float)buffer->width)) + y;

		row += buffer->pitch;
	}
}

void displayBuffer(struct bufferInfo *buffer, HDC deviceContext, int width, int height)
{
	StretchDIBits(deviceContext, 0, 0, 
		width, height, 0, 0, 
		buffer->width, 
		buffer->height,
		buffer->memory,
		&buffer->info,
		DIB_RGB_COLORS,
		SRCCOPY);
}
