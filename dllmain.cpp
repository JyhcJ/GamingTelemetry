// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "pch.h"
#include "lol.h"
#include "thread"
#include "ThreadSafeLogger.h"
#include "common.h"
#include "val.h"
#include "dllmain.h"

BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
)
{
	ThreadSafeLogger::GetInstance().SetOutputFile("monitor111.log");

	//SetConsoleOutputCP(CP_UTF8);
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		// 设置日志输出到文件
		

	#ifdef _DEBUG
	// Debug 模式
		ThreadSafeLogger::GetInstance().SetMinLogLevel(LogLevel::DEBUG1);
	#else
	// Release 模式
		ThreadSafeLogger::GetInstance().SetMinLogLevel(LogLevel::INFO);
	#endif
		

		
	
		break;
	case DLL_THREAD_ATTACH:break;
	case DLL_THREAD_DETACH:break;
	case DLL_PROCESS_DETACH:
		cleanUp();
		break;
	}
	return TRUE;
}


void cleanUp() {

	MitmDumpController::getInstance().stop();
}