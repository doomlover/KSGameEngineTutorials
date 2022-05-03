#include "engine_pch.h"
#include "Core/Asset/AssetManager.h"
#include "Core/Asset/MeshAsset.h"
#include "Core/Asset/MaterialAsset.h"
#include "Core/Asset/Assets.h"

namespace ks
{
	FAssetManager* GAssetManager = nullptr;

	ks::FAssetManager* FAssetManager::Create()
	{
		return GAssetManager = new FAssetManager;
	}

	void FAssetManager::Init()
	{
		KS_INFO(TEXT("FAssetManager::Init"));
	}

	void FAssetManager::Shutdown()
	{
		KS_INFO(TEXT("\tFAssetManager::Shutdown"));
		/*for (auto& asset : Assets)
		{
			KS_INFOA(std::format("{}", asset.second.use_count()).c_str());
		}*/
		Assets.clear();
	}

	std::shared_ptr<FSceneAsset> FAssetManager::CreateSceneAsset(const std::string& InGLTFPath)
	{
		KS_INFO(TEXT("FAssetManager::Load GLTF Scene"));

		if (InGLTFPath.empty())
		{
			return nullptr;
		}
		assert(InGLTFPath.ends_with(".gltf"));
		assert(Assets.find(InGLTFPath) == Assets.end());
		std::shared_ptr<FSceneAsset> SceneAsset = std::make_shared<FSceneAsset>(InGLTFPath);
		Assets.insert({ InGLTFPath, SceneAsset });
		return SceneAsset;
	}

	std::shared_ptr<ks::FStaticMeshAsset> FAssetManager::CreateStaticMeshAsset(const FMeshData& MeshData)
	{
		std::shared_ptr<FStaticMeshAsset> Asset = std::make_shared<FStaticMeshAsset>(MeshData);
		Assets.insert({ Asset->GetPath(), Asset });
		return Asset;
	}

	std::shared_ptr<ks::FMaterialAsset> FAssetManager::CreateMaterialAsset(const FMaterialData& MaterialData)
	{
		std::shared_ptr<FMaterialAsset> Asset = std::make_shared<FMaterialAsset>(MaterialData);
		Assets.insert({Asset->GetPath(), Asset});
		return Asset;
	}

	FAssetManager::SharedAssetPtr FAssetManager::GetAsset(const std::string& Path)
	{
		if (Assets.contains(Path))
		{
			return Assets.at(Path);
		}
		return nullptr;
	}
}