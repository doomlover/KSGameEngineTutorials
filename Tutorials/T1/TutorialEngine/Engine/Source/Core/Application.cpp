#include "engine_pch.h"

using namespace ks;

IApp* GApp = nullptr;

IApp::~IApp()
{
	KS_INFO(TEXT("~IApp"));

	if (Engine)
	{
		delete Engine;
	}
}

void IApp::Init()
{
	KS_INFO(TEXT("IApp::Init"));
	Engine = FEngine::Create();
	Engine->Init();
	OnInit();
}

void IApp::Shutdown()
{
	KS_INFO(TEXT("IApp::Shutdown"));
	OnShutdown();
	Engine->Shutdown();
}
