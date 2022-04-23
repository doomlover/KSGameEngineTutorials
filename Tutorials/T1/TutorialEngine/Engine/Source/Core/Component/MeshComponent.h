#pragma once

#include "Core/CoreMinimal.h"
#include "Core/Component/Component.h"
#include "Core/Bounds.h"

namespace ks
{
	class FStaticMeshAsset;
	
	class FStaticMeshComponent : public IComponent
	{
	public:
		FStaticMeshComponent(FSceneNode* _Owner, std::shared_ptr<FStaticMeshAsset> StaticMeshAsset);
		virtual ~FStaticMeshComponent() = default;
		void SetStaticMesh(std::shared_ptr<FStaticMeshAsset> InAsset);
		FStaticMeshAsset* GetStaticMesh() { return StaticMeshAsset.get(); }
		const glm::mat4& GetWorldTrans() const { return WorldTrans; }
		const FBounds& GetBounds() const { return Bounds; }
		void UpdateWorldTrans();
		void UpdateBounds();
	protected:
		FBounds Bounds;
		glm::mat4 WorldTrans{ 1.f };
		std::shared_ptr<FStaticMeshAsset> StaticMeshAsset{nullptr};
		void Init();
	};
}