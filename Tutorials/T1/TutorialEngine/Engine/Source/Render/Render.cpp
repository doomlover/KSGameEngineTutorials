#include "engine_pch.h"
#include "Render/Render.h"
#include "Render/MeshRenderData.h"
#include "Render/RenderPass.h"
#include "Core/Scene.h"
#include "Core/Asset/MeshAsset.h"
#include "Core/Asset/MaterialAsset.h"
#include "RHI/RHI.h"

namespace ks
{
	FRenderPrimitive::FRenderPrimitive(FStaticMeshComponent* MeshComponent)
		:RenderData(MeshComponent->GetStaticMesh()->GetRenderData())
	{
		FPrimitiveConstBufferParameter ConstBufferParameter;
		ConstBufferParameter.WorldTrans = MeshComponent->GetWorldTrans();
		ConstBufferParameter.InvTranspWorldTrans = glm::affineInverse(ConstBufferParameter.WorldTrans);
		ConstBufferParameter.WorldTrans = glm::transpose(ConstBufferParameter.WorldTrans);

		FMaterialAsset* MaterialAsset{ MeshComponent->GetStaticMesh()->GetMaterialAsset() };
		if (MaterialAsset)
		{
			const FMaterialData& MaterialData{ MaterialAsset->GetMaterialData() };
			memcpy(&ConstBufferParameter.BaseColorFactor, &MaterialData.BaseColorFactor[0], sizeof(float) * _countof(MaterialData.BaseColorFactor));
			ConstBufferParameter.RoughnessMetallicFactor.x = MaterialData.RoughnessFactor;
			ConstBufferParameter.RoughnessMetallicFactor.y = MaterialData.MetallicFactor;
		}

		PrimitiveConstBuffer = std::shared_ptr<TConstBuffer<FPrimitiveConstBufferParameter>>(
			TConstBuffer<FPrimitiveConstBufferParameter>::CreateConstBuffer(ConstBufferParameter));
		PrimitiveConstBuffer->GetRHIConstBuffer()->SetLocationIndex(0);
	}

	/**********************************************************************/

	FRenderScene::FRenderScene(FScene* InScene)
		:Scene(InScene)
	{
		// base pass const buffer
		FViewConstBufferParameter ViewConstBufferParm;
		// view projection matrix
		glm::mat4 view_mat{ Scene->GetViewTrans() };
		glm::mat4 proj_mat{ Scene->GetProjectionTrans() };
		ViewConstBufferParm.ViewProjTrans = proj_mat * view_mat;
		ViewConstBufferParm.ViewProjTrans = glm::transpose(ViewConstBufferParm.ViewProjTrans);
		// directional light direction and intensity
		glm::vec3 LightDir;
		float LightIns;
		Scene->GetDirectionalLight(LightDir, LightIns);
		ViewConstBufferParm.D_LightDirectionAndInstensity = glm::vec4(LightDir, LightIns);
		// get look direction
		FSceneNode* Camera{Scene->GetCamera()};
		glm::mat4 Eye2World{Camera->GetWorldTrans()};
		ViewConstBufferParm.EyePos = Eye2World[3];

		BasePassConstBuffer = std::shared_ptr<TConstBuffer<FViewConstBufferParameter>>(
			TConstBuffer<FViewConstBufferParameter>::CreateConstBuffer(ViewConstBufferParm));
		BasePassConstBuffer->GetRHIConstBuffer()->SetLocationIndex(1);
	}

	void FRenderScene::AddPrimitive(FStaticMeshComponent* MeshComponent)
	{
		auto Primitive = std::make_unique<FRenderPrimitive>(MeshComponent);
		Primitives.push_back(std::move(Primitive));
	}

	void FRenderScene::Update()
	{

	}

	/**********************************************************************/

	std::unordered_map<const FRenderScene*, std::unique_ptr<FRenderScene>> FRenderer::RenderScenes;

	ks::FRenderScene* FRenderer::CreateRenderScene(FScene* Scene)
	{
		std::unique_ptr<FRenderScene> RenderScene = std::make_unique<FRenderScene>(Scene);
		FRenderScene* pRenderScene = RenderScene.get();
		bool ok = RenderScenes.insert({ pRenderScene, std::move(RenderScene) }).second;
		assert(ok);
		return pRenderScene;
	}

	FRenderer::FRenderer()
	{
		Init();
	}

	void FRenderer::Render()
	{
		GRHI->BeginFrame();

		RenderBasePass();

		GRHI->EndFrame();
	}

	void FRenderer::Shutdown()
	{
		KS_INFO(TEXT("FRenderer::Shutdown"));
		if(RenderScene)
		{
			KS_INFO(TEXT("\tFree RenderScene"));
			FRenderer::RenderScenes.erase(RenderScene);
			RenderScene = nullptr;
		}
		FRenderPass::ReleaseAllPasses();
	}

	void FRenderer::Init()
	{
		// create base pass
		FRenderPassDesc Desc{};
		Desc.Name = "BasePass";
		Desc.Renderer = this;
		Desc.PipelineStateDesc;
		FRenderPass* Pass = FRenderPass::CreatePass(Desc);
		assert(Pass);
	}

	void FRenderer::RenderBasePass()
	{
		FRenderPass* Pass = FRenderPass::GetPass("BasePass");
		Pass->Render();
	}
}