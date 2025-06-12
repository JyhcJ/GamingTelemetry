// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "pch.h"
#include "lol.h"
#include "thread"
#include "ThreadSafeLogger.h"
#include "common.h"

BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
)
{
	//SetConsoleOutputCP(CP_UTF8);
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		// 设置日志输出到文件
		ThreadSafeLogger::GetInstance().SetOutputFile("monitor.log");

	#ifdef _DEBUG
	// Debug 模式
		ThreadSafeLogger::GetInstance().SetMinLogLevel(LogLevel::DEBUG1);
	#else
	// Release 模式
		ThreadSafeLogger::GetInstance().SetMinLogLevel(LogLevel::INFO);
	#endif
		

		
	
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}
