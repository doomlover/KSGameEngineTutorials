#include "engine_pch.h"
#include "Core/Component/LightComponent.h"
#include "Core/Scene.h"

namespace ks
{
	const glm::vec3 FLightComponent::GetDirection() const
	{
		glm::mat4 WorldTrans = Owner->GetWorldTrans();
		return glm::normalize(glm::vec3{WorldTrans[2]});
	}

	/*const glm::mat4 FLightComponent::WorldToLightMatrix() const
	{
		glm::mat4 Direction = GetDirection();
		glm::vec3 Position = glm::vec3(WorldTrans[3]);
		return glm::lookAt(Position, Position - Direction, glm::vec3(0.f, 1.f, 0.f));
	}*/

}