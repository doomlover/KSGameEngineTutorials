#pragma once
#include "RHI/RHI.h"
#include "RHI/D3D12/D3D12Common.h"

namespace ks::d3d12
{
	class FD3D12PipelineState : public IRHIPipelineState
	{
		friend class FD3D12RHI;
	public:
		FD3D12PipelineState(const FRHIPipelineStateDesc& Desc);
		virtual ~FD3D12PipelineState(){}
	private:
		ComPtr<ID3D12PipelineState> PipelineState{nullptr};
		ID3D12RootSignature* pRootSignature{nullptr};
		D3D12_PRIMITIVE_TOPOLOGY PrimitiveType{ D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST };
		std::unordered_map<std::string, ComPtr<ID3DBlob>> Shaders;
	};
}


