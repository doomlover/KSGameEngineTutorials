#include "GameProject.h"

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

KS_MAIN_ENTRY(GameApp)
