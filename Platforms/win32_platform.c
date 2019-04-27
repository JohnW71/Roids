#define _CRT_SECURE_NO_WARNINGS
#define DEV_MODE 1

#include <windows.h>
#include <stdio.h> // sprintf
#include <stdbool.h>
#include <stdint.h>
#include <xinput.h>
#include <dsound.h>

#include "win32_platform.h"
#include "..\GameDLL\roids.h"

static struct win32displayBuffer backBuffer;
static bool running;
static bool paused;
static char logFile[] = "log.txt";
static LPDIRECTSOUNDBUFFER secondaryBuffer;

// macros with parameters
#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)
#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter)

// function pointer type typedefs
typedef X_INPUT_GET_STATE(x_input_get_state);
// expands to:-
// typedef DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
// typedef DWORD WINAPI x_input_get_state(DWORD dwUserIndex, XINPUT_STATE *pState)
typedef X_INPUT_SET_STATE(x_input_set_state);
typedef DIRECT_SOUND_CREATE(win32directSoundCreate);

// stubs to do nothing if not loaded
X_INPUT_GET_STATE(XInputGetStateStub)
// expands to:-
// DWORD WINAPI XInputGetStateStub (DWORD dwUserIndex, XINPUT_STATE *pState)
{
	return(ERROR_DEVICE_NOT_CONNECTED);
}

X_INPUT_SET_STATE(XInputSetStateStub)
{
	return(ERROR_DEVICE_NOT_CONNECTED);
}

// function pointers with _ to avoid collisions with existing names
// define a global variable of type x_input_get_state* (our typedef), called XInputGetState_ (underscore to not collide with the real XInputGetState from the XInput.h header) and initialize it to the stub
static x_input_get_state *XInputGetState_ = XInputGetStateStub;
static x_input_set_state *XInputSetState_ = XInputSetStateStub;

// tell the preprocessor to replace all occurences of XInputGetState below this line by XInputGetState_ so that it calls our function pointer instead of the one declared in XInput.h
#define XInputGetState XInputGetState_
#define XInputSetState XInputSetState_

