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

	LOG_IMMEDIATE("DLL监视程序已启动");

	g_hostName = WStringToString(GetComputerNameWString());

	LOG_INFO("天下谁与争锋!!!");
	//std::string str = "{\"juediqiusheng\":{\"offset\":[283363440,24,1232,0],\"type\":\"module\",\"value\":\"TslGame.exe\"},\"yongjiewujian\":{\"token\":\"2gYhJUz6USFQV5Lx3iG7q1XktvCHWmzMJT_QepHr\"}}";
	//nlohmann::json jsonData = nlohmann::json::parse(str);
	//LOG_IMMEDIATE("真实:        " + jsonData.dump());
	//LOG_IMMEDIATE(jsonData.dump() + "cjmofang.com.");
	//LOG_IMMEDIATE(generate_md5(jsonData.dump() + "cjmofang.com."));

	try {

		main_delta();

		//启动绝地求生监控
		ProcessMonitor_PUBG monitor_pubg;
		monitor_pubg.start();

		//永劫无间
		std::thread([]() {
			NarakaStateMonitor monitor_naraka;
			monitor_naraka.MonitorLoop();
			}).detach();


		// 启动无畏契约监控线程
		std::thread([]() {
			ValStateMonitor monitor_val;
			monitor_val.MonitorLoop();
			}).detach();

		// 启动CS2监控
		cs2Monitor();

		// 启动英雄联盟监控
		LoLStateMonitor monitor;
		std::thread([&monitor]() {
			monitor.MonitorLoop();
			}).detach();



		// 主循环
		while (true) {
			std::this_thread::sleep_for(std::chrono::seconds(10));
		}
	}
	catch (const std::exception& e) {
		LOG_ERROR(e.what());
		return 1;
	}
	catch (...) {
		LOG_IMMEDIATE_ERROR("main :::Unknown exception occurred");
	}

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
