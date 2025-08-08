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

// ���ò���
const std::wstring LOL_PROCESS_NAME = L"LeagueClient.exe";
const std::wstring LOL_GAME_PROCESS_NAME = L"League of Legends.exe";
const int CHECK_INTERVAL_MS = 5000;

// ȫ�ֱ���
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

	_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);  // ��������������������ǵ���
	_CrtSetReportHook([](int reportType, char* msg, int* retVal) {
		OutputDebugStringA(msg);  // �ض��򵽵������
		*retVal = 1;  // ����Ĭ�ϵ���
		return TRUE;
		});


	// ������־������ļ�
	ThreadSafeLogger::GetInstance().SetOutputFile("monitorDLL.log");
#ifdef _DEBUG
	ThreadSafeLogger::GetInstance().SetMinLogLevel(LogLevel::DEBUG1);
	LOG_IMMEDIATE("Debugģʽ");
#else
	ThreadSafeLogger::GetInstance().SetMinLogLevel(LogLevel::INFO);
	LOG_IMMEDIATE("Releaseģʽ");
#endif

	LOG_IMMEDIATE("DLL���ӳ���������");

	g_hostName = WStringToString(GetComputerNameWString());

	LOG_INFO("����˭������!!!");
	//std::string str = "{\"juediqiusheng\":{\"offset\":[283363440,24,1232,0],\"type\":\"module\",\"value\":\"TslGame.exe\"},\"yongjiewujian\":{\"token\":\"2gYhJUz6USFQV5Lx3iG7q1XktvCHWmzMJT_QepHr\"}}";
	//nlohmann::json jsonData = nlohmann::json::parse(str);
	//LOG_IMMEDIATE("��ʵ:        " + jsonData.dump());
	//LOG_IMMEDIATE(jsonData.dump() + "cjmofang.com.");
	//LOG_IMMEDIATE(generate_md5(jsonData.dump() + "cjmofang.com."));

	try {

		main_delta();

		//���������������
		ProcessMonitor_PUBG monitor_pubg;
		monitor_pubg.start();

		//�����޼�
		std::thread([]() {
			NarakaStateMonitor monitor_naraka;
			monitor_naraka.MonitorLoop();
			}).detach();


		// ������η��Լ����߳�
		std::thread([]() {
			ValStateMonitor monitor_val;
			monitor_val.MonitorLoop();
			}).detach();

		// ����CS2���
		cs2Monitor();

		// ����Ӣ�����˼��
		LoLStateMonitor monitor;
		std::thread([&monitor]() {
			monitor.MonitorLoop();
			}).detach();



		// ��ѭ��
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
	LOG_IMMEDIATE("��ʼ��g_domain: " + WStringToString(get_g_domain()));
	return main();
}
