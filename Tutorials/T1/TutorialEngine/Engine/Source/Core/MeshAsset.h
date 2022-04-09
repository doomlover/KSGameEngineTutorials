#pragma once

#include "Core/CoreMinimal.h"
#include "Core/Assets.h"
#include "Render/MeshRenderData.h"

namespace ks {

	struct FMeshAttributeData
	{
		uint32 Count{0};
		uint32 Stride{0};
		std::vector<uint8> Data;
		FMeshAttributeData() = default;
		FMeshAttributeData(FMeshAttributeData&& Tmp) noexcept {
			*this = std::move(Tmp);
		}
		FMeshAttributeData& operator=(FMeshAttributeData&& Tmp) noexcept {
			Count = Tmp.Count;
			Stride = Tmp.Stride;
			Data = std::move(Tmp.Data);
			return *this;
		}
	};

	struct FMeshData
	{
		// root/name
		std::string KeyName;
		// indices
		EDATA_TYPE IndexDataType{EDATA_TYPE::INVALID};
		std::vector<uint8> IndexRawData;
		// positions
		FMeshAttributeData PositionData;
		// other attributes include normal, texcoord, etc
		FMeshAttributeData AttributeData;

		FMeshData() = default;
		FMeshData(FMeshData&& TempMeshData) noexcept {
			*this = std::move(TempMeshData);
		}
		FMeshData& operator=(FMeshData&& Tmp) noexcept {
			KeyName = std::move(Tmp.KeyName);
			IndexDataType = Tmp.IndexDataType;
			IndexRawData = std::move(Tmp.IndexRawData);
			PositionData = std::move(Tmp.PositionData);
			AttributeData = std::move(Tmp.AttributeData);
			return *this;
		}
	};

	class FStaticMeshAsset : public IAsset
	{
	public:
		FStaticMeshAsset(const std::string& Path, FMeshData& InMeshData) :IAsset(Path), MeshData(std::move(InMeshData)) {}
		virtual ~FStaticMeshAsset() {}
		virtual void PostLoad() override;
		FMeshRenderData* GetRenderData() { return RenderData.get(); }

	private:
		void InitRenderData();

		// raw data
		FMeshData MeshData;
		// rendering, render data
		std::unique_ptr<FMeshRenderData> RenderData;
	};

}