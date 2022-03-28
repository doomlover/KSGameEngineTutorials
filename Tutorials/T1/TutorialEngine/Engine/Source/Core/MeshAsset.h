#pragma once

#include "Core/CoreMinimal.h"
#include "Core/Assets.h"
#include "Render/MeshRenderData.h"

namespace ks {

	struct FMeshData
	{
		// root/name
		std::string KeyName;
		// indices
		EDATA_TYPE IndexDataType = EDATA_TYPE::INVALID;
		std::vector<uint8> IndexRawData;
		// positions
		EELEM_TYPE PositionElemType = EELEM_TYPE::INVALID;
		EDATA_TYPE PositionDataType = EDATA_TYPE::INVALID;
		std::vector<uint8> PositionRawData;

		FMeshData() = default;
		FMeshData(FMeshData&& TempMeshData) noexcept
		{
			*this = std::move(TempMeshData);
		}
		FMeshData& operator=(FMeshData&& TempMeshData) noexcept
		{
			KeyName = std::move(TempMeshData.KeyName);
			IndexDataType = TempMeshData.IndexDataType;
			IndexRawData = std::move(TempMeshData.IndexRawData);
			PositionElemType = TempMeshData.PositionElemType;
			PositionDataType = TempMeshData.PositionDataType;
			PositionRawData = std::move(TempMeshData.PositionRawData);
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