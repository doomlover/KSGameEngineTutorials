#pragma once

namespace ks
{
	class FEngine;

	class KS_API IApp
	{
		friend FEngine;
	public:
		IApp() : Engine(nullptr), WinX(0), WinY(0), ResX(800), ResY(600){}
		virtual ~IApp();
		void PreInit();
		virtual void Init();
		virtual void Tick();
		virtual void Shutdown();
		void GetWindowSize(uint32_t& _ResX, uint32_t& _ResY) { _ResX = ResX; _ResY = ResY; }
		float GetWindowAspect() { return static_cast<float>(ResX) / ResY; }

	protected:
		virtual void OnInit() = 0;
		virtual void OnTick() = 0;
		virtual void OnShutdown() = 0;
		// the engine instance
		FEngine* Engine;
		// IApp暂时记录Config参数
		std::string StartMap;
		// window settings
		uint32_t WinX, WinY;
		uint32_t ResX, ResY;
	};

	extern KS_API IApp* GApp;
}