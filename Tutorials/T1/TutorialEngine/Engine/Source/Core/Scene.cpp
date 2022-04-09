#include "engine_pch.h"
#include "Core/Scene.h"
#include "Core/Assets.h"
#include "Core/MeshAsset.h"
#include "Render/Render.h"
#include "Core/Application.h"

namespace ks
{
	FScene::FScene(std::shared_ptr<FSceneAsset> _SceneAsset)
		: SceneAsset(_SceneAsset)
		, Camera(nullptr)
		, RenderScene(nullptr)
	{
		// create scene nodes
		std::map<int32, FSceneNode*> SceneNodeMap;
		std::vector<FSceneNodeInfo> SceneNodeInfos = SceneAsset->GetSceneNodeInfos();
		for (auto& SceneNodeInfo : SceneNodeInfos)
		{
			SceneNodes.push_back(std::make_unique<FSceneNode>(SceneNodeInfo));
			auto& SceneNode{SceneNodes.back()};
			// get camera node
			if (SceneNode->CameraComponent)
			{
				Camera = SceneNode.get();
			}
			// get directional light node
			else if (SceneNode->LightComponent)
			{
				auto& LightComponent{SceneNode->LightComponent};
				ELightType LightType{LightComponent->GetType()};
				switch (LightType)
				{
				case ELightType::DIRECTIONAL:
					DirectionalLight = SceneNode.get();
					break;
				default:
					assert(false);
					break;
				}
			}
			// insert to map
			SceneNodeMap.insert({SceneNodeInfo.id, SceneNode.get()});
		}
		// setup parent and children links
		for (auto& SceneNodeInfo : SceneNodeInfos)
		{
			FSceneNode*& pParent = SceneNodeMap.at(SceneNodeInfo.id);
			for (auto& ChildId : SceneNodeInfo.ChildrenIds)
			{
				FSceneNode*& pChild = SceneNodeMap.at(ChildId);
				pParent->AddChild(pChild);
			}
		}
		
		// rendering, create render scene
		RenderScene = FRenderer::CreateRenderScene(this);

		// rendering, add mesh components to render scene
		for (int32 i = 0; i < SceneNodes.size(); ++i)
		{
			auto& SceneNode = SceneNodes.at(i);
			FStaticMeshComponent* MeshComponent = SceneNode->GetMeshComponent();
			if (MeshComponent) { RenderScene->AddPrimitive(MeshComponent); }
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
			StaticMeshComponent->ParentNode = this;
			std::shared_ptr<IAsset> Asset = GAssetManager->GetAsset(NodeInfo.MeshAssetKeyName);
			StaticMeshComponent->SetStaticMesh(std::dynamic_pointer_cast<FStaticMeshAsset>(Asset));
		}
		if (NodeInfo.Camera.Type != ECameraType::INVALID)
		{
			CameraComponent = std::make_unique<FCameraComponent>(NodeInfo.Camera);
		}
		if (NodeInfo.Light.Type != ELightType::INVALID)
		{
			LightComponent = std::make_unique<FLightComponent>(NodeInfo.Light, this);
		}
	}

	glm::mat4 FCameraComponent::GetProjectionTrans() const
	{
		switch (CameraInfo.Type)
		{
		case ECameraType::PERSPECTIVE:
			return glm::perspective(
				glm::degrees(CameraInfo.CameraData.PersCamera.YFov), 
				GApp->GetWindowAspect(), 
				CameraInfo.CameraData.PersCamera.ZNear,
				CameraInfo.CameraData.PersCamera.ZFar);
			break;
		}
		return glm::mat4(1.0);
	}

}