#pragma once

#include "RHIPipelineStateDesc.h"

#define RHICONSTBUFFER_V1 1
#define RHIVERTBUFFER_V1 1
#define RHIINDEXBUFFER_V1 1

namespace ks
{
	class FRenderer;

	struct FViewPort
	{
		uint32_t TopLeftX{ 0 };
		uint32_t TopLeftY{ 0 };
		uint32_t Width{ 0 };
		uint32_t Height{ 0 };
	};

	struct FRHIConfig
	{
		FViewPort ViewPort;
		EELEM_FORMAT BackBufferFormat;
		EELEM_FORMAT DepthBufferFormat;
		uint32_t ShadowMapSize{ 1024 };
	};

	struct FRenderPassDesc
	{
		std::string Name{};
		FViewPort ViewPort;
		FRenderer* Renderer{ nullptr };
		FRHIPipelineStateDesc PipelineStateDesc{};
	};

	struct FTexture2DDesc
	{
		int32_t Width;
		int32_t Height;
		EELEM_FORMAT Format{ EELEM_FORMAT::UNKNOWN };
	};

	struct FRenderTargetDesc
	{

	};
}

namespace ks
{
	class IRHIResource
	{
	public:
		virtual ~IRHIResource() {}
		virtual void* Map() = 0;
		virtual void Unmap() = 0;
		uint32 Size{ 0 };
	};

	class IRHIConstBuffer
	{
	public:
		IRHIConstBuffer() = default;
		virtual ~IRHIConstBuffer() = 0 {}
		virtual void UpdateData(const void* pData, uint32 Size) {
			void* pDest = RHIResource->Map();
			memcpy(pDest, pData, Size);
			RHIResource->Unmap();
		}
		void SetLocationIndex(int32 Index) { LocationIndex = Index; }
		int32 GetLocationIndex() const { return LocationIndex; }
	protected:
		int32 LocationIndex{ -1 };
		std::unique_ptr<IRHIResource> RHIResource;
	};

	class IRHIBuffer
	{
	public:
		IRHIBuffer() = delete;
		IRHIBuffer(uint32 _Size)
			:Size(_Size)
		{}
		virtual ~IRHIBuffer() {}
	protected:
		uint32 Size{ 0 };
		std::unique_ptr<IRHIResource> RHIResource;
	};

	class IRHIIndexBuffer
	{
	public:
		IRHIIndexBuffer(EELEM_FORMAT InElemFormat, uint32 _Count, uint32 InSize) :ElemFormat(InElemFormat), Count(_Count), Size(InSize) {}
		virtual ~IRHIIndexBuffer() {}
		uint32 GetIndexCount() const { return Count; }
	protected:
		EELEM_FORMAT ElemFormat{ EELEM_FORMAT::UNKNOWN };
		uint32 Count{ 0 };
		uint32 Size{ 0 };
		std::shared_ptr<IRHIBuffer> RHIBuffer;
	};

	class IRHIVertexBuffer
	{
	public:
		IRHIVertexBuffer(uint32 InStride, uint32 InSize) :Stride(InStride), Size(InSize) {}
		virtual ~IRHIVertexBuffer() {}
	protected:
		uint32 Stride{ 0 };
		uint32 Size{ 0 };
		std::shared_ptr<IRHIBuffer> RHIBuffer;
	};

	class IRHITexture2D
	{
	public:
		IRHITexture2D(const FTexture2DDesc& _Desc) :Desc(_Desc) {}
		virtual ~IRHITexture2D() = 0 {}
		const FTexture2DDesc& GetDesc() { return Desc; }
		void SetLocationIndex(int32_t _Index) { LocationIndex = _Index; }
		int32_t GetLocationIndex() const { return LocationIndex; }
	protected:
		FTexture2DDesc Desc;
		int32_t LocationIndex{-1};
	};

	/**************************************************************************************/
	/**************************************************************************************/
	/**************************************************************************************/
	class IRHIConstBuffer1
	{
	public:
		IRHIConstBuffer1() = default;
		virtual ~IRHIConstBuffer1() = 0 {}
		virtual void SetData(const void* Data, uint32 Size) = 0;
		void SetLocationIndex(int32 Index) { LocationIndex = Index; }
		int32 GetLocationIndex() const { return LocationIndex; }
	protected:
		int32 LocationIndex{ -1 };
	};

	class IRHIIndexBuffer1
	{
	public:
		IRHIIndexBuffer1(EELEM_FORMAT _ElemFormat, uint32 _Count, uint32 _Size) :ElemFormat(_ElemFormat), Count(_Count), Size(_Size) {}
		virtual ~IRHIIndexBuffer1() = 0 {}
		uint32 GetCount() const { return Count; }
	protected:
		EELEM_FORMAT ElemFormat{EELEM_FORMAT::UNKNOWN};
		uint32 Count{0};
		uint32 Size{0};
	};

	class IRHIVertexBuffer1
	{
	public:
		IRHIVertexBuffer1(uint32 InStride, uint32 InSize) :Stride(InStride), Size(InSize) {}
		virtual ~IRHIVertexBuffer1() {}
	protected:
		uint32 Stride{ 0 };
		uint32 Size{ 0 };
	};

	class IRHIDepthStencilBuffer
	{
	public:
		IRHIDepthStencilBuffer(const FTexture2DDesc& _Desc) :Desc(Desc) {}
		virtual ~IRHIDepthStencilBuffer() = 0 {}
		virtual IRHITexture2D* GetTexture2D() = 0;
	protected:
		FTexture2DDesc Desc;
	};

	class IRHIRenderTarget
	{
	public:
		IRHIRenderTarget(const FRenderTargetDesc& _Desc) :Desc(_Desc) {}
		virtual ~IRHIRenderTarget() = 0 {}
	protected:
		FRenderTargetDesc Desc;
	};
}