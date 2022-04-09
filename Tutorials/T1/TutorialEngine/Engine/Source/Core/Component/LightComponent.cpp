#include "engine_pch.h"
#include "Core/Component/LightComponent.h"
#include "Core/Scene.h"

namespace ks
{
	const glm::vec3 FLightComponent::GetDirection() const
	{
		glm::mat4 WorldTrans{Owner->GetWorldTrans()};
		return glm::normalize(glm::vec3{WorldTrans[2]});
	}
}