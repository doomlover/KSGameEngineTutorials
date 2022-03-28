#pragma once

#include "RHI/RHI.h"
#include "RHI/D3D12/D3D12Common.h"

namespace ks
{
	class FD3D12RHI : public IRHI
	{
	public:
		FD3D12RHI() {}
		virtual ~FD3D12RHI();
		virtual void Init() override;
		virtual void Shutdown() override;

	private:
		ComPtr<IDXGIFactory7> DXGIFactory;
		ComPtr<IDXGISwapChain> DXGISwapChain;
		ComPtr<ID3D12Device> D3D12Device;

		ComPtr<ID3D12Fence> D3D12Fence;
		UINT64 mCurrentFence = 0;

		ComPtr<ID3D12CommandQueue> D3D12CommandQueue;
		ComPtr<ID3D12CommandAllocator> D3D12CommandAllocator;
		ComPtr<ID3D12GraphicsCommandList> D3D12GfxCommandList;

		static const int SwapChainBufferCount = 2;
		int CurrentBackBuffer = 0;
		ComPtr<ID3D12Resource> D3D12SwapChainBuffers[SwapChainBufferCount];
		ComPtr<ID3D12Resource> D3D12DepthStencilBuffer;

		ComPtr<ID3D12DescriptorHeap> D3D12RTVHeap;
		ComPtr<ID3D12DescriptorHeap> D3D12DSVHeap;

		D3D12_VIEWPORT D3D12Viewport;
		D3D12_RECT ScissorRect;

		UINT RTVSize = 0;
		UINT DSVSize = 0;
		UINT CBVSize = 0;

		D3D_DRIVER_TYPE D3DDriverType = D3D_DRIVER_TYPE_HARDWARE;
		DXGI_FORMAT BackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
		DXGI_FORMAT DepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	};
}