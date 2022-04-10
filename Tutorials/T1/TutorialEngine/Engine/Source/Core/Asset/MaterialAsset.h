#pragma once
#include "Core/Asset/Assets.h"
#include "Core/Asset/MaterialData.h"

namespace ks
{
	class FMaterialAsset : public IAsset
	{
	public:
		FMaterialAsset(const FMaterialData& MaterialData);
		FMaterialAsset(FMaterialData&& TmpMaterialData);
		virtual ~FMaterialAsset(){}
		const FMaterialData& GetMaterialData() const { return MaterialData; }
	private:
		FMaterialData MaterialData;
	};
}

