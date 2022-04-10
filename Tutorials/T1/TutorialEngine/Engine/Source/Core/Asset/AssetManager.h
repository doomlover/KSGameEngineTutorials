#pragma once

namespace ks
{
	struct FMeshData;
	struct FMaterialData;
	class IAsset;
	class FSceneAsset;
	class FStaticMeshAsset;
	class FMaterialAsset;

	class FAssetManager
	{
	public:
		using SharedAssetPtr = std::shared_ptr<IAsset>;

		static FAssetManager* Create();
		~FAssetManager() {
			KS_INFO(TEXT("~FAssetManager"));
		}
		void Init();
		void Shutdown();
		// Create scene asset from gltf file
		std::shared_ptr<FSceneAsset> CreateSceneAsset(const std::string& GLTFScenePath);
		// Create static mesh asset
		std::shared_ptr<FStaticMeshAsset> CreateStaticMeshAsset(const FMeshData& MeshData);
		// Create material asset
		std::shared_ptr<FMaterialAsset> CreateMaterialAsset(const FMaterialData& MaterialData);
		// Get asset shared ptr form path
		SharedAssetPtr GetAsset(const std::string& Path);

	private:
		std::unordered_map<std::string, SharedAssetPtr> Assets;
		//using FSizeType = std::unordered_map<std::string, SharedAssetPtr>::size_type;
	};

	extern FAssetManager* GAssetManager;
}