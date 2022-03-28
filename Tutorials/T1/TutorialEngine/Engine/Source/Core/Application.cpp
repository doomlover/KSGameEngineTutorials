#include "engine_pch.h"
#include "Core/Application.h"
#include "Engine.h"

namespace ks
{
	IApp* GApp = nullptr;

	IApp::~IApp()
	{
		KS_INFO(TEXT("~IApp"));

		assert(Engine);
		if (Engine) delete Engine;
	}

	void IApp::PreInit()
	{
		// IApp暂时负责参数解析
		std::vector<std::string> Args;
		std::string ArgsStr = ks::FString::WS2S(GCmdLineArgs);
		std::istringstream InStrStream(ArgsStr);
		std::string TmpStr;
		while (getline(InStrStream, TmpStr, ' '))
		{
			Args.push_back(TmpStr);
			KS_INFO(ks::FString::S2WS(TmpStr).c_str());
			if (TmpStr.find("-map=") == 0)
			{
				StartMap = TmpStr.substr(std::string("-map=").length());
			}
			else if (TmpStr.find("-WinX") == 0)
			{
				WinX = std::atoi(TmpStr.substr(std::string("-WinX=").length()).c_str());
			}
			else if (TmpStr.find("-WinY") == 0)
			{
				WinY = std::atoi(TmpStr.substr(std::string("-WinY=").length()).c_str());
			}
			else if (TmpStr.find("-ResX") == 0)
			{
				ResX = std::atoi(TmpStr.substr(std::string("-ResX=").length()).c_str());
			}
			else if (TmpStr.find("-ResY") == 0)
			{
				ResY = std::atoi(TmpStr.substr(std::string("-ResY=").length()).c_str());
			}
		}
		// ...
	}

	void IApp::Init()
	{
		KS_INFO(TEXT("IApp::Init"));
		Engine = FEngine::Create();
		Engine->Init();
		OnInit();
	}

	void IApp::Tick()
	{
		OnTick();
		Engine->Tick();
	}

	void IApp::Shutdown()
	{
		KS_INFO(TEXT("IApp::Shutdown"));
		OnShutdown();
		Engine->Shutdown();
	}
}
