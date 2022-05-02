#pragma once

#include "RHI/RHIPipelineStateDesc.h"

namespace ks
{
	class FRenderer;
	class IRHIPipelineState;

	class FRenderPassDesc
	{
	public:
		FRenderPassDesc() = default;
		std::string Name{};
		FRenderer* Renderer{ nullptr };
		FRHIPipelineStateDesc PipelineStateDesc{};
	};

	class FRenderPass
	{
	public:
		static FRenderPass* CreatePass(const FRenderPassDesc& Desc) {
			const auto& KeyName{ Desc.Name };
			assert(RenderPasses.find(KeyName) == RenderPasses.end());
			RenderPasses.insert({ KeyName, std::make_unique<FRenderPass>(Desc) }) ;
			return RenderPasses.at(KeyName).get();
		}
		static FRenderPass* GetPass(const std::string& KeyName) {
			return RenderPasses.at(KeyName).get();
		}
		static void ReleaseAllPasses() {
			KS_INFOA("RenderPass : Release All Passes.");
			RenderPasses.clear();
		}
		FRenderPass(const FRenderPassDesc& Desc);
		void Render();
	private:
		std::string Name{};
		FRenderer* Renderer{nullptr};
		std::unique_ptr<IRHIPipelineState> RHIPipelineState;
		static std::unordered_map<std::string, std::unique_ptr<FRenderPass>> RenderPasses;
	};


}


