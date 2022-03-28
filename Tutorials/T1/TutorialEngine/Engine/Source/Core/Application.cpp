#include "engine_pch.h"
#include "Core/Application.h"
#include "Engine.h"

namespace ks
{
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

		// IApp暂时负责参数解析
		{
			std::vector<std::string> Args;
			std::string ArgsStr = ks::FString::WS2S(GCmdLineArgs);
			std::istringstream sstream(ArgsStr);
			std::string tmpstr;
			while (getline(sstream, tmpstr, ' '))
			{
				Args.push_back(tmpstr);
				KS_INFO(ks::FString::S2WS(tmpstr).c_str());
				if (tmpstr.find("-map=") == 0)
				{
					StartMap = tmpstr.substr(std::string("-map=").length());
				}
			}
		}
		// ...

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
}
