#include "engine_pch.h"
#include "Core/MeshAsset.h"

namespace ks
{
namespace
{
	int32 GetElemNum(EELEM_TYPE ElemType)
	{
		switch (ElemType)
		{
		case EELEM_TYPE::SCALAR:
			return 1;
		case EELEM_TYPE::VEC2:
			return 2;
		case EELEM_TYPE::VEC3:
			return 3;
		case EELEM_TYPE::VEC4:
			return 4;
		case EELEM_TYPE::MAT2:
			return 4;
		case EELEM_TYPE::MAT3:
			return 9;
		case EELEM_TYPE::MAT4:
			return 16;
		default:
			assert(false);
			break;
		}
		return 0;
	}
}

	FMeshRenderData::FMeshRenderData(const FMeshData& _MeshData)
		:MeshData(_MeshData)
	{
		InitRHI();
	}

	void FMeshRenderData::InitRHI()
	{
		{
			assert(!RHIIndexBuffer);
			EELEM_FORMAT ElemFormat = ks::GetElemFormat(MeshData.IndexDataType, EELEM_TYPE::SCALAR);
			uint32 Stride = GetDataTypeSize(MeshData.IndexDataType);
			const uint32 Size{static_cast<uint32>(MeshData.IndexRawData.size())};

			IRHIIndexBuffer* _RHIBuffer = GRHI->CreateIndexBuffer(ElemFormat, Stride, Size, MeshData.IndexRawData.data());
			RHIIndexBuffer.reset(_RHIBuffer);
		}

		{
			assert(!RHIVertBuffer);
			uint32 Stride = GetElemNum(MeshData.PositionElemType) * GetDataTypeSize(MeshData.PositionDataType);
			uint32 Size = static_cast<uint32>(MeshData.PositionRawData.size());
			IRHIVertexBuffer* _VertBuffer = GRHI->CreateVertexBuffer(Stride, Size, MeshData.PositionRawData.data());
			RHIVertBuffer.reset(_VertBuffer);
		}
	}

}