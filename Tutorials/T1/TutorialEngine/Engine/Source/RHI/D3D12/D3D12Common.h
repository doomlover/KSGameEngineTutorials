#pragma once

#include <wrl.h> // For ComPtr
#include <dxgi1_6.h>
#include <d3d12.h>
#include "RHI/D3D12/d3d12x.h" // helper structures and functions

using Microsoft::WRL::ComPtr;

#ifdef KS_DEBUG_BUILD
#define KS_D3D12_CALL(x)\
	if(FAILED(x)) {\
		__debugbreak();\
	}
#else
#define KS_D3D12_CALL(x) x
#endif // KS_DEBUG_BUILD
