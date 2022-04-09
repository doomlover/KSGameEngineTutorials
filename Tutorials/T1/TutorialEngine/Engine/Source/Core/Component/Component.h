#pragma once

namespace ks
{
	struct FSceneNode;

	class IComponent
	{
	public:
		IComponent() = delete;
		IComponent(FSceneNode* _Owner):Owner(_Owner){}
		virtual ~IComponent(){}
		FSceneNode* GetOwner() { return Owner; }
		const FSceneNode* GetOwner() const { return Owner; }
	protected:
		FSceneNode* Owner{nullptr};
	};
}