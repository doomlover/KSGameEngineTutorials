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
		static bool bQuit;
		static void PeekMessages();
		static bool RequestExit();

		FWinApp();
		virtual ~FWinApp();
		virtual void Init() override;
		HWND GetHWindow() { return hWnd; }

	private:
		HWND hWnd;
		int CreateAndShowWindow();
	};

	typedef FWinApp FGenericApp;
}