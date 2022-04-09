#pragma once

#include "Core/Math.h"
#include "Core/Component/Component.h"

namespace ks
{
	enum class ELightType : unsigned short
	{
		DIRECTIONAL,
		SPOT,
		POINT,
		INVALID,
	};

	struct FLightInfo
	{
		ELightType Type{ ELightType::INVALID };
		float Intensity{ 0 };
	};

	class FLightComponent : public IComponent
	{
	public:
		FLightComponent(const FLightInfo& _LightInfo, FSceneNode* _Owner) :IComponent(_Owner),LightInfo(_LightInfo) {}
		float GetIntensity() const { return LightInfo.Intensity; }
		ELightType GetType() const { return LightInfo.Type; }
		const glm::vec3 GetDirection() const;
	private:
		FLightInfo LightInfo;
	};

namespace util
{
	inline ELightType GetLightType(const std::string& LightTypeName) {
		if (LightTypeName == "directional")
		{
			return ELightType::DIRECTIONAL;
		}
		return ELightType::INVALID;
	}
}
}