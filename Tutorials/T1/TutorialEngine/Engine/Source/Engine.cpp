/**/
#include "engine_pch.h"
#include "Engine.h"
#include "RHI/RHI.h"
#include "Core/Assets.h"
#include "Core/Scene.h"

namespace ks
{
	FEngine* GEngine = nullptr;
	std::wstring GCmdLineArgs;

	namespace engine
	{
		void Init()
		{
			KS_INFO(TEXT("engine::Init"));
			GApp->Init();
		}

		void Tick()
		{
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
	}

	void FEngine::Init()
	{
		KS_INFO(TEXT("FEngine::Init"));
		RHI.reset(IRHI::Create());
		RHI->Init();

		AssetManager.reset(FAssetManager::Create());
		AssetManager->Init();

		// load startup scene
		LoadScene();
	}

	void FEngine::Tick()
	{
	}

	void FEngine::Shutdown()
	{
		KS_INFO(TEXT("FEngine::Shutdown\n["));
		KS_INFO(TEXT("\tRelease Scene"));
		Scene.reset();
		AssetManager->Shutdown();
		RHI->Shutdown();
		KS_INFO(TEXT("]//!FEngine::Shutdown"));
	}

	void FEngine::LoadScene()
	{
		std::wstring StartMap{ FString::S2WS(GApp->StartMap) };
		KS_INFO((TEXT("Loading : ") + StartMap).c_str());
		// create scene asset from gltf file
		auto SceneAsset = AssetManager->CreateSceneAsset(GApp->StartMap);
		// create scene from scene asset
		Scene = std::make_unique<FScene>();
		if (SceneAsset)
		{
			Scene = std::make_unique<FScene>(SceneAsset);
		}
	}
}


