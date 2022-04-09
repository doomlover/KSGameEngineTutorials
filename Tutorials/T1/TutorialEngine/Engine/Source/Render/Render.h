#pragma once

#include "Core/Math.h"
#include "RHI/RHI.h"

namespace ks
{

	class FScene;
	class FMeshRenderData;
	struct FStaticMeshComponent;

	struct FViewConstBufferParameter
	{
		glm::mat4 ViewProjectionTrans;
		glm::vec3 DirectionalLight;
		float DirectionalLightIntensity{0.f};
	};

	struct FPrimitiveConstBufferParameter
	{
		glm::mat4 WorldTrans;
		glm::mat4 InvTWorldTrans;
	};

	class FRenderPrimitive
	{
	public:
		using ConstBufferType = TConstBuffer<FPrimitiveConstBufferParameter>;
		using ConstBufferPtrType = ConstBufferType::PtrType;
		FRenderPrimitive(FStaticMeshComponent* MeshComponent);
		ConstBufferPtrType GetPrimitiveConstBuffer() { return PrimitiveConstBuffer.get(); }
		const FMeshRenderData* GetRenderData() const { return RenderData; }
	private:
		// reference FStaticMeshAsset::RenderData
		FMeshRenderData* RenderData;
		// primitive constant buffer
		std::shared_ptr<ConstBufferType> PrimitiveConstBuffer;
	};
	/**********************************************************************/

	class FRenderScene
	{
		friend class FRenderer;
		friend class FRenderPass;
	public:
		FRenderScene(FScene* InScene);
		~FRenderScene() {}
		void AddPrimitive(FStaticMeshComponent* MeshComponent);
		void Update();
	private:
		FScene* Scene{nullptr};
		std::vector<std::unique_ptr<FRenderPrimitive>> Primitives;
		// base pass const buffer
		std::shared_ptr<TConstBuffer<FViewConstBufferParameter>> BasePassConstBuffer;
	};
	/**********************************************************************/

	class FRenderer
	{
		friend class FRenderPass;
	public:
		static FRenderScene* CreateRenderScene(FScene* Scene);
		static FRenderer* Create() { return new FRenderer; }
		FRenderer();
		void Render();
		void Shutdown();
		void SetScene(FRenderScene* _RenderScene) { RenderScene = _RenderScene; }

	private:
		void Init();
		void RenderBasePass();
		// referenced render scene by this instance
		FRenderScene* RenderScene{nullptr};
		// global allocated render scenes management
		static std::unordered_map<const FRenderScene*, std::unique_ptr<FRenderScene>> RenderScenes;
	};
}