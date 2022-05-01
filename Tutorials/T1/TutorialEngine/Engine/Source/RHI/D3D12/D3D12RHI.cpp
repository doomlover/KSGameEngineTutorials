#include "engine_pch.h"

#include "RHI/D3D12/D3D12RHI.h"
#include "D3D12Resource.h"
#include "D3D12PipelineState.h"

// Link d3d12 libs
#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dxgi.lib")

namespace ks::d3d12
{
	const uint32 MAX_CBV_NUM = 128;
	FD3D12RHI* GD3D12RHI = nullptr;
	ID3D12Device* GD3D12Device = nullptr;
	ID3D12GraphicsCommandList* GGfxCmdlist = nullptr;

	DXGI_FORMAT GetDXGIFormat(EELEM_FORMAT ElemFormat)
	{
		static const std::unordered_map<EELEM_FORMAT, DXGI_FORMAT> FormatTable = {
			{EELEM_FORMAT::R8_UINT,				DXGI_FORMAT::DXGI_FORMAT_R8_UINT},
			{EELEM_FORMAT::R8_INT,				DXGI_FORMAT::DXGI_FORMAT_R8_SINT},
			{EELEM_FORMAT::R16_INT,				DXGI_FORMAT::DXGI_FORMAT_R16_SINT},
			{EELEM_FORMAT::R16_UINT,			DXGI_FORMAT::DXGI_FORMAT_R16_UINT},
			{EELEM_FORMAT::R32G32B32_FLOAT,		DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT},
		};
		return FormatTable.at(ElemFormat);
	}

	void FDescriptorHeap::Init(uint32 _Capacity, bool bShaderVisiable)
	{
		assert(Capacity == 0 && CurrFreeHanle == 0);
		Capacity = _Capacity;
		FreeHandles.reserve(Capacity);

		for (uint32 i{0}; i < _Capacity; ++i)
		{
			FreeHandles.push_back(i);
		}

		if (Type == D3D12_DESCRIPTOR_HEAP_TYPE_DSV ||
			Type == D3D12_DESCRIPTOR_HEAP_TYPE_RTV)
		{
			bShaderVisiable = false;
		}

		D3D12_DESCRIPTOR_HEAP_DESC desc{};
		desc.Flags = bShaderVisiable
			? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
			: D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		desc.NumDescriptors = Capacity;
		desc.Type = Type;
		desc.NodeMask = 0;
		KS_D3D12_CALL(GD3D12Device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&Heap)));

		DescriptorSize = GD3D12Device->GetDescriptorHandleIncrementSize(Type);

		CpuStart = Heap->GetCPUDescriptorHandleForHeapStart();
		GpuStart = bShaderVisiable
			? Heap->GetGPUDescriptorHandleForHeapStart()
			: D3D12_GPU_DESCRIPTOR_HANDLE{ 0 };
	}

	d3d12::FDescriptorHandle FDescriptorHeap::Allocate()
	{
		const uint32 CurrIndex{FreeHandles.at(CurrFreeHanle)};
		++CurrFreeHanle;

		CD3DX12_CPU_DESCRIPTOR_HANDLE CpuFreeHandle(CpuStart);
		CpuFreeHandle.Offset(CurrIndex, DescriptorSize);
		
		FDescriptorHandle Handle;
		Handle.CpuHandle.ptr = CpuFreeHandle.ptr;

		if (IsShaderVisiable())
		{
			CD3DX12_GPU_DESCRIPTOR_HANDLE GpuFreeHandle(GpuStart);
			GpuFreeHandle.Offset(CurrIndex, DescriptorSize);
			Handle.GpuHandle.ptr = GpuFreeHandle.ptr;
		}
		return Handle;
	}

	class FScopedCommandRecorder
	{
	public:
		FScopedCommandRecorder(
			ID3D12GraphicsCommandList* InGfxCommandList,
			ID3D12CommandAllocator* InCommandAllocator,
			ID3D12CommandQueue* InCommandQueue)
			:GfxCommandList(InGfxCommandList)
			,CommandAllocator(InCommandAllocator)
			,CommandQueue(InCommandQueue)
		{
			KS_D3D12_CALL(GfxCommandList->Reset(CommandAllocator, nullptr));
		}
		~FScopedCommandRecorder() {
			// Done recording commands.
			KS_D3D12_CALL(GfxCommandList->Close());
			// Add the command list to the queue for execution.
			ID3D12CommandList* cmdsLists[] = { GfxCommandList };
			CommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);
			// flush and wait
			GD3D12RHI->FlushRenderingCommands();
		}
		ID3D12GraphicsCommandList* GfxCommandList{nullptr};
		ID3D12CommandAllocator* CommandAllocator{nullptr};
		ID3D12CommandQueue* CommandQueue{nullptr};
	};
}
namespace ks::d3d12
{
namespace
{
	inline uint32 CalcConstantBufferByteSize(uint32 byteSize) {
		// Constant buffers must be a multiple of the minimum hardware
		// allocation size (usually 256 bytes).  So round up to nearest
		// multiple of 256.  We do this by adding 255 and then masking off
		// the lower 2 bytes which store all bits < 256.
		// Example: Suppose byteSize = 300.
		// (300 + 255) & ~255
		// 555 & ~255
		// 0x022B & ~0x00ff
		// 0x022B & 0xff00
		// 0x0200
		// 512
		return (byteSize + 255) & ~255;
	}

