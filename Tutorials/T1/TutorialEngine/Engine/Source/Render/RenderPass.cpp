#include "engine_pch.h"
#include "RenderPass.h"
#include "RHI/RHI.h"
#include "Render/Render.h"
#include "Render/MeshRenderData.h"

namespace ks
{

	FRenderPass::FRenderPass(const FRenderPassDesc& Desc)
		:Name(Desc.Name)
		,Renderer(Desc.Renderer)
	{
		auto pPipelineState = GRHI->CreatePipelineState(Desc.PipelineStateDesc);
		RHIPipelineState.reset(pPipelineState);
	}

	void FRenderPass::Render()
	{
		// begin
		// set pipeline state and root signature and primitive type
		GRHI->SetPipelineState(RHIPipelineState.get());
		
		// bind pass shader parameter
#if !RHICONSTBUFFER_V1
		GRHI->SetShaderConstBuffer(Renderer->RenderScene->BasePassConstBuffer->GetRHIConstBuffer());
#else
		GRHI->SetConstBuffer(Renderer->RenderScene->BasePassConstBuffer1.get());
#endif
		
		// bind primitive shader parameter
		for (int32 i{0}; i < Renderer->RenderScene->Primitives.size(); ++i)
		{
			FRenderPrimitive* Prim = Renderer->RenderScene->Primitives.at(i).get();
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

	std::unordered_map<std::string, std::unique_ptr<FRenderPass>> FRenderPass::RenderPasses;
}
