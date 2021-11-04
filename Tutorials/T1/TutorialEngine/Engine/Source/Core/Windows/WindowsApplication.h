#pragma once

#include "Core/Application.h"

KS_API void Foo();

KS_API int WinMainEntry(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd);

class KS_API FApp : public IApp
{

public:
	FApp() {}
	virtual ~FApp() {}
	virtual void Init() override;
	virtual void Shutdown() override {}
	static void Foo();
};

//extern IApp* CreateApplication();