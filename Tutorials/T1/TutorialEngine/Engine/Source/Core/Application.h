#pragma once

class KS_API IApp
{
public:
	virtual ~IApp() {}
	virtual void Init() {
		OnInit();
	}
	virtual void Tick() {
		OnTick();
	}
	virtual void Shutdown() {
		OnShutdown();
	}

protected:
	virtual void OnInit() = 0;
	virtual void OnTick() = 0;
	virtual void OnShutdown() = 0;
};

extern KS_API IApp* GApp;