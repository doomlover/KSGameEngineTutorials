#pragma once

#include "Core/Math.h"
#include "RHI/RHI.h"
#include "Core/Bounds.h"

namespace ks
{

	class FScene;
	class FMeshRenderData;
	class FStaticMeshComponent;

	struct FViewConstBufferParameter
	{
		glm::mat4 ViewProjTrans{1.f};
		glm::mat4 LightProj{1.f};
		glm::mat4 LightProjTex{1.f};
		glm::vec4 D_LightDirectionAndInstensity{0.f};
		glm::vec3 EyePos{0.f};
	};

	struct FPrimitiveConstBufferParameter
	{
		glm::mat4 WorldTrans{1.f};
		glm::mat4 InvTranspWorldTrans{1.f};
		glm::vec4 BaseColorFactor{1.f};
		glm::vec4 RoughnessMetallicFactor{1.f};
	};

	class FRenderPrimitive
	{
	public:
		using ConstBufferType = TConstBuffer<FPrimitiveConstBufferParameter>;
		using ConstBufferPtrType = ConstBufferType::PtrType;
		FRenderPrimitive(FStaticMeshComponent* MeshComponent);
		~FRenderPrimitive() {}
		ConstBufferPtrType GetPrimitiveConstBuffer() { return PrimitiveConstBuffer.get(); }
		IRHIConstBuffer1* GetConstBuffer() { return PrimitiveConstBuffer1.get(); }
		const FMeshRenderData* GetRenderData() const { return RenderData; }
	private:
		// reference FStaticMeshAsset::RenderData
		FMeshRenderData* RenderData{nullptr};
		// primitive constant buffer
		std::shared_ptr<ConstBufferType> PrimitiveConstBuffer;
		std::shared_ptr<IRHIConstBuffer1> PrimitiveConstBuffer1;
		// bounds
		FBounds Bounds{};
	};
	/**********************************************************************/

	class FRenderScene
	{
		friend class FRenderer;
		friend class FRenderPass;
	public:
		using PrimPtr = std::unique_ptr<FRenderPrimitive>;
		FRenderScene(FScene* InScene);
		~FRenderScene() {}
		void AddPrimitive(FStaticMeshComponent* MeshComponent);
		void Update();
		IRHIConstBuffer1* GetBasePassConstBuffer() { return BasePassConstBuffer1.get(); }
		const std::vector<PrimPtr>& GetPrimitives() { return Primitives; }
	private:
		FScene* Scene{nullptr};
		std::vector<PrimPtr> Primitives;
		// base pass const buffer
		std::shared_ptr<TConstBuffer<FViewConstBufferParameter>> BasePassConstBuffer;
		std::shared_ptr<IRHIConstBuffer1> BasePassConstBuffer1;
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
		FRenderScene* GetScene() { return RenderScene; }

	private:
		void Init();
		void RenderBasePass();
		void RenderShadowPass();
		void RenderPostProcessPass();
		// referenced render scene by this instance
		FRenderScene* RenderScene{nullptr};
		// global allocated render scenes management
		static std::unordered_map<const FRenderScene*, std::unique_ptr<FRenderScene>> RenderScenes;
	};
}