int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance,
					_In_ LPWSTR lpCmdLine, _In_ int nShowCmd)
{
	struct win32state state = {0};

	// get processor cycle speed
	LARGE_INTEGER perfCountFrequencyResult;
	QueryPerformanceFrequency(&perfCountFrequencyResult);
	int64_t perfCountFrequency = perfCountFrequencyResult.QuadPart;

	// define filenames for hot loading
	getExeFilename(&state);
	char srcGameCodeDLLpath[MAX_PATH];
	char tmpGameCodeDLLpath[MAX_PATH];
	buildDLLPathFilename(&state, "Roids.dll", sizeof(srcGameCodeDLLpath), srcGameCodeDLLpath);
	buildDLLPathFilename(&state, "Roids_tmp.dll", sizeof(tmpGameCodeDLLpath), tmpGameCodeDLLpath);

	// set windows scheduler granularity to 1ms
	UINT desiredSchedulerMs = 1;
	bool sleepIsGranular = (timeBeginPeriod(desiredSchedulerMs) == TIMERR_NOERROR);

	// create log file
	FILE *lf = fopen(logFile, "w");
	if (lf == NULL)
		MessageBox(NULL, "Can't open log file", "Error", MB_ICONEXCLAMATION | MB_OK);
	else
		fclose(lf);

	loadXInput();
	createBuffer(&backBuffer, WINDOW_WIDTH, WINDOW_HEIGHT);

	// MSG msg = {0};
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
								"Roids v0.2",
								WS_BORDER | WS_CAPTION | WS_VISIBLE, // WS_EX_TOPMOST
								CW_USEDEFAULT, CW_USEDEFAULT,
								WINDOW_WIDTH+20, WINDOW_HEIGHT+40,
								NULL, NULL,	hInstance, NULL);
	//HWND hwnd =
	//	CreateWindowExA(
	//		0, // WS_EX_TOPMOST|WS_EX_LAYERED,
	//		wc.lpszClassName,
	//		"Handmade Hero",
	//		WS_OVERLAPPEDWINDOW | WS_VISIBLE,
	//		CW_USEDEFAULT,
	//		CW_USEDEFAULT,
	//		CW_USEDEFAULT,
	//		CW_USEDEFAULT,
	//		0,
	//		0,
	//		hInstance,
	//		0);

	if (!hwnd)
	{
		MessageBox(NULL, "Window creation failed", "Error", MB_ICONEXCLAMATION | MB_OK);
		return 0;
	}

	// set up refresh rate
	int refreshHz = 60;
	HDC refreshDC = GetDC(hwnd);
	int refreshRate = GetDeviceCaps(refreshDC, VREFRESH);
	ReleaseDC(hwnd, refreshDC);
	if (refreshRate > 1)
		refreshHz = refreshRate;
	float gameUpdateHz = refreshHz / 2.0f; // aim for half of refresh rate
	float targetSecondsPerFrame = 1.0f / gameUpdateHz;

	// initialize sound
	struct win32soundOutput soundOutput = {0};
	soundOutput.samplesPerSecond = 48000;
	soundOutput.bytesPerSample = sizeof(int16_t) * 2;
	soundOutput.secondaryBufferSize = soundOutput.samplesPerSecond * soundOutput.bytesPerSample;
	soundOutput.safetyBytes = (int)(((float)soundOutput.samplesPerSecond * (float)soundOutput.bytesPerSample / gameUpdateHz) / 3.0f);
	initDSound(hwnd, soundOutput.samplesPerSecond, soundOutput.secondaryBufferSize);
	clearSoundBuffer(&soundOutput);
	secondaryBuffer->lpVtbl->Play(secondaryBuffer, 0, 0, DSBPLAY_LOOPING);

	int16_t *samples = (int16_t *)VirtualAlloc(0, soundOutput.secondaryBufferSize,
												MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
	if (!samples)
	{
		outs("Memory allocation failure: samples");
		MessageBox(NULL, "Memory allocation failure", "Error", MB_ICONEXCLAMATION | MB_OK);
		VirtualFree(samples, 0, MEM_RELEASE);
		return 0;
	}

	// reserve memory block
#ifdef DEV_MODE
	LPVOID baseAddress = (LPVOID)terabytes(2);
#else
	LPVOID baseAddress = 0;
#endif
	struct gameMemory memory = {0};
	memory.permanentStorageSize = megabytes(16);
	memory.transientStorageSize = megabytes(16);
	state.totalSize = memory.permanentStorageSize + memory.transientStorageSize;
	state.gameMemoryBlock = VirtualAlloc(baseAddress, (size_t)state.totalSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

	if (!state.gameMemoryBlock)
	{
		outs("Memory allocation failure: game memory block");
		MessageBox(NULL, "Memory allocation failure", "Error", MB_ICONEXCLAMATION | MB_OK);
		VirtualFree(state.gameMemoryBlock, 0, MEM_RELEASE);
		VirtualFree(samples, 0, MEM_RELEASE);
		return 0;
	}

	memory.permanentStorage = state.gameMemoryBlock;
	memory.transientStorage = ((uint8_t *)memory.permanentStorage + memory.permanentStorageSize);

	// misc setup
	struct gameInput input[2] = {0};
	struct gameInput *newInput = &input[0];
	struct gameInput *oldInput = &input[1];
	LARGE_INTEGER lastCounter = getWallClock();
	LARGE_INTEGER flipWallClock = getWallClock();
	// int debugTimeMarkerIndex = 0;
	// struct win32debugTimeMarker debugTimeMarkers[30] = {0};
	//DWORD audioLatencyBytes = 0;
	//float audioLatencySeconds = 0;
	bool soundIsValid = false;
	struct win32gameCode game = loadGameCode(srcGameCodeDLLpath, tmpGameCodeDLLpath);

	running = true;
	paused = false;
	float FPS = 0.0f;

	//uint64_t lastCycleCount = __rdtsc();

	while (running)
	{
		newInput->dtForFrame = targetSecondsPerFrame;

		// reload DLL if time changed
		FILETIME newDLLwriteTime = getLastWriteTime(srcGameCodeDLLpath);
		if (CompareFileTime(&newDLLwriteTime, &game.DLLlastWriteTime) != 0)
		{
			unloadGameCode(&game);
			game = loadGameCode(srcGameCodeDLLpath, tmpGameCodeDLLpath);
		}

		// zero controller info
		struct gameControllerInput *oldKeyboardController = getController(oldInput, 0);
		struct gameControllerInput *newKeyboardController = getController(newInput, 0);
		*newKeyboardController = (struct gameControllerInput){0};
		newKeyboardController->isConnected = true;
		for (int buttonIndex = 0; buttonIndex < arrayCount(newKeyboardController->buttons); ++buttonIndex)
			newKeyboardController->buttons[buttonIndex].endedDown = oldKeyboardController->buttons[buttonIndex].endedDown;

		processPendingMessages(&state, newKeyboardController);

		if (!paused)
		{
			// get mouse position and button states
			//POINT mouseP;
			//GetCursorPos(&mouseP);
			//ScreenToClient(hwnd, &mouseP); // re-map co-ordinates from screen to window
			//newInput->mouseX = mouseP.x;
			//newInput->mouseY = mouseP.y;
			//newInput->mouseZ = 0; // mousewheel
			//processKeyboardMessage(&newInput->mouseButtons[0], GetKeyState(VK_LBUTTON) & (1 << 15));
			//processKeyboardMessage(&newInput->mouseButtons[1], GetKeyState(VK_MBUTTON) & (1 << 15));
			//processKeyboardMessage(&newInput->mouseButtons[2], GetKeyState(VK_RBUTTON) & (1 << 15));
			//processKeyboardMessage(&newInput->mouseButtons[3], GetKeyState(VK_XBUTTON1) & (1 << 15));
			//processKeyboardMessage(&newInput->mouseButtons[4], GetKeyState(VK_XBUTTON2) & (1 << 15));

			//TODO don't poll disconnected controllers as it causes a frame hit
			DWORD maxControllerCount = XUSER_MAX_COUNT;
			if (maxControllerCount > (arrayCount(newInput->controllers) - 1))
				maxControllerCount = (arrayCount(newInput->controllers) - 1);

			// poll controllers
			for (DWORD ControllerIndex = 0; ControllerIndex < maxControllerCount; ++ControllerIndex)
			{
				DWORD ourControllerIndex = ControllerIndex + 1;
				struct gameControllerInput *oldController = getController(oldInput, ourControllerIndex);
				struct gameControllerInput *newController = getController(newInput, ourControllerIndex);

				XINPUT_STATE controllerState;
				if (XInputGetState(ControllerIndex, &controllerState) == ERROR_SUCCESS)
				{
					newController->isConnected = true;
					newController->isAnalog = oldController->isAnalog;

					//TODO test if controllerState.dwPacketNumber increments too fast
					XINPUT_GAMEPAD *pad = &controllerState.Gamepad;

					// square deadzone
					newController->stickAverageX = processXinputStickValue(pad->sThumbLX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
					newController->stickAverageY = processXinputStickValue(pad->sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);

					//if ((newController->stickAverageX != 0.0f) || (newController->stickAverageY != 0.0f))
					//	newController->isAnalog = true;

					//if (pad->wButtons & XINPUT_GAMEPAD_DPAD_UP)
					//{
					//	newController->stickAverageY = 1.0f;
					//	newController->isAnalog = false;
					//}
					//if (pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN)
					//{
					//	newController->stickAverageY = -1.0f;
					//	newController->isAnalog = false;
					//}
					//if (pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT)
					//{
					//	newController->stickAverageX = -1.0f;
					//	newController->isAnalog = false;
					//}
					//if (pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT)
					//{
					//	newController->stickAverageX = 1.0f;
					//	newController->isAnalog = false;
					//}

					//float threshold = 0.5f;
					//processXinputDigitalButton((newController->stickAverageY < threshold) ? 1 : 0, &oldController->moveUp, 1, &newController->moveUp);
					//processXinputDigitalButton((newController->stickAverageY < -threshold) ? 1 : 0, &oldController->moveDown, 1, &newController->moveDown);
					//processXinputDigitalButton((newController->stickAverageX < -threshold) ? 1 : 0, &oldController->moveLeft, 1, &newController->moveLeft);
					//processXinputDigitalButton((newController->stickAverageX < threshold) ? 1 : 0, &oldController->moveRight, 1, &newController->moveRight);

					processXinputDigitalButton(pad->wButtons, &oldController->actionUp, XINPUT_GAMEPAD_Y, &newController->actionUp);
					processXinputDigitalButton(pad->wButtons, &oldController->actionDown, XINPUT_GAMEPAD_A, &newController->actionDown);
					processXinputDigitalButton(pad->wButtons, &oldController->actionLeft, XINPUT_GAMEPAD_X, &newController->actionLeft);
					processXinputDigitalButton(pad->wButtons, &oldController->actionRight, XINPUT_GAMEPAD_B, &newController->actionRight);
					processXinputDigitalButton(pad->wButtons, &oldController->leftShoulder, XINPUT_GAMEPAD_LEFT_SHOULDER, &newController->leftShoulder);
					processXinputDigitalButton(pad->wButtons, &oldController->rightShoulder, XINPUT_GAMEPAD_RIGHT_SHOULDER, &newController->rightShoulder);
					processXinputDigitalButton(pad->wButtons, &oldController->start, XINPUT_GAMEPAD_START, &newController->start);
					processXinputDigitalButton(pad->wButtons, &oldController->back, XINPUT_GAMEPAD_BACK, &newController->back);
//if (aButton)
//{
//	XINPUT_VIBRATION vibration;
//	vibration.wLeftMotorSpeed = 60000;
//	vibration.wRightMotorSpeed = 60000;
//	XInputSetState(0, &vibration);
//}
				}
				else
				{
					newController->isConnected = false;
				}
			}

			struct threadContext thread = {0};

			// update buffer
			struct gameDisplayBuffer gameBuffer = {0};
			gameBuffer.memory = backBuffer.memory;
			gameBuffer.width = backBuffer.width;
			gameBuffer.height = backBuffer.height;
			gameBuffer.pitch = backBuffer.pitch;
			gameBuffer.bytesPerPixel = backBuffer.bytesPerPixel;

			// update display
			if (game.updateAndRender)
				game.updateAndRender(&thread, &memory, newInput, &gameBuffer, FPS);
			else
				outs("game.updateAndRender is null!");

			LARGE_INTEGER audioWallClock = getWallClock();
			float fromBeginToAudioSeconds = getSecondsElapsed(flipWallClock, audioWallClock, perfCountFrequency);

			DWORD playCursor = 0;
			DWORD writeCursor = 0;

			if (secondaryBuffer->lpVtbl->GetCurrentPosition(secondaryBuffer, &playCursor, &writeCursor) == DS_OK)
			{
				if (!soundIsValid)
				{
					soundOutput.runningSampleIndex = writeCursor / soundOutput.bytesPerSample;
					soundIsValid = true;
				}

				DWORD byteToLock = ((soundOutput.runningSampleIndex * soundOutput.bytesPerSample) % soundOutput.secondaryBufferSize);
				DWORD expectedSoundBytesPerFrame = (int)((float)(soundOutput.samplesPerSecond * soundOutput.bytesPerSample) / gameUpdateHz);
				float secondsLeftUntilFlip = (targetSecondsPerFrame - fromBeginToAudioSeconds);
				DWORD expectedBytesUntilFlip = (DWORD)((secondsLeftUntilFlip / targetSecondsPerFrame) * (float)expectedSoundBytesPerFrame);
				DWORD expectedFrameBoundaryByte = playCursor + expectedBytesUntilFlip;
				DWORD safeWriteCursor = writeCursor;

				if (safeWriteCursor < playCursor)
					safeWriteCursor += soundOutput.secondaryBufferSize;

				assert(safeWriteCursor >= playCursor);
				safeWriteCursor += soundOutput.safetyBytes;

				bool audioCardIsLowLatency = (safeWriteCursor < expectedFrameBoundaryByte);

				DWORD targetCursor = 0;
				if (audioCardIsLowLatency)
					targetCursor = expectedFrameBoundaryByte + expectedSoundBytesPerFrame;
				else
					targetCursor = writeCursor + expectedSoundBytesPerFrame + soundOutput.safetyBytes;
				targetCursor = targetCursor % soundOutput.secondaryBufferSize;

				DWORD bytesToWrite = 0;
				if (byteToLock > targetCursor)
				{
					bytesToWrite = (soundOutput.secondaryBufferSize - byteToLock);
					bytesToWrite += targetCursor;
				}
				else
					bytesToWrite = targetCursor - byteToLock;

				struct gameSoundOutputBuffer soundBuffer = {0};
				soundBuffer.samplesPerSecond = soundOutput.samplesPerSecond;
				soundBuffer.sampleCount = bytesToWrite / soundOutput.bytesPerSample;
				soundBuffer.samples = samples;

				// update sound
				if (game.getSoundSamples)
					game.getSoundSamples(&thread, &memory, &soundBuffer);
				else
					outs("game.getSoundSamples is null!");

				// audio debugging info
				// struct win32debugTimeMarker *marker = &debugTimeMarkers[debugTimeMarkerIndex];
				// marker->outputPlayCursor = playCursor;
				// marker->outputWriteCursor = writeCursor;
				// marker->outputLocation = byteToLock;
				// marker->outputByteCount = bytesToWrite;
				// marker->expectedFlipPlayCursor = expectedFrameBoundaryByte;

				DWORD unwrappedWriteCursor = writeCursor;
				if (unwrappedWriteCursor < playCursor)
					unwrappedWriteCursor += soundOutput.secondaryBufferSize;

				//audioLatencyBytes = unwrappedWriteCursor - playCursor;
				//audioLatencySeconds = (((float)audioLatencyBytes / (float)soundOutput.bytesPerSample) / (float)soundOutput.samplesPerSecond);

				fillSoundBuffer(&soundOutput, byteToLock, bytesToWrite, &soundBuffer);
			}
			else
				soundIsValid = false;

			LARGE_INTEGER workCounter = getWallClock();
			float workSecondsElapsed = getSecondsElapsed(lastCounter, workCounter, perfCountFrequency);
			float secondsElapsedForFrame = workSecondsElapsed;

			if (secondsElapsedForFrame < targetSecondsPerFrame)
			{
				if (sleepIsGranular)
				{
					DWORD sleepMs = (DWORD)(1000.0f * (targetSecondsPerFrame - secondsElapsedForFrame));
					if (sleepMs > 0)
						Sleep(sleepMs);
					FPS = (float)sleepMs;
				}

				float testSecondsElapsedForFrame = getSecondsElapsed(lastCounter, getWallClock(), perfCountFrequency);
				if (testSecondsElapsedForFrame < targetSecondsPerFrame)
				{
					// missed sleep here
					//outs("Missed sleep");
				}

				while (secondsElapsedForFrame < targetSecondsPerFrame)
					secondsElapsedForFrame = getSecondsElapsed(lastCounter, getWallClock(), perfCountFrequency);
			}
			else
			{
				// missed frame rate
				outs("Missed frame rate");
			}

			LARGE_INTEGER endCounter = getWallClock();
			float msPerFrame = 1000.0f * getSecondsElapsed(lastCounter, endCounter, perfCountFrequency);
			lastCounter = endCounter;

			HDC deviceContext = GetDC(hwnd);
			displayBuffer(&backBuffer, deviceContext);//, WINDOW_WIDTH, WINDOW_HEIGHT);
			ReleaseDC(hwnd, deviceContext);

			flipWallClock = getWallClock();

			// debug code
			// {
			// 	DWORD playCursor2;
			// 	DWORD writeCursor2;
			// 	if (secondaryBuffer->lpVtbl->GetCurrentPosition(secondaryBuffer, &playCursor2, &writeCursor2) == DS_OK)
			// 	{
			// 		assert(debugTimeMarkerIndex < arrayCount(debugTimeMarkers));
			// 		struct win32debugTimeMarker *marker = &debugTimeMarkers[debugTimeMarkerIndex];
			// 		marker->flipPlayCursor = playCursor2;
			// 		marker->flipWriteCursor = writeCursor2;
			// 	}
			// }

			struct gameInput *temp = newInput;
			newInput = oldInput;
			oldInput = temp;

			// get and display end counts
			//uint64_t endCycleCount = __rdtsc();
			//uint64_t cyclesElapsed = endCycleCount - lastCycleCount;

			//LARGE_INTEGER endCounter;
			//QueryPerformanceCounter(&endCounter);
			//int64_t counterElapsed = endCounter.QuadPart - lastCounter.QuadPart;
			//float msPerFrame = ((1000.0f * (float)counterElapsed) / (float)perfCountFrequency);
			//float FPS = (float)perfCountFrequency / (float)counterElapsed;
			//float MCPF = (float)cyclesElapsed / (1000.0f * 1000.0f);

			//char buf[256];
			//sprintf(buf, "%.02fms, %.02f fps, %.02f megacycles per frame\n", msPerFrame, FPS, MCPF);
			//OutputDebugStringA(buf);

			//lastCounter = endCounter;
			//lastCycleCount = endCycleCount;

			// ++debugTimeMarkerIndex;
			// if (debugTimeMarkerIndex == arrayCount(debugTimeMarkers))
			// 	debugTimeMarkerIndex = 0;
		}
	}

	return 0;
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
		case WM_CLOSE:
			DestroyWindow(hwnd);
			break;
		// case WM_ACTIVATEAPP:
		// 	if(wParam == TRUE)
		// 		SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), 255, LWA_ALPHA);
		// 	else
		// 		SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), 64, LWA_ALPHA);
		// 	break;
		case WM_PAINT:
		{
			PAINTSTRUCT paint;
			HDC deviceContext = BeginPaint(hwnd, &paint);
			displayBuffer(&backBuffer, deviceContext);//, WINDOW_WIDTH, WINDOW_HEIGHT);
			EndPaint(hwnd, &paint);
			break;
		}
		case WM_SYSKEYDOWN:
		case WM_SYSKEYUP:
		case WM_KEYDOWN:
		case WM_KEYUP:
			assert(!"Keyboard input arrived through a non-dispatch message");
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

// strcat()
static void catStrings(size_t sourceAcount, char *sourceA,
				size_t sourceBcount, char *sourceB,
				size_t destCount, char *dest)
{
	for (int i = 0; i < sourceAcount; ++i)
		*dest++ = *sourceA++;
	for (int i = 0; i < sourceBcount; ++i)
		*dest++ = *sourceB++;
	*dest++ = 0;
}

// strlen()
static int stringLength(char *string)
{
	int count = 0;
	while (*string++)
		++count;
	return count;
}

// build path of current exe path + DLL name
static void buildDLLPathFilename(struct win32state *state, char *filename, int destCount, char *dest)
{
	catStrings(state->onePastLastSlash - state->exeFilename,
			   state->exeFilename, stringLength(filename), filename, destCount, dest);
}

// store current exe filename & size
static void getExeFilename(struct win32state *state)
{
	//DWORD sizeOfFilename = // will this be needed?
	GetModuleFileNameA(0, state->exeFilename, sizeof(state->exeFilename));
	state->onePastLastSlash = state->exeFilename;

	// get position of first char of filename after last slash
	for (char *scan = state->exeFilename; *scan; ++scan)
		if (*scan == '\\')
			state->onePastLastSlash = scan + 1;
}

static FILETIME getLastWriteTime(char *filename)
{
	FILETIME lastWriteTime = {0};
	WIN32_FILE_ATTRIBUTE_DATA data;

	if (GetFileAttributesEx(filename, GetFileExInfoStandard, &data))
		lastWriteTime = data.ftLastWriteTime;

	return lastWriteTime;
}

// load functions by pointer and copy DLL to tmp
struct win32gameCode loadGameCode(char *sourceDLLname, char *tempDLLname)
{
	struct win32gameCode result = {0};
	result.DLLlastWriteTime = getLastWriteTime(sourceDLLname);
	CopyFile(sourceDLLname, tempDLLname, FALSE);
	result.gameCodeDLL = LoadLibraryA(tempDLLname);

	if (result.gameCodeDLL)
	{
		result.updateAndRender = (game_UpdateAndRender *)GetProcAddress(result.gameCodeDLL, "gameUpdateAndRender");
		result.getSoundSamples = (game_GetSoundSamples *)GetProcAddress(result.gameCodeDLL, "gameGetSoundSamples");
		result.isValid = (result.updateAndRender && result.getSoundSamples);
	}

	if (!result.isValid)
	{
		outs("Failed loading game code DLL");
		result.updateAndRender = 0;
		result.getSoundSamples = 0;
	}

	return result;
}

// unload DLL
static void unloadGameCode(struct win32gameCode *gameCode)
{
	if (gameCode->gameCodeDLL)
	{
		FreeLibrary(gameCode->gameCodeDLL);
		gameCode->gameCodeDLL = 0;
	}

	gameCode->isValid = false;
	gameCode->updateAndRender = 0;
	gameCode->getSoundSamples = 0;
}

// fill out bitmap info and assign memory for pixels
static void createBuffer(struct win32displayBuffer *buffer, int width, int height)
{
	if (buffer->memory)
		VirtualFree(buffer->memory, 0, MEM_RELEASE);

	buffer->width = width;
	buffer->height = height;
	buffer->bytesPerPixel = 4;
	buffer->info.bmiHeader.biSize = sizeof(buffer->info.bmiHeader);
	buffer->info.bmiHeader.biWidth = buffer->width;
	buffer->info.bmiHeader.biHeight = -buffer->height;
	buffer->info.bmiHeader.biPlanes = 1;
	buffer->info.bmiHeader.biBitCount = 32;
	buffer->info.bmiHeader.biCompression = BI_RGB;

	int bitmapMemorySize = buffer->width * buffer->height * buffer->bytesPerPixel;
	buffer->memory = VirtualAlloc(0, bitmapMemorySize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	buffer->pitch = buffer->width * buffer->bytesPerPixel;
}

// display stored bitmap info
static void displayBuffer(struct win32displayBuffer *buffer, HDC deviceContext)//, int width, int height)
{
	//int offsetX = 10;
	//int offsetY = 10;

	//PatBlt(deviceContext, 0, 0, buffer->height, buffer->width, BLACKNESS);
	//PatBlt(deviceContext, 0, OffsetY + Buffer->Height, WindowWidth, WindowHeight, BLACKNESS);
	//PatBlt(deviceContext, 0, 0, OffsetX, WindowHeight, BLACKNESS);
	//PatBlt(deviceContext, OffsetX + Buffer->Width, 0, WindowWidth, WindowHeight, BLACKNESS);

	StretchDIBits(deviceContext,
				  0, 0, buffer->width, buffer->height,
				  0, 0, buffer->width, buffer->height,
				  buffer->memory, &buffer->info,
				  DIB_RGB_COLORS, SRCCOPY);
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
		// GetProcAddress returns void* so cast to our function pointer type and assign to our pointer to a function
		XInputGetState = (x_input_get_state *)GetProcAddress(XInputLibrary, "XInputGetState");

		// if it fails to retrieve the symbol set it to the stub
		if (!XInputGetState)
			XInputGetState = XInputGetStateStub;

		// GetProcAddress returns void* so cast to our function pointer type and assign to our pointer to a function
		XInputSetState = (x_input_set_state *)GetProcAddress(XInputLibrary, "XInputSetState");

		// if it fails to retrieve the symbol set it to the stub
		if (!XInputSetState)
			XInputSetState = XInputSetStateStub;

		outs("XInput loaded");
	}
	else
		outs("XInput not loaded");
}

static void initDSound(HWND hwnd, int32_t samplesPerSecond, int32_t bufferSize)
{
	// load the library
	HMODULE DSoundLibrary = LoadLibraryA("dsound.dll");
	if(DSoundLibrary)
	{
		// get a directSound object! - cooperative
		win32directSoundCreate *directSoundCreate = (win32directSoundCreate *)GetProcAddress(DSoundLibrary, "DirectSoundCreate");
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
						outs("Failed setting format for primary sound buffer");
					}
				}
				else
				{
					// Diagnostic
					outs("Failed creating primary sound buffer");
				}
			}
			else
			{
				// Diagnostic
				outs("Failed setting co-operative level");
			}

			DSBUFFERDESC bufferDescription = {0};
			bufferDescription.dwSize = sizeof(bufferDescription);
			bufferDescription.dwFlags = DSBCAPS_GETCURRENTPOSITION2; // 0;
			bufferDescription.dwBufferBytes = bufferSize;
			bufferDescription.lpwfxFormat = &waveFormat;
			HRESULT Error = directSound->lpVtbl->CreateSoundBuffer(directSound, &bufferDescription, &secondaryBuffer, 0);
			if (SUCCEEDED(Error))
			{
				OutputDebugStringA("Secondary buffer created successfully.\n");
			}
			else
			{
				// Diagnostic
				outs("Failed creating secondary sound buffer");
			}
		}
		else
		{
			// Diagnostic
			outs("DirectSound creation failed");
		}
		outs("DirectSound loaded");
	}
	else
	{
		// Diagnostic
		outs("DirectSound not loaded");
	}
}

