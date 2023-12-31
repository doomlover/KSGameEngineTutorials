#pragma once

#include "Core/CoreMinimal.h"

namespace ks
{
	struct FInputElemDesc
	{
		std::string SemanticName{};
		int32 SemanticIndex{-1};
		EELEM_FORMAT ElemFormat{EELEM_FORMAT::UNKNOWN};
		int32 InputSlot{-1};
		int32 Stride{0};
	};

	enum class EShaderType
	{
		VertexShader,
		PixelShader,
		INVALID,
	};

	template<EShaderType _ShaderType>
	struct FShaderDesc
	{
		static const EShaderType ShaderType = _ShaderType;
		std::string Name;
		std::string File;
		std::string EntryPoint;
	};

	using FVertexShaderDesc = FShaderDesc<EShaderType::VertexShader>;
	using FPixelShaderDesc = FShaderDesc<EShaderType::PixelShader>;

	struct FRHIPipelineStateDesc
	{
		struct FRasterizerState
		{
			int32_t FrontCounterClockwise{1};
			int32_t DepthBias{0};
			float DepthBiasClamp{0.f};
			float SlopeScaledDepthBias{0.f};
		};
		struct FDepthStencilState
		{
			int32_t DepthEnable = 1;
		};
		std::vector<FInputElemDesc> InputLayout;
		FVertexShaderDesc VertexShaderDesc;
		FPixelShaderDesc PixelShaderDesc;
		uint32_t NumRenderTargets{1};
		EELEM_FORMAT RenderTargetFormats[8];
		EELEM_FORMAT DepthBufferFormat;
		FRasterizerState RasterizerState;
		FDepthStencilState DepthStencilState;
	};
}