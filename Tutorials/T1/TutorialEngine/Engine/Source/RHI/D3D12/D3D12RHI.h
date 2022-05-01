#pragma once

#include "RHI/RHI.h"
#include "RHI/D3D12/D3D12Common.h"

namespace ks::d3d12
{
	class FD3D12Resource;
	class FD3D12RHI;

	extern FD3D12RHI* GD3D12RHI;
	extern ID3D12Device* GD3D12Device;
	extern ID3D12GraphicsCommandList* GGfxCmdlist;

	class FDescriptorHandle
	{
	public:
		D3D12_CPU_DESCRIPTOR_HANDLE CpuHandle{};
		D3D12_GPU_DESCRIPTOR_HANDLE GpuHandle{};
	};

	class FDescriptorHeap
	{
	public:
		explicit FDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE _Type) : Type{ _Type } {}
		void Init(uint32 _Capacity, bool bShaderVisiable);
		FDescriptorHandle Allocate();
		ID3D12DescriptorHeap* GetHeap() { return Heap.Get(); }
		D3D12_CPU_DESCRIPTOR_HANDLE GetCpuStart() const { return CpuStart; }
		D3D12_GPU_DESCRIPTOR_HANDLE GetGpuStart() const { return GpuStart; }
		uint32 GetSize() const { return CurrFreeHanle; }
		uint32 GetDescriptorSize() const { return DescriptorSize; }
		bool IsShaderVisiable() const { return GpuStart.ptr != 0; }

	private:
		ComPtr<ID3D12DescriptorHeap> Heap;
		D3D12_CPU_DESCRIPTOR_HANDLE CpuStart{};
		D3D12_GPU_DESCRIPTOR_HANDLE GpuStart{};
		std::vector<uint32> FreeHandles{};
		uint32 Capacity{ 0 };
		uint32 CurrFreeHanle{ 0 };
		uint32 DescriptorSize{ 0 };
		const D3D12_DESCRIPTOR_HEAP_TYPE Type{};
	};

	class FD3D12RHI : public IRHI
	{
	public:
		// RHI interface
		virtual ~FD3D12RHI();
		virtual void Init() override;
		virtual void Shutdown() override;
		virtual void ResizeWindow() override;
		virtual void FlushRenderingCommands() override;
		virtual void BeginFrame() override;
		virtual void EndFrame() override;
		virtual IRHIConstBuffer* CreateConstBuffer(const void* Data, uint32 Size) override;
		virtual IRHIConstBuffer1* CreateConstBuffer1(const void* Data, uint32 Size) override;
		virtual IRHIBuffer* CreateBuffer(uint32 Size, const void* Data) override;
		virtual void SetPipelineState(IRHIPipelineState* PipelineState) override;
		virtual IRHIPipelineState* CreatePipelineState(const FRHIPipelineStateDesc& Desc) override;
		virtual void SetShaderConstBuffer(IRHIConstBuffer* ConstBuffer) override;
		virtual void SetConstBuffer(IRHIConstBuffer1* ConstBuffer) override;
		virtual void SetVertexBuffer(const IRHIVertexBuffer* VertexBuffer) override;
		virtual void SetVertexBuffers(const IRHIVertexBuffer** VertexBuffers, int32 Num) override;
		virtual IRHIVertexBuffer* CreateVertexBuffer(uint32 Stride, uint32 Size, const void* Data) override;
		virtual IRHIIndexBuffer* CreateIndexBuffer(EELEM_FORMAT ElemFormat, uint32 Count, uint32 Size, const void* Data) override;
		virtual void DrawIndexedPrimitive(const IRHIIndexBuffer* IndexBuffer) override;
		// static helper functions
		static d3d12::FD3D12Resource* CreateConstBufferResource(size_t Size);
		// d3d12 interface
		FD3D12RHI() = default;
		d3d12::FDescriptorHeap& GetCBVHeap() { return CBVHeap; }
		DXGI_FORMAT GetBackbufferFormat() const { return BackBufferFormat; }
		DXGI_FORMAT GetDepthbufferFormat() const { return DepthBufferFormat; }
		ID3D12RootSignature* GetGlobalRootSignature() { return GlobalRootSignature.Get(); }
	private:
		ID3D12Resource* _CreateBuffer(uint32_t Size, D3D12_HEAP_TYPE HeapType, D3D12_RESOURCE_STATES ResStats);
	private:
		HWND hWnd;
		ComPtr<IDXGIFactory7> DXGIFactory;
		ComPtr<IDXGISwapChain> DXGISwapChain;
		ComPtr<ID3D12Device> D3D12Device;

		ComPtr<ID3D12Fence> D3D12Fence;
		UINT64 CurrentFence = 0;

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

		D3D_DRIVER_TYPE D3DDriverType = D3D_DRIVER_TYPE_HARDWARE;
		DXGI_FORMAT BackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
		DXGI_FORMAT DepthBufferFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

		D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentBackBufferView();
		ID3D12Resource* GetCurrentBackBuffer();

		/***************************************************************/
		d3d12::FDescriptorHeap CBVHeap{D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV};
		ComPtr<ID3D12RootSignature> GlobalRootSignature{ nullptr };
	};
} // ~ks::d3d12