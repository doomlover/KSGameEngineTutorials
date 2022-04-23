#pragma once

#include "Core/Math.h"

namespace ks
{
	struct FBounds
	{
		FBounds() = default;
		FBounds(const glm::vec3 Min, const glm::vec3 Max) {
			Box.Min = Min;
			Box.Max = Max;
			Sphere.Center = Center();
			const glm::vec3 Extents = Extent();
			Sphere.Radius = glm::max(Extents.x, Extents.y, Extents.z) / 2.f;
		}
		inline glm::vec3 Extent() const {
			return Box.Extent();
		}
		inline glm::vec3 Center() const {
			return Box.Center();
		}

		struct FBox
		{
			glm::vec3 Min{};
			glm::vec3 Max{};
			inline glm::vec3 Center() const {
				return Min + Extent() / 2.f;
			}
			inline glm::vec3 Extent() const {
				return Max - Min;
			}
		};

		struct FSphere
		{
			glm::vec3 Center{};
			float Radius{0};
		};

		FBox Box;
		FSphere Sphere;
	};
}