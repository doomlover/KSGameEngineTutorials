#include "GameProject.h"
#include "Core/CoreMinimal.h"
#include "Core/EntryPoint.h"

class GameApp : public FGenericApp
{
public:
	GameApp() {}
	virtual ~GameApp() {}
protected:
	virtual void OnInit() override {}
	virtual void OnTick() override {}
	virtual void OnShutdown() override {}
};

KS_CREATE_APP(GameApp)
