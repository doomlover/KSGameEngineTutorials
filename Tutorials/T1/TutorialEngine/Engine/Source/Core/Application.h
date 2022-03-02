#pragma once

namespace ks
{
	class FEngine;
}

class KS_API IApp
{
	friend ks::FEngine;
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

	ks::FEngine* Engine = nullptr;
	// IApp暂时记录Config参数
	std::wstring StartMap;
};

extern KS_API IApp* GApp;