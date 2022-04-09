#pragma once

#pragma warning(disable:4251)

#include <algorithm>
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <memory>
#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>
#include <array>
#include <set>
#include <queue>
#include <deque>

#include "Core/Platform.h"
#include "Core/Math.h"
#include "Core/Json.h"

using json = nlohmann::json;

namespace ks
{
	/* enum version of gltf accessor.componentType */
	enum class EDATA_TYPE : unsigned short
	{
		BYTE = 5120,
		UNSIGNED_BYTE = 5121,
		SHORT = 5122,
		UNSIGNED_SHORT = 5123,
		UNSIGNED_INT = 5125,
		FLOAT = 5126,
		INVALID,
	};
	/* enum version of gltf accessor.type */
	enum class EELEM_TYPE : unsigned short
	{
		SCALAR,
		VEC2,
		VEC3,
		VEC4,
		MAT2,
		MAT3,
		MAT4,
		INVALID,
	};

	enum class EELEM_FORMAT : unsigned int
	{
		R8_INT,
		R8_UINT,
		R16_INT,
		R16_UINT,
		R32G32B32_FLOAT,
		R32G32B32A32_FLOAT,
		INVALID,
	};

namespace util
{
	EELEM_FORMAT GetElemFormat(EDATA_TYPE DataType, EELEM_TYPE ElemType);

	size_t GetDataTypeSize(EDATA_TYPE DataType);

	uint32 GetElemNum(EELEM_TYPE ElemType);

	inline size_t GetStride(const EELEM_TYPE& ElemType, const EDATA_TYPE& DataType) {
		return GetElemNum(ElemType) * GetDataTypeSize(DataType);
	}

	std::string GetContentPath(const std::string& Path);

	std::string GetShaderPath(const std::string& Path);
}

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

		static std::string GetRootPath(const std::string& Path)
		{
			size_t RootPos = Path.find_last_of("/");
			assert(RootPos != std::string::npos);
			return Path.substr(0, RootPos);
		}
	};
}