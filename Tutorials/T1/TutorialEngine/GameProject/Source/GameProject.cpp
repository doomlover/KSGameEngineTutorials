#include "GameProject.h"

//KS_MAIN_ENTRY()

class GameApp : public IApp
{
public:
	GameApp() {}
	virtual ~GameApp() {}
	virtual void Init() override {}
	virtual void Shutdown() override {}
};

//IMPL_APP(GameApp)

int CALLBACK WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
	auto App = new GameApp;
	App->Init();
	delete App;
}
