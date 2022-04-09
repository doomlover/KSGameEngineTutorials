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
		GRHI->SetShaderConstBuffer(Renderer->RenderScene->BasePassConstBuffer->GetRHIConstBuffer());
		
		// bind primitive shader parameter
		for (int32 i{0}; i < Renderer->RenderScene->Primitives.size(); ++i)
		{
			FRenderPrimitive* Prim = Renderer->RenderScene->Primitives.at(i).get();
			GRHI->SetShaderConstBuffer(Prim->GetPrimitiveConstBuffer()->GetRHIConstBuffer());

			// set vertex input
			const FMeshRenderData* MeshRenderData{ Prim->GetRenderData() };
			const IRHIVertexBuffer* VertexBuffers[] = {MeshRenderData->GetRHIVertexBuffer(), MeshRenderData->GetRHIAttrBuffer()};
			GRHI->SetVertexBuffers(VertexBuffers, _countof(VertexBuffers));

			// set index buffer and draw primitives
			const IRHIIndexBuffer* IndexBuffer{ MeshRenderData->GetRHIIndexBuffer() };
			GRHI->DrawIndexedPrimitive(IndexBuffer);
		}
		// end
	}

	std::unordered_map<std::string, std::unique_ptr<FRenderPass>> FRenderPass::RenderPasses;
}
