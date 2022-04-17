#pragma once

#include "Core/CoreMinimal.h"

namespace ks
{
	struct FInputElemDesc
	{
		std::string SemanticName{};
		int32 SemanticIndex{-1};
		EELEM_FORMAT ElemFormat{EELEM_FORMAT::INVALID};
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

	class FRHIPipelineStateDesc
	{
	public:
		FRHIPipelineStateDesc() = default;
		std::vector<FInputElemDesc> InputLayout;
		FVertexShaderDesc VertexShaderDesc;
		FPixelShaderDesc PixelShaderDesc;
	};
}