	/*
	* #define GRootSignature \
	* "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT)," \
	* "DescriptorTable(" \
	* "CBV(b0)," \
	* "CBV(b1)),"
	*/
	void CreateGlobalRootSignature(ComPtr<ID3D12RootSignature>& RootSignature) {
		CD3DX12_DESCRIPTOR_RANGE cbvTable0;
		cbvTable0.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);

		CD3DX12_DESCRIPTOR_RANGE cbvTable1;
		cbvTable1.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1);

		// Root parameter can be a table, root descriptor or root constants.
		CD3DX12_ROOT_PARAMETER slotRootParameter[2];

		// Create root CBVs.
		slotRootParameter[0].InitAsDescriptorTable(1, &cbvTable0);
		slotRootParameter[1].InitAsDescriptorTable(1, &cbvTable1);

		// A root signature is an array of root parameters.
		CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(2, slotRootParameter, 0, nullptr,
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		// create a root signature with a single slot which points to a descriptor range consisting of a single constant buffer
		ComPtr<ID3DBlob> serializedRootSig = nullptr;
		ComPtr<ID3DBlob> errorBlob = nullptr;
		KS_D3D12_CALL(D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
			serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf()));

		if (errorBlob != nullptr)
		{
			KS_INFOA((char*)errorBlob->GetBufferPointer());
		}

		KS_D3D12_CALL(GD3D12Device->CreateRootSignature(
			0,
			serializedRootSig->GetBufferPointer(),
			serializedRootSig->GetBufferSize(),
			IID_PPV_ARGS(RootSignature.GetAddressOf())));
	}
}
}
namespace ks::d3d12
{
	FD3D12RHI::~FD3D12RHI()
	{
		KS_INFO(TEXT("~FD3D12RHI"));
	}

	void FD3D12RHI::Init()
	{
		KS_INFO(TEXT("FD3D12RHI::Init"));

		hWnd = dynamic_cast<FWinApp*>(GApp)->GetHWindow();

#ifdef KS_DEBUG_BUILD
		{
			ComPtr<ID3D12Debug1> DebugController;
			KS_D3D12_CALL(D3D12GetDebugInterface(IID_PPV_ARGS(&DebugController)));
			DebugController->EnableDebugLayer();
		}
#endif
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
			GD3D12Device = D3D12Device.Get();
		}

		{
			KS_D3D12_CALL(D3D12Device->CreateFence(0, D3D12_FENCE_FLAG_NONE,
				IID_PPV_ARGS(&D3D12Fence)));
		}

		{
			RTVSize = D3D12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
			DSVSize = D3D12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
		}

