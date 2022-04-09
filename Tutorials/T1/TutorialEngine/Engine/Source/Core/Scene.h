#pragma once

#include "Core/Math.h"

namespace ks
{
	class FSceneAsset;
	class FStaticMeshAsset;
	class FCameraComponent;
	class FRenderScene;
	struct FStaticMeshComponent;

	enum class ECameraType : unsigned short
	{
		PERSPECTIVE,
		ORTHOGRAPHIC,
		INVALID,
	};

	struct FPerspectiveCamera
	{
		float YFov;
		float ZFar;
		float ZNear;
	};

	struct FOrthographicCamera
	{
		float XMag;
		float YMag;
		float ZFar;
		float ZNear;
	};

	struct FCameraInfo
	{
		ECameraType Type{ ECameraType::INVALID };
		union FCameraData
		{
			FOrthographicCamera OrthCamera;
			FPerspectiveCamera PersCamera;
		};
		FCameraData CameraData{};
	};

	struct FSceneNodeInfo
	{
		int32 id;
		std::vector<int32> ChildrenIds;
		std::string Name;
		glm::vec3 Translate;
		glm::quat Rotation;
		glm::vec3 Scale;
		std::string MeshAssetKeyName;
		FCameraInfo Camera;
	};

	struct FSceneNode
	{
		FSceneNode() = delete;
		FSceneNode(const FSceneNodeInfo& NodeInfo);
		FStaticMeshComponent* GetMeshComponent() {
			return StaticMeshComponent.get();
		}
		void AddChild(FSceneNode* Child) {
			assert(Child);
			assert(!Child->Parent);
			Child->Parent = this;
			Children.push_back(Child);
		}
		glm::mat4 GetWorldTrans() const {

			glm::mat4 ParentWorldTrans = Parent
				? Parent->GetWorldTrans()
				: glm::mat4(1.0);
			return ParentWorldTrans * GetLocalTrans();
		}
		glm::mat4 GetLocalTrans() const {
			glm::mat4 LocalTrans(1.0);
			LocalTrans = glm::scale(LocalTrans, Scale);
			LocalTrans = glm::mat4_cast(Rotation) * LocalTrans;
			//LocalTrans = glm::translate(LocalTrans, Translate);
			LocalTrans[3] = glm::vec4(Translate, 1.0f);
			return LocalTrans;
			
		}
		std::string Name;
		// transform
		glm::vec3 Translate;
		glm::quat Rotation;
		glm::vec3 Scale;
		// mesh component
		std::unique_ptr<FStaticMeshComponent> StaticMeshComponent;
		// camera component
		std::unique_ptr<FCameraComponent> CameraComponent;
		// tree info
		FSceneNode* Parent{ nullptr };
		std::vector<FSceneNode*> Children;
	};

	struct FStaticMeshComponent
	{
		void SetStaticMesh(std::shared_ptr<FStaticMeshAsset> InAsset)
		{ 
			assert(InAsset);
			StaticMeshAsset = InAsset;
		}
		FStaticMeshAsset* GetStaticMesh() { return StaticMeshAsset.get(); }
		glm::mat4 GetWorldTrans() const { return ParentNode->GetWorldTrans(); }
		std::shared_ptr<FStaticMeshAsset> StaticMeshAsset;
		FSceneNode* ParentNode{nullptr};
	};

	class FCameraComponent
	{
	public:
		FCameraComponent() = delete;
		FCameraComponent(const FCameraInfo& _CameraInfo) : CameraInfo(_CameraInfo) {}
		glm::mat4 GetProjectionTrans() const;
	private:
		FCameraInfo CameraInfo;
	};

	class FScene
	{
	public:
		FScene() = default;
		FScene(std::shared_ptr<FSceneAsset> SceneAsset);
		FRenderScene* GetRenderScene() { return RenderScene; }
		void Update(){}
		glm::mat4 GetViewTrans() {
			glm::mat4 Eye2World{ Camera->GetWorldTrans() };
			glm::mat4 World2Eye{ glm::affineInverse(Eye2World) };
			return World2Eye;
		}
		glm::mat4 GetProjectionTrans() {
			return Camera->CameraComponent->GetProjectionTrans();
		}
	private:
		FSceneNode* Camera{nullptr};
		// ref the scene asset
		std::shared_ptr<FSceneAsset> SceneAsset;
		// contain all the scene nodes
		std::vector<std::unique_ptr<FSceneNode>> SceneNodes;
		// rendering
		FRenderScene* RenderScene{nullptr};
	};
}