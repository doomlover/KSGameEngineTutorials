#include "engine_pch.h"
#include "Core/Asset/MeshAsset.h"

namespace ks
{
namespace
{
	
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
			EELEM_FORMAT ElemFormat = util::GetElemFormat(MeshData.IndexDataType, EELEM_TYPE::SCALAR);
			uint32 Stride = static_cast<uint32>(util::GetDataTypeSize(MeshData.IndexDataType));
			const uint32 Size{static_cast<uint32>(MeshData.IndexRawData.size())};

			IRHIIndexBuffer* _RHIBuffer = GRHI->CreateIndexBuffer(ElemFormat, Stride, Size, MeshData.IndexRawData.data());
			RHIIndexBuffer.reset(_RHIBuffer);
		}

		auto CreateRHIVertBuffer = [](const FMeshAttributeData& MeshAttriData, std::unique_ptr<IRHIVertexBuffer>& RHIVertBuffer) {
			assert(!RHIVertBuffer);
			const uint32& Stride { MeshAttriData.Stride };
			const uint32 Size{ Stride * MeshAttriData.Count };
			const void* Data{ MeshAttriData.Data.data() };
			IRHIVertexBuffer* VertBuffer = GRHI->CreateVertexBuffer(Stride, Size, Data);
			RHIVertBuffer.reset(VertBuffer);
		};

		CreateRHIVertBuffer(MeshData.PositionData, RHIVertBuffer);
		CreateRHIVertBuffer(MeshData.AttributeData, RHIAttrBuffer);
	}

}