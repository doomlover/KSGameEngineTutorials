#pragma once

namespace ks
{
	namespace engine
	{
		void Init();
		void Tick();
		void Shutdown();
	}

	class IRHI;

	class FEngine
	{
	public:
		static FEngine* Create();
		~FEngine();
		void Init();
		void Tick();
		void Shutdown();
		void LoadMap();

	protected:
		IRHI* RHI = nullptr;
	};

	extern FEngine* GEngine;
	extern std::wstring GCmdLineArgs;
}