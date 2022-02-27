#pragma once

#include <cstdint>

#ifdef KS_ENGINE
#define KS_API __declspec(dllexport)
#else
#define KS_API __declspec(dllimport)
#endif

#ifdef _WINDOWS
#define WINDOWS_PLATFORM
#define PLATFORM_HEADER Windows
#endif

#ifdef _DEBUG
#define KS_DEBUG_BUILD
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

// base types
typedef int8_t		int8;
typedef int32_t		int32;
typedef int64_t		int64;
typedef uint8_t		uint8;
typedef uint32_t	uint32;
typedef uint64_t	uint64;

#if !defined(TEXT)
#if PLATFORM_TCHAR_IS_CHAR16
#define TEXT_PASTE(x) u ## x
#else
#define TEXT_PASTE(x) L ## x
#endif
#define TEXT(x) TEXT_PASTE(x)
#endif

using namespace std;