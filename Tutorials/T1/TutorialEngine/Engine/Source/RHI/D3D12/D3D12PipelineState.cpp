#include "engine_pch.h"
#include "RHI/D3D12/D3D12RHI.h"
#include "RHI/D3D12/D3D12PipelineState.h"

namespace ks::d3d12
{
namespace
{
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

	void BuildVertexShader(std::unordered_map<std::string, ComPtr<ID3DBlob>>& Shaders, const FVertexShaderDesc& ShaderDesc)
	{
		auto ShaderFile = FString::S2WS(ShaderDesc.File);
		Shaders[ShaderDesc.Name] = CompileShader(ShaderFile, nullptr, ShaderDesc.EntryPoint, "vs_5_1");
	}

	void BuildPixelShader(std::unordered_map<std::string, ComPtr<ID3DBlob>>& Shaders, const FPixelShaderDesc& ShaderDesc)
	{
		auto ShaderFile = FString::S2WS(ShaderDesc.File);
		Shaders[ShaderDesc.Name] = CompileShader(ShaderFile, nullptr, ShaderDesc.EntryPoint, "ps_5_1");
	}
}

	FD3D12PipelineState::FD3D12PipelineState(const FRHIPipelineStateDesc& Desc)
		:IRHIPipelineState(Desc)
	{
		const auto& InputElemDesc{Desc.InputLayout};
		std::vector<D3D12_INPUT_ELEMENT_DESC> InputLayout(Desc.InputLayout.size());
		for (int32 i{0}; i < Desc.InputLayout.size(); ++i)
		{
			InputLayout[i].SemanticName = InputElemDesc[i].SemanticName.c_str();
			InputLayout[i].SemanticIndex = InputElemDesc[i].SemanticIndex;
			InputLayout[i].Format = GetDXGIFormat(InputElemDesc[i].ElemFormat);
			InputLayout[i].InputSlot = InputElemDesc[i].InputSlot;
			InputLayout[i].AlignedByteOffset = InputElemDesc[i].Stride;
			InputLayout[i].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
			InputLayout[i].InstanceDataStepRate = 0;
		}

		D3D12_GRAPHICS_PIPELINE_STATE_DESC D3D12Desc;
		ZeroMemory(&D3D12Desc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));

		D3D12Desc.InputLayout = {InputLayout.data(), static_cast<UINT>(InputLayout.size())};

		pRootSignature = GD3D12RHI->GetGlobalRootSignature();
		D3D12Desc.pRootSignature = pRootSignature;

		if (!Desc.VertexShaderDesc.File.empty())
		{
			BuildVertexShader(Shaders, Desc.VertexShaderDesc);
			D3D12Desc.VS =
			{
				reinterpret_cast<BYTE*>(Shaders[Desc.VertexShaderDesc.Name]->GetBufferPointer()),
				Shaders[Desc.VertexShaderDesc.Name]->GetBufferSize()
			};
		}
		if (!Desc.PixelShaderDesc.File.empty())
		{
			BuildPixelShader(Shaders, Desc.PixelShaderDesc);
			D3D12Desc.PS =
			{
				reinterpret_cast<BYTE*>(Shaders[Desc.PixelShaderDesc.Name]->GetBufferPointer()),
				Shaders[Desc.PixelShaderDesc.Name]->GetBufferSize()
			};
		}

		D3D12Desc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		D3D12Desc.RasterizerState.FrontCounterClockwise = TRUE;
		D3D12Desc.RasterizerState.DepthBias = Desc.RasterizerState.DepthBias;
		D3D12Desc.RasterizerState.DepthBiasClamp = Desc.RasterizerState.DepthBiasClamp;
		D3D12Desc.RasterizerState.SlopeScaledDepthBias = Desc.RasterizerState.SlopeScaledDepthBias;

		D3D12Desc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		D3D12Desc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		D3D12Desc.SampleMask = UINT_MAX;
		D3D12Desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

		D3D12Desc.NumRenderTargets = Desc.NumRenderTargets;
		if (Desc.NumRenderTargets)
		{
			for (uint32_t i{ 0 }; i < Desc.NumRenderTargets; ++i)
			{
				D3D12Desc.RTVFormats[i] = GetDXGIFormat(Desc.RenderTargetFormats[i]);
			}
		}
		else
		{
			D3D12Desc.RTVFormats[0] = DXGI_FORMAT_UNKNOWN;
		}
		
		D3D12Desc.DSVFormat = GetDXGIFormat(Desc.DepthBufferFormat);

		D3D12Desc.SampleDesc.Count = 1;
		D3D12Desc.SampleDesc.Quality = 0;
		KS_D3D12_CALL(GD3D12Device->CreateGraphicsPipelineState(&D3D12Desc, IID_PPV_ARGS(PipelineState.GetAddressOf())));
	}

}

