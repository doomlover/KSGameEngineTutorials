#pragma once

#include "Core/Application.h"

int KS_API WinMainEntry(IApp* App, _In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd);

class KS_API FWinApp : public IApp
{
public:
	HINSTANCE hInstance;

public:
	FWinApp() {}
	virtual ~FWinApp();
	virtual void Init() override;
	virtual void Tick() override {}
	virtual void Shutdown() override {}
};

typedef FWinApp FGenericApp;