#pragma once

#include "Core/Application.h"

namespace ks
{
	int KS_API WindowsEntry(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance,
		_In_ LPSTR lpCmdLine, _In_ int nShowCmd);

	extern HINSTANCE GHINSTANCE;

	class KS_API FWinApp : public IApp
	{
	public:
		int WindowWidth;
		int WindowHeight;

	public:
		FWinApp();
		virtual ~FWinApp();
		virtual void Init() override;
		//virtual void Tick() override {}
		//virtual void Shutdown() override {}

	private:
		int CreateAndShowWindow();
	};

	typedef FWinApp FGenericApp;
}