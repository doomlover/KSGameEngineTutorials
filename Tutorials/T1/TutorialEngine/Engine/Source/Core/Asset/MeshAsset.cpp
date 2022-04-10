#include "engine_pch.h"
#include "Core/Asset/MeshAsset.h"
#include "Core/Asset/AssetManager.h"
#include "Core/Asset/MaterialAsset.h"

namespace ks
{
	FStaticMeshAsset::FStaticMeshAsset(const FMeshData& _MeshData)
		:IAsset(_MeshData.KeyName)
		,MeshData(_MeshData)
	{
		CreateMaterialAssetInter();
	}

	FStaticMeshAsset::FStaticMeshAsset(FMeshData&& _MeshData)
		:IAsset(_MeshData.KeyName)
		,MeshData(std::move(_MeshData))
	{
		CreateMaterialAssetInter();
	}

	void FStaticMeshAsset::PostLoad()
	{
		InitRenderData();
	}

	void FStaticMeshAsset::InitRenderData()
	{
		RenderData.reset(new FMeshRenderData(MeshData));
	}

	void FStaticMeshAsset::CreateMaterialAssetInter()
	{
		FAssetManager::SharedAssetPtr Asset = GAssetManager->GetAsset(MeshData.MaterialData.KeyName);
		std::shared_ptr<FMaterialAsset> _MaterialAsset;
		if (!Asset)
		{
			_MaterialAsset = GAssetManager->CreateMaterialAsset(MeshData.MaterialData);
		}
		else
		{
			_MaterialAsset = std::dynamic_pointer_cast<FMaterialAsset>(Asset);
		}
		MaterialAsset = _MaterialAsset.get();
		RefAssets.push_back(_MaterialAsset);
	}

}