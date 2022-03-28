#include "engine_pch.h"

#include "RHI/D3D12/D3D12RHI.h"

// Link d3d12 libs
#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dxgi.lib")

namespace ks
{

	FD3D12RHI::~FD3D12RHI()
	{
		KS_INFO(TEXT("~FD3D12RHI"));
	}

	void FD3D12RHI::Init()
	{
		KS_INFO(TEXT("FD3D12RHI::Init"));
		// Create and initialize RHI
		{
			KS_D3D12_CALL(CreateDXGIFactory1(IID_PPV_ARGS(&DXGIFactory)));
			// Try to create hardware device.
			HRESULT _HRESULT = D3D12CreateDevice(
				nullptr,             // default adapter
				D3D_FEATURE_LEVEL_11_0,
				IID_PPV_ARGS(&D3D12Device));

			// Fall back to WARP device.
			if (FAILED(_HRESULT))
			{
				ComPtr<IDXGIAdapter> DXGIAdapter;
				KS_D3D12_CALL(DXGIFactory->EnumWarpAdapter(IID_PPV_ARGS(&DXGIAdapter)));

				KS_D3D12_CALL(D3D12CreateDevice(
					DXGIAdapter.Get(),
					D3D_FEATURE_LEVEL_11_0,
					IID_PPV_ARGS(&D3D12Device)));
			}

			KS_D3D12_CALL(D3D12Device->CreateFence(0, D3D12_FENCE_FLAG_NONE,
				IID_PPV_ARGS(&D3D12Fence)));

			RTVSize = D3D12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
			DSVSize = D3D12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
			CBVSize = D3D12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		}
	}

	void FD3D12RHI::Shutdown()
	{
		KS_INFO(TEXT("\tFD3D12RHI::Shutdown"));
		DXGIFactory.Reset();
		D3D12Device.Reset();
		D3D12Fence.Reset();
	}

}

