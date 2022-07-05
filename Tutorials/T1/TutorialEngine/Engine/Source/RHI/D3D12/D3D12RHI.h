#pragma once

#include "RHI/RHI.h"
#include "D3D12Resource.h"
#include "RHI/D3D12/D3D12Common.h"

namespace ks::d3d12
{
	class FD3D12Resource;
	class FD3D12RHI;
	struct FD3D12RHIContext;

	extern FD3D12RHI* GD3D12RHI;

	class FDescriptorHeap
	{
	public:
		explicit FDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE _Type) : Type{ _Type } {}
		virtual ~FDescriptorHeap() {}
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
		virtual void Init(const FRHIConfig& Config) override;
		virtual void Shutdown() override;
		virtual void ResizeWindow() override;
		virtual void FlushRenderingCommands() override;
		virtual void BeginFrame() override;
		virtual void EndFrame() override;
		virtual IRHIConstBuffer* CreateConstBuffer(const void* Data, uint32 Size) override;
		virtual IRHIConstBuffer1* CreateConstBuffer1(const void* Data, uint32 Size) override;
		virtual void SetPipelineState(IRHIPipelineState* PipelineState) override;
		virtual IRHIPipelineState* CreatePipelineState(const FRHIPipelineStateDesc& Desc) override;
		virtual void SetShaderConstBuffer(IRHIConstBuffer* ConstBuffer) override;
		virtual void SetConstBuffer(IRHIConstBuffer1* ConstBuffer) override;
		virtual void SetVertexBuffer(const IRHIVertexBuffer* VertexBuffer) override;
		virtual void SetVertexBuffer1(const IRHIVertexBuffer1* VertexBuffer) override;
		virtual void SetVertexBuffers(const IRHIVertexBuffer** VertexBuffers, int32 Num) override;
		virtual void SetVertexBuffers1(const IRHIVertexBuffer1** VertexBuffers, int32 Num) override;
		virtual IRHIVertexBuffer* CreateVertexBuffer(uint32 Stride, uint32 Size, const void* Data) override;
		virtual IRHIVertexBuffer1* CreateVertexBuffer1(uint32 Stride, uint32 Size, const void* Data) override;
		virtual IRHIIndexBuffer* CreateIndexBuffer(EELEM_FORMAT ElemFormat, uint32 Count, uint32 Size, const void* Data) override;
		virtual IRHIIndexBuffer1* CreateIndexBuffer1(EELEM_FORMAT ElemFormat, uint32 Count, uint32 Size, const void* Data) override;
		virtual void DrawIndexedPrimitive(const IRHIIndexBuffer* IndexBuffer) override;
		virtual void DrawIndexedPrimitive1(const IRHIIndexBuffer1* IndexBuffer) override;
		virtual IRHITexture2D* CreateTexture2D(const FTexture2DDesc& Desc) override;
		virtual IRHIDepthStencilBuffer* CreateDepthStencilBuffer(const FTexture2DDesc& Desc) override;
		virtual void SetViewports(uint32_t Num, const FViewPort* Viewports) override;
		virtual void ClearRenderTarget(const FColor& Color) override;
		virtual void SetRenderTarget(IRHIRenderTarget* RenderTarget, IRHIDepthStencilBuffer* DepthBuffer) override;
		virtual void ClearDepthStencilBuffer() override;
		virtual void BeginPass() override;
		virtual void EndPass() override;
		virtual IRHIRenderTarget* GetCurrentBackBuffer() override;
		virtual IRHIDepthStencilBuffer* GetDefaultDepthStencilBuffer() override;
		virtual void SetTexture2D(IRHITexture2D* Texture2D) override;
		virtual IRHIRenderTarget* CreateRenderTarget(const FTexture2DDesc& Desc) override;
		// static helper functions
		static d3d12::FD3D12Resource* CreateConstBufferResource(size_t Size);
		// d3d12 interface
		FD3D12RHI() = default;
		FDescriptorHeap& GetCBVHeap();
		DXGI_FORMAT GetBackbufferFormat() const { return BackBufferFormat; }
		DXGI_FORMAT GetDepthbufferFormat() const { return DepthBufferFormat; }
		ID3D12RootSignature* GetGlobalRootSignature();
		static const int SwapChainBufferCount = 2;
	private:
		IRHIBuffer* CreateBuffer(uint32 Size, const void* Data);
		void CreateBuffer1(uint32_t Size, D3D12_HEAP_TYPE HeapType, D3D12_RESOURCE_STATES ResStats, ComPtr<ID3D12Resource>& OutResource);
		int32_t UploadResourceData(ID3D12Resource* DestResource, const void* Data, uint32_t Size);
		D3D12_CPU_DESCRIPTOR_HANDLE GetDefaultBackBufferView();
		ID3D12Resource* GetDefaultBackBufferResource();
		D3D12_CPU_DESCRIPTOR_HANDLE GetDefaultDepthBufferView();
	private:
		FD3D12RHIContext* Context{ nullptr };
		HWND hWnd;
		/***************************************************************/
		ComPtr<IDXGIFactory7> DXGIFactory;
		ComPtr<ID3D12Device> D3D12Device;

		UINT64 CurrentFence = 0;
		int CurrentBackBuffer = 0;

		D3D12_VIEWPORT D3D12Viewport;
		D3D12_RECT ScissorRect;

		// going to be deprecated
		UINT RTVSize = 0;
		UINT DSVSize = 0;

		D3D_DRIVER_TYPE D3DDriverType = D3D_DRIVER_TYPE_HARDWARE;
		DXGI_FORMAT BackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
		DXGI_FORMAT DepthBufferFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	};
} // ~ks::d3d12