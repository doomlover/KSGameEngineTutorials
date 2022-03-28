#include "engine_pch.h"
#include "RHI/D3D12/D3D12RHI.h"
#include "RHI/D3D12/D3D12PipelineState.h"

namespace ks::d3d12
{
namespace
{
	void BuildRootSignature(ComPtr<ID3D12RootSignature>& RootSignature)
	{
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

	ComPtr<ID3DBlob> CompileShader(
		const std::wstring& filename,
		const D3D_SHADER_MACRO* defines,
		const std::string& entrypoint,
		const std::string& target)
	{
		UINT compileFlags = 0;
#if defined(KS_DEBUG_BUILD) || defined(_DEBUG)  
		compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

		HRESULT hr = S_OK;

		ComPtr<ID3DBlob> byteCode = nullptr;
		ComPtr<ID3DBlob> errors;
		hr = D3DCompileFromFile(filename.c_str(), defines, D3D_COMPILE_STANDARD_FILE_INCLUDE,
			entrypoint.c_str(), target.c_str(), compileFlags, 0, &byteCode, &errors);

		if (errors != nullptr)
		{
			KS_INFOA((char*)errors->GetBufferPointer());
		}
		KS_D3D12_CALL(hr);

		return byteCode;
	}

	void BuildShaders(std::unordered_map<std::string, ComPtr<ID3DBlob>>& Shaders)
	{
		std::wstring ShaderPath{FString::S2WS(::ks::GetShaderPath("color.hlsl"))};
		Shaders["standardVS"] = CompileShader(ShaderPath, nullptr, "VS", "vs_5_1");
		Shaders["opaquePS"] = CompileShader(ShaderPath, nullptr, "PS", "ps_5_1");
	}
}

	FD3D12PipelineState::FD3D12PipelineState(const FRHIPipelineStateDesc& Desc)
		:IRHIPipelineState(Desc)
	{
		std::vector<D3D12_INPUT_ELEMENT_DESC> InputLayout =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			//{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};
		D3D12_GRAPHICS_PIPELINE_STATE_DESC D3D12Desc;
		ZeroMemory(&D3D12Desc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));

		D3D12Desc.InputLayout = {InputLayout.data(), static_cast<UINT>(InputLayout.size())};

		BuildRootSignature(RootSignature);
		D3D12Desc.pRootSignature = RootSignature.Get();

		BuildShaders(Shaders);
		D3D12Desc.VS =
		{
			reinterpret_cast<BYTE*>(Shaders["standardVS"]->GetBufferPointer()),
			Shaders["standardVS"]->GetBufferSize()
		};
		D3D12Desc.PS =
		{
			reinterpret_cast<BYTE*>(Shaders["opaquePS"]->GetBufferPointer()),
			Shaders["opaquePS"]->GetBufferSize()
		};
		D3D12Desc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		D3D12Desc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
		D3D12Desc.RasterizerState.FrontCounterClockwise = TRUE;
		D3D12Desc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		D3D12Desc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		D3D12Desc.SampleMask = UINT_MAX;
		D3D12Desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		D3D12Desc.NumRenderTargets = 1;
		D3D12Desc.RTVFormats[0] = d3d12::GD3D12RHI->GetBackbufferFormat();
		D3D12Desc.SampleDesc.Count = 1;
		D3D12Desc.SampleDesc.Quality = 0;
		D3D12Desc.DSVFormat = d3d12::GD3D12RHI->GetDepthbufferFormat();
		KS_D3D12_CALL(GD3D12Device->CreateGraphicsPipelineState(&D3D12Desc, IID_PPV_ARGS(PipelineState.GetAddressOf())));
	}

}

