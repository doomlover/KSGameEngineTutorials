#include "engine_pch.h"

#include "Engine.h"

#ifdef WINDOWS_PLATFORM

#include "Core/Windows/WindowsApplication.h"

using namespace ks;

#define MAX_NAME_STRING 256
#define HInstance() GetModuleHandle(NULL)

WCHAR WindowClass[MAX_NAME_STRING];
WCHAR WindowTitle[MAX_NAME_STRING];

namespace ks
{
	void CreateWindowClass();

	HINSTANCE GHINSTANCE;

	FWinApp::FWinApp()
		:WindowWidth(800)
		,WindowHeight(600)
	{
	}

	FWinApp::~FWinApp()
	{
	}

	void FWinApp::Init()
	{
		KS_INFO(TEXT("FWinApp::Init"));

		assert(HInstance() == GHINSTANCE);

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
			return 0;
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

	int KS_API WindowsEntry(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
	{
		GHINSTANCE = hInstance;

		// Get command line arguments
		std::wstring CmdLineArgs = TEXT("");
		{
			int argc = 0;
			LPWSTR* argv = ::CommandLineToArgvW(::GetCommandLineW(), &argc);
			if (argv != nullptr)
			{
				// jump the program name
				for (int32 i = 1; i < argc; ++i)
				{
					CmdLineArgs += TEXT(" ");
					std::wstring ArgStr = argv[i];
					CmdLineArgs += ArgStr;
				}
				::LocalFree(argv);
			}
		}
		GCmdLineArgs = CmdLineArgs;
		KS_INFO(CmdLineArgs.c_str());

		/*
		engine::Init();
		while(!engine::RequestExit())
		{
			engine::PumpMessages();
			engine::Tick();
		}
		engine::Shutdown();
		*/

		engine::Init();

		bool bQuit = false;
		while (!bQuit)
		{
			/* Listen for Message events*/
			MSG msg = { 0 };
			// If there are Window messages then process them
			while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
				if (msg.message == WM_QUIT) bQuit = true;
			}
			engine::Tick();
		}

		engine::Shutdown();

		return 0;
	}
}

#endif
