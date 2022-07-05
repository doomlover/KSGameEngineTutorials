#include "engine_pch.h"

#include "RHI/D3D12/D3D12RHI.h"
#include "D3D12Resource.h"
#include "D3D12PipelineState.h"

// Link d3d12 libs
#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dxgi.lib")

#define RTV_V1 1

namespace ks::d3d12
{
	const uint32_t MAX_CBV_NUM = 128;
	const uint32_t MAX_RTV_NUM = 8;
	const uint32_t MAX_DSV_NUM = 8;

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
			{EELEM_FORMAT::D24_UNORM_S8_UINT,	DXGI_FORMAT::DXGI_FORMAT_D24_UNORM_S8_UINT},
			{EELEM_FORMAT::R8G8B8A8_UNORM,		DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM},
			{EELEM_FORMAT::R32G32B32_FLOAT,		DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT},
			{EELEM_FORMAT::R16G16B16A16_FLOAT,	DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT },
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
		constexpr int32_t NumRootParameters = 4;
		// primitive const buffer parameter
		CD3DX12_DESCRIPTOR_RANGE cbvTable0;
		/* table_type table_number register_start_index */
		cbvTable0.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);

		// pass const buffer parameter
		CD3DX12_DESCRIPTOR_RANGE cbvTable1;
		cbvTable1.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1);

		// shadow map parameter
		CD3DX12_DESCRIPTOR_RANGE ShadowMapSRVTable;
		ShadowMapSRVTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

		// scene color map parameter
		CD3DX12_DESCRIPTOR_RANGE SceneColorMapSRVTable;
		SceneColorMapSRVTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1);

		CD3DX12_ROOT_PARAMETER slotRootParameter[NumRootParameters];
		slotRootParameter[0].InitAsDescriptorTable(1, &cbvTable0);
		slotRootParameter[1].InitAsDescriptorTable(1, &cbvTable1);
		slotRootParameter[2].InitAsDescriptorTable(1, &ShadowMapSRVTable, D3D12_SHADER_VISIBILITY_PIXEL);
		slotRootParameter[3].InitAsDescriptorTable(1, &SceneColorMapSRVTable, D3D12_SHADER_VISIBILITY_PIXEL);

		// static samplers
		const CD3DX12_STATIC_SAMPLER_DESC SamplerShadow(
			0,
			D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT,
			D3D12_TEXTURE_ADDRESS_MODE_BORDER,
			D3D12_TEXTURE_ADDRESS_MODE_BORDER,
			D3D12_TEXTURE_ADDRESS_MODE_BORDER,
			0.0f,
			16,
			D3D12_COMPARISON_FUNC_LESS_EQUAL,
			D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK
		);

		const CD3DX12_STATIC_SAMPLER_DESC SamplerLinearClamp(
			1, // shaderRegister
			D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
			D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
			D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
			D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

		std::array<CD3DX12_STATIC_SAMPLER_DESC, 2> staticSamplers =
		{
			SamplerShadow, SamplerLinearClamp,
		};

		// A root signature is an array of root parameters.
		CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(NumRootParameters, slotRootParameter,
			(UINT)staticSamplers.size(), staticSamplers.data(),
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
	struct FD3D12RHIContext
	{
		FD3D12RenderTarget* CurrentRenderTarget{ nullptr };
		FD3D12DepthStencilBuffer1* CurrentDepthStencilBuffer{ nullptr };
		std::vector<std::unique_ptr<FD3D12RenderTarget>> DefaultRenderTargets;
		std::unique_ptr<FD3D12DepthStencilBuffer1> DefaultDepthStencilBuffer;
		ComPtr<ID3D12Resource> D3D12SwapChainBuffers[FD3D12RHI::SwapChainBufferCount];
		ComPtr<ID3D12Resource> D3D12DepthStencilBuffer;
		FDescriptorHeap CBVHeap{ D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV };
		FDescriptorHeap RTVHeap{ D3D12_DESCRIPTOR_HEAP_TYPE_RTV };
		FDescriptorHeap DSVHeap{ D3D12_DESCRIPTOR_HEAP_TYPE_DSV };
		ComPtr<ID3D12RootSignature> GlobalRootSignature{ nullptr };
		ComPtr<ID3D12Fence> D3D12Fence;
		ComPtr<ID3D12CommandQueue> D3D12CommandQueue;
		ComPtr<ID3D12CommandAllocator> D3D12CommandAllocator;
		ComPtr<ID3D12GraphicsCommandList> D3D12GfxCommandList;
		ComPtr<ID3D12DescriptorHeap> D3D12RTVHeap;
		ComPtr<ID3D12DescriptorHeap> D3D12DSVHeap;
		ComPtr<IDXGISwapChain> DXGISwapChain;
	};

	FD3D12RHI::~FD3D12RHI()
	{
		KS_INFO(TEXT("~FD3D12RHI"));
	}

	void FD3D12RHI::Init(const FRHIConfig& Config)
	{
		KS_INFO(TEXT("FD3D12RHI::Init"));
		Context = new FD3D12RHIContext;

		hWnd = dynamic_cast<FWinApp*>(GApp)->GetHWindow();
		BackBufferFormat = GetDXGIFormat(Config.BackBufferFormat);
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
#ifdef KS_DEBUG_BUILD
		{
			ComPtr<ID3D12InfoQueue> InfoQueue;
			KS_D3D12_CALL(D3D12Device->QueryInterface(IID_PPV_ARGS(&InfoQueue)));
			InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
			InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);
			InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
		}
#endif
		{
			KS_D3D12_CALL(D3D12Device->CreateFence(0, D3D12_FENCE_FLAG_NONE,
				IID_PPV_ARGS(&Context->D3D12Fence)));
		}
		// to be deprecate
		{
			RTVSize = D3D12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
			DSVSize = D3D12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
		}
		{
			D3D12_COMMAND_QUEUE_DESC queueDesc = {};
			queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
			queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
			KS_D3D12_CALL(D3D12Device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&Context->D3D12CommandQueue)));

			KS_D3D12_CALL(D3D12Device->CreateCommandAllocator(
				D3D12_COMMAND_LIST_TYPE_DIRECT,
				IID_PPV_ARGS(Context->D3D12CommandAllocator.GetAddressOf())));

			KS_D3D12_CALL(D3D12Device->CreateCommandList(
				0,
				D3D12_COMMAND_LIST_TYPE_DIRECT,
				Context->D3D12CommandAllocator.Get(), // Associated command allocator
				nullptr,                   // Initial PipelineStateObject
				IID_PPV_ARGS(Context->D3D12GfxCommandList.GetAddressOf())));

			// Start off in a closed state.  This is because the first time we refer 
			// to the command list we will Reset it, and it needs to be closed before
			// calling Reset.
			Context->D3D12GfxCommandList->Close();

			GGfxCmdlist = Context->D3D12GfxCommandList.Get();
		}
		{
			// Release the previous swapchain we will be recreating.
			Context->DXGISwapChain.Reset();

			auto WndResX = Config.ViewPort.Width;
			auto WndResY = Config.ViewPort.Height;
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
				Context->D3D12CommandQueue.Get(),
				&sd,
				Context->DXGISwapChain.GetAddressOf()));
		}
		{
			D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
			rtvHeapDesc.NumDescriptors = SwapChainBufferCount;
			rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
			rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
			rtvHeapDesc.NodeMask = 0;
			KS_D3D12_CALL(D3D12Device->CreateDescriptorHeap(
				&rtvHeapDesc, IID_PPV_ARGS(Context->D3D12RTVHeap.GetAddressOf())));


			D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
			dsvHeapDesc.NumDescriptors = 1;
			dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
			dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
			dsvHeapDesc.NodeMask = 0;
			KS_D3D12_CALL(D3D12Device->CreateDescriptorHeap(
				&dsvHeapDesc, IID_PPV_ARGS(Context->D3D12DSVHeap.GetAddressOf())));
		}

		// create constant buffer descriptor heap
		Context->CBVHeap.Init(MAX_CBV_NUM, true);
		Context->RTVHeap.Init(MAX_RTV_NUM, false);
		Context->DSVHeap.Init(MAX_DSV_NUM, false);

		// create global root signature
		CreateGlobalRootSignature(Context->GlobalRootSignature);

		// setup the global RHI pointer
		d3d12::GD3D12RHI = this;

		ResizeWindow();
	}

	void FD3D12RHI::Shutdown()
	{
		KS_INFO(TEXT("\tFD3D12RHI::Shutdown"));

		assert(Context);
		delete Context;
		Context = nullptr;
		
		DXGIFactory.Reset();
#ifdef KS_DEBUG_BUILD
		{
			{
				ComPtr<ID3D12InfoQueue> InfoQueue;
				KS_D3D12_CALL(D3D12Device->QueryInterface(IID_PPV_ARGS(&InfoQueue)));
				InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, false);
				InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, false);
				InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, false);
			}
			ComPtr<ID3D12DebugDevice2> DebugDevice;
			KS_D3D12_CALL(D3D12Device->QueryInterface(IID_PPV_ARGS(&DebugDevice)));
			D3D12Device.Reset();
			KS_D3D12_CALL(DebugDevice->ReportLiveDeviceObjects(
				D3D12_RLDO_SUMMARY | D3D12_RLDO_DETAIL | D3D12_RLDO_IGNORE_INTERNAL
			));
		}
