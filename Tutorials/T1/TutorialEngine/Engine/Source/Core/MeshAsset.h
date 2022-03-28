#pragma once

#include "Core/Assets.h"

namespace ks {
	/* enum version of gltf accessor.componentType */
	enum class EDATA_TYPE : unsigned short
	{
		BYTE = 5120,
		UNSIGNED_BYTE = 5121,
		SHORT = 5122,
		UNSIGNED_SHORT = 5123,
		UNSIGNED_INT = 5125,
		FLOAT = 5126,
		INVALID,
	};
	/* enum version of gltf accessor.type */
	enum class EELEM_TYPE : unsigned short
	{
		SCALAR,
		VEC2,
		VEC3,
		VEC4,
		MAT2,
		MAT3,
		MAT4,
		INVALID,
	};

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
			KeyName = std::move(TempMeshData.KeyName);
			IndexDataType = TempMeshData.IndexDataType;
			IndexRawData = std::move(TempMeshData.IndexRawData);
			PositionElemType = TempMeshData.PositionElemType;
			PositionDataType = TempMeshData.PositionDataType;
			PositionRawData = std::move(TempMeshData.PositionRawData);
		}
	};

	class FStaticMeshAsset : public IAsset
	{
	public:
		FStaticMeshAsset(const std::string& Path, FMeshData& InMeshData) :IAsset(Path), MeshData(std::move(InMeshData)) {}
		virtual ~FStaticMeshAsset() {}

	private:
		FMeshData MeshData;
	};
}