#pragma once

#include "Core/Math.h"
#include "Component/LightComponent.h"
#include "Component/MeshComponent.h"
#include "Core/Bounds.h"

namespace ks
{
	class FSceneAsset;
	class FCameraComponent;
	class FRenderScene;
	class FStaticMeshComponent;

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
		FLightInfo Light;
	};

	struct FSceneNode
	{
		FSceneNode() = delete;
		FSceneNode(const FSceneNodeInfo& NodeInfo);
		FStaticMeshComponent* GetMeshComponent() {
			return StaticMeshComponent.get();
		}
		void AddChild(FSceneNode* Child) {
			assert(Child && !Child->Parent);
			Child->Parent = this;
			Children.push_back(Child);
		}
		glm::mat4 GetWorldTrans() const {
			glm::mat4 ParentWorldTrans = Parent
				? Parent->GetWorldTrans()
				: glm::mat4{1.0f};
			return ParentWorldTrans * GetLocalTrans();
		}
		glm::mat4 GetLocalTrans() const {
			glm::mat4 LocalTrans{1.0f};
			LocalTrans = glm::scale(LocalTrans, Scale);
			LocalTrans = glm::mat4_cast(Rotation) * LocalTrans;
			LocalTrans[3] = glm::vec4(Translate, 1.0f);
			/*
			When using glm::translate( X, vec3 ), you are multiplying
            X * glm::translate( Identity, vec3 )
			This means translate first, then X
			so if we want scale first, then rot, then trans, we need the following
			glm::mat4 lt = glm::translate(glm::mat4{ 1.0f }, Translate);
			lt = lt * glm::mat4_cast(Rotation);
			lt = glm::scale(lt, Scale);
			*/
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
		// light component
		std::unique_ptr<FLightComponent> LightComponent;
		// tree info
		FSceneNode* Parent{ nullptr };
		std::vector<FSceneNode*> Children;
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
		FSceneNode* GetCamera() { return Camera; }
		glm::mat4 GetViewTrans() {
			glm::mat4 Eye2World{ Camera->GetWorldTrans() };
			glm::mat4 World2Eye{ glm::affineInverse(Eye2World) };
			return World2Eye;
		}
		glm::mat4 GetProjectionTrans() {
			return Camera->CameraComponent->GetProjectionTrans();
		}
		void GetDirectionalLight(glm::vec3& Direction, float& Intensity) {
			Direction = DirectionalLight ? DirectionalLight->LightComponent->GetDirection() : glm::vec3(0, 1, 0);
			Intensity = DirectionalLight ? DirectionalLight->LightComponent->GetIntensity() : 0.f;
		}
		FSceneNode* GetLightNode() { return DirectionalLight; }
		const FBounds& GetSceneBounds() const { return SceneBounds; }
	private:
		FSceneNode* Camera{nullptr};
		FSceneNode* DirectionalLight{nullptr};
		// ref the scene asset
		std::shared_ptr<FSceneAsset> SceneAsset;
		// contain all the scene nodes
		std::vector<std::unique_ptr<FSceneNode>> SceneNodes;
		// rendering
		FRenderScene* RenderScene{nullptr};
		// scene bounds
		FBounds SceneBounds;
	};
}