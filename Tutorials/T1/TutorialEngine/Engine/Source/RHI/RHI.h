#pragma once
#include "Core/CoreMinimal.h"
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


	class IRHIResource
	{
	public:
		virtual ~IRHIResource(){}
		virtual void* Map() = 0;
		virtual void Unmap() = 0;
		uint32 Size{0};
	};

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

	class IRHIConstBuffer
	{
	public:
		IRHIConstBuffer() = default;
		virtual ~IRHIConstBuffer(){}
		virtual void UpdateData(const void* pData, uint32 Size) {
			void* pDest = RHIResource->Map();
			memcpy(pDest, pData, Size);
			RHIResource->Unmap();
		}
		void SetLocationIndex(int32 Index) { LocationIndex = Index; }
		int32 GetLocationIndex() const { return LocationIndex; }
	protected:
		int32 LocationIndex{-1};
		std::unique_ptr<IRHIResource> RHIResource;
	};

	class IRHIBuffer
	{
	public:
		IRHIBuffer() = delete;
		IRHIBuffer(uint32 _Size)
			:Size(_Size)
		{}
		virtual ~IRHIBuffer(){}
	protected:
		uint32 Size{0};
		std::unique_ptr<IRHIResource> RHIResource;
	};

	class IRHIIndexBuffer
	{
	public:
		IRHIIndexBuffer(EELEM_FORMAT InElemFormat, uint32 _Count, uint32 InSize):ElemFormat(InElemFormat),Count(_Count),Size(InSize){}
		virtual ~IRHIIndexBuffer(){}
		uint32 GetIndexCount() const { return Count; }
	protected:
		EELEM_FORMAT ElemFormat{ EELEM_FORMAT::UNKNOWN };
		uint32 Count{0};
		uint32 Size{0};
		std::shared_ptr<IRHIBuffer> RHIBuffer;
	};

	class IRHIVertexBuffer
	{
	public:
		IRHIVertexBuffer(uint32 InStride, uint32 InSize) :Stride(InStride), Size(InSize) {}
		virtual ~IRHIVertexBuffer(){}
	protected:
		uint32 Stride{0};
		uint32 Size{0};
		std::shared_ptr<IRHIBuffer> RHIBuffer;
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