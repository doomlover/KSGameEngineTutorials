#pragma once

#include "RHI/RHIResource.h"

namespace ks
{
	struct FMeshData;

	class FMeshRenderData
	{
	public:
		FMeshRenderData(const FMeshData& _MeshData);
#if RHIINDEXBUFFER_V1
		const IRHIIndexBuffer1* GetRHIIndexBuffer1() const { return RHIIndexBuffer1.get(); }
#else
		const IRHIIndexBuffer* GetRHIIndexBuffer() const { return RHIIndexBuffer.get(); }
#endif
#if !RHIVERTBUFFER_V1
		const IRHIVertexBuffer* GetRHIVertexBuffer() const { return RHIVertBuffer.get(); }
		const IRHIVertexBuffer* GetRHIAttrBuffer() const { return RHIAttrBuffer.get(); }
#else
		const IRHIVertexBuffer1* GetRHIVertexBuffer1() const { return RHIVertBuffer1.get(); }
		const IRHIVertexBuffer1* GetRHIAttrBuffer1() const { return RHIAttrBuffer1.get(); }
#endif
	private:
		void InitRHI();
		const FMeshData& MeshData;
#if RHIINDEXBUFFER_V1
		std::unique_ptr<IRHIIndexBuffer1> RHIIndexBuffer1;
#else
		std::unique_ptr<IRHIIndexBuffer> RHIIndexBuffer;
#endif
#if !RHIVERTBUFFER_V1
		std::unique_ptr<IRHIVertexBuffer> RHIVertBuffer;
		std::unique_ptr<IRHIVertexBuffer> RHIAttrBuffer;
#else
		std::unique_ptr<IRHIVertexBuffer1> RHIAttrBuffer1;
		std::unique_ptr<IRHIVertexBuffer1> RHIVertBuffer1;
#endif
	};
}