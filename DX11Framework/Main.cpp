#include <windows.h>
#include "DX11Framework.h"

//Dependencies:user32.lib;d3d11.lib;d3dcompiler.lib;dxgi.lib;

int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	DX11Framework application = DX11Framework();

	if (FAILED(application.Initialise(hInstance, nCmdShow)))
	{
		return -1;
	}

	// Main message loop
	MSG msg = { 0 };

	while (WM_QUIT != msg.message)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{

			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		}
		else
		{
			application.Update();
			application.Draw();
		}
	}

	return (int)msg.wParam;
}