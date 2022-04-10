#include "engine_pch.h"
#include "MaterialAsset.h"
#include "MaterialData.h"

namespace ks
{

	FMaterialAsset::FMaterialAsset(const FMaterialData& _MaterialData)
		:IAsset(_MaterialData.KeyName)
		,MaterialData(_MaterialData)
	{

	}

	FMaterialAsset::FMaterialAsset(FMaterialData&& TmpMaterialData)
		:IAsset(TmpMaterialData.KeyName)
		,MaterialData(std::move(TmpMaterialData))
	{

	}

}
