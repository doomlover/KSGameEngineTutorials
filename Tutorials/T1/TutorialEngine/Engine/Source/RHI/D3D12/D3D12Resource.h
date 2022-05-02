#pragma once
#include "RHI/RHICommon.h"
#include "RHI/D3D12/D3D12Common.h"

namespace ks::d3d12
{
	class FD3D12Resource : public IRHIResource
	{
	public:
		FD3D12Resource(D3D12_HEAP_TYPE HeapType, uint32 _Size, D3D12_RESOURCE_STATES ResourceStates) {
			assert(_Size);
			Size = _Size;
			CD3DX12_HEAP_PROPERTIES HeapProp(HeapType);
			CD3DX12_RESOURCE_DESC ResDesc =
				CD3DX12_RESOURCE_DESC::Buffer(Size);
			KS_D3D12_CALL(GD3D12Device->CreateCommittedResource(
				&HeapProp,
				D3D12_HEAP_FLAG_NONE,
				&ResDesc,
				ResourceStates,
				nullptr,
				IID_PPV_ARGS(&D3D12Resource)
			));
		}
		virtual ~FD3D12Resource() {
			//D3D12Resource->Unmap(0, nullptr);
			D3D12Resource.Reset();
		}
		virtual void* Map() override
		{
			void* ppData = nullptr;
			D3D12Resource->Map(0, nullptr, &ppData);
			return ppData;
		}
		virtual void Unmap() override
		{
			D3D12Resource->Unmap(0, nullptr);
		}
		ID3D12Resource* GetID3D12Resource() {
			return D3D12Resource.Get();
		}
	private:
		ComPtr<ID3D12Resource> D3D12Resource;
	};

	class FD3D12Buffer : public IRHIBuffer
	{
		friend class FD3D12RHI;
	public:
		using IRHIBuffer::IRHIBuffer;
		virtual ~FD3D12Buffer() {}
		FD3D12Resource* GetD3D12Resource() {
			IRHIResource* pRHIResource = RHIResource.get();
			return dynamic_cast<FD3D12Resource*>(pRHIResource);
		}
	private:
		std::unique_ptr<FD3D12Resource> UploadResource;
	};


	class FD3D12ConstBuffer : public IRHIConstBuffer
	{
		friend FD3D12RHI;
	public:
		using IRHIConstBuffer::IRHIConstBuffer;
		virtual ~FD3D12ConstBuffer() {}
		const FDescriptorHandle& GetViewHandle() const { return ViewHandle; }
	private:
		FDescriptorHandle ViewHandle;
	};

	class FD3D12IndexBuffer : public IRHIIndexBuffer
	{
		friend FD3D12RHI;
	public:
		using IRHIIndexBuffer::IRHIIndexBuffer;
		virtual ~FD3D12IndexBuffer() {}
		const D3D12_INDEX_BUFFER_VIEW& GetIndexBufferView() const { return BufferView; }
	private:
		D3D12_INDEX_BUFFER_VIEW BufferView;
	};

	class FD3D12VertexBuffer : public IRHIVertexBuffer
	{
		friend FD3D12RHI;
	public:
		using IRHIVertexBuffer::IRHIVertexBuffer;
		virtual ~FD3D12VertexBuffer() {}
		const D3D12_VERTEX_BUFFER_VIEW& GetVertexBufferView() const { return BufferView; }
	private:
		D3D12_VERTEX_BUFFER_VIEW BufferView;
	};

	/**************************************************************************************/
	/**************************************************************************************/
	/**************************************************************************************/
	class FD3D12Resource1
	{
	public:
		FD3D12Resource1() = default;
		virtual ~FD3D12Resource1() = 0 {
			D3D12Resource->Release();
		}
		ID3D12Resource* GetResource() { return D3D12Resource.Get(); }
	protected:
		ComPtr<ID3D12Resource> D3D12Resource;
	};

	class FD3D12ConstBuffer1 : public IRHIConstBuffer1, public FD3D12Resource1
	{
		friend class FD3D12RHI;
	public:
		FD3D12ConstBuffer1(uint32_t _Size);
		virtual ~FD3D12ConstBuffer1();
		virtual void SetData(const void* Data, uint32_t Size) override;
		const FDescriptorHandle& GetViewHandle() const { return ViewHandle; }
		uint32_t GetAllocSize() const { return AllocSize; }
	private:
		void* MapData{ nullptr };
		uint32_t AllocSize{ 0 };
		FDescriptorHandle ViewHandle;
	};

	class FD3D12IndexBuffer1 : public IRHIIndexBuffer1, public FD3D12Resource1
	{
		friend class FD3D12RHI;
	public:
		FD3D12IndexBuffer1(EELEM_FORMAT _ElemFormat, uint32 _Count, uint32 _Size);
		virtual ~FD3D12IndexBuffer1() {}
		const D3D12_INDEX_BUFFER_VIEW& GetIndexBufferView() const { return BufferView; }
	private:
		D3D12_INDEX_BUFFER_VIEW BufferView;
	};

	class FD3D12VertexBuffer1 : public IRHIVertexBuffer1, public FD3D12Resource1
	{
		friend class FD3D12RHI;
	public:
		FD3D12VertexBuffer1(uint32_t Stride, uint32_t Size);
		virtual ~FD3D12VertexBuffer1() = default;
		const D3D12_VERTEX_BUFFER_VIEW& GetVertexBufferView() const { return BufferView; }
	private:
		D3D12_VERTEX_BUFFER_VIEW BufferView;
	};

	class FD3D12Texture2D : public IRHITexture2D, public FD3D12Resource1
	{
		friend class FD3D12RHI;
	public:
		FD3D12Texture2D(const FTexture2DDesc& _Desc) :IRHITexture2D(_Desc) {}
		virtual ~FD3D12Texture2D() = default;
		int32_t GetLocationIndex() const { return LocaltionIndex; }
		void SetLocationIndex(int32_t _LocationIndex) { LocaltionIndex = _LocationIndex; }
	private:
		int32_t LocaltionIndex{ -1 };
		FDescriptorHandle ViewHandle;
	};
}