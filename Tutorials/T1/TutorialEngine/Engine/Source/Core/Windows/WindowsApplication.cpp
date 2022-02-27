#include "engine_pch.h"

#ifdef WINDOWS_PLATFORM

#include "Core/Windows/WindowsApplication.h"
#include "Engine.h"

#define MAX_NAME_STRING 256
#define HInstance() GetModuleHandle(NULL)

WCHAR WindowClass[MAX_NAME_STRING];
WCHAR WindowTitle[MAX_NAME_STRING];

void CreateWindowClass();

FWinApp::FWinApp()
	:hInstance(nullptr)
	,WindowWidth(800)
	,WindowHeight(600)
{
}

FWinApp::~FWinApp()
{
}

void FWinApp::Init()
{
	hInstance = HInstance();

	// TODO : use config file
	/* Initialize Global Variables */
	wcscpy_s(WindowClass, TEXT("TutorialGameWindow"));
	wcscpy_s(WindowTitle, TEXT("TutorialWindow"));
	WindowWidth = 800;
	WindowHeight = 600;

	/* Create Window Class */
	CreateWindowClass();

	/* Create and Display our Window */
	CreateAndShowWindow();

	IApp::Init();
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

int FWinApp::CreateAndShowWindow()
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

int KS_API WinMainEntry(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
	ks::engine::Init();

	/* Listen for Message events*/
	MSG msg = { 0 };
	while (msg.message != WM_QUIT)
	{
		// If there are Window messages then process them
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		ks::engine::Tick();
	}

	ks::engine::Shutdown();

	return 0;
}

#endif
