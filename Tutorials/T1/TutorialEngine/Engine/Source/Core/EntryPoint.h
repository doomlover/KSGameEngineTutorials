#pragma once

#include "Core/Platform.h"

#ifdef WINDOWS_PLATFORM

int CALLBACK WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
	KS_INFO(TEXT("WinMain"));
	extern std::unique_ptr<IApp> CreateApplication();
	auto App = CreateApplication();
	extern int KS_API WindowsEntry(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd);
	return WindowsEntry(hInstance, hPrevInstance, lpCmdLine, nShowCmd);
}

#define KS_CREATE_APP(AppClass) \
std::unique_ptr<IApp> CreateApplication() { \
	auto App = std::make_unique<AppClass>();\
	GApp = App.get();\
	return App;\
}
#else
#error ONLY SUPPORT WINDOWS
#endif