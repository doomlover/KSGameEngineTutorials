#pragma once
#include "RHICommon.h"
#include "RHI/RHIPipelineStateDesc.h"

namespace ks
{
	class IRHIResource;
	class IRHIBuffer;
	class IRHIConstBuffer;
	class IRHIPipelineState;
	class IRHIVertexBuffer;
	class IRHIIndexBuffer;

	class IRHI
	{
	public:
		virtual ~IRHI() {}
		static IRHI* Create();
		virtual void Init() = 0;
		virtual void Shutdown() = 0;
		virtual void ResizeWindow() = 0;
		virtual void FlushRenderingCommands() = 0;
		virtual void BeginFrame() = 0;
		virtual void EndFrame() = 0;
		virtual IRHIConstBuffer* CreateConstBuffer(const void* Data, uint32 Size) = 0;
		virtual IRHIBuffer* CreateBuffer(uint32 Size, const void* Data) = 0;
		virtual void SetPipelineState(IRHIPipelineState* PipelineState) = 0;
		virtual IRHIPipelineState* CreatePipelineState(const FRHIPipelineStateDesc& Desc) = 0;
		virtual void SetShaderConstBuffer(IRHIConstBuffer* ConstBuffer) = 0;
		virtual void SetVertexBuffer(const IRHIVertexBuffer* VertexBuffer) = 0;
		virtual void SetVertexBuffers(const IRHIVertexBuffer** VertexBuffers, int32 Num) = 0;
		virtual IRHIVertexBuffer* CreateVertexBuffer(uint32 Stride, uint32 Size, const void* Data) = 0;
		virtual IRHIIndexBuffer* CreateIndexBuffer(EELEM_FORMAT ElemFormat, uint32 Stride, uint32 Size, const void* Data) = 0;
		virtual void DrawIndexedPrimitive(const IRHIIndexBuffer* IndexBuffer) = 0;
	};

	extern IRHI* GRHI;

	template<typename _TParmType>
	class TConstBuffer
	{
	public:
		using PtrType = TConstBuffer<_TParmType>*;
		TConstBuffer(const _TParmType& ParmValue)
		{
			RHIConstBuffer.reset(GRHI->CreateConstBuffer(&ParmValue, static_cast<uint32>(sizeof(_TParmType))));
		}

		IRHIConstBuffer* GetRHIConstBuffer() const { return RHIConstBuffer.get(); }

		template<typename _TBufferType = TConstBuffer<_TParmType>>
		static _TBufferType* CreateConstBuffer(const _TParmType& ParmValue)
		{
			_TBufferType* NewBuffer = new _TBufferType(ParmValue);
			return NewBuffer;
		}
	private:
		std::unique_ptr<IRHIConstBuffer> RHIConstBuffer;
	};

	class IRHIPipelineState
	{
	public:
		IRHIPipelineState(const FRHIPipelineStateDesc& _Desc) : Desc(_Desc){}
		virtual ~IRHIPipelineState(){}
	protected:
		FRHIPipelineStateDesc Desc;
	};

	
}