#endif
		D3D12Device.Reset();
	}

	void FD3D12RHI::ResizeWindow()
	{
		auto& D3D12GfxCommandList{ Context->D3D12GfxCommandList };
		auto& D3D12CommandAllocator{ Context->D3D12CommandAllocator };
		auto WndResX = GRHIConfig.ViewPort.Width;
		auto WndResY = GRHIConfig.ViewPort.Height;

		// Flush before changing any resources.
		FlushRenderingCommands();

		KS_D3D12_CALL(D3D12GfxCommandList->Reset(D3D12CommandAllocator.Get(), nullptr));

		// Resize the swap chain.
		KS_D3D12_CALL(Context->DXGISwapChain->ResizeBuffers(
			SwapChainBufferCount,
			WndResX, WndResY,
			BackBufferFormat,
			DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));

		CurrentBackBuffer = 0;
#if !RTV_V1
		auto& D3D12SwapChainBuffers{ Context->D3D12SwapChainBuffers };
		// Release the previous resources we will be recreating.
		for (int i = 0; i < SwapChainBufferCount; ++i)
			D3D12SwapChainBuffers[i].Reset();

		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(Context->D3D12RTVHeap->GetCPUDescriptorHandleForHeapStart());
		for (UINT i = 0; i < SwapChainBufferCount; i++)
		{
			KS_D3D12_CALL(Context->DXGISwapChain->GetBuffer(i, IID_PPV_ARGS(&D3D12SwapChainBuffers[i])));
			D3D12Device->CreateRenderTargetView(D3D12SwapChainBuffers[i].Get(), nullptr, rtvHeapHandle);
			rtvHeapHandle.Offset(1, RTVSize);
		}
#else
		// recreate default back buffers
		{
			auto& DefaultRenderTargets{ Context->DefaultRenderTargets };
			DefaultRenderTargets.clear();
			DefaultRenderTargets.reserve(SwapChainBufferCount);
			FTexture2DDesc Desc;
			for (int32_t i{ 0 }; i < SwapChainBufferCount; ++i)
			{
				std::unique_ptr<FD3D12RenderTarget> RT = std::make_unique<FD3D12RenderTarget>(Desc);
				FDescriptorHandle RTV = Context->RTVHeap.Allocate();
				RT->SetRenderTargetView(RTV);
				KS_D3D12_CALL(Context->DXGISwapChain->GetBuffer(i, IID_PPV_ARGS(&RT->GetComPtr())));
				D3D12Device->CreateRenderTargetView(RT->GetResource(), nullptr, RTV.CpuHandle);
				DefaultRenderTargets.push_back(std::move(RT));
				KS_NAME_D3D12_OBJECT(DefaultRenderTargets.back()->GetResource(), std::format(TEXT("DefaultRenderTarget{}"), i).c_str());
			}
		}
#endif
#if RTV_V1
		// create default depth buffer
		{
			auto& DefaultDepthBuffer{ Context->DefaultDepthStencilBuffer };
			FTexture2DDesc Desc;
			Desc.Width = WndResX;
			Desc.Height = WndResY;
			FD3D12DepthStencilBuffer1* DefaultDepthBuferPtr = dynamic_cast<FD3D12DepthStencilBuffer1*>(CreateDepthStencilBuffer(Desc));
			assert(DefaultDepthBuferPtr);
			DefaultDepthBuffer.reset(DefaultDepthBuferPtr);

			// Transition the resource from its initial state to be used as a depth buffer.
			/*CD3DX12_RESOURCE_BARRIER ResBarrier = CD3DX12_RESOURCE_BARRIER::Transition(DefaultDepthBuffer->GetResource(),
				D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_DEPTH_WRITE);
			D3D12GfxCommandList->ResourceBarrier(1, &ResBarrier);*/
		}
#else
		auto& D3D12DepthStencilBuffer{ Context->D3D12DepthStencilBuffer };
		// Create the depth/stencil buffer and view.
		D3D12DepthStencilBuffer.Reset();

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
			IID_PPV_ARGS(&D3D12DepthStencilBuffer)));

		// Create descriptor to mip level 0 of entire resource using the format of the resource.
		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
		dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		dsvDesc.Format = DepthBufferFormat;
		dsvDesc.Texture2D.MipSlice = 0;

		D3D12_CPU_DESCRIPTOR_HANDLE DSVHandle = Context->D3D12DSVHeap->GetCPUDescriptorHandleForHeapStart();
		D3D12Device->CreateDepthStencilView(D3D12DepthStencilBuffer.Get(), &dsvDesc, DSVHandle);

		// Transition the resource from its initial state to be used as a depth buffer.
		CD3DX12_RESOURCE_BARRIER ResBarrier = CD3DX12_RESOURCE_BARRIER::Transition(D3D12DepthStencilBuffer.Get(),
			D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE);
		D3D12GfxCommandList->ResourceBarrier(1, &ResBarrier);
