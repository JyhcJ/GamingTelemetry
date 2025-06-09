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

		// 设置最低日志级别
		ThreadSafeLogger::GetInstance().SetMinLogLevel(LogLevel::INFO);

		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}
