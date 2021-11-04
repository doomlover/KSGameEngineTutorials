#pragma once

class KS_API IApp
{
public:
	virtual ~IApp() {}
	virtual void Init() = 0;
	virtual void Tick() = 0;
	virtual void Shutdown() = 0;
};

extern KS_API IApp* GApp;