#endif
		// Execute the resize commands.
		KS_D3D12_CALL(D3D12GfxCommandList->Close());
		ID3D12CommandList* cmdsLists[] = { D3D12GfxCommandList.Get() };
		Context->D3D12CommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

		// Wait until resize is complete.
		FlushRenderingCommands();

		// Update the viewport transform to cover the client area.
		D3D12Viewport.TopLeftX = 0;
		D3D12Viewport.TopLeftY = 0;
		D3D12Viewport.Width = static_cast<float>(WndResX);
		D3D12Viewport.Height = static_cast<float>(WndResY);
		D3D12Viewport.MinDepth = 0.0f;
		D3D12Viewport.MaxDepth = 1.0f;

		ScissorRect = { 0, 0, static_cast<LONG>(WndResX), static_cast<LONG>(WndResY) };
	}

	void FD3D12RHI::FlushRenderingCommands()
	{
		// Advance the fence value to mark commands up to this fence point.
		CurrentFence++;

		// Add an instruction to the command queue to set a new fence point.  Because we 
		// are on the GPU timeline, the new fence point won't be set until the GPU finishes
		// processing all the commands prior to this Signal().
		auto D3D12Fence{ Context->D3D12Fence.Get() };
		KS_D3D12_CALL(Context->D3D12CommandQueue->Signal(D3D12Fence, CurrentFence));

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
		auto& D3D12CommandAllocator{ Context->D3D12CommandAllocator };
		auto& D3D12GfxCommandList{ Context->D3D12GfxCommandList };
		// Reuse the memory associated with command recording.
		// We can only reset when the associated command lists have finished execution on the GPU.
		KS_D3D12_CALL(D3D12CommandAllocator->Reset());

		// A command list can be reset after it has been added to the command queue via ExecuteCommandList.
		// Reusing the command list reuses memory.
		KS_D3D12_CALL(D3D12GfxCommandList->Reset(D3D12CommandAllocator.Get(), nullptr));

		ID3D12DescriptorHeap* Heaps[] = {Context->CBVHeap.GetHeap()};
		D3D12GfxCommandList->SetDescriptorHeaps(_countof(Heaps), Heaps);

		D3D12GfxCommandList->SetGraphicsRootSignature(Context->GlobalRootSignature.Get());

		// transition back buffers
		{
			CD3DX12_RESOURCE_BARRIER ResBarrier = CD3DX12_RESOURCE_BARRIER::Transition(GetDefaultBackBufferResource(),
				D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_GENERIC_READ);
			GGfxCmdlist->ResourceBarrier(1, &ResBarrier);
		}
	}

	void FD3D12RHI::EndFrame()
	{
		auto& D3D12GfxCommandList{ Context->D3D12GfxCommandList };

		// transition back buffers
		{
			CD3DX12_RESOURCE_BARRIER ResBarrier = CD3DX12_RESOURCE_BARRIER::Transition(GetDefaultBackBufferResource(),
				D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_PRESENT);
			D3D12GfxCommandList->ResourceBarrier(1, &ResBarrier);
		}

		// Done recording commands.
		KS_D3D12_CALL(D3D12GfxCommandList->Close());

		// Add the command list to the queue for execution.
		ID3D12CommandList* cmdsLists[] = { D3D12GfxCommandList.Get() };
		Context->D3D12CommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

		// swap the back and front buffers
		KS_D3D12_CALL(Context->DXGISwapChain->Present(1, 0));

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

	IRHIConstBuffer* FD3D12RHI::CreateConstBuffer(const void* Data, uint32 Size)
	{
		d3d12::FD3D12Resource* Resource{ FD3D12RHI::CreateConstBufferResource(Size) };
		Size = Resource->Size;
		d3d12::FDescriptorHandle ViewHandle{ GD3D12RHI->GetCBVHeap().Allocate() };
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
			FScopedCommandRecorder CommandListRecordHelper(GGfxCmdlist, Context->D3D12CommandAllocator.Get(), Context->D3D12CommandQueue.Get());

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
		GGfxCmdlist->IASetPrimitiveTopology(D3D12PipelineState->PrimitiveType);
	}

	IRHIPipelineState* FD3D12RHI::CreatePipelineState(const FRHIPipelineStateDesc& Desc)
	{
		return new d3d12::FD3D12PipelineState(Desc);
	}

	D3D12_CPU_DESCRIPTOR_HANDLE FD3D12RHI::GetDefaultBackBufferView()
	{
#if RTV_V1
		return Context->DefaultRenderTargets[CurrentBackBuffer]->GetViewHandle().CpuHandle;
#else
		return CD3DX12_CPU_DESCRIPTOR_HANDLE(
			Context->D3D12RTVHeap->GetCPUDescriptorHandleForHeapStart(),
			CurrentBackBuffer,
			RTVSize);
#endif
	}

	ID3D12Resource* FD3D12RHI::GetDefaultBackBufferResource()
	{
#if RTV_V1
		return Context->DefaultRenderTargets[CurrentBackBuffer]->GetResource();
#else
		return Context->D3D12SwapChainBuffers[CurrentBackBuffer].Get();
#endif
	}

	void FD3D12RHI::SetShaderConstBuffer(IRHIConstBuffer* ConstBuffer)
	{
		auto& D3D12GfxCommandList{ Context->D3D12GfxCommandList };
		FD3D12ConstBuffer* D3D12ConstBuffer = dynamic_cast<FD3D12ConstBuffer*>(ConstBuffer);
		assert(D3D12ConstBuffer);
		const FDescriptorHandle& ViewHandle = D3D12ConstBuffer->GetViewHandle();
		D3D12GfxCommandList->SetGraphicsRootDescriptorTable(static_cast<UINT>(ConstBuffer->GetLocationIndex()), ViewHandle.GpuHandle);
	}

	void FD3D12RHI::SetVertexBuffer(const IRHIVertexBuffer* InVertexBuffer)
	{
		auto& D3D12GfxCommandList{ Context->D3D12GfxCommandList };
		const FD3D12VertexBuffer* VertexBuffer{dynamic_cast<const FD3D12VertexBuffer*>(InVertexBuffer)};
		D3D12GfxCommandList->IASetVertexBuffers(0, 1, &VertexBuffer->GetVertexBufferView());
	}

	void FD3D12RHI::SetVertexBuffers(const IRHIVertexBuffer** VertexBuffers, int32 Num)
	{
		auto& D3D12GfxCommandList{ Context->D3D12GfxCommandList };
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
		auto& D3D12GfxCommandList{ Context->D3D12GfxCommandList };
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
		CreateBuffer1(AllocSize, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ, ConstBuffer->D3D12Resource);
		ConstBuffer->D3D12Resource->Map(0, nullptr, &ConstBuffer->MapData);
		ConstBuffer->SetData(Data, Size);
		ConstBuffer->ViewHandle = Context->CBVHeap.Allocate();
		D3D12_CONSTANT_BUFFER_VIEW_DESC desc{};
		desc.BufferLocation = ConstBuffer->D3D12Resource->GetGPUVirtualAddress();
		desc.SizeInBytes = AllocSize;
		GD3D12Device->CreateConstantBufferView(&desc, ConstBuffer->ViewHandle.CpuHandle);
#ifdef KS_DEBUG_BUILD
		KS_NAME_D3D12_OBJECT(ConstBuffer->D3D12Resource.Get(), TEXT("ConstBuffer"));
#endif
		return ConstBuffer;
	}

	void FD3D12RHI::CreateBuffer1(uint32_t Size, D3D12_HEAP_TYPE HeapType, D3D12_RESOURCE_STATES ResStats, ComPtr<ID3D12Resource>& OutResource)
	{
		CD3DX12_HEAP_PROPERTIES HeapProp(HeapType);
		CD3DX12_RESOURCE_DESC ResDesc = CD3DX12_RESOURCE_DESC::Buffer(Size);
		KS_D3D12_CALL(GD3D12Device->CreateCommittedResource(
			&HeapProp,
			D3D12_HEAP_FLAG_NONE,
			&ResDesc,
			ResStats,
			nullptr,
			IID_PPV_ARGS(&OutResource)
		));
	}

	void FD3D12RHI::SetConstBuffer(IRHIConstBuffer1* ConstBuffer)
	{
		auto& D3D12GfxCommandList{ Context->D3D12GfxCommandList };
		assert(ConstBuffer);
		FD3D12ConstBuffer1* D3D12ConstBuffer = dynamic_cast<FD3D12ConstBuffer1*>(ConstBuffer);
		assert(D3D12ConstBuffer);
		const FDescriptorHandle& ViewHandle = D3D12ConstBuffer->GetViewHandle();
		D3D12GfxCommandList->SetGraphicsRootDescriptorTable(static_cast<UINT>(ConstBuffer->GetLocationIndex()), ViewHandle.GpuHandle);
	}

	ks::IRHIVertexBuffer1* FD3D12RHI::CreateVertexBuffer1(uint32 Stride, uint32 Size, const void* Data)
	{
		FD3D12VertexBuffer1* VertBuffer = new FD3D12VertexBuffer1(Stride, Size);
		CreateBuffer1(Size, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_COMMON, VertBuffer->D3D12Resource);
		VertBuffer->BufferView.BufferLocation = VertBuffer->D3D12Resource->GetGPUVirtualAddress();
		VertBuffer->BufferView.StrideInBytes = Stride;
		VertBuffer->BufferView.SizeInBytes = Size;
		if (Data)
		{
			UploadResourceData(VertBuffer->D3D12Resource.Get(), Data, Size);
		}
#ifdef KS_DEBUG_BUILD
		KS_NAME_D3D12_OBJECT(VertBuffer->D3D12Resource.Get(), TEXT("VertexBuffer"));
#endif
		return VertBuffer;
	}

	int32_t FD3D12RHI::UploadResourceData(ID3D12Resource* DestResource, const void* Data, uint32_t Size)
	{
		assert(DestResource && Data && Size);
		if (!DestResource || !Data || !Size)
		{
			return -1;
		}
		ComPtr<ID3D12Resource> UploadBufferPtr;
		CreateBuffer1(Size, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ, UploadBufferPtr);
		ID3D12Resource* UploadBuffer{UploadBufferPtr.Get()};
		ID3D12Resource* DefaultBuffer{DestResource};
		{
			FScopedCommandRecorder ScopedCmdRecorder(
				GGfxCmdlist, Context->D3D12CommandAllocator.Get(), Context->D3D12CommandQueue.Get());
			
			// Schedule to copy the data to the default buffer resource.  At a high level, the helper function UpdateSubresources
			// will copy the CPU memory into the intermediate upload heap.  Then, using ID3D12CommandList::CopySubresourceRegion,
			// the intermediate upload heap data will be copied to mBuffer.
			D3D12_SUBRESOURCE_DATA subResourceData = { Data, Size, Size };
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
		auto ref = UploadBufferPtr.Reset();
		return 0;
	}

	void FD3D12RHI::SetVertexBuffer1(const IRHIVertexBuffer1* _VertexBuffer)
	{
		const FD3D12VertexBuffer1* VertexBuffer{ dynamic_cast<const FD3D12VertexBuffer1*>(_VertexBuffer) };
		GGfxCmdlist->IASetVertexBuffers(0, 1, &VertexBuffer->GetVertexBufferView());
	}

	void FD3D12RHI::SetVertexBuffers1(const IRHIVertexBuffer1** VertexBuffers, int32 Num)
	{
		if (VertexBuffers && Num)
		{
			for (int32 i = 0; i < Num; ++i)
			{
				const FD3D12VertexBuffer1* VertexBuffer{ dynamic_cast<const FD3D12VertexBuffer1*>(VertexBuffers[i]) };
				GGfxCmdlist->IASetVertexBuffers(i, 1, &VertexBuffer->GetVertexBufferView());
			}
		}
	}

	IRHIIndexBuffer1* FD3D12RHI::CreateIndexBuffer1(EELEM_FORMAT ElemFormat, uint32 Count, uint32 Size, const void* Data)
	{
		FD3D12IndexBuffer1* IndexBuffer = new FD3D12IndexBuffer1(ElemFormat, Count, Size);
		CreateBuffer1(Size, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_COMMON, IndexBuffer->D3D12Resource);
		IndexBuffer->BufferView.BufferLocation = IndexBuffer->D3D12Resource->GetGPUVirtualAddress();
		IndexBuffer->BufferView.Format = d3d12::GetDXGIFormat(ElemFormat);
		IndexBuffer->BufferView.SizeInBytes = Size;
		if (Data)
		{
			int32_t error = UploadResourceData(IndexBuffer->D3D12Resource.Get(), Data, Size);
			assert(!error);
		}
#ifdef KS_DEBUG_BUILD
		KS_NAME_D3D12_OBJECT(IndexBuffer->D3D12Resource.Get(), TEXT("IndexBuffer"));
#endif
		return IndexBuffer;
	}

	void FD3D12RHI::DrawIndexedPrimitive1(const IRHIIndexBuffer1* _IndexBuffer)
	{
		const FD3D12IndexBuffer1* IndexBuffer{ dynamic_cast<const FD3D12IndexBuffer1*>(_IndexBuffer) };
		const D3D12_INDEX_BUFFER_VIEW IndexBufferView{ IndexBuffer->GetIndexBufferView() };
		GGfxCmdlist->IASetIndexBuffer(&IndexBufferView);

		const UINT IndexCount{ IndexBuffer->GetCount() };
		GGfxCmdlist->DrawIndexedInstanced(IndexCount, 1, 0, 0, 0);
	}

	ks::IRHITexture2D* FD3D12RHI::CreateTexture2D(const FTexture2DDesc& Desc)
	{
		FD3D12Texture2D* Texture = new FD3D12Texture2D(Desc);


		return Texture;
	}

	ks::IRHIDepthStencilBuffer* FD3D12RHI::CreateDepthStencilBuffer(const FTexture2DDesc& Desc)
	{
#if  RTV_V1
		FD3D12DepthStencilBuffer1* DepthBuffer = new FD3D12DepthStencilBuffer1(Desc);
		auto& D3D12ResComPtr{DepthBuffer->GetComPtr()};

		D3D12_RESOURCE_DESC ResDesc;
		ResDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		ResDesc.Alignment = 0;
		ResDesc.Width = Desc.Width;
		ResDesc.Height = Desc.Height;
		ResDesc.DepthOrArraySize = 1;
		ResDesc.MipLevels = 1;
		ResDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
		ResDesc.SampleDesc.Count = 1;
		ResDesc.SampleDesc.Quality = 0;
		ResDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		ResDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

		D3D12_CLEAR_VALUE ClearVal;
		ClearVal.Format = DepthBufferFormat;
		ClearVal.DepthStencil.Depth = 1.0f;
		ClearVal.DepthStencil.Stencil = 0;

		CD3DX12_HEAP_PROPERTIES HeapProp(D3D12_HEAP_TYPE_DEFAULT);

		KS_D3D12_CALL(D3D12Device->CreateCommittedResource(
			&HeapProp,
			D3D12_HEAP_FLAG_NONE,
			&ResDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			&ClearVal,
			IID_PPV_ARGS(&D3D12ResComPtr)));

		// create depth stencil view
		D3D12_DEPTH_STENCIL_VIEW_DESC DSVDesc;
		DSVDesc.Flags = D3D12_DSV_FLAG_NONE;
		DSVDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		DSVDesc.Format = DepthBufferFormat;
		DSVDesc.Texture2D.MipSlice = 0;
		DepthBuffer->DSVHandle = Context->DSVHeap.Allocate();
		D3D12Device->CreateDepthStencilView(DepthBuffer->GetResource(), &DSVDesc, DepthBuffer->DSVHandle.CpuHandle);

		// create shader resource view
		D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc;
		SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		SRVDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
		SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		SRVDesc.Texture2D.MostDetailedMip = 0;
		SRVDesc.Texture2D.MipLevels = 1;
		SRVDesc.Texture2D.ResourceMinLODClamp = 0.f;
		SRVDesc.Texture2D.PlaneSlice = 0;
		DepthBuffer->ViewHandle = Context->CBVHeap.Allocate();
		D3D12Device->CreateShaderResourceView(DepthBuffer->GetResource(), &SRVDesc, DepthBuffer->ViewHandle.CpuHandle);

		return DepthBuffer;
#else
		FD3D12DepthStencilBuffer* DepthBuffer = new FD3D12DepthStencilBuffer(Desc);
		return DepthBuffer;
#endif
	}

	void FD3D12RHI::SetViewports(uint32_t Num, const FViewPort* Viewports)
	{
		assert(Num <= 8);
		static const uint32_t NumMaxView{4};
		static D3D12_VIEWPORT D3D12Viewports[NumMaxView];
		static D3D12_RECT D3D12ScissorRects[NumMaxView];
		for (uint32_t i{0}; i < Num && i < NumMaxView; ++i)
		{
			D3D12Viewports[i] = {0, 0,
				static_cast<float>(Viewports[i].Width),
				static_cast<float>(Viewports[i].Height),
				0.f, 1.f};
			D3D12ScissorRects[i] = {0, 0,
				static_cast<LONG>(Viewports[i].Width),
				static_cast<LONG>(Viewports[i].Height)};
		}
		GGfxCmdlist->RSSetViewports(Num, D3D12Viewports);
		GGfxCmdlist->RSSetScissorRects(Num, D3D12ScissorRects);
	}

	void FD3D12RHI::ClearRenderTarget(const FColor& Color)
	{
		auto& PassRenderTarget{ Context->CurrentRenderTarget };
		assert(PassRenderTarget);
		const auto& RTV{PassRenderTarget->GetRenderTargetView().CpuHandle};
		GGfxCmdlist->ClearRenderTargetView(RTV, Color, 0, nullptr);
	}

	void FD3D12RHI::SetRenderTarget(IRHIRenderTarget* RenderTarget, IRHIDepthStencilBuffer* DepthStencilBuffer)
	{
		auto& CurrentRenderTarget{ Context->CurrentRenderTarget };
		auto& CurrentDepthStencilBuffer{ Context->CurrentDepthStencilBuffer };
		//if (RenderTarget != CurrentRenderTarget || DepthStencilBuffer != CurrentDepthStencilBuffer)
		{
			CurrentRenderTarget = dynamic_cast<FD3D12RenderTarget*>(RenderTarget);
			int32_t NumRenderTargets = CurrentRenderTarget ? 1 : 0;
			const D3D12_CPU_DESCRIPTOR_HANDLE* pRTV{nullptr};
			if (NumRenderTargets)
			{
				pRTV = &CurrentRenderTarget->GetRenderTargetView().CpuHandle;
			}
			bool bSingleHandle = NumRenderTargets != 0;

			CurrentDepthStencilBuffer = dynamic_cast<FD3D12DepthStencilBuffer1*>(DepthStencilBuffer);
			const D3D12_CPU_DESCRIPTOR_HANDLE* pDSV = CurrentDepthStencilBuffer ?
				&CurrentDepthStencilBuffer->GetDepthStencilView().CpuHandle : nullptr;
			GGfxCmdlist->OMSetRenderTargets(NumRenderTargets, pRTV, bSingleHandle, pDSV);
		}
	}

	IRHIRenderTarget* FD3D12RHI::GetCurrentBackBuffer()
	{
		return Context->DefaultRenderTargets[CurrentBackBuffer].get();
	}

	ks::d3d12::FDescriptorHeap& FD3D12RHI::GetCBVHeap()
	{
		return Context->CBVHeap;
	}

	ID3D12RootSignature* FD3D12RHI::GetGlobalRootSignature()
	{
		return Context->GlobalRootSignature.Get();
	}

	D3D12_CPU_DESCRIPTOR_HANDLE FD3D12RHI::GetDefaultDepthBufferView()
	{
		return Context->DefaultDepthStencilBuffer->GetViewHandle().CpuHandle;
	}

	void FD3D12RHI::BeginPass()
	{
		// transition render target state
		auto& CurrentRenderTarget{ Context->CurrentRenderTarget };
		if (CurrentRenderTarget)
		{
			CD3DX12_RESOURCE_BARRIER RenderTargetBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
				CurrentRenderTarget->GetResource(),
				D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET);
			GGfxCmdlist->ResourceBarrier(1, &RenderTargetBarrier);
		}
		

		// transition depth stencil state
		auto& CurrentDepthStencilBuffer{ Context->CurrentDepthStencilBuffer };
		CD3DX12_RESOURCE_BARRIER DepthStencilBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
			CurrentDepthStencilBuffer->GetResource(),
			D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_DEPTH_WRITE);
		GGfxCmdlist->ResourceBarrier(1, &DepthStencilBarrier);
	}

	void FD3D12RHI::EndPass()
	{
		auto& CurrentRenderTarget{ Context->CurrentRenderTarget };
		if (CurrentRenderTarget) {
			
			CD3DX12_RESOURCE_BARRIER ResBarrier = CD3DX12_RESOURCE_BARRIER::Transition(CurrentRenderTarget->GetResource(),
				D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ);
			GGfxCmdlist->ResourceBarrier(1, &ResBarrier);
		}
		{
			auto& CurrentDepthStencilBuffer{ Context->CurrentDepthStencilBuffer };
			CD3DX12_RESOURCE_BARRIER ResBarrier = CD3DX12_RESOURCE_BARRIER::Transition(CurrentDepthStencilBuffer->GetResource(),
				D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_GENERIC_READ);
			GGfxCmdlist->ResourceBarrier(1, &ResBarrier);
		}
	}

	void FD3D12RHI::ClearDepthStencilBuffer()
	{
		const auto& DSV{ Context->CurrentDepthStencilBuffer->GetDepthStencilView().CpuHandle };
		GGfxCmdlist->ClearDepthStencilView(DSV,
			D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
	}

	ks::IRHIDepthStencilBuffer* FD3D12RHI::GetDefaultDepthStencilBuffer()
	{
		return Context->DefaultDepthStencilBuffer.get();
	}

	void FD3D12RHI::SetTexture2D(IRHITexture2D* Texture2D)
	{
		FD3D12Texture2D1* D3D12Texture2D = dynamic_cast<FD3D12Texture2D1*>(Texture2D);
		const FDescriptorHandle& SRV{D3D12Texture2D->GetViewHandle()};
		GGfxCmdlist->SetGraphicsRootDescriptorTable(static_cast<UINT>(Texture2D->GetLocationIndex()), SRV.GpuHandle);
	}

	IRHIRenderTarget* FD3D12RHI::CreateRenderTarget(const FTexture2DDesc& Desc)
	{
		FD3D12RenderTarget* RenderTarget = new FD3D12RenderTarget(Desc);
		auto& D3D12ResComPtr{RenderTarget->GetComPtr()};

		D3D12_RESOURCE_DESC ResDesc{};
		std::memset(&ResDesc, 0, sizeof(D3D12_RESOURCE_DESC));
		ResDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		ResDesc.Alignment = 0;
		ResDesc.Width = Desc.Width;
		ResDesc.Height = Desc.Height;
		ResDesc.DepthOrArraySize = 1;
		ResDesc.MipLevels = 1;
		ResDesc.Format = /*DXGI_FORMAT_R16G16B16A16_TYPELESS*/GetDXGIFormat(Desc.Format);
		ResDesc.SampleDesc.Count = 1;
		ResDesc.SampleDesc.Quality = 0;
		ResDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		ResDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

		float ClearColor[] = { 0.f, 0.f, 0.f, 1.f };
		CD3DX12_CLEAR_VALUE OptClearVal{ GetDXGIFormat(Desc.Format), ClearColor};
		CD3DX12_HEAP_PROPERTIES HeapProp(D3D12_HEAP_TYPE_DEFAULT);
		KS_D3D12_CALL(D3D12Device->CreateCommittedResource(
			&HeapProp,
			D3D12_HEAP_FLAG_NONE,
			&ResDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			&OptClearVal,
			IID_PPV_ARGS(&D3D12ResComPtr)
		));

		D3D12_RENDER_TARGET_VIEW_DESC RTVDesc{};
		RTVDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		RTVDesc.Format = GetDXGIFormat(Desc.Format);
		RTVDesc.Texture2D.MipSlice = 0;
		RTVDesc.Texture2D.PlaneSlice = 0;
		RenderTarget->RTVHandle = Context->RTVHeap.Allocate();
		D3D12Device->CreateRenderTargetView(RenderTarget->GetResource(), &RTVDesc, RenderTarget->RTVHandle.CpuHandle);

		D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc{};
		SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		SRVDesc.Format = GetDXGIFormat(Desc.Format);
		SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		SRVDesc.Texture2D.MostDetailedMip = 0;
		SRVDesc.Texture2D.MipLevels = 1;
		RenderTarget->ViewHandle = Context->CBVHeap.Allocate();
		D3D12Device->CreateShaderResourceView(RenderTarget->GetResource(), &SRVDesc, RenderTarget->ViewHandle.CpuHandle);

		return RenderTarget;
	}

}


