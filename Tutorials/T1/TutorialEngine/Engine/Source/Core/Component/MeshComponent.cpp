#include "engine_pch.h"
#include "MeshComponent.h"
#include "Core/Asset/MeshAsset.h"
#include "Core/Scene.h"

namespace ks
{
	FStaticMeshComponent::FStaticMeshComponent(FSceneNode* _Owner, std::shared_ptr<FStaticMeshAsset> _StaticMeshAsset)
		:IComponent(_Owner)
		,StaticMeshAsset(_StaticMeshAsset)
	{
		Init();
	}
	void FStaticMeshComponent::SetStaticMesh(std::shared_ptr<FStaticMeshAsset> InAsset)
	{
		assert(InAsset);
		if (StaticMeshAsset != InAsset)
		{
			StaticMeshAsset = InAsset;
			Init();
		}
	}

	void FStaticMeshComponent::UpdateWorldTrans()
	{
		WorldTrans = Owner->GetWorldTrans();
	}

	void FStaticMeshComponent::UpdateBounds()
	{
		glm::vec3 WorldScale{ WorldTrans[0][0], WorldTrans[1][1], WorldTrans[2][2] };
		const FMeshData& MeshData = StaticMeshAsset->GetMeshData();
		glm::vec3 Min = MeshData.Min * WorldScale;
		glm::vec3 Max = MeshData.Max * WorldScale;
		Bounds = FBounds(Min, Max);
	}

	void FStaticMeshComponent::Init()
	{
		UpdateWorldTrans();
		UpdateBounds();
	}

}