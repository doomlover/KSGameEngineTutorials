/**/
#include "engine_pch.h"
#include "Engine.h"
#include "RHI/RHI.h"
#include "Core/Asset/Assets.h"
#include "Core/Asset/AssetManager.h"
#include "Core/Scene.h"
#include "Render/Render.h"

namespace ks
{
	FEngine* GEngine = nullptr;
	std::wstring GCmdLineArgs;

	namespace engine
	{
		namespace
		{
			void Init()
			{
				KS_INFO(TEXT("engine::Init"));
				GApp->PreInit();
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

			void PeekMessages()
			{
				FGenericApp::PeekMessages();
			}

			bool RequestExit()
			{
				return FGenericApp::RequestExit();
			}
		}

		void Loop()
		{
			engine::Init();
			while (!engine::RequestExit())
			{
				engine::PeekMessages();
				engine::Tick();
			}
			engine::Shutdown();
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

		uint32_t WindowSizeX, WindowSizeY;
		GApp->GetWindowSize(WindowSizeX, WindowSizeY);
		GRHIConfig.ViewPort.Width = WindowSizeX;
		GRHIConfig.ViewPort.Height = WindowSizeY;
		GRHIConfig.BackBufferFormat = EELEM_FORMAT::R8G8B8A8_UNORM;
		GRHIConfig.DepthBufferFormat = EELEM_FORMAT::D24_UNORM_S8_UINT;
		GRHIConfig.ShadowMapSize = 2048;

		RHI.reset(IRHI::Create());
		RHI->Init(GRHIConfig);

		AssetManager.reset(FAssetManager::Create());
		AssetManager->Init();

		Renderer.reset(FRenderer::Create());

		// load startup scene
		LoadScene();

		// rendering, set render scene to renderer
		Renderer->SetScene(Scene->GetRenderScene());
	}

	void FEngine::Tick()
	{
		Scene->Update();
		Renderer->Render();
	}

	void FEngine::Shutdown()
	{
		KS_INFO(TEXT("FEngine::Shutdown\n["));
		Renderer->Shutdown();
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

namespace util
{
	EELEM_FORMAT GetElemFormat(EDATA_TYPE DataType, EELEM_TYPE ElemType)
	{
		switch (DataType)
		{
		case EDATA_TYPE::BYTE:
		{
			switch (ElemType)
			{
			case EELEM_TYPE::SCALAR:
				return EELEM_FORMAT::R8_INT;
			}
		}
		case EDATA_TYPE::UNSIGNED_BYTE:
		{
			switch (ElemType)
			{
			case EELEM_TYPE::SCALAR:
				return EELEM_FORMAT::R8_UINT;
			}
		}
		case EDATA_TYPE::SHORT:
			switch (ElemType)
			{
			case EELEM_TYPE::SCALAR:
				return EELEM_FORMAT::R16_INT;
			}
		case EDATA_TYPE::UNSIGNED_SHORT:
			switch (ElemType)
			{
			case EELEM_TYPE::SCALAR:
				return EELEM_FORMAT::R16_UINT;
			}
		default:
			break;
		}
		assert(false);
		return EELEM_FORMAT::UNKNOWN;
	}

	uint32_t GetElemFormatSize(EELEM_FORMAT ElemFormat)
	{
		static const std::unordered_map<EELEM_FORMAT, uint32_t> FormatSizeMap = {
			{EELEM_FORMAT::R16_INT,		2},
			{EELEM_FORMAT::R16_UINT,	2},
			{EELEM_FORMAT::R8_INT,		1},
			{EELEM_FORMAT::R8_UINT,		1},
			{EELEM_FORMAT::R32G32B32_FLOAT,		12},
			{EELEM_FORMAT::R32G32B32A32_FLOAT,	16},
		};
		return FormatSizeMap.at(ElemFormat);
	}

	size_t GetDataTypeSize(EDATA_TYPE DataType)
	{
		switch (DataType)
		{
		case EDATA_TYPE::BYTE:
		case EDATA_TYPE::UNSIGNED_BYTE:
			return sizeof(uint8);
		case EDATA_TYPE::SHORT:
		case EDATA_TYPE::UNSIGNED_SHORT:
			return sizeof(uint16);
		case EDATA_TYPE::UNSIGNED_INT:
			return sizeof(int32);
		case EDATA_TYPE::FLOAT:
			return sizeof(float);
		default:
			assert(false);
			break;
		}
		return 0;
	}

	uint32 GetElemNum(EELEM_TYPE ElemType)
	{
		switch (ElemType)
		{
		case EELEM_TYPE::SCALAR:
			return 1;
		case EELEM_TYPE::VEC2:
			return 2;
		case EELEM_TYPE::VEC3:
			return 3;
		case EELEM_TYPE::VEC4:
			return 4;
		case EELEM_TYPE::MAT2:
			return 4;
		case EELEM_TYPE::MAT3:
			return 9;
		case EELEM_TYPE::MAT4:
			return 16;
		default:
			assert(false);
			break;
		}
		return 0;
	}

	std::string GetContentPath(const std::string& Path)
	{
		return "./Content" + Path;
	}

	std::string GetShaderPath(const std::string& Path)
	{
		return "./Shaders/" + Path;
	}
}
}


