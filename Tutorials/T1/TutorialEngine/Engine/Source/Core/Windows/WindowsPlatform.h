#pragma once

#include <windows.h>
#include "Core/Windows/WindowsApplication.h"

#ifndef KS_INFO
#define KS_INFO(x) \
OutputDebugString(x);\
OutputDebugString(TEXT("\n"))
#endif

#ifndef KS_INFOA
#define KS_INFOA(x) \
OutputDebugStringA(x);\
OutputDebugStringA("\n")
#endif

