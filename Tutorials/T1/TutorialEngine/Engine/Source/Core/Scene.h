#pragma once

#include "Core/Math.h"

namespace ks
{
	class FSceneAsset;
	class FStaticMeshAsset;
	struct FStaticMeshComponent;

	struct FSceneNodeInfo
	{
		std::string Name;
		glm::vec3 Translate;
		glm::quat Rotation;
		glm::vec3 Scale;
		std::string MeshAssetKeyName;
	};

	struct FSceneNode
	{
		FSceneNode() = delete;
		FSceneNode(const FSceneNodeInfo& NodeInfo);

		std::string Name;

		// transform
		glm::vec3 Translate;
		glm::quat Rotation;
		glm::vec3 Scale;

		// mesh component
		std::unique_ptr<FStaticMeshComponent> StaticMeshComponent;
	};

	struct FStaticMeshComponent
	{
		void SetStaticMesh(std::shared_ptr<FStaticMeshAsset> InAsset)
		{ 
			assert(InAsset);
			StaticMeshAsset = InAsset;
		}
		std::shared_ptr<FStaticMeshAsset> StaticMeshAsset;
	};

	class FScene
	{
	public:
		FScene() = default;
		FScene(std::shared_ptr<FSceneAsset> SceneAsset);
		// ref the scene asset
		std::shared_ptr<FSceneAsset> SceneAsset;
		// contain all the scene nodes
		std::vector<std::unique_ptr<FSceneNode>> SceneNodes;
	};
}