#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <stdio.h> // sprintf
#include <stdbool.h>
#include <stdint.h>
#include <xinput.h>
#include <dsound.h>

#include "win32_platform.h"
#include "roids.h"

struct displayBuffer backBuffer;
struct gameDisplayBuffer gameBuffer;
int yOffset; //TODO remove this

static bool running = true;
static char logFile[] = "log.txt";
static LPDIRECTSOUNDBUFFER secondaryBuffer;

// manual function definitions to avoid requiring xinput lib during build or failing if it's missing
#define XInputGetState XInputGetState_
#define XInputSetState XInputSetState_
#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)
#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter)

// function typedefs
typedef X_INPUT_GET_STATE(x_input_get_state);
typedef X_INPUT_SET_STATE(x_input_set_state);
typedef DIRECT_SOUND_CREATE(direct_sound_create);

// stubs to do nothing if called unexpectedly
X_INPUT_GET_STATE(XInputGetStateStub)
{
	return(ERROR_DEVICE_NOT_CONNECTED);
}

X_INPUT_SET_STATE(XInputSetStateStub)
{
	return(ERROR_DEVICE_NOT_CONNECTED);
}

// function pointers
static x_input_get_state *XInputGetState_ = XInputGetStateStub;
static x_input_set_state *XInputSetState_ = XInputSetStateStub;

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

	loadXInput();
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

	// initialize sound
	struct soundOutput soundOutput = {0};
	soundOutput.samplesPerSecond = 48000;
	soundOutput.bytesPerSample = sizeof(int16_t) * 2;
	soundOutput.secondaryBufferSize = soundOutput.samplesPerSecond * soundOutput.bytesPerSample;
	soundOutput.latencySampleCount = soundOutput.samplesPerSecond / 15;
	initDSound(hwnd, soundOutput.samplesPerSecond, soundOutput.secondaryBufferSize);
	clearSoundBuffer(&soundOutput);
	secondaryBuffer->lpVtbl->Play(secondaryBuffer, 0, 0, DSBPLAY_LOOPING);

	//TODO pool with bitmap VirtualAlloc
	int16_t *samples = (int16_t *)VirtualAlloc(0, soundOutput.secondaryBufferSize,
												MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);

	// reserve memory block
	struct gameMemory memory = {0};
	memory.storageSize = 1073741824; // 1GB
	memory.storage = VirtualAlloc(0, memory.storageSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	if (!memory.storage)
	{
		outs("Memory allocation failure");
		MessageBox(NULL, "Memory allocation failure", "Error", MB_ICONEXCLAMATION | MB_OK);
		VirtualFree(samples, 0, MEM_RELEASE);
		return 0;
	}
	memory.isInitialized = true;

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

		// poll controllers
		for (DWORD ControllerIndex = 0; ControllerIndex < XUSER_MAX_COUNT; ++ControllerIndex)
		{
			XINPUT_STATE ControllerState;

			if (XInputGetState(ControllerIndex, &ControllerState) == ERROR_SUCCESS)
			{
				XINPUT_GAMEPAD *pad = &ControllerState.Gamepad;

				bool up = (pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
				bool down = (pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
				bool left = (pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
				bool right = (pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
				bool start = (pad->wButtons & XINPUT_GAMEPAD_START);
				bool back = (pad->wButtons & XINPUT_GAMEPAD_BACK);
				bool leftShoulder = (pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
				bool rightShoulder = (pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
				bool aButton = (pad->wButtons & XINPUT_GAMEPAD_A);
				bool bButton = (pad->wButtons & XINPUT_GAMEPAD_B);
				bool xButton = (pad->wButtons & XINPUT_GAMEPAD_X);
				bool yButton = (pad->wButtons & XINPUT_GAMEPAD_Y);

				int16_t stickX = pad->sThumbLX;
				int16_t stickY = pad->sThumbLY;

				yOffset += stickY / 4096;

				if (aButton)
				{
					XINPUT_VIBRATION vibration;
					vibration.wLeftMotorSpeed = 60000;
					vibration.wRightMotorSpeed = 60000;
					XInputSetState(0, &vibration);
				}
				else
				{
					XINPUT_VIBRATION vibration;
					vibration.wLeftMotorSpeed = 0;
					vibration.wRightMotorSpeed = 0;
					XInputSetState(0, &vibration);
				}
			}
			else
			{
				// other controllers are not available
			}
		}

		DWORD byteToLock = 0;
		DWORD targetCursor = 0;
		DWORD bytesToWrite = 0;
		DWORD playCursor = 0;
		DWORD writeCursor = 0;
		bool soundIsValid = false;

		//TODO tighten up sound logic so that we know where we should be writing to and can anticipate the time spent in the game update.
		if (SUCCEEDED(secondaryBuffer->lpVtbl->GetCurrentPosition(secondaryBuffer, &playCursor, &writeCursor)))
		{
			byteToLock = ((soundOutput.runningSampleIndex * soundOutput.bytesPerSample) %
			soundOutput.secondaryBufferSize);

			targetCursor = ((playCursor +
			(soundOutput.latencySampleCount * soundOutput.bytesPerSample)) %
			soundOutput.secondaryBufferSize);

			if (byteToLock > targetCursor)
			{
				bytesToWrite = (soundOutput.secondaryBufferSize - byteToLock);
				bytesToWrite += targetCursor;
			}
			else
			{
				bytesToWrite = targetCursor - byteToLock;
			}

			soundIsValid = true;
		}

		struct gameSoundOutputBuffer soundBuffer = {0};
		soundBuffer.samplesPerSecond = soundOutput.samplesPerSecond;
		soundBuffer.sampleCount = bytesToWrite / soundOutput.bytesPerSample;
		soundBuffer.samples = samples;

		// update and display buffer
		HDC deviceContext = GetDC(hwnd);
		gameBuffer.memory = backBuffer.memory;
		gameBuffer.width = backBuffer.width;
		gameBuffer.height = backBuffer.height;
		gameBuffer.pitch = backBuffer.pitch;
		updateAndRender(&memory, &gameBuffer, &soundBuffer, yOffset);

		// DirectSound output test
		if (soundIsValid)
		{
			fillSoundBuffer(&soundOutput, byteToLock, bytesToWrite, &soundBuffer);
		}

		displayBuffer(&backBuffer, deviceContext, WINDOW_WIDTH, WINDOW_HEIGHT);
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
		OutputDebugStringA(buf);

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
		case WM_SYSKEYDOWN:
		case WM_SYSKEYUP:
		case WM_KEYDOWN:
		case WM_KEYUP:
			switch (wParam)
			{
				case VK_LEFT:
					outs("left");
					OutputDebugStringA("left\n");
					break;
				case VK_RIGHT:
					outs("right");
					OutputDebugStringA("right\n");
					break;
				case VK_UP:
					outs("up");
					OutputDebugStringA("up\n");
					break;
				case VK_DOWN:
					outs("down");
					OutputDebugStringA("down\n");
					break;
				case VK_SPACE:
					outs("space");
					OutputDebugStringA("space\n");
					break;
				case VK_CONTROL:
					outs("CTRL");
					OutputDebugStringA("CTRL\n");
					break;
				case VK_ESCAPE:
					outs("ESC");
					OutputDebugStringA("ESC\n");
					DestroyWindow(hwnd);
					break;
			}

			bool altKeyWasDown = (lParam & (1 << 29));
			if ((wParam == VK_F4) && altKeyWasDown)
			{
				outs("ALT F4");
				OutputDebugStringA("ALT F4\n");
				DestroyWindow(hwnd);
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

static void createBuffer(struct displayBuffer *buffer, int width, int height)
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

static void displayBuffer(struct displayBuffer *buffer, HDC deviceContext, int width, int height)
{
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

// load dll manually to avoid requiring xinput lib during build
static void loadXInput(void)
{
	HMODULE XInputLibrary = LoadLibraryA("xinput1_4.dll");
	if (!XInputLibrary)
		XInputLibrary = LoadLibraryA("xinput9_1_0.dll");

	if (!XInputLibrary)
		XInputLibrary = LoadLibraryA("xinput1_3.dll");

	if (XInputLibrary)
	{
		XInputGetState = (x_input_get_state *)GetProcAddress(XInputLibrary, "XInputGetState");
		if (!XInputGetState)
			XInputGetState = XInputGetStateStub;

		XInputSetState = (x_input_set_state *)GetProcAddress(XInputLibrary, "XInputSetState");
		if (!XInputSetState)
			XInputSetState = XInputSetStateStub;

		outs("XInput loaded");
	}
	else
	{
		outs("XInput not loaded");
	}
}

static void initDSound(HWND hwnd, int32_t samplesPerSecond, int32_t bufferSize)
{
	// load the library
	HMODULE DSoundLibrary = LoadLibraryA("dsound.dll");
	if(DSoundLibrary)
	{
		// get a directSound object! - cooperative
		direct_sound_create *directSoundCreate = (direct_sound_create *)GetProcAddress(DSoundLibrary, "DirectSoundCreate");

		//TODO double-check that this works on XP - DirectSound8 or 7??
		LPDIRECTSOUND directSound;
		if (directSoundCreate && SUCCEEDED(directSoundCreate(0, &directSound, 0)))
		{
			WAVEFORMATEX waveFormat = {0};
			waveFormat.wFormatTag = WAVE_FORMAT_PCM;
			waveFormat.nChannels = 2;
			waveFormat.nSamplesPerSec = samplesPerSecond;
			waveFormat.wBitsPerSample = 16;
			waveFormat.nBlockAlign = (waveFormat.nChannels * waveFormat.wBitsPerSample) / 8;
			waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec*waveFormat.nBlockAlign;
			waveFormat.cbSize = 0;

			if (SUCCEEDED(directSound->lpVtbl->SetCooperativeLevel(directSound, hwnd, DSSCL_PRIORITY)))
			{
				DSBUFFERDESC bufferDescription = {0};
				bufferDescription.dwSize = sizeof(bufferDescription);
				bufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;

				// create a primary buffer
				LPDIRECTSOUNDBUFFER primaryBuffer;
				if (SUCCEEDED(directSound->lpVtbl->CreateSoundBuffer(directSound, &bufferDescription, &primaryBuffer, 0)))
				{
					HRESULT error = primaryBuffer->lpVtbl->SetFormat(primaryBuffer, &waveFormat);
					if (SUCCEEDED(error))
					{
						// we have set the format
						OutputDebugStringA("Primary buffer format was set.\n");
					}
					else
					{
						// Diagnostic
					}
				}
				else
				{
					// Diagnostic
				}
			}
			else
			{
				// Diagnostic
			}

			//TODO DSBCAPS_GETCURRENTPOSITION2
			DSBUFFERDESC bufferDescription = {0};
			bufferDescription.dwSize = sizeof(bufferDescription);
			bufferDescription.dwFlags = 0;
			bufferDescription.dwBufferBytes = bufferSize;
			bufferDescription.lpwfxFormat = &waveFormat;
			HRESULT Error = directSound->lpVtbl->CreateSoundBuffer(directSound, &bufferDescription, &secondaryBuffer, 0);
			if (SUCCEEDED(Error))
			{
				OutputDebugStringA("Secondary buffer created successfully.\n");
			}
		}
		else
		{
			// Diagnostic
		}
	}
	else
	{
		// Diagnostic
	}
}

static void clearSoundBuffer(struct soundOutput *soundOutput)
{
	VOID *region1;
	DWORD region1Size;
	VOID *region2;
	DWORD region2Size;
	if (SUCCEEDED(secondaryBuffer->lpVtbl->Lock(secondaryBuffer, 0, soundOutput->secondaryBufferSize,
												&region1, &region1Size,
												&region2, &region2Size, 0)))
	{
		//TODO assert that region1Size/region2Size is valid
		uint8_t *destSample = (uint8_t *)region1;

		for (DWORD byteIndex = 0; byteIndex < region1Size; ++byteIndex)
			*destSample++ = 0;

		destSample = (uint8_t *)region2;

		for (DWORD byteIndex = 0; byteIndex < region2Size; ++byteIndex)
			*destSample++ = 0;

		secondaryBuffer->lpVtbl->Unlock(secondaryBuffer, region1, region1Size, region2, region2Size);
	}
}

static void fillSoundBuffer(struct soundOutput *soundOutput, DWORD byteToLock,
							DWORD bytesToWrite, struct gameSoundOutputBuffer *sourceBuffer)
{
	VOID *region1;
	DWORD region1Size;
	VOID *region2;
	DWORD region2Size;
	if (SUCCEEDED(secondaryBuffer->lpVtbl->Lock(secondaryBuffer, byteToLock, bytesToWrite,
												&region1, &region1Size,
												&region2, &region2Size, 0)))
	{
		DWORD region1SampleCount = region1Size/soundOutput->bytesPerSample;
		int16_t *destSample = (int16_t *)region1;
		int16_t *sourceSample = sourceBuffer->samples;

		for (DWORD sampleIndex = 0; sampleIndex < region1SampleCount; ++sampleIndex)
		{
			*destSample++ = *sourceSample++;
			*destSample++ = *sourceSample++;
			++soundOutput->runningSampleIndex;
		}

		DWORD region2SampleCount = region2Size/soundOutput->bytesPerSample;
		destSample = (int16_t *)region2;

		for (DWORD sampleIndex = 0; sampleIndex < region2SampleCount; ++sampleIndex)
		{
			*destSample++ = *sourceSample++;
			*destSample++ = *sourceSample++;
			++soundOutput->runningSampleIndex;
		}

		secondaryBuffer->lpVtbl->Unlock(secondaryBuffer, region1, region1Size, region2, region2Size);
	}
}
