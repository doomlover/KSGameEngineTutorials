#include "engine_pch.h"
#include "RenderPass.h"
#include "RHI/RHI.h"
#include "Render/Render.h"
#include "Render/MeshRenderData.h"

namespace ks
{
	std::unordered_map<std::string, std::unique_ptr<FRenderPass>> FRenderPass::RenderPasses;

	FRenderPass::FRenderPass(const FRenderPassDesc& _Desc)
		:Desc(_Desc)
	{
		auto pPipelineState = GRHI->CreatePipelineState(Desc.PipelineStateDesc);
		RHIPipelineState.reset(pPipelineState);
	}

	/*****************************************************************************/
	FBasePass::FBasePass(const FRenderPassDesc& _Desc)
		:FRenderPass(_Desc)
	{
		FTexture2DDesc SceneColorDesc;
		SceneColorDesc.Width = Desc.ViewPort.Width;
		SceneColorDesc.Height = Desc.ViewPort.Height;
		SceneColorDesc.Format = _Desc.PipelineStateDesc.RenderTargetFormats[0];
		SceneColorMap.reset(GRHI->CreateRenderTarget(SceneColorDesc));
		SceneColorMap->GetTexture2D()->SetLocationIndex(3);
	}

	FBasePass::~FBasePass()
	{

	}

	void FBasePass::Begin()
	{
		// Set the viewport and scissor rect.  This needs to be reset whenever the command list is reset.
		GRHI->SetViewports(1, &Desc.ViewPort);

		// set render target and depth stencil buffer
		//GRHI->SetRenderTarget(GRHI->GetCurrentBackBuffer(), GRHI->GetDefaultDepthStencilBuffer());
		GRHI->SetRenderTarget(SceneColorMap.get(), GRHI->GetDefaultDepthStencilBuffer());

		GRHI->BeginPass();

		// Clear the back buffer
		GRHI->ClearRenderTarget(color::Black);

		// clear the depth buffer
		GRHI->ClearDepthStencilBuffer();
	}

	void FBasePass::Render()
	{
		auto Renderer{Desc.Renderer};
		auto RenderScene = Renderer->GetScene();

		// set pipeline state and root signature and primitive type
		GRHI->SetPipelineState(RHIPipelineState.get());

		// bind shadow map parameter
		auto ShadowPass = dynamic_cast<FShadowPass*>(FRenderPass::GetPass("ShadowPass"));
		GRHI->SetTexture2D(ShadowPass->GetShadowTexture2D());
		
		// bind pass shader parameter
#if !RHICONSTBUFFER_V1
		GRHI->SetShaderConstBuffer(Renderer->RenderScene->BasePassConstBuffer->GetRHIConstBuffer());
#else
		GRHI->SetConstBuffer(RenderScene->GetBasePassConstBuffer());
#endif
		
		auto& Primitives{RenderScene->GetPrimitives()};
		// bind primitive shader parameter
		for (int32 i{0}; i < Primitives.size(); ++i)
		{
			FRenderPrimitive* Prim{Primitives.at(i).get()};
#if !RHICONSTBUFFER_V1
			GRHI->SetShaderConstBuffer(Prim->GetPrimitiveConstBuffer()->GetRHIConstBuffer());
#else
			GRHI->SetConstBuffer(Prim->GetConstBuffer());
#endif
			// set vertex input
			const FMeshRenderData* MeshRenderData{ Prim->GetRenderData() };
#if !RHIVERTBUFFER_V1
			const IRHIVertexBuffer* VertexBuffers[] = {MeshRenderData->GetRHIVertexBuffer(), MeshRenderData->GetRHIAttrBuffer()};
			GRHI->SetVertexBuffers(VertexBuffers, _countof(VertexBuffers));
#else
			const IRHIVertexBuffer1* VertexBuffers1[] = {MeshRenderData->GetRHIVertexBuffer1(), MeshRenderData->GetRHIAttrBuffer1()};
			GRHI->SetVertexBuffers1(VertexBuffers1, _countof(VertexBuffers1));
#endif
			// set index buffer and draw primitives
#if RHIINDEXBUFFER_V1
			const IRHIIndexBuffer1* IndexBuffer{ MeshRenderData->GetRHIIndexBuffer1() };
			GRHI->DrawIndexedPrimitive1(IndexBuffer);
#else
			const IRHIIndexBuffer* IndexBuffer{ MeshRenderData->GetRHIIndexBuffer() };
			GRHI->DrawIndexedPrimitive(IndexBuffer);
#endif
		}
		// end
	}

	void FBasePass::End()
	{
		GRHI->EndPass();
	}

