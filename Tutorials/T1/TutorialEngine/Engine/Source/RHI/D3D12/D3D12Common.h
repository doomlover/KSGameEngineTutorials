#pragma once

#ifdef KS_DEBUG_BUILD
#define KS_D3D12_CALL(x)\
	if(FAILED(x)) {\
		__debugbreak();\
	}
#else
#define KS_D3D12_CALL(x) x
#endif // KS_DEBUG_BUILD
