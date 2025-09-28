#include "pch.h"
#include <windows.h>
#include <tlhelp32.h>
#include <iostream>
#include <chrono>
#include <thread>
#include <string>
#include <ctime>
#include "monitor.h"
#include "common.h"
#include "lol.h"
#include "ThreadWrapper.h"
#include "ThreadSafeLogger.h"
#include "lol_before.h"
#include "LoLStateMonitor.h"
#include "cs2.h"
#include "ProcessMemoryReader.h"
#include "pubg.h"
#include "ValStateMonitor.h"
#include "NarakaStateMonitor.h"
#include "delta.h"


bool is_lol_running = false;

// 配置部分
const std::wstring LOL_PROCESS_NAME = L"LeagueClient.exe";
const std::wstring LOL_GAME_PROCESS_NAME = L"League of Legends.exe";
const int CHECK_INTERVAL_MS = 5000;

// 全局变量

std::vector<std::thread> g_monitorThreads;
std::atomic<bool> g_shouldExit{ false };
std::atomic<bool> g_isRunning{ false };
HANDLE g_mainThread = NULL;
DWORD g_mainThreadId = 0;

bool is_lol_game_running = false;
std::string g_hostName;

std::string GetCurrentTimeString() {
	auto now = std::chrono::system_clock::now();
	auto in_time_t = std::chrono::system_clock::to_time_t(now);
	std::tm tm;
	localtime_s(&tm, &in_time_t);

	char buffer[80];
	strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &tm);
	return std::string(buffer);
}

DWORD GetPidByName(const std::wstring& processName) {
	PROCESSENTRY32W pe32;
	pe32.dwSize = sizeof(PROCESSENTRY32W);

	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnapshot == INVALID_HANDLE_VALUE) {
		return 0;
	}

	if (Process32FirstW(hSnapshot, &pe32)) {
		do {
			if (_wcsicmp(pe32.szExeFile, processName.c_str()) == 0) {
				CloseHandle(hSnapshot);
				return pe32.th32ProcessID;
			}
		} while (Process32NextW(hSnapshot, &pe32));
	}

	CloseHandle(hSnapshot);
	return 0;
}

double GetProcessUptime(DWORD pid) {
	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
	if (hProcess == NULL) {
		return -1.0;
	}

	FILETIME createTime, exitTime, kernelTime, userTime;
	if (!GetProcessTimes(hProcess, &createTime, &exitTime, &kernelTime, &userTime)) {
		CloseHandle(hProcess);
		return -1.0;
	}

	CloseHandle(hProcess);

	ULARGE_INTEGER uliCreateTime;
	uliCreateTime.LowPart = createTime.dwLowDateTime;
	uliCreateTime.HighPart = createTime.dwHighDateTime;

	SYSTEMTIME currentSysTime;
	GetSystemTime(&currentSysTime);
	FILETIME currentFileTime;
	SystemTimeToFileTime(&currentSysTime, &currentFileTime);
	ULARGE_INTEGER uliCurrentTime;
	uliCurrentTime.LowPart = currentFileTime.dwLowDateTime;
	uliCurrentTime.HighPart = currentFileTime.dwHighDateTime;

	return (uliCurrentTime.QuadPart - uliCreateTime.QuadPart) / 1e7;
}

int main() {

	_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);  // 将错误输出到调试器而非弹窗
	_CrtSetReportHook([](int reportType, char* msg, int* retVal) {
		OutputDebugStringA(msg);  // 重定向到调试输出
		*retVal = 1;  // 抑制默认弹窗
		return TRUE;
		});




	// 设置日志输出到文件
	ThreadSafeLogger::GetInstance().SetOutputFile("monitorDLL.log");
#ifdef _DEBUG
	ThreadSafeLogger::GetInstance().SetMinLogLevel(LogLevel::DEBUG1);
	LOG_IMMEDIATE("Debug模式");
#else
	ThreadSafeLogger::GetInstance().SetMinLogLevel(LogLevel::INFO);
	LOG_IMMEDIATE("Release模式");