	/*****************************************************************************/
	FShadowPass::FShadowPass(const FRenderPassDesc& Desc)
		:FRenderPass(Desc)
	{
		FTexture2DDesc DepthDesc;
		DepthDesc.Width = Desc.ViewPort.Width;
		DepthDesc.Height = Desc.ViewPort.Height;
		ShadowMap.reset(GRHI->CreateDepthStencilBuffer(DepthDesc));
		ShadowMap->GetTexture2D()->SetLocationIndex(2);
	}

	void FShadowPass::Begin()
	{
		// Set the viewport and scissor rect.  This needs to be reset whenever the command list is reset.
		GRHI->SetViewports(1, &Desc.ViewPort);

		// set render target and depth stencil buffer
		GRHI->SetRenderTarget(nullptr, ShadowMap.get());

		GRHI->BeginPass();

		// clear the depth buffer
		GRHI->ClearDepthStencilBuffer();
	}

	void FShadowPass::Render()
	{
		auto Renderer{ Desc.Renderer };
		auto RenderScene{ Renderer->GetScene() };

		// set pipeline state and root signature and primitive type
		GRHI->SetPipelineState(RHIPipelineState.get());

		// bind pass shader parameter
		GRHI->SetConstBuffer(RenderScene->GetBasePassConstBuffer());

		auto& Primitives{ RenderScene->GetPrimitives() };
		// bind primitive shader parameter
		for (int32 i{ 0 }; i < Primitives.size(); ++i)
		{
			FRenderPrimitive* Prim{ Primitives.at(i).get() };

			GRHI->SetConstBuffer(Prim->GetConstBuffer());
			// set vertex input
			const FMeshRenderData* MeshRenderData{ Prim->GetRenderData() };
			const IRHIVertexBuffer1* VertexBuffers1[] = { MeshRenderData->GetRHIVertexBuffer1(), MeshRenderData->GetRHIAttrBuffer1() };
			GRHI->SetVertexBuffers1(VertexBuffers1, _countof(VertexBuffers1));
			// set index buffer and draw primitives
			const IRHIIndexBuffer1* IndexBuffer{ MeshRenderData->GetRHIIndexBuffer1() };
			GRHI->DrawIndexedPrimitive1(IndexBuffer);
		}
	}

	void FShadowPass::End()
	{
		GRHI->EndPass();
	}

	const FPostProcessPass::FTriangleMesh FPostProcessPass::TriangleMesh = {
		{0, 2, 1},
		{{-3.f, -1.f, 0.f}, {1.f, 3.f, 0.f}, {1.f, -1.f, 0.f}}
	};

	FPostProcessPass::FPostProcessPass(const FRenderPassDesc& Desc)
		:FRenderPass(Desc)
	{
		constexpr uint32_t Stride = sizeof(float) * 3;
		auto RHIIndexBuffer = GRHI->CreateIndexBuffer1(EELEM_FORMAT::R16_UINT, 3, static_cast<uint32_t>(TriangleMesh.Indices.size() * sizeof(uint16_t)), TriangleMesh.Indices.data());
		TriangleMeshIndexBuffer.reset(RHIIndexBuffer);
		auto RHIVertexBuffer = GRHI->CreateVertexBuffer1(Stride, static_cast<uint32_t>(TriangleMesh.Vertices.size() * sizeof(glm::vec3)), TriangleMesh.Vertices.data());
		TriangleMeshVertexBuffer.reset(RHIVertexBuffer);
	}

	void FPostProcessPass::Begin()
	{
		GRHI->SetViewports(1, &Desc.ViewPort);
		GRHI->SetRenderTarget(GRHI->GetCurrentBackBuffer(), GRHI->GetDefaultDepthStencilBuffer());
		GRHI->BeginPass();
		GRHI->ClearRenderTarget(color::Red);
		GRHI->ClearDepthStencilBuffer();
	}

	void FPostProcessPass::Render()
	{
		GRHI->SetPipelineState(RHIPipelineState.get());

		// bind scene color texture
		FBasePass* BasePass = dynamic_cast<FBasePass*>(FRenderPass::GetPass("BasePass"));
		IRHITexture2D* SceneColorTexture2D = BasePass->GetSceneColorTexture2D();
		GRHI->SetTexture2D(SceneColorTexture2D);

		GRHI->SetVertexBuffer1(TriangleMeshVertexBuffer.get());
		GRHI->DrawIndexedPrimitive1(TriangleMeshIndexBuffer.get());
	}

	void FPostProcessPass::End()
	{
		GRHI->EndPass();
	}
	

}
