#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <stdio.h> // sprintf
#include <stdbool.h>
#include <stdint.h>

#include "win32_platform.h"

bool debugging = true;
struct bufferInfo backBuffer;

static bool running = true;
static char logFile[] = "log.txt";

int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance,
					_In_ LPWSTR lpCmdLine, _In_ int nShowCmd)
{
	// get processor cycle speed
	LARGE_INTEGER perfCountFrequencyResult;
	QueryPerformanceFrequency(&perfCountFrequencyResult);
	int64_t perfCountFrequency = perfCountFrequencyResult.QuadPart;

	// create log file
	FILE *lf = fopen(logFile, "w");
	if (lf == NULL)
		MessageBox(NULL, "Can't open log file", "Error", MB_ICONEXCLAMATION | MB_OK);
	else
		fclose(lf);

	createBuffer(&backBuffer, WINDOW_WIDTH, WINDOW_HEIGHT);

	MSG msg = {0};
	WNDCLASSEX wc = {0};
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
		MessageBox(NULL, "Window registration failed", "Error", MB_ICONEXCLAMATION | MB_OK);
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
		MessageBox(NULL, "Window creation failed", "Error", MB_ICONEXCLAMATION | MB_OK);
		return 0;
	}

	// reserve memory block
	struct gameMemory memory = {0};
	memory.storageSize = 1073741824; // 1GB
	memory.storage = VirtualAlloc(0, memory.storageSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	if (!memory.storage)
	{
		outs("Memory allocation failure");
		MessageBox(NULL, "Memory allocation failure", "Error", MB_ICONEXCLAMATION | MB_OK);
		return 0;
	}
	memory.IsInitialized = true;

	// get current cycle count
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

		// update and display buffer
		HDC deviceContext = GetDC(hwnd);
		struct bufferInfo frontBuffer = {0};
		frontBuffer.memory = backBuffer.memory;
		frontBuffer.width = backBuffer.width;
		frontBuffer.height = backBuffer.height;
		frontBuffer.pitch = backBuffer.pitch;
		updateAndRender(&memory, &frontBuffer);
		displayBuffer(&frontBuffer, deviceContext, WINDOW_WIDTH, WINDOW_HEIGHT);
		ReleaseDC(hwnd, deviceContext);

		// get and display end counts
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

static void createBuffer(struct bufferInfo *buffer, int width, int height)
{
	if (debugging)
		outs("createBuffer()");

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

static void displayBuffer(struct bufferInfo *buffer, HDC deviceContext, int width, int height)
{
	if (debugging)
		outs("displayBuffer()");

	StretchDIBits(deviceContext,
					0, 0,
					width, height,
					0, 0,
					buffer->width,
					buffer->height,
					buffer->memory,
					&buffer->info,
					DIB_RGB_COLORS,
					SRCCOPY);
}
