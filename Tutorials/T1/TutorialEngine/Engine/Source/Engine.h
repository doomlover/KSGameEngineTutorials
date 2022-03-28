#pragma once

namespace ks
{
	//class FEngine;
	class IRHI;
	class FAssetManager;
	class FScene;
	class FRenderer;

	extern FEngine* GEngine;
	extern std::wstring GCmdLineArgs;

	/* see WindowsEntry */
	namespace engine
	{
		void Loop();
	}

	class FEngine
	{
	public:
		static FEngine* Create();
		~FEngine();
		void Init();
		void Tick();
		void Shutdown();

	protected:
		void LoadScene();

		std::unique_ptr<IRHI> RHI;
		std::unique_ptr<FAssetManager> AssetManager;
		std::unique_ptr<FScene> Scene;
		std::unique_ptr<FRenderer> Renderer;
	};
}