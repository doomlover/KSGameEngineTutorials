#include "engine_pch.h"
#include "D3D12Resource.h"

namespace ks::d3d12
{
	FD3D12ConstBuffer1::FD3D12ConstBuffer1(uint32_t _Size)
		:AllocSize(_Size)
	{
	}

	FD3D12ConstBuffer1::~FD3D12ConstBuffer1()
	{
		if (D3D12Resource)
		{
			D3D12Resource->Unmap(0, nullptr);
		}
	}

	void FD3D12ConstBuffer1::SetData(const void* Data, uint32_t Size)
	{
		memcpy(MapData, Data, Size);
	}

	FD3D12IndexBuffer1::FD3D12IndexBuffer1(EELEM_FORMAT _ElemFormat, uint32 _Count, uint32 _Size)
		:IRHIIndexBuffer1(_ElemFormat, _Count, _Size)
	{

	}

	FD3D12VertexBuffer1::FD3D12VertexBuffer1(uint32_t _Stride, uint32_t _Size)
		:IRHIVertexBuffer1(_Stride, _Size)
	{

	}

	FD3D12DepthStencilBuffer1::FD3D12DepthStencilBuffer1(const FTexture2DDesc& _Desc)
		:IRHIDepthStencilBuffer(_Desc)
		,FD3D12Texture2D1(_Desc)
	{

	}

}
