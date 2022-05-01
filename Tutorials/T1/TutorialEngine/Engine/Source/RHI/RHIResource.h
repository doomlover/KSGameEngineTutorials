#pragma once

#define RHICONSTBUFFER_V1 1
#define RHIVERTBUFFER_V1 1
#define RHIINDEXBUFFER_V1 1

namespace ks
{
	/*enum class ERHIResourceType
	{
		BUFFER = 0,
		TEX2D = 1,
		TEX3D = 2,
		UNKNOWN,
	};

	struct FRHIResourceDesc
	{
		ERHIResourceType ResourceType{ERHIResourceType::UNKNOWN};
		uint64_t Alignment{0};
		uint64_t Width;
		uint32_t Height;
		uint16_t DepthOrArraySize{1};
		uint16_t MipLevels{0};
		EELEM_FORMAT Format{EELEM_FORMAT::UNKNOWN};

		static inline FRHIResourceDesc Buffer(
			uint64_t _Width,
			uint64_t _Alignment = 0)
		{
			return FRHIResourceDesc(
				ERHIResourceType::BUFFER, _Alignment, _Width,
				1, 1, 1, EELEM_FORMAT::UNKNOWN);
		}
		static inline FRHIResourceDesc Tex2D(
			uint64_t _Width,
			uint32_t _Height,
			EELEM_FORMAT _Format,
			uint16_t _ArraySize = 1,
			uint16_t _MipLevels = 0,
			uint64_t _Alignment = 0)
		{
			return FRHIResourceDesc(
				ERHIResourceType::TEX2D, _Alignment, _Width, _Height,
				_ArraySize, _MipLevels, _Format);
		}
	};*/

	/*class IRHIResource
	{
	public:
		IRHIResource(const FRHIResourceDesc& _Desc) :Desc(_Desc) {}
		virtual ~IRHIResource() {}
		virtual void* Map() = 0;
		virtual void Unmap() = 0;
		const FRHIResourceDesc& GetDesc() const { return Desc; }
	protected:
		FRHIResourceDesc Desc;
	};*/

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
		std::shared_ptr<IRHIResource> RHIResource;
	};
}