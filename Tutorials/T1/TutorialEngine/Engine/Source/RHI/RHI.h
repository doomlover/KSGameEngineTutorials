#pragma once

#include "RHICommon.h"

namespace ks
{
	class IRHIPipelineState;

	class IRHI
	{
	public:
		virtual ~IRHI() {}
		static IRHI* Create();
		virtual void Init(const FRHIConfig& Config) = 0;
		virtual void Shutdown() = 0;
		virtual void ResizeWindow() = 0;
		virtual void FlushRenderingCommands() = 0;
		virtual void BeginFrame() = 0;
		virtual void EndFrame() = 0;
		virtual IRHIConstBuffer* CreateConstBuffer(const void* Data, uint32 Size) = 0;
		virtual IRHIConstBuffer1* CreateConstBuffer1(const void* Data, uint32 Size) = 0;
		virtual void SetPipelineState(IRHIPipelineState* PipelineState) = 0;
		virtual IRHIPipelineState* CreatePipelineState(const FRHIPipelineStateDesc& Desc) = 0;
		virtual void SetShaderConstBuffer(IRHIConstBuffer* ConstBuffer) = 0;
		virtual void SetConstBuffer(IRHIConstBuffer1* ConstBuffer) = 0;
		virtual void SetVertexBuffer(const IRHIVertexBuffer* VertexBuffer) = 0;
		virtual void SetVertexBuffer1(const IRHIVertexBuffer1* VertexBuffer) = 0;
		virtual void SetVertexBuffers(const IRHIVertexBuffer** VertexBuffers, int32 Num) = 0;
		virtual void SetVertexBuffers1(const IRHIVertexBuffer1** VertexBuffers, int32 Num) = 0;
		virtual IRHIVertexBuffer* CreateVertexBuffer(uint32 Stride, uint32 Size, const void* Data) = 0;
		virtual IRHIVertexBuffer1* CreateVertexBuffer1(uint32 Stride, uint32 Size, const void* Data) = 0;
		virtual IRHIIndexBuffer* CreateIndexBuffer(EELEM_FORMAT ElemFormat, uint32 Count, uint32 Size, const void* Data) = 0;
		virtual IRHIIndexBuffer1* CreateIndexBuffer1(EELEM_FORMAT ElemFormat, uint32 Count, uint32 Size, const void* Data) = 0;
		virtual void DrawIndexedPrimitive(const IRHIIndexBuffer* IndexBuffer) = 0;
		virtual void DrawIndexedPrimitive1(const IRHIIndexBuffer1* IndexBuffer) = 0;
		virtual IRHITexture2D* CreateTexture2D(const FTexture2DDesc& Desc) = 0;
		virtual IRHIDepthStencilBuffer* CreateDepthStencilBuffer(const FTexture2DDesc& Desc) = 0;
		virtual void SetViewports(uint32_t Num, const FViewPort* Viewports) = 0;
		virtual void ClearRenderTarget(const FColor& Color) = 0;
		virtual void SetRenderTarget(IRHIRenderTarget* RenderTarget, IRHIDepthStencilBuffer* DepthBuffer) = 0;
		virtual void ClearDepthStencilBuffer() = 0;
		virtual void BeginPass() = 0;
		virtual void EndPass() = 0;
		virtual IRHIRenderTarget* GetCurrentBackBuffer() = 0;
		virtual IRHIDepthStencilBuffer* GetDefaultDepthStencilBuffer() = 0;
		virtual void SetTexture2D(IRHITexture2D* Texture2D) = 0;
	};

	extern IRHI* GRHI;
	extern FRHIConfig GRHIConfig;

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