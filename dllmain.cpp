// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "pch.h"
#include "val.h"
#include "dllmain.h"
#include "cs2.h"
#include "pubg_name.h"

BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
)
{

	//SetConsoleOutputCP(CP_UTF8);
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		break;
	case DLL_THREAD_ATTACH:break;
	case DLL_THREAD_DETACH:break;
	case DLL_PROCESS_DETACH:
		StopDriver(L"KMDFDriver2");
		cs2Cleanup();
		std::this_thread::sleep_for(std::chrono::seconds(1));
		break;
	}
	return TRUE;
}