static void clearSoundBuffer(struct win32soundOutput *soundOutput)
{
	VOID *region1;
	DWORD region1Size;
	VOID *region2;
	DWORD region2Size;
	if (SUCCEEDED(secondaryBuffer->lpVtbl->Lock(secondaryBuffer, 0, soundOutput->secondaryBufferSize,
												&region1, &region1Size,
												&region2, &region2Size, 0)))
	{
		uint8_t *destSample = (uint8_t *)region1;

		for (DWORD byteIndex = 0; byteIndex < region1Size; ++byteIndex)
			*destSample++ = 0;

		destSample = (uint8_t *)region2;

		for (DWORD byteIndex = 0; byteIndex < region2Size; ++byteIndex)
			*destSample++ = 0;

		secondaryBuffer->lpVtbl->Unlock(secondaryBuffer, region1, region1Size, region2, region2Size);
	}
}

static void fillSoundBuffer(struct win32soundOutput *soundOutput, DWORD byteToLock,
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

static void processKeyboardMessage(struct gameButtonState *state, bool isDown)
{
	if (state->endedDown != isDown)
	{
		state->endedDown = isDown;
		++state->halfTransitionCount;
	}
}

static void processXinputDigitalButton(DWORD XinputButtonState, struct gameButtonState *oldState, DWORD buttonBit, struct gameButtonState *newState)
{
	newState->endedDown = ((XinputButtonState & buttonBit) == buttonBit);
	newState->halfTransitionCount = (oldState->endedDown != newState->endedDown) ? 1 : 0;
}

static float processXinputStickValue(SHORT value, SHORT deadzoneThreshold)
{
	float result = 0;

	if (value < -deadzoneThreshold)
		result = (float)((value + deadzoneThreshold) / (32768.0f - deadzoneThreshold));
	else if (value > deadzoneThreshold)
		result = (float)((value - deadzoneThreshold) / (32767.0f - deadzoneThreshold));

	return result;
}

static void processPendingMessages(struct win32state *state, struct gameControllerInput *keyboardController)
{
	MSG msg;
	while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
	{
		switch (msg.message)
		{
			case WM_QUIT:
				running = false;
				break;
			case WM_SYSKEYDOWN:
			case WM_SYSKEYUP:
			case WM_KEYDOWN:
			case WM_KEYUP:
			{
				uint32_t VKCode = (uint32_t)msg.wParam;

				bool wasDown = ((msg.lParam & (1 << 30)) != 0);
				//TODO maybe can fix cppcheck error here with (uint32_t) 1
				bool isDown = ((msg.lParam & (1 << 31)) == 0);

				if (wasDown != isDown)
				{
					if (VKCode == 'W')
						processKeyboardMessage(&keyboardController->moveUp, isDown);
					else if (VKCode == 'S')
						processKeyboardMessage(&keyboardController->moveDown, isDown);
					else if (VKCode == 'A')
						processKeyboardMessage(&keyboardController->moveLeft, isDown);
					else if (VKCode == 'D')
						processKeyboardMessage(&keyboardController->moveRight, isDown);
					else if (VKCode == 'Q')
						processKeyboardMessage(&keyboardController->leftShoulder, isDown);
					else if (VKCode == 'E')
						processKeyboardMessage(&keyboardController->rightShoulder, isDown);
					else if (VKCode == 'F')
						processKeyboardMessage(&keyboardController->fps, isDown);
					else if (VKCode == 'H')
						processKeyboardMessage(&keyboardController->hud, isDown);
					else if (VKCode == 'R')
						processKeyboardMessage(&keyboardController->reset, isDown);
					else if (VKCode == '1')
						processKeyboardMessage(&keyboardController->one, isDown);
					else if (VKCode == '2')
						processKeyboardMessage(&keyboardController->two, isDown);
					else if (VKCode == '3')
						processKeyboardMessage(&keyboardController->three, isDown);
					else if (VKCode == '4')
						processKeyboardMessage(&keyboardController->four, isDown);
					else if (VKCode == VK_UP)
						processKeyboardMessage(&keyboardController->actionUp, isDown);
					else if (VKCode == VK_DOWN)
						processKeyboardMessage(&keyboardController->actionDown, isDown);
					else if (VKCode == VK_LEFT)
						processKeyboardMessage(&keyboardController->actionLeft, isDown);
					else if (VKCode == VK_RIGHT)
						processKeyboardMessage(&keyboardController->actionRight, isDown);
					else if (VKCode == VK_SPACE)
						processKeyboardMessage(&keyboardController->shoot, isDown);
					else if (VKCode == VK_ESCAPE)
					{
						running = false;
						processKeyboardMessage(&keyboardController->start, isDown);
					}
					else if (VKCode == 'P')
					{
						if (isDown)
							paused = !paused;
					}
				}

				bool altKeyWasDown = (msg.lParam & (1 << 29));
				if ((VKCode == VK_F4) && altKeyWasDown)
				{
					//OutputDebugStringA("ALT F4\n");
					running = false;
				}
			}
				break;
			default:
				TranslateMessage(&msg);
				DispatchMessageA(&msg);
				break;
		}
	}
}

static LARGE_INTEGER getWallClock(void)
{
	LARGE_INTEGER result;
	QueryPerformanceCounter(&result);
	return result;
}

static float getSecondsElapsed(LARGE_INTEGER start, LARGE_INTEGER end, int64_t perfCountFrequency)
{
	float result = ((float)(end.QuadPart - start.QuadPart) / (float)perfCountFrequency);
	return result;
}
