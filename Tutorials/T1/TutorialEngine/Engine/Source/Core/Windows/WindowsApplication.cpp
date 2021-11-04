#include "engine_pch.h"

#ifdef WINDOWS_PLATFORM

#include "Core/Windows/WindowsApplication.h"

#define MAX_NAME_STRING 256
#define HInstance() GetModuleHandle(NULL)

WCHAR WindowClass[MAX_NAME_STRING];
WCHAR WindowTitle[MAX_NAME_STRING];
int WindowWidth;
int WindowHeight;

FWinApp::~FWinApp()
{
}

void FWinApp::Init()
{
}

LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	// TODO : move to your app implication
	switch (message) {
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	}
	return DefWindowProc(hWnd, message, wparam, lparam);
}

void InitConfig()
{
	wcscpy_s(WindowClass, TEXT("TutorialGameWindow"));
	wcscpy_s(WindowTitle, TEXT("TutorialWindow"));
	WindowWidth = 800;
	WindowHeight = 600;

	FWinApp* WinApp = static_cast<FWinApp*>(GApp);
	WinApp->hInstance = HInstance();
	WinApp->Init();
}

void CreateWindowClass()
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;

	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);

	wcex.hIcon = LoadIcon(0, IDI_APPLICATION);
	wcex.hIconSm = LoadIcon(0, IDI_APPLICATION);

	wcex.lpszClassName = WindowClass;
	wcex.lpszMenuName = nullptr;

	wcex.hInstance = HInstance();

	/* Your custom window process */
	wcex.lpfnWndProc = WindowProc;
	RegisterClassEx(&wcex);
}

int CreateAndShowWindow()
{
	HWND hWnd = CreateWindow(WindowClass, WindowTitle, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, WindowWidth, WindowHeight, nullptr, nullptr, HInstance(), nullptr);
	if (!hWnd)
	{
		MessageBox(0, TEXT("Failed to Create Window!"), 0, 0);
		return -1;
	}
	return ShowWindow(hWnd, SW_SHOW);
}

void GameLoop()
{
	MSG msg = { 0 };
	while (msg.message != WM_QUIT)
	{
		// If there are Window messages then process them
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		GApp->Tick();
	}
	GApp->Shutdown();
}

int WinMainEntry(IApp* App, _In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
	GApp = App;

	/* Initialize Global Variables */
	InitConfig();

	/* Create Window Class */
	CreateWindowClass();

	/* Create and Display our Window */
	if (CreateAndShowWindow() != 0) return -1;

	/* Listen for Message events*/
	GameLoop();

	return 0;
}

#endif