		{
			D3D12_COMMAND_QUEUE_DESC queueDesc = {};
			queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
			queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
			KS_D3D12_CALL(D3D12Device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&D3D12CommandQueue)));

			KS_D3D12_CALL(D3D12Device->CreateCommandAllocator(
				D3D12_COMMAND_LIST_TYPE_DIRECT,
				IID_PPV_ARGS(D3D12CommandAllocator.GetAddressOf())));

			KS_D3D12_CALL(D3D12Device->CreateCommandList(
				0,
				D3D12_COMMAND_LIST_TYPE_DIRECT,
				D3D12CommandAllocator.Get(), // Associated command allocator
				nullptr,                   // Initial PipelineStateObject
				IID_PPV_ARGS(D3D12GfxCommandList.GetAddressOf())));

			// Start off in a closed state.  This is because the first time we refer 
			// to the command list we will Reset it, and it needs to be closed before
			// calling Reset.
			D3D12GfxCommandList->Close();

			GGfxCmdlist = D3D12GfxCommandList.Get();
		}

		{
			// Release the previous swapchain we will be recreating.
			DXGISwapChain.Reset();

			int WndResX, WndResY;
			GApp->GetWindowSize(WndResX, WndResY);

			DXGI_SWAP_CHAIN_DESC sd;
			sd.BufferDesc.Width = WndResX;
			sd.BufferDesc.Height = WndResY;
			sd.BufferDesc.RefreshRate.Numerator = 60;
			sd.BufferDesc.RefreshRate.Denominator = 1;
			sd.BufferDesc.Format = BackBufferFormat;
			sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
			sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
			sd.SampleDesc.Count = 1;
			sd.SampleDesc.Quality = 0;
			sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
			sd.BufferCount = SwapChainBufferCount;
			sd.OutputWindow = hWnd;
			sd.Windowed = true;
			sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
			sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

			// Note: Swap chain uses queue to perform flush.
			KS_D3D12_CALL(DXGIFactory->CreateSwapChain(
				D3D12CommandQueue.Get(),
				&sd,
				DXGISwapChain.GetAddressOf()));
		}
		{
			D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
			rtvHeapDesc.NumDescriptors = SwapChainBufferCount;
			rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
			rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
			rtvHeapDesc.NodeMask = 0;
			KS_D3D12_CALL(D3D12Device->CreateDescriptorHeap(
				&rtvHeapDesc, IID_PPV_ARGS(D3D12RTVHeap.GetAddressOf())));


			D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
			dsvHeapDesc.NumDescriptors = 1;
			dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
			dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
			dsvHeapDesc.NodeMask = 0;
			KS_D3D12_CALL(D3D12Device->CreateDescriptorHeap(
				&dsvHeapDesc, IID_PPV_ARGS(D3D12DSVHeap.GetAddressOf())));
		}

		// create constant buffer descriptor heap
		CBVHeap.Init(d3d12::MAX_CBV_NUM, true);

		// create global root signature
		CreateGlobalRootSignature(GlobalRootSignature);

		// setup the global RHI pointer
		d3d12::GD3D12RHI = this;

		ResizeWindow();
	}

	void FD3D12RHI::Shutdown()
	{
		KS_INFO(TEXT("\tFD3D12RHI::Shutdown"));
		DXGIFactory.Reset();
		D3D12Device.Reset();
		D3D12Fence.Reset();
	}

	void FD3D12RHI::ResizeWindow()
	{
		assert(D3D12Device);
		assert(DXGISwapChain);
		assert(D3D12CommandAllocator);

		int WndResX, WndResY;
		FWinApp *WinApp = dynamic_cast<FWinApp *>(GApp);
		assert(WinApp);
		WinApp->GetWindowSize(WndResX, WndResY);

		// Flush before changing any resources.
		FlushRenderingCommands();

		KS_D3D12_CALL(D3D12GfxCommandList->Reset(D3D12CommandAllocator.Get(), nullptr));

		// Release the previous resources we will be recreating.
		for (int i = 0; i < SwapChainBufferCount; ++i)
			D3D12SwapChainBuffers[i].Reset();
		D3D12DepthStencilBuffer.Reset();

		// Resize the swap chain.
		KS_D3D12_CALL(DXGISwapChain->ResizeBuffers(
			SwapChainBufferCount,
			WndResX, WndResY,
			BackBufferFormat,
			DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));

		CurrentBackBuffer = 0;

		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(D3D12RTVHeap->GetCPUDescriptorHandleForHeapStart());
		for (UINT i = 0; i < SwapChainBufferCount; i++)
		{
			KS_D3D12_CALL(DXGISwapChain->GetBuffer(i, IID_PPV_ARGS(&D3D12SwapChainBuffers[i])));
			D3D12Device->CreateRenderTargetView(D3D12SwapChainBuffers[i].Get(), nullptr, rtvHeapHandle);
			rtvHeapHandle.Offset(1, RTVSize);
		}

		// Create the depth/stencil buffer and view.
		D3D12_RESOURCE_DESC depthStencilDesc;
		depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		depthStencilDesc.Alignment = 0;
		depthStencilDesc.Width = WndResX;
		depthStencilDesc.Height = WndResY;
		depthStencilDesc.DepthOrArraySize = 1;
		depthStencilDesc.MipLevels = 1;

		// Correction 11/12/2016: SSAO chapter requires an SRV to the depth buffer to read from 
		// the depth buffer.  Therefore, because we need to create two views to the same resource:
		//   1. SRV format: DXGI_FORMAT_R24_UNORM_X8_TYPELESS
		//   2. DSV Format: DXGI_FORMAT_D24_UNORM_S8_UINT
		// we need to create the depth buffer resource with a typeless format.  
		depthStencilDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;

		depthStencilDesc.SampleDesc.Count = 1;
		depthStencilDesc.SampleDesc.Quality = 0;
		depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

		D3D12_CLEAR_VALUE optClear;
		optClear.Format = DepthBufferFormat;
		optClear.DepthStencil.Depth = 1.0f;
		optClear.DepthStencil.Stencil = 0;
		CD3DX12_HEAP_PROPERTIES HeapProp(D3D12_HEAP_TYPE_DEFAULT);
		KS_D3D12_CALL(D3D12Device->CreateCommittedResource(
			&HeapProp,
			D3D12_HEAP_FLAG_NONE,
			&depthStencilDesc,
			D3D12_RESOURCE_STATE_COMMON,
			&optClear,
			IID_PPV_ARGS(D3D12DepthStencilBuffer.GetAddressOf())));

		// Create descriptor to mip level 0 of entire resource using the format of the resource.
		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
		dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		dsvDesc.Format = DepthBufferFormat;
		dsvDesc.Texture2D.MipSlice = 0;

		D3D12_CPU_DESCRIPTOR_HANDLE DSVHandle = D3D12DSVHeap->GetCPUDescriptorHandleForHeapStart();
		D3D12Device->CreateDepthStencilView(D3D12DepthStencilBuffer.Get(), &dsvDesc, DSVHandle);

		// Transition the resource from its initial state to be used as a depth buffer.
		CD3DX12_RESOURCE_BARRIER ResBarrier = CD3DX12_RESOURCE_BARRIER::Transition(D3D12DepthStencilBuffer.Get(),
			D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE);
		D3D12GfxCommandList->ResourceBarrier(1, &ResBarrier);

		// Execute the resize commands.
		KS_D3D12_CALL(D3D12GfxCommandList->Close());
		ID3D12CommandList* cmdsLists[] = { D3D12GfxCommandList.Get() };
		D3D12CommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

		// Wait until resize is complete.
		FlushRenderingCommands();

		// Update the viewport transform to cover the client area.
		D3D12Viewport.TopLeftX = 0;
		D3D12Viewport.TopLeftY = 0;
		D3D12Viewport.Width = static_cast<float>(WndResX);
		D3D12Viewport.Height = static_cast<float>(WndResY);
		D3D12Viewport.MinDepth = 0.0f;
		D3D12Viewport.MaxDepth = 1.0f;

		ScissorRect = { 0, 0, WndResX, WndResY };
	}

	void FD3D12RHI::FlushRenderingCommands()
	{
		// Advance the fence value to mark commands up to this fence point.
		CurrentFence++;

		// Add an instruction to the command queue to set a new fence point.  Because we 
		// are on the GPU timeline, the new fence point won't be set until the GPU finishes
		// processing all the commands prior to this Signal().
		KS_D3D12_CALL(D3D12CommandQueue->Signal(D3D12Fence.Get(), CurrentFence));

		// Wait until the GPU has completed commands up to this fence point.
		if (D3D12Fence->GetCompletedValue() < CurrentFence)
		{
			HANDLE eventHandle = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);

			// Fire event when GPU hits current fence.  
			KS_D3D12_CALL(D3D12Fence->SetEventOnCompletion(CurrentFence, eventHandle));

			// Wait until the GPU hits current fence event is fired.
			WaitForSingleObject(eventHandle, INFINITE);
			CloseHandle(eventHandle);
		}
	}

	void FD3D12RHI::BeginFrame()
	{
		// Reuse the memory associated with command recording.
// We can only reset when the associated command lists have finished execution on the GPU.
		KS_D3D12_CALL(D3D12CommandAllocator->Reset());

		// A command list can be reset after it has been added to the command queue via ExecuteCommandList.
		// Reusing the command list reuses memory.
		KS_D3D12_CALL(D3D12GfxCommandList->Reset(D3D12CommandAllocator.Get(), nullptr));

		// Indicate a state transition on the resource usage.
		CD3DX12_RESOURCE_BARRIER ResBarrier = CD3DX12_RESOURCE_BARRIER::Transition(GetCurrentBackBuffer(),
			D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
		D3D12GfxCommandList->ResourceBarrier(1, &ResBarrier);

		// Set the viewport and scissor rect.  This needs to be reset whenever the command list is reset.
		D3D12GfxCommandList->RSSetViewports(1, &D3D12Viewport);
		D3D12GfxCommandList->RSSetScissorRects(1, &ScissorRect);

		D3D12_CPU_DESCRIPTOR_HANDLE BackbufferView = GetCurrentBackBufferView();

		// Clear the back buffer and depth buffer.
		D3D12GfxCommandList->ClearRenderTargetView(BackbufferView, d3d12::Colors::LightSteelBlue, 0, nullptr);

		D3D12_CPU_DESCRIPTOR_HANDLE DSVHandle = D3D12DSVHeap->GetCPUDescriptorHandleForHeapStart();
		D3D12GfxCommandList->ClearDepthStencilView(DSVHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

		// Specify the buffers we are going to render to.
		D3D12GfxCommandList->OMSetRenderTargets(1, &BackbufferView, true, &DSVHandle);

		ID3D12DescriptorHeap* Heaps[] = {CBVHeap.GetHeap()};
		D3D12GfxCommandList->SetDescriptorHeaps(_countof(Heaps), Heaps);
	}

	void FD3D12RHI::EndFrame()
	{
		// Indicate a state transition on the resource usage.
		CD3DX12_RESOURCE_BARRIER ResBarrier = CD3DX12_RESOURCE_BARRIER::Transition(GetCurrentBackBuffer(),
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
		D3D12GfxCommandList->ResourceBarrier(1, &ResBarrier);

		// Done recording commands.
		KS_D3D12_CALL(D3D12GfxCommandList->Close());

		// Add the command list to the queue for execution.
		ID3D12CommandList* cmdsLists[] = { D3D12GfxCommandList.Get() };
		D3D12CommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

		// swap the back and front buffers
		KS_D3D12_CALL(DXGISwapChain->Present(0, 0));
		CurrentBackBuffer = (CurrentBackBuffer + 1) % SwapChainBufferCount;

		// Wait until frame commands are complete.  This waiting is inefficient and is
		// done for simplicity.  Later we will show how to organize our rendering code
		// so we do not have to wait per frame.
		FlushRenderingCommands();
	}

	d3d12::FD3D12Resource* FD3D12RHI::CreateConstBufferResource(size_t Size)
	{
		uint32 ResSize = CalcConstantBufferByteSize(static_cast<uint32>(Size));
		ks::d3d12::FD3D12Resource* NewResource =
			new ks::d3d12::FD3D12Resource(
				D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD,
				ResSize,
				D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ
			);
		return NewResource;
	}

	ks::IRHIConstBuffer* FD3D12RHI::CreateConstBuffer(const void* Data, uint32 Size)
	{
		d3d12::FD3D12Resource* Resource{ FD3D12RHI::CreateConstBufferResource(Size) };
		Size = Resource->Size;
		d3d12::FDescriptorHandle ViewHandle{ d3d12::GD3D12RHI->GetCBVHeap().Allocate() };
		{
			ID3D12Resource* D3D12Resource{ Resource->GetID3D12Resource() };
			D3D12_CONSTANT_BUFFER_VIEW_DESC desc{};
			desc.BufferLocation = D3D12Resource->GetGPUVirtualAddress();
			desc.SizeInBytes = Size;
			GD3D12Device->CreateConstantBufferView(&desc, ViewHandle.CpuHandle);
		}

		d3d12::FD3D12ConstBuffer* Buffer = new d3d12::FD3D12ConstBuffer;
		Buffer->RHIResource.reset(Resource);
		Buffer->ViewHandle = ViewHandle;
		Buffer->UpdateData(Data, Size);
		return Buffer;
	}

	IRHIBuffer* FD3D12RHI::CreateBuffer(uint32 Size, const void* Data)
	{
		/*
		* create default buffer
		* create upload buffer
		* copy data
		*/
		auto _RHIBuffer = new d3d12::FD3D12Buffer(Size);

		// create upload type resource
		_RHIBuffer->UploadResource =
			std::make_unique<d3d12::FD3D12Resource>(
				D3D12_HEAP_TYPE_UPLOAD,
				Size,
				D3D12_RESOURCE_STATE_GENERIC_READ
				);

		// create default type resource
		_RHIBuffer->RHIResource = std::make_unique<d3d12::FD3D12Resource>(
			D3D12_HEAP_TYPE_DEFAULT,
			Size,
			D3D12_RESOURCE_STATE_COMMON
			);

		assert(Data);
		if (Data) {
			FScopedCommandRecorder CommandListRecordHelper(D3D12GfxCommandList.Get(), D3D12CommandAllocator.Get(), D3D12CommandQueue.Get());

			ID3D12Resource* UploadBuffer = dynamic_cast<d3d12::FD3D12Resource*>(_RHIBuffer->UploadResource.get())->GetID3D12Resource();
			ID3D12Resource* DefaultBuffer = dynamic_cast<d3d12::FD3D12Resource*>(_RHIBuffer->RHIResource.get())->GetID3D12Resource();
			// Describe the data we want to copy into the default buffer.
			D3D12_SUBRESOURCE_DATA subResourceData = {Data, Size, Size};

			// Schedule to copy the data to the default buffer resource.  At a high level, the helper function UpdateSubresources
			// will copy the CPU memory into the intermediate upload heap.  Then, using ID3D12CommandList::CopySubresourceRegion,
			// the intermediate upload heap data will be copied to mBuffer.
			{
				CD3DX12_RESOURCE_BARRIER ResrcBarrier = CD3DX12_RESOURCE_BARRIER::Transition(DefaultBuffer,
					D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
				GGfxCmdlist->ResourceBarrier(1, &ResrcBarrier);
			}
			UpdateSubresources<1>(GGfxCmdlist, DefaultBuffer, UploadBuffer, 0, 0, 1, &subResourceData);
			{
				CD3DX12_RESOURCE_BARRIER ResrcBarrier = CD3DX12_RESOURCE_BARRIER::Transition(DefaultBuffer,
					D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);
				GGfxCmdlist->ResourceBarrier(1, &ResrcBarrier);
			}
		}

		return _RHIBuffer;
	}

	void FD3D12RHI::SetPipelineState(IRHIPipelineState* PipelineState)
	{
		d3d12::FD3D12PipelineState* D3D12PipelineState = dynamic_cast<d3d12::FD3D12PipelineState*>(PipelineState);
		assert(D3D12PipelineState);
		ID3D12PipelineState* pPipelineState{D3D12PipelineState->PipelineState.Get()};
		GGfxCmdlist->SetPipelineState(pPipelineState);

		GGfxCmdlist->SetGraphicsRootSignature(D3D12PipelineState->pRootSignature);

		GGfxCmdlist->IASetPrimitiveTopology(D3D12PipelineState->PrimitiveType);
	}

	IRHIPipelineState* FD3D12RHI::CreatePipelineState(const FRHIPipelineStateDesc& Desc)
	{
		return new d3d12::FD3D12PipelineState(Desc);
	}

	D3D12_CPU_DESCRIPTOR_HANDLE FD3D12RHI::GetCurrentBackBufferView()
	{
		return CD3DX12_CPU_DESCRIPTOR_HANDLE(
			D3D12RTVHeap->GetCPUDescriptorHandleForHeapStart(),
			CurrentBackBuffer,
			RTVSize);
	}

	ID3D12Resource* FD3D12RHI::GetCurrentBackBuffer()
	{
		return D3D12SwapChainBuffers[CurrentBackBuffer].Get();
	}

	void FD3D12RHI::SetShaderConstBuffer(IRHIConstBuffer* ConstBuffer)
	{
		FD3D12ConstBuffer* D3D12ConstBuffer = dynamic_cast<FD3D12ConstBuffer*>(ConstBuffer);
		assert(D3D12ConstBuffer);
		const FDescriptorHandle& ViewHandle = D3D12ConstBuffer->GetViewHandle();
		D3D12GfxCommandList->SetGraphicsRootDescriptorTable(static_cast<UINT>(ConstBuffer->GetLocationIndex()), ViewHandle.GpuHandle);
	}

	void FD3D12RHI::SetVertexBuffer(const IRHIVertexBuffer* InVertexBuffer)
	{
		const FD3D12VertexBuffer* VertexBuffer{dynamic_cast<const FD3D12VertexBuffer*>(InVertexBuffer)};
		D3D12GfxCommandList->IASetVertexBuffers(0, 1, &VertexBuffer->GetVertexBufferView());
	}

	void FD3D12RHI::SetVertexBuffers(const IRHIVertexBuffer** VertexBuffers, int32 Num)
	{
		if (!VertexBuffers || !Num)
		{
			return;
		}
		for (int32 i{ 0 }; i < Num; ++i)
		{
			const FD3D12VertexBuffer* VertexBuffer{ dynamic_cast<const FD3D12VertexBuffer*>(VertexBuffers[i]) };
			D3D12GfxCommandList->IASetVertexBuffers(i, 1, &VertexBuffer->GetVertexBufferView());
		}
	}

	ks::IRHIVertexBuffer* FD3D12RHI::CreateVertexBuffer(uint32 Stride, uint32 Size, const void* Data)
	{
		FD3D12VertexBuffer* D3D12VertBuffer = new FD3D12VertexBuffer(Stride, Size);
		D3D12VertBuffer->RHIBuffer = std::shared_ptr<IRHIBuffer>(CreateBuffer(Size, Data));
		auto D3D12Buffer{std::dynamic_pointer_cast<FD3D12Buffer>(D3D12VertBuffer->RHIBuffer)};
		D3D12VertBuffer->BufferView.BufferLocation = D3D12Buffer->GetD3D12Resource()->GetID3D12Resource()->GetGPUVirtualAddress();
		D3D12VertBuffer->BufferView.StrideInBytes = Stride;
		D3D12VertBuffer->BufferView.SizeInBytes = Size;
		return D3D12VertBuffer;
	}

	ks::IRHIIndexBuffer* FD3D12RHI::CreateIndexBuffer(EELEM_FORMAT ElemFormat, uint32 Count, uint32 Size, const void* Data)
	{
		FD3D12IndexBuffer* IndexBuffer = new FD3D12IndexBuffer(ElemFormat, Count, Size);
		IndexBuffer->RHIBuffer = std::shared_ptr<IRHIBuffer>(CreateBuffer(Size, Data));
		auto D3D12Buffer{std::dynamic_pointer_cast<FD3D12Buffer>(IndexBuffer->RHIBuffer)};
		IndexBuffer->BufferView.BufferLocation = D3D12Buffer->GetD3D12Resource()->GetID3D12Resource()->GetGPUVirtualAddress();
		IndexBuffer->BufferView.Format = GetDXGIFormat(ElemFormat);
		assert(IndexBuffer->BufferView.Format != DXGI_FORMAT::DXGI_FORMAT_UNKNOWN);
		IndexBuffer->BufferView.SizeInBytes = Size;
		return IndexBuffer;
	}

	void FD3D12RHI::DrawIndexedPrimitive(const IRHIIndexBuffer* InIndexBuffer)
	{
		const FD3D12IndexBuffer* IndexBuffer{dynamic_cast<const FD3D12IndexBuffer*>(InIndexBuffer)};
		const D3D12_INDEX_BUFFER_VIEW IndexBufferView{IndexBuffer->GetIndexBufferView()};
		D3D12GfxCommandList->IASetIndexBuffer(&IndexBufferView);

		const UINT IndexCount{IndexBuffer->GetIndexCount()};
		D3D12GfxCommandList->DrawIndexedInstanced(IndexCount, 1, 0, 0, 0);
	}

	IRHIConstBuffer1* FD3D12RHI::CreateConstBuffer1(const void* Data, uint32 Size)
	{
		uint32_t AllocSize = CalcConstantBufferByteSize(Size);
		FD3D12ConstBuffer1* ConstBuffer = new FD3D12ConstBuffer1(AllocSize);
		ConstBuffer->D3D12Resource = _CreateBuffer(AllocSize, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ);
		ConstBuffer->D3D12Resource->Map(0, nullptr, &ConstBuffer->MapData);
		ConstBuffer->SetData(Data, Size);
		ConstBuffer->ViewHandle = CBVHeap.Allocate();

		D3D12_CONSTANT_BUFFER_VIEW_DESC desc{};
		desc.BufferLocation = ConstBuffer->D3D12Resource->GetGPUVirtualAddress();
		desc.SizeInBytes = AllocSize;
		GD3D12Device->CreateConstantBufferView(&desc, ConstBuffer->ViewHandle.CpuHandle);
		return ConstBuffer;
	}

	ID3D12Resource* FD3D12RHI::_CreateBuffer(uint32_t Size, D3D12_HEAP_TYPE HeapType, D3D12_RESOURCE_STATES ResStats)
	{
		ID3D12Resource* D3D12Resource{nullptr};
		CD3DX12_HEAP_PROPERTIES HeapProp(HeapType);
		CD3DX12_RESOURCE_DESC ResDesc = CD3DX12_RESOURCE_DESC::Buffer(Size);
		KS_D3D12_CALL(GD3D12Device->CreateCommittedResource(
			&HeapProp,
			D3D12_HEAP_FLAG_NONE,
			&ResDesc,
			ResStats,
			nullptr,
			IID_PPV_ARGS(&D3D12Resource)
		));
		return D3D12Resource;
	}

	void FD3D12RHI::SetConstBuffer(IRHIConstBuffer1* ConstBuffer)
	{
		assert(ConstBuffer);
		FD3D12ConstBuffer1* D3D12ConstBuffer = dynamic_cast<FD3D12ConstBuffer1*>(ConstBuffer);
		assert(D3D12ConstBuffer);
		const FDescriptorHandle& ViewHandle = D3D12ConstBuffer->GetViewHandle();
		D3D12GfxCommandList->SetGraphicsRootDescriptorTable(static_cast<UINT>(ConstBuffer->GetLocationIndex()), ViewHandle.GpuHandle);
	}

	ks::IRHIVertexBuffer1* FD3D12RHI::CreateVertexBuffer1(uint32 Stride, uint32 Size, const void* Data)
	{
		FD3D12VertexBuffer1* VertBuffer = new FD3D12VertexBuffer1(Stride, Size);
		VertBuffer->D3D12Resource = _CreateBuffer(Size, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_COMMON);
		VertBuffer->BufferView.BufferLocation = VertBuffer->D3D12Resource->GetGPUVirtualAddress();
		VertBuffer->BufferView.StrideInBytes = Stride;
		VertBuffer->BufferView.SizeInBytes = Size;
		if (Data)
		{
			UploadResourceData(VertBuffer->D3D12Resource.Get(), Data, Size);
		}
		return VertBuffer;
	}

	int32_t FD3D12RHI::UploadResourceData(ID3D12Resource* DestResource, const void* Data, uint32_t Size)
	{
		assert(DestResource && Data && Size);
		if (!DestResource || !Data || !Size)
		{
			return -1;
		}
		ComPtr<ID3D12Resource> UploadBufferPtr = _CreateBuffer(
			Size, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ);
		ID3D12Resource* UploadBuffer{UploadBufferPtr.Get()};
		ID3D12Resource* DefaultBuffer{DestResource};
		{
			FScopedCommandRecorder ScopedCmdRecorder(
				D3D12GfxCommandList.Get(), D3D12CommandAllocator.Get(), D3D12CommandQueue.Get());
			
			// Schedule to copy the data to the default buffer resource.  At a high level, the helper function UpdateSubresources
			// will copy the CPU memory into the intermediate upload heap.  Then, using ID3D12CommandList::CopySubresourceRegion,
			// the intermediate upload heap data will be copied to mBuffer.
			D3D12_SUBRESOURCE_DATA subResourceData = { Data, Size, Size };
			{
				CD3DX12_RESOURCE_BARRIER ResrcBarrier = CD3DX12_RESOURCE_BARRIER::Transition(DefaultBuffer,
					D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
				D3D12GfxCommandList->ResourceBarrier(1, &ResrcBarrier);
			}
			UpdateSubresources<1>(D3D12GfxCommandList.Get(), DefaultBuffer, UploadBuffer, 0, 0, 1, &subResourceData);
			{
				CD3DX12_RESOURCE_BARRIER ResrcBarrier = CD3DX12_RESOURCE_BARRIER::Transition(DefaultBuffer,
					D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);
				D3D12GfxCommandList->ResourceBarrier(1, &ResrcBarrier);
			}
		}
		UploadBufferPtr->Release();
		return 0;
	}

	void FD3D12RHI::SetVertexBuffer1(const IRHIVertexBuffer1* _VertexBuffer)
	{
		const FD3D12VertexBuffer1* VertexBuffer{ dynamic_cast<const FD3D12VertexBuffer1*>(_VertexBuffer) };
		D3D12GfxCommandList->IASetVertexBuffers(0, 1, &VertexBuffer->GetVertexBufferView());
	}

	void FD3D12RHI::SetVertexBuffers1(const IRHIVertexBuffer1** VertexBuffers, int32 Num)
	{
		if (VertexBuffers && Num)
		{
			for (int32 i = 0; i < Num; ++i)
			{
				const FD3D12VertexBuffer1* VertexBuffer{ dynamic_cast<const FD3D12VertexBuffer1*>(VertexBuffers[i]) };
				D3D12GfxCommandList->IASetVertexBuffers(i, 1, &VertexBuffer->GetVertexBufferView());
			}
		}
	}

}


