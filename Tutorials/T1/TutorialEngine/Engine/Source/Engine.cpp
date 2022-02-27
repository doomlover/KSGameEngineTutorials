
#include "engine_pch.h"
#include "Engine.h"

#include <wrl.h> // For ComPtr
#include <dxgi1_6.h>
#include <d3d12.h>
#include "RHI/D3D12/d3d12x.h" // helper structures and functions
#include "RHI/D3D12/D3D12Common.h"

// Link d3d12 libs
#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dxgi.lib")

using Microsoft::WRL::ComPtr;

namespace ks
{
namespace d3d12
{
	Microsoft::WRL::ComPtr<IDXGIFactory4> mdxgiFactory;
	Microsoft::WRL::ComPtr<IDXGISwapChain> mSwapChain;
	Microsoft::WRL::ComPtr<ID3D12Device> md3dDevice;

	Microsoft::WRL::ComPtr<ID3D12Fence> mFence;
	UINT64 mCurrentFence = 0;

	Microsoft::WRL::ComPtr<ID3D12CommandQueue> mCommandQueue;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> mDirectCmdListAlloc;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> mCommandList;

	static const int SwapChainBufferCount = 2;
	int mCurrBackBuffer = 0;
	Microsoft::WRL::ComPtr<ID3D12Resource> mSwapChainBuffer[SwapChainBufferCount];
	Microsoft::WRL::ComPtr<ID3D12Resource> mDepthStencilBuffer;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mRtvHeap;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mDsvHeap;

	D3D12_VIEWPORT mScreenViewport;
	D3D12_RECT mScissorRect;

	UINT mRtvDescriptorSize = 0;
	UINT mDsvDescriptorSize = 0;
	UINT mCbvSrvUavDescriptorSize = 0;

	D3D_DRIVER_TYPE md3dDriverType = D3D_DRIVER_TYPE_HARDWARE;
	DXGI_FORMAT mBackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	DXGI_FORMAT mDepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
}
}

void ks::engine::Init()
{
	GApp->Init();

	gfx::Init();
	graphics::Init();
	RHI::Init();
	// RHI::DrawIndexedInstance(MeshBatch/Primitive/RenderItem/GeoMesh);

	// Create and initialize RHI
	{
		KS_D3D12_CALL(CreateDXGIFactory1(IID_PPV_ARGS(&ks::d3d12::mdxgiFactory)));
		// Try to create hardware device.
		HRESULT hardwareResult = D3D12CreateDevice(
			nullptr,             // default adapter
			D3D_FEATURE_LEVEL_11_0,
			IID_PPV_ARGS(&ks::d3d12::md3dDevice));

		// Fall back to WARP device.
		if (FAILED(hardwareResult))
		{
			ComPtr<IDXGIAdapter> pWarpAdapter;
			KS_D3D12_CALL(ks::d3d12::mdxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&pWarpAdapter)));

			KS_D3D12_CALL(D3D12CreateDevice(
				pWarpAdapter.Get(),
				D3D_FEATURE_LEVEL_11_0,
				IID_PPV_ARGS(&ks::d3d12::md3dDevice)));
		}

		KS_D3D12_CALL(ks::d3d12::md3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE,
			IID_PPV_ARGS(&ks::d3d12::mFence)));

		ks::d3d12::mRtvDescriptorSize = ks::d3d12::md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		ks::d3d12::mDsvDescriptorSize = ks::d3d12::md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
		ks::d3d12::mCbvSrvUavDescriptorSize = ks::d3d12::md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}
}

void ks::engine::Tick()
{
	GApp->Tick();
	// RHI::Tick();
}

void ks::engine::Shutdown()
{
	{
		ks::d3d12::mdxgiFactory.Reset();
		ks::d3d12::md3dDevice.Reset();
		ks::d3d12::mFence.Reset();
	}
	GApp->Shutdown();
}
