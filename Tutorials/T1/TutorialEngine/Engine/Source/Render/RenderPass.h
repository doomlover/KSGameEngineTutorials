#pragma once

#include "RHI/RHICommon.h"

namespace ks
{
	class IRHIPipelineState;

	class FRenderPass
	{
	public:
		template<typename _PassType>
		static FRenderPass* CreatePass(const FRenderPassDesc& Desc) {
			const auto& KeyName{ Desc.Name };
			assert(RenderPasses.find(KeyName) == RenderPasses.end());
			RenderPasses.insert({ KeyName, std::make_unique<_PassType>(Desc) }) ;
			return RenderPasses.at(KeyName).get();
		}
		static FRenderPass* GetPass(const std::string& KeyName) {
			return RenderPasses.at(KeyName).get();
		}
		static void ReleaseAllPasses() {
			KS_INFOA("RenderPass : Release All Passes.");
			RenderPasses.clear();
		}
		FRenderPass() = default;
		FRenderPass(const FRenderPassDesc& _Desc);
		virtual ~FRenderPass() = 0 {}
		virtual void Begin() = 0;
		virtual void Render() = 0;
		virtual void End() = 0;
	protected:
		FRenderPassDesc Desc;
		std::unique_ptr<IRHIPipelineState> RHIPipelineState;
		static std::unordered_map<std::string, std::unique_ptr<FRenderPass>> RenderPasses;
	};

	class FBasePass : public FRenderPass
	{
	public:
		FBasePass(const FRenderPassDesc& Desc);
		virtual ~FBasePass();
		virtual void Begin() override;
		virtual void Render() override;
		virtual void End() override;
	};

	class FShadowPass : public FRenderPass
	{
	public:
		FShadowPass(const FRenderPassDesc& Desc);
		virtual void Begin() override;
		virtual void Render() override;
		virtual void End() override;
		IRHITexture2D* GetShadowTexture2D() { return ShadowMap->GetTexture2D(); }
	protected:
		std::unique_ptr<IRHIDepthStencilBuffer> ShadowMap;

	};
}


