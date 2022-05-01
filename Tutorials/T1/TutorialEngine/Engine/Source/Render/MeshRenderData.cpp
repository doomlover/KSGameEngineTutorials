#include "engine_pch.h"
#include "Core/Asset/MeshAsset.h"
#include "RHI/RHI.h"

namespace ks
{
	FMeshRenderData::FMeshRenderData(const FMeshData& _MeshData)
		:MeshData(_MeshData)
	{
		InitRHI();
	}

	void FMeshRenderData::InitRHI()
	{
		{
			assert(!RHIIndexBuffer);
			EELEM_FORMAT ElemFormat = util::GetElemFormat(MeshData.IndexData.DataType, EELEM_TYPE::SCALAR);
			const auto& Count{MeshData.IndexData.Count};
			const uint32_t Size{static_cast<uint32_t>(MeshData.IndexData.Data.size())};
			IRHIIndexBuffer* _RHIBuffer = GRHI->CreateIndexBuffer(ElemFormat, Count, Size, MeshData.IndexData.Data.data());
			RHIIndexBuffer.reset(_RHIBuffer);
		}
#if !RHIRESOURCE_V1
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
#else
		auto CreateRHIVertBuffer = [](const FMeshAttributeData& MeshAttriData, std::unique_ptr<IRHIVertexBuffer1>& RHIVertBuffer) {
			assert(!RHIVertBuffer);
			const uint32& Stride{ MeshAttriData.Stride };
			const uint32 Size{ Stride * MeshAttriData.Count };
			const void* Data{ MeshAttriData.Data.data() };
			IRHIVertexBuffer1* VertBuffer = GRHI->CreateVertexBuffer1(Stride, Size, Data);
			RHIVertBuffer.reset(VertBuffer);
		};

		CreateRHIVertBuffer(MeshData.PositionData, RHIVertBuffer1);
		CreateRHIVertBuffer(MeshData.AttributeData, RHIAttrBuffer1);
#endif
	}

}