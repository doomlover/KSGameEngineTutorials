#pragma once

#pragma warning(disable:4251)

#include <codecvt>
#include <memory>
#include <cassert>
#include <string>
#include <sstream>
#include <iostream>
#include <vector>

#include "Core/Platform.h"
#include "RHI/RHI.h"
#include "Engine.h"

using namespace std;

namespace ks
{
	class KS_API FString
	{
	public:
		static std::string WS2S(const std::wstring& wstr)
		{
#ifdef WINDOWS_PLATFORM
			if (wstr.empty()) return std::string();
			int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
			std::string strTo(size_needed, 0);
			WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
			return strTo;
#else
#error NOT IMPLEMENTED!
#endif
		}

		static std::wstring S2WS(const std::string& str)
		{
#ifdef WINDOWS_PLATFORM
			if (str.empty()) return std::wstring();
			int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
			std::wstring wstrTo(size_needed, 0);
			MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
			return wstrTo;
#else
#error NOT IMPLEMENTED!
#endif
		}
	};
}