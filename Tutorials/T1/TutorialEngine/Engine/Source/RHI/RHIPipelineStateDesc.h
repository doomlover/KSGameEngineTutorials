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

	class FRHIPipelineStateDesc
	{
	public:
		FRHIPipelineStateDesc() = default;
		std::vector<FInputElemDesc> InputLayout;
	};
}