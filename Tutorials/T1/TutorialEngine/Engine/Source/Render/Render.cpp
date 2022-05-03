#include "engine_pch.h"
#include "Render/Render.h"
#include "Render/MeshRenderData.h"
#include "Render/RenderPass.h"
#include "Core/Scene.h"
#include "Core/Asset/MeshAsset.h"
#include "Core/Asset/MaterialAsset.h"
#include "Core/Component/MeshComponent.h"
#include "RHI/RHI.h"

namespace ks
{
	FRenderPrimitive::FRenderPrimitive(FStaticMeshComponent* MeshComponent)
		:RenderData(MeshComponent->GetStaticMesh()->GetRenderData())
		,Bounds(MeshComponent->GetBounds())
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

#if !RHICONSTBUFFER_V1
		PrimitiveConstBuffer = std::shared_ptr<TConstBuffer<FPrimitiveConstBufferParameter>>(
			TConstBuffer<FPrimitiveConstBufferParameter>::CreateConstBuffer(ConstBufferParameter));
		PrimitiveConstBuffer->GetRHIConstBuffer()->SetLocationIndex(0);
#else
		PrimitiveConstBuffer1.reset(GRHI->CreateConstBuffer1(&ConstBufferParameter, sizeof(FPrimitiveConstBufferParameter)));
		PrimitiveConstBuffer1->SetLocationIndex(0);
#endif
	}

	/**********************************************************************/

	FRenderScene::FRenderScene(FScene* InScene)
		:Scene(InScene)
	{
		// base pass const buffer
		FViewConstBufferParameter ViewConstBufferParm;

		// camera-view projection matrix
		// todo : move to ticking
		{
			glm::mat4 CameraView{ Scene->GetViewTrans() };
			glm::mat4 PersProj{ Scene->GetProjectionTrans() };
			ViewConstBufferParm.ViewProjTrans = glm::transpose(PersProj * CameraView);
		}
		// directional light direction and intensity
		// todo : move to ticking
		{
			glm::vec3 LightDir;
			float LightIns;
			auto LightNode = Scene->GetLightNode();
			assert(LightNode && LightNode->LightComponent);
			glm::mat4 LightToWorld = LightNode->GetWorldTrans();
			glm::mat4 WorldToLight = glm::affineInverse(LightToWorld);
			LightDir = glm::normalize(glm::vec3(LightToWorld[2]));
			LightIns = LightNode->LightComponent->GetIntensity();
			ViewConstBufferParm.D_LightDirectionAndInstensity = glm::vec4(LightDir, LightIns);

			const auto& SceneBounds{ Scene->GetSceneBounds() };
			const float& SceneBoundsRadius{ SceneBounds.Sphere.Radius };
			glm::vec4 Center = WorldToLight * glm::vec4(SceneBounds.Sphere.Center, 1.f);
			glm::mat4 OrthoProj = glm::ortho(
				Center.x-SceneBoundsRadius, Center.x+SceneBoundsRadius,
				Center.y-SceneBoundsRadius, Center.y+SceneBoundsRadius,
				Center.z-SceneBoundsRadius, Center.z+SceneBoundsRadius);
			ViewConstBufferParm.LightProj = glm::transpose(OrthoProj * WorldToLight);

			glm::mat4 NDC2Tex(
				0.5f, 0.0f, 0.0f, 0.0f,
				0.0f, -0.5f, 0.0f, 0.0f,
				0.0f, 0.0f, 1.0f, 0.0f,
				0.5f, 0.5f, 0.0f, 1.0f
			);
			ViewConstBufferParm.LightProjTex = glm::transpose(NDC2Tex * OrthoProj * WorldToLight);
		}

		// get look direction
		// todo : move to ticking
		{
			FSceneNode* Camera{ Scene->GetCamera() };
			glm::mat4 Eye2World{ Camera->GetWorldTrans() };
			ViewConstBufferParm.EyePos = Eye2World[3];
		}


#if !RHICONSTBUFFER_V1
		BasePassConstBuffer = std::shared_ptr<TConstBuffer<FViewConstBufferParameter>>(
			TConstBuffer<FViewConstBufferParameter>::CreateConstBuffer(ViewConstBufferParm));
		BasePassConstBuffer->GetRHIConstBuffer()->SetLocationIndex(1);
#else
		BasePassConstBuffer1.reset(GRHI->CreateConstBuffer1(&ViewConstBufferParm, sizeof(FViewConstBufferParameter)));
		BasePassConstBuffer1->SetLocationIndex(1);
#endif
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

		RenderShadowPass();

		RenderBasePass();

		GRHI->EndFrame();
	}

	void FRenderer::Shutdown()
	{
		KS_INFO(TEXT("FRenderer::Shutdown"));
		if(RenderScene)
		{
			KS_INFO(TEXT("\tFree RenderScene"));
			size_t NumErase = FRenderer::RenderScenes.erase(RenderScene);
			RenderScene = nullptr;
		}
		FRenderPass::ReleaseAllPasses();
	}

	void FRenderer::Init()
	{
		// create base pass
		{
			FRenderPassDesc Desc{};
			Desc.Name = "BasePass";
			Desc.ViewPort = GRHIConfig.ViewPort;
			Desc.Renderer = this;
			Desc.PipelineStateDesc.InputLayout = {
				/*SemanticName, SemanticIndex, Format, InputSlot, Stride*/
				{"POSITION", 0, EELEM_FORMAT::R32G32B32_FLOAT, 0, 0},
				{"NORMAL", 0, EELEM_FORMAT::R32G32B32_FLOAT, 1, 0}
			};
			Desc.PipelineStateDesc.VertexShaderDesc = { "BasePassVS", util::GetShaderPath("BasePass.hlsl"), "VS" };
			Desc.PipelineStateDesc.PixelShaderDesc = { "BasePassPS", util::GetShaderPath("BasePass.hlsl"), "PS" };
			Desc.PipelineStateDesc.NumRenderTargets = 1;
			Desc.PipelineStateDesc.RenderTargetFormats[0] = GRHIConfig.BackBufferFormat;
			Desc.PipelineStateDesc.DepthBufferFormat = GRHIConfig.DepthBufferFormat;
			FRenderPass* Pass = FRenderPass::CreatePass<FBasePass>(Desc);
			assert(Pass);
		}

		// setup shadow pass
		{
			FRenderPassDesc Desc{};
			Desc.Name = "ShadowPass";
			Desc.ViewPort = { 0, 0, GRHIConfig.ShadowMapSize, GRHIConfig.ShadowMapSize };
			Desc.Renderer = this;
			Desc.PipelineStateDesc.InputLayout = {
				/*SemanticName, SemanticIndex, Format, InputSlot, Stride*/
				{"POSITION", 0, EELEM_FORMAT::R32G32B32_FLOAT, 0, 0},
				{"NORMAL", 0, EELEM_FORMAT::R32G32B32_FLOAT, 1, 0}
			};
			Desc.PipelineStateDesc.VertexShaderDesc = { "ShadowPassVS", util::GetShaderPath("ShadowPass.hlsl"), "VS" };
			Desc.PipelineStateDesc.PixelShaderDesc = { "ShadowPassPS", util::GetShaderPath("ShadowPass.hlsl"), "PS" };
			Desc.PipelineStateDesc.NumRenderTargets = 0;
			Desc.PipelineStateDesc.RenderTargetFormats[0] = EELEM_FORMAT::UNKNOWN;
			Desc.PipelineStateDesc.DepthBufferFormat = GRHIConfig.DepthBufferFormat;

			Desc.PipelineStateDesc.RasterizerState.DepthBias = 100000;
			Desc.PipelineStateDesc.RasterizerState.SlopeScaledDepthBias = 1.0f;

			FRenderPass* Pass = FRenderPass::CreatePass<FShadowPass>(Desc);
			assert(Pass);
		}
	}

	void FRenderer::RenderBasePass()
	{
		FRenderPass* Pass = FRenderPass::GetPass("BasePass");
		Pass->Begin();
		Pass->Render();
		Pass->End();
	}

	void FRenderer::RenderShadowPass()
	{
		static FRenderPass* Pass = FRenderPass::GetPass("ShadowPass");
		Pass->Begin();
		Pass->Render();
		Pass->End();
	}

}