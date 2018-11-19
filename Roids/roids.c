#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>	// exit(), rand(), malloc()
#include <time.h>	// time()
#include <stdbool.h>
#include "roids.h"

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

bool running = true;
char logFile[] = "log.txt";
char *buffer;

int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance,
					_In_ LPWSTR lpCmdLine, _In_ int nShowCmd)
{
	FILE *lf = fopen(logFile, "w");
	if (lf == NULL)
		MessageBox(NULL, "Can't open log file", "Error", MB_ICONEXCLAMATION | MB_OK);
	else
		fclose(lf);

	MSG msg = {0};
	WNDCLASSEX wc;
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
	wc.hInstance = hInstance;
	wc.lpfnWndProc = WndProc;
	wc.lpszClassName = "ArseSteroids";
	wc.lpszMenuName = NULL;
	wc.style = CS_HREDRAW | CS_VREDRAW;

	if (!RegisterClassEx(&wc))
	{
		MessageBox(NULL, "Window registration failed", "Window registration failed", MB_ICONEXCLAMATION | MB_OK);
		return 0;
	}

	HWND hwnd = CreateWindowEx(WS_EX_OVERLAPPEDWINDOW,
								wc.lpszClassName,
								"ArseSteroids",
								WS_BORDER | WS_CAPTION | WS_VISIBLE,
								CW_USEDEFAULT, CW_USEDEFAULT,
								WINDOW_WIDTH, WINDOW_HEIGHT,
								NULL, NULL,	hInstance, NULL);

	if (!hwnd)
	{
		MessageBox(NULL, "Window creation failed", "Window creation failed", MB_ICONEXCLAMATION | MB_OK);
		return 0;
	}

	shipConfig();
	ShowWindow(hwnd, nShowCmd);

	while (running)
	{
		if (GetMessage(&msg, NULL, 0, 0))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		if (!UpdateWindow(hwnd))
		{
			MessageBox(NULL, "Window update failed", "Window update failed", MB_ICONEXCLAMATION | MB_OK);
			free(buffer);
			return 0;
		}
	}

	free(buffer);
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

			buffer = NULL;
			buffer = (char *)malloc(WINDOW_WIDTH * WINDOW_HEIGHT);

			if (!buffer)
			{
				outs("Memory allocation error for buffer");
				DestroyWindow(hwnd);
			}

			for (int x = 0; x < WINDOW_HEIGHT; ++x)
				for (int y = 0; y < WINDOW_WIDTH; ++y)
				{
					// fill buffer
				}

			break;
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

void shipConfig(void)
{
	ship.x = (GetSystemMetrics(SM_CXSCREEN) - WINDOW_WIDTH) / 2;
	ship.y = (GetSystemMetrics(SM_CYSCREEN) - WINDOW_HEIGHT) / 2;
	ship.facing = 0;
	ship.trajectory = 0;
	ship.speed = 0;
	ship.acceleration = 0;
	ship.lives = 3;
	outs("Ship created");
}
