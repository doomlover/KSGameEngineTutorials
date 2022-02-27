
#include "engine_pch.h"

namespace ks
{
	FEngine* GEngine = nullptr;

	namespace engine
	{
		void Init()
		{
			KS_INFO(TEXT("engine::Init"));
			GApp->Init();
		}

		void Tick()
		{
			//KS_INFO(TEXT("engine::Tick"));
			GApp->Tick();
		}

		void Shutdown()
		{
			KS_INFO(TEXT("engine::Shutdown"));
			GApp->Shutdown();
		}
	}


	FEngine* FEngine::Create()
	{
		GEngine = new FEngine;
		return GEngine;
	}

	FEngine::~FEngine()
	{
		KS_INFO(TEXT("~FEngine"));
		if (RHI)
		{
			delete RHI;
		}
	}

	void FEngine::Init()
	{
		KS_INFO(TEXT("FEngine::Init"));
		RHI = IRHI::Create();
		RHI->Init();
	}

	void FEngine::Tick()
	{
		//KS_INFO(TEXT("Engine::Tick"));
	}

	void FEngine::Shutdown()
	{
		KS_INFO(TEXT("FEngine::Shutdown"));
		RHI->Shutdown();
	}

}


