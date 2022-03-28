#pragma once

#include "RHI/RHI.h"

namespace ks
{
	struct FMeshData;

	class FMeshRenderData
	{
	public:
		FMeshRenderData(const FMeshData& _MeshData);
		const IRHIVertexBuffer* GetRHIVertexBuffer() const { return RHIVertBuffer.get(); }
		const IRHIIndexBuffer* GetRHIIndexBuffer() const { return RHIIndexBuffer.get(); }
	private:
		void InitRHI();
		const FMeshData& MeshData;
		std::unique_ptr<IRHIIndexBuffer> RHIIndexBuffer;
		std::unique_ptr<IRHIVertexBuffer> RHIVertBuffer;
	};
}