#pragma once

namespace ks
{
	class FEngine;

	class KS_API IApp
	{
		friend FEngine;
	public:
		virtual ~IApp();
		virtual void Init();
		virtual void Tick() {
			OnTick();
		}
		virtual void Shutdown();

	protected:
		virtual void OnInit() = 0;
		virtual void OnTick() = 0;
		virtual void OnShutdown() = 0;
		// the engine instance
		FEngine* Engine = nullptr;
		// IApp暂时记录Config参数
		std::string StartMap;
	};

	extern KS_API IApp* GApp;
}