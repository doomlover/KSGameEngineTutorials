#include "GameProject.h"

class GameApp : public FGenericApp
{
public:
	GameApp() {}
	virtual ~GameApp() {}
	virtual void Init() override {}
	virtual void Shutdown() override {}
};

KS_MAIN_ENTRY(GameApp)