#endif

	// 记录主线程信息
	g_mainThread = GetCurrentThread();
	g_mainThreadId = GetCurrentThreadId();
	g_shouldExit = false;
	g_isRunning = true;


	LOG_IMMEDIATE("DLL监视程序已启动");

	g_hostName = WStringToString(GetComputerNameWString());

	LOG_INFO("天下谁与争锋!!!");
	//std::string str = "{\"juediqiusheng\":{\"offset\":[283363440,24,1232,0],\"type\":\"module\",\"value\":\"TslGame.exe\"},\"yongjiewujian\":{\"token\":\"2gYhJUz6USFQV5Lx3iG7q1XktvCHWmzMJT_QepHr\"}}";
	//std::string str = "{\"juediqiusheng\":{\"offset\":[283363440,24,1232,0],\"type\":\"module\",\"value\":\"TslGame.exe\"},\"yongjiewujian\":{\"token\":\"L2DgCvsWBq2p3df9J0U2fvqq4PhPDO4qAbBnkdTZ\"}}";
	//nlohmann::json jsonData = nlohmann::json::parse(str);
	//LOG_IMMEDIATE("真实:        " + jsonData.dump());
	//LOG_IMMEDIATE(jsonData.dump() + "cjmofang.com.");
	//LOG_IMMEDIATE(generate_md5(jsonData.dump() + "cjmofang.com."));
	try {

		std::thread delta_thread([]() {
			main_delta();
			});
		if (delta_thread.joinable()) {
				g_monitorThreads.push_back(std::move(delta_thread));
		}

		//启动绝地求生监控
		std::thread pubg_thread([]() {
	
			ProcessMonitor_PUBG monitor_pubg;
			monitor_pubg.start();
			});
		if (pubg_thread.joinable()) {
			g_monitorThreads.push_back(std::move(pubg_thread));
		}


		// 永劫无间监控线程
		std::thread naraka_thread([]() {
			NarakaStateMonitor monitor_naraka;
			monitor_naraka.MonitorLoop();
			});
		if (naraka_thread.joinable()) {
			g_monitorThreads.push_back(std::move(naraka_thread));
		}

		// 无畏契约监控线程
		std::thread val_thread([]() {
			ValStateMonitor monitor_val;
			monitor_val.MonitorLoop();
			});
		if (val_thread.joinable()) {
			g_monitorThreads.push_back(std::move(val_thread));
		}

		// CS2监控
		std::thread cs2_thread([]() {
			cs2Monitor();
			});
		if (cs2_thread.joinable()) {
			g_monitorThreads.push_back(std::move(cs2_thread));
		}

		// 英雄联盟监控线程
		std::thread lol_thread([]() {
			LoLStateMonitor monitor;
			monitor.MonitorLoop();
			});
		if (lol_thread.joinable()) {
			g_monitorThreads.push_back(std::move(lol_thread));
		}

		// 主循环
		while (!g_shouldExit) {
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			//exitMonitor();
		}

		// 等待所有线程结束（设置超时）
		for (auto& thread : g_monitorThreads) {
			if (thread.joinable()) {
				// 使用超时等待，避免无限阻塞
				// 注意：std::thread没有内置超时机制，这里仅示意
				// 实际应用中可能需要使用其他同步机制
			}
		}
		//g_monitorThreads.clear();
	}
	catch (const std::exception& e) {
		LOG_EXCEPTION_WITH_STACK(e);
		g_isRunning = false;
		return 1;
	}
	catch (...) {
		LOG_IMMEDIATE_ERROR("main :::Unknown exception occurred");
		g_isRunning = false;
	}
	g_isRunning = false;
	return 0;
}

extern "C" __declspec(dllexport) const int monitorLOL() {
	return main();
}
extern "C" __declspec(dllexport) const int monitorLOL1(int index) {
	if (index == 2) {
		set_g_domain(L"asz.cjmofang.com");
	}
	else {
		set_g_domain(L"dev-asz.cjmofang.com");
	}
	LOG_IMMEDIATE("初始化g_domain: " + WStringToString(get_g_domain()));
	return main();
}

extern "C" __declspec(dllexport) const void exitMonitor() {
	// 设置退出标志
	g_shouldExit = true;

	// 给线程一些时间正常退出
	std::this_thread::sleep_for(std::chrono::milliseconds(500));

	// 强制终止所有创建的线程
	for (auto& thread : g_monitorThreads) {
		if (thread.joinable()) {
			// 获取线程native handle并强制终止
			// 注意：C++ std::thread不直接支持TerminateThread
			// 需要使用Windows API
		}
	}

	// 清理线程容器
	//g_monitorThreads.clear();

	// 如果有其他需要清理的资源，在这里添加

	// 重置状态
	g_isRunning = false;

	// 确保日志被刷新
	ThreadSafeLogger::GetInstance().Flush();
	return;
}