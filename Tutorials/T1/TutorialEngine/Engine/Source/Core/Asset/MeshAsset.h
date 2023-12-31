#pragma once

#include "Core/CoreMinimal.h"
#include "Core/Asset/Assets.h"
#include "Render/MeshRenderData.h"
#include "Core/Asset/MaterialData.h"

namespace ks {

	struct FMeshAttributeData
	{
		uint32 Count{0};
		uint32 Stride{0};
		EDATA_TYPE DataType{EDATA_TYPE::INVALID};
		std::vector<uint8> Data;
		FMeshAttributeData() = default;
		FMeshAttributeData(const FMeshAttributeData& Other) {
			*this = Other;
		}
		FMeshAttributeData& operator=(const FMeshAttributeData& Other) {
			Count = Other.Count;
			Stride = Other.Stride;
			DataType = Other.DataType;
			Data = Other.Data;
			return *this;
		}
		FMeshAttributeData(FMeshAttributeData&& Tmp) noexcept {
			*this = std::move(Tmp);
		}
		FMeshAttributeData& operator=(FMeshAttributeData&& Tmp) noexcept {
			Count = Tmp.Count;
			Stride = Tmp.Stride;
			DataType = Tmp.DataType;
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
		FMeshAttributeData IndexData;
		// positions
		FMeshAttributeData PositionData;
		// other attributes include normal, texcoord, etc
		FMeshAttributeData AttributeData;
		// material, TODO : is redundant when material asset is created
		FMaterialData MaterialData;
		// bounds
		glm::vec3 Min{0.f};
		glm::vec3 Max{0.f};

		FMeshData() = default;

		FMeshData(const FMeshData& Other) {
			*this = Other;
		}
		FMeshData& operator=(const FMeshData& Other) {
			KeyName = Other.KeyName;
			IndexDataType = Other.IndexDataType;
			IndexRawData = Other.IndexRawData;
			IndexData = Other.IndexData;
			PositionData = Other.PositionData;
			AttributeData = Other.AttributeData;
			MaterialData = Other.MaterialData;
			Min = Other.Min;
			Max = Other.Max;
			return *this;
		}
		FMeshData(FMeshData&& TempMeshData) noexcept {
			*this = std::move(TempMeshData);
		}
		FMeshData& operator=(FMeshData&& Tmp) noexcept {
			KeyName = std::move(Tmp.KeyName);
			IndexDataType = Tmp.IndexDataType;
			IndexRawData = std::move(Tmp.IndexRawData);
			IndexData = std::move(Tmp.IndexData);
			PositionData = std::move(Tmp.PositionData);
			AttributeData = std::move(Tmp.AttributeData);
			MaterialData = std::move(Tmp.MaterialData);
			Min = std::move(Tmp.Min);
			Max = std::move(Tmp.Max);
			return *this;
		}
	};

	class FStaticMeshAsset : public IAsset
	{
	public:
		FStaticMeshAsset(const FMeshData& _MeshData);
		FStaticMeshAsset(FMeshData&& _MeshData);
		virtual ~FStaticMeshAsset() {}
		virtual void PostLoad() override;
		FMeshRenderData* GetRenderData() { return RenderData.get(); }
		class FMaterialAsset* GetMaterialAsset() { return MaterialAsset; }
		const FMeshData& GetMeshData() { return MeshData; }
	private:
		void InitRenderData();
		void CreateMaterialAssetInter();
		// raw data
		FMeshData MeshData;
		// rendering, render data
		std::unique_ptr<FMeshRenderData> RenderData;
		// material
		FMaterialAsset* MaterialAsset{nullptr};
	};

}