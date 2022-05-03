#include "engine_pch.h"

#include "RHI/D3D12/D3D12RHI.h"

namespace ks
{
	IRHI* GRHI = nullptr;
	FRHIConfig GRHIConfig;

	/*IRHI::~IRHI()
	{
		KS_INFO(TEXT("~IRHI"));
	}*/

	IRHI* IRHI::Create()
	{
		KS_INFO(TEXT("IRHI::Create"));
		GRHI = new d3d12::FD3D12RHI;
		return GRHI;
	}

}

