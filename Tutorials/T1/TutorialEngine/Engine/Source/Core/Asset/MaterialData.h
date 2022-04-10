#pragma once

namespace ks
{
	struct FMaterialData
	{
		std::string KeyName;
		float BaseColorFactor[4]{0.f};
		float MetallicFactor{0.f};
		float RoughnessFactor{0.f};

		FMaterialData() = default;

		FMaterialData(const FMaterialData& Other) {
			*this = Other;
		}

		FMaterialData& operator=(const FMaterialData& Other) {
			KeyName = Other.KeyName;
			memcpy(&BaseColorFactor[0], &Other.BaseColorFactor[0], sizeof(float)*_countof(BaseColorFactor));
			MetallicFactor = Other.MetallicFactor;
			RoughnessFactor = Other.RoughnessFactor;
			return *this;
		}

		FMaterialData(FMaterialData&& Tmp) noexcept {
			*this = std::move(Tmp);
		}
		FMaterialData& operator=(FMaterialData&& Tmp) noexcept {
			KeyName = std::move(Tmp.KeyName);
			return *this;
		}
	};
}