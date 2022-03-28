#include "GameProject.h"
#include "Core/CoreMinimal.h"
#include "Core/EntryPoint.h"

class GameApp : public ks::FGenericApp
{
public:
	GameApp() {}
	virtual ~GameApp() {}
protected:
	virtual void OnInit() override {
		KS_INFO(TEXT("GameApp::OnInit"));
	}
	virtual void OnTick() override {
		//KS_INFO(TEXT("GameApp::OnTick"));
	}
	virtual void OnShutdown() override {
		KS_INFO(TEXT("GameApp::OnShutdown"));
	}
};

KS_CREATE_APP(GameApp)
