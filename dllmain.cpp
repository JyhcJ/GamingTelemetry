// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "pch.h"
#include "val.h"
#include "dllmain.h"
#include "cs2.h"

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
		cs2Cleanup();
		break;
	}
	return TRUE;
}


