#include "engine_pch.h"
#include "Render/Render.h"
#include "Render/MeshRenderData.h"
#include "Render/RenderPass.h"
#include "Core/Scene.h"
#include "Core/MeshAsset.h"
#include "RHI/RHI.h"

namespace ks
{
	FRenderPrimitive::FRenderPrimitive(FStaticMeshComponent* MeshComponent)
		:RenderData(MeshComponent->GetStaticMesh()->GetRenderData())
	{
		FPrimitiveConstBufferParameter ConstBufferParameter;
		ConstBufferParameter.WorldTrans = MeshComponent->GetWorldTrans();
		ConstBufferParameter.WorldTrans = glm::transpose(ConstBufferParameter.WorldTrans);
		ConstBufferParameter.InvTransposeWorldTrans = glm::inverse(ConstBufferParameter.WorldTrans);

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
		glm::mat4 view_mat{ Scene->GetViewTrans() };
		glm::mat4 proj_mat{Scene->GetProjectionTrans()};
		ViewConstBufferParm.ViewProjectionTrans = proj_mat * view_mat;
		ViewConstBufferParm.ViewProjectionTrans = glm::transpose(ViewConstBufferParm.ViewProjectionTrans);
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