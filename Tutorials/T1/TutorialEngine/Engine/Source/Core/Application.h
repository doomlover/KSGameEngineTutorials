#pragma once

//#include "Core/CoreMinimal.h"

class KS_API IApp
{
public:
	virtual ~IApp() {}
	virtual void Init() {}
	virtual void Shutdown() {}
};

extern KS_API IApp* GApp;

#define IMPL_APP(x) IApp* CreateApplication() {return new x;}