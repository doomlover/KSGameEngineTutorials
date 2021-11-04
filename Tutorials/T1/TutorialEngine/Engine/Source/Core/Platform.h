#pragma once

#ifdef KS_ENGINE
#define KS_API __declspec(dllexport)
#else
#define KS_API __declspec(dllimport)
#endif

#ifdef _WINDOWS
#define WINDOWS_PLATFORM
#define PLATFORM_HEADER Windows
#endif

// Turns an preprocessor token into a real string (see UBT_COMPILED_PLATFORM)
#define PREPROCESSOR_TO_STRING(x) PREPROCESSOR_TO_STRING_INNER(x)
#define PREPROCESSOR_TO_STRING_INNER(x) #x

// Concatenates two preprocessor tokens, performing macro expansion on them first
#define PREPROCESSOR_JOIN(x, y) PREPROCESSOR_JOIN_INNER(x, y)
#define PREPROCESSOR_JOIN_INNER(x, y) x##y

// Creates a string that can be used to include a header in the form "Platform/PlatformHeader.h", like "Windows/WindowsPlatformFile.h"
#define COMPILED_PLATFORM_HEADER(Suffix) PREPROCESSOR_TO_STRING(PREPROCESSOR_JOIN(PLATFORM_HEADER/PLATFORM_HEADER, Suffix))

#include COMPILED_PLATFORM_HEADER(Platform.h)

#ifdef WINDOWS_PLATFORM
#define KS_MAIN_ENTRY(AppClass) \
int CALLBACK WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd) {\
	IApp* App = new AppClass;\
	int Result = WinMainEntry(App, hInstance, hPrevInstance, lpCmdLine, nShowCmd);\
	delete App;\
	return Result;\
}
#else
#define KS_MAIN_ENTRY(AppClass)\
int main(int argc, const char* argv[]) {\
	return 0;\
}
#endif