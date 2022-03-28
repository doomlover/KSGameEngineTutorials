#include "engine_pch.h"
#include "Core/Scene.h"
#include "Core/Assets.h"
#include "Core/MeshAsset.h"

namespace ks
{
	FScene::FScene(std::shared_ptr<FSceneAsset> _SceneAsset)
		:SceneAsset(_SceneAsset)
	{
		// create all the scene nodes
		int32 NumNodes = SceneAsset->NumNodes();
		SceneNodes.reserve(NumNodes);
		for (int32 i = 0; i < NumNodes; ++i)
		{
			// only deal with the node containing mesh
			FSceneNodeInfo NodeInfo;
			SceneAsset->GetSceneNodeInfo(i, NodeInfo);
			SceneNodes.push_back(std::make_unique<FSceneNode>(NodeInfo));
		}
	}

	FSceneNode::FSceneNode(const FSceneNodeInfo& NodeInfo)
		:Name(NodeInfo.Name)
		,Translate(NodeInfo.Translate)
		,Rotation(NodeInfo.Rotation)
		,Scale(NodeInfo.Scale)
	{
		if (!NodeInfo.MeshAssetKeyName.empty())
		{
			StaticMeshComponent = std::make_unique<FStaticMeshComponent>();
			std::shared_ptr<IAsset> Asset = GAssetManager->GetAsset(NodeInfo.MeshAssetKeyName);
			StaticMeshComponent->SetStaticMesh(std::dynamic_pointer_cast<FStaticMeshAsset>(Asset));
		}
	}

}