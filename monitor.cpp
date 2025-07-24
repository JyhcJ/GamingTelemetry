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

//#include "ValStateMonitor.h"
//#include "py.h"
#include "cs2.h"
#include "ProcessMemoryReader.h"
#include "pubg_name.h"
#include "pubg.h"
#include "ValStateMonitor.h"

//extern bool lol_running;
//extern std::string BEFORE_STATE;
extern std::mutex g_mtx;
// 配置部分
const std::wstring LOL_PROCESS_NAME = L"LeagueClient.exe";
const std::wstring LOL_GAME_PROCESS_NAME = L"League of Legends.exe";
const int CHECK_INTERVAL_MS = 5000; // 检查间隔5秒

// 全局变量
std::chrono::system_clock::time_point lol_start_time;
bool is_lol_running = false;
bool is_lol_game_running = false;
std::string g_hostName;
double total_lol_time = 0.0;



// 获取当前时间字符串
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
	PROCESSENTRY32W pe32; // 注意使用 W 版本
	pe32.dwSize = sizeof(PROCESSENTRY32W);

	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnapshot == INVALID_HANDLE_VALUE) {
		/*std::wcerr << L"Failed to create process snapshot (Error: " << GetLastError() << L")" << std::endl;*/
		return 0;
	}

	if (Process32FirstW(hSnapshot, &pe32)) { // 使用宽字符版本 Process32FirstW
		do {
			if (_wcsicmp(pe32.szExeFile, processName.c_str()) == 0) { // 宽字符比较
				CloseHandle(hSnapshot);
				return pe32.th32ProcessID;
			}
		} while (Process32NextW(hSnapshot, &pe32)); // 使用宽字符版本
	}

	CloseHandle(hSnapshot);
	return 0; // 未找到进程
}

double GetProcessUptime(DWORD pid) {
	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
	if (hProcess == NULL) {
		//std::cerr << "Failed to open process (Error: " << GetLastError() << ")" << std::endl;
		return -1.0;
	}

	FILETIME createTime, exitTime, kernelTime, userTime;
	if (!GetProcessTimes(hProcess, &createTime, &exitTime, &kernelTime, &userTime)) {
		//std::cerr << "Failed to get process times (Error: " << GetLastError() << ")" << std::endl;
		CloseHandle(hProcess);
		return -1.0;
	}

	CloseHandle(hProcess);

	// 将 FILETIME 转换为 64 位整数
	ULARGE_INTEGER uliCreateTime;
	uliCreateTime.LowPart = createTime.dwLowDateTime;
	uliCreateTime.HighPart = createTime.dwHighDateTime;

	// 获取当前系统时间
	SYSTEMTIME currentSysTime;
	GetSystemTime(&currentSysTime);
	FILETIME currentFileTime;
	SystemTimeToFileTime(&currentSysTime, &currentFileTime);
	ULARGE_INTEGER uliCurrentTime;
	uliCurrentTime.LowPart = currentFileTime.dwLowDateTime;
	uliCurrentTime.HighPart = currentFileTime.dwHighDateTime;

	// 计算运行时间（单位：秒）
	double uptime = (uliCurrentTime.QuadPart - uliCreateTime.QuadPart) / 1e7; // 100ns → s
	return uptime;
}

// 主监控函数(线程)
void MonitorGameProcess() {
	g_mtx.lock();
	//g_hostName = gethostName();
	g_mtx.unlock();
	is_lol_running = true;

	while (true) {
		bool currently_running = IsProcessRunning(LOL_PROCESS_NAME);
		bool currently_game_running = IsProcessRunning(LOL_GAME_PROCESS_NAME);
		double runTime = GetProcessUptime(GetPidByName(LOL_PROCESS_NAME));
		//if (currently_running && !is_lol_running && !is_lol_game_running) {
		if (currently_running && !is_lol_game_running) {
			//if (currently_running ) {
				// 英雄联盟刚启动
				//lol_start_time = std::chrono::system_clock::now();


			if (runTime < 20.0)
			{
				LOG_IMMEDIATE(" 英雄联盟已启动\n");
				g_mtx.lock();
				//is_lol_running = true;
				//is_lol_game_running = false;
				g_mtx.unlock();
				_sendHttp_LOL("RUN", "");
				//// 启动线程
				ThreadWrapper thread(pollRankNum);
				thread.Start();
				// 等待线程初始化完成（根据实际情况调整条件）
				//while (thread.GetState() != ThreadWrapper::ThreadState::RUNNING) {
				std::this_thread::sleep_for(std::chrono::seconds(1));
				//}

				thread.Detach();
			}



			// 客户端启动后不断更新 段位和队伍人数(队伍人数要测试)
			// 对局结束后更新连胜并发送(发送总胜利数,连胜数)
			// 对局中发送信息(段位,队伍人数,对局ID,连杀累杀,对局模式)

		}
		//else if (!currently_running && is_lol_running && !is_lol_game_running) {
		else if (!currently_running && !is_lol_game_running) {
			//else if (!currently_running) {
				// 英雄联盟客户端关闭
			g_mtx.lock();
			//is_lol_running = false;
			is_lol_game_running = false;
			g_mtx.unlock();

			//std::cout << "[" << GetCurrentTimeString() << "] 英雄联盟已关闭\n";
			LOG_IMMEDIATE(" 英雄联盟已关闭\n");
			_sendHttp_LOL("KILL", "");
			// LOG_IMMEDIATE(std::string("本次游戏时长") + std::to_string(hours));
			// 如果需要上传时间

		}
		//else if (currently_game_running && !is_lol_game_running ) {
		//else if (currently_game_running && !is_lol_game_running && is_lol_running) {
		else if (currently_game_running && !is_lol_game_running) {
			//else if (currently_game_running ) {
				// 英雄联盟对局开始
			LOG_IMMEDIATE(" 英雄联盟对局已启动,开始监视对局信息\n");

			g_mtx.lock();
			//is_lol_running = false;
			is_lol_game_running = true;
			g_mtx.unlock();

			//// 启动线程
			ThreadWrapper thread(pollEvents);
			thread.Start();
			std::this_thread::sleep_for(std::chrono::seconds(1));
			thread.Detach();

		}
		//else if (!currently_game_running && is_lol_game_running && !is_lol_running) {
		else if (!currently_game_running && is_lol_game_running) {
			//else if (!currently_game_running) {



			/*	auto end_time = std::chrono::system_clock::now();
				std::chrono::duration<double> elapsed = end_time - lol_start_time;
				total_lol_time += elapsed.count();


				int hours = static_cast<int>(elapsed.count() / 3600);
				int minutes = static_cast<int>(fmod(elapsed.count(), 3600) / 60);
				int seconds = static_cast<int>(fmod(elapsed.count(), 60));*/

				//LOG_IMMEDIATE(" 英雄联盟对局结束?/ 掉线? / 重开?");
				//LOG_IMMEDIATE(" 英雄联盟对局状态:" + BEFORE_STATE);

				//查询滞留的 gameid 和 accountid 匹配则统计 或者已经做好了?


			Game_Before gb;
			// 获取 data
			gb.before_main("END");


			// 构建data ,不能通过 查询对局ID是否结束



		/*	std::cout << "本次游戏时长: "
				<< hours << "小时 "
				<< minutes << "分钟 "
				<< seconds << "秒\n";
			std::cout << "累计游戏时长: "
				<< static_cast<int>(total_lol_time / 3600) << "小时 "
				<< static_cast<int>(fmod(total_lol_time, 3600) / 60) << "分钟\n";*/

				// TODO 查询对局ID是否结束
				//if (BEFORE_STATE == "EndOfGame")
				//{	// 游戏结束 等待统计结果 (必须客户端执行结算界面才能捕获.)
				//	// 或者查询最近游戏中gameID是否结束???
				//	_sendHttp_LOL("END", "");
				//}

		}

		// 检查其他游戏是否运行 
		// if (IsProcessRunning(L"无畏契约?CSGO?绝地求生")) {
		//     std::cout << "检测到其他游戏正在运行\n";
		// }

		std::this_thread::sleep_for(std::chrono::milliseconds(CHECK_INTERVAL_MS));
	}
}

int main() {
	// 设置日志输出到文件
	ThreadSafeLogger::GetInstance().SetOutputFile("monitor.log");
#ifdef _DEBUG
	// Debug 模式
	ThreadSafeLogger::GetInstance().SetMinLogLevel(LogLevel::DEBUG1);
#else
	// Release 模式
	ThreadSafeLogger::GetInstance().SetMinLogLevel(LogLevel::INFO);
#endif

	LOG_IMMEDIATE("DLL监视程序已启动");
	//pymain();

	g_mtx.lock();
	g_hostName = WStringToString(GetComputerNameWString());
	std::cout << g_hostName << std::endl;
	g_mtx.unlock();
	try {
		std::thread monitor_val([]() {
			ValStateMonitor monitor_val;
			monitor_val.MonitorLoop();

			});
		monitor_val.detach();

		// 启动所有工作线程
		ProcessMonitor_PUBG monitor_pubg;
		monitor_pubg.start();
		//if (!EnableDebugPrivilege(TRUE)) {
		//	printf("[-] Failed to enable SeDebugPrivilege\n");
		//	return 1;
		//}

		//printf("[+] SeDebugPrivilege enabled: %d\n", IsDebugPrivilegeEnabled());

		//try {
		//	// 1. 创建读取器实例
		//	//ProcessMemoryReader reader(L"TslGame.exe");
	
		//
		//	//// 2. 定义指针链 [[[[1F1804A0060+0]+C0]+318]+18
		//	//uintptr_t baseAddress = 0x1F1804A0060;
		//	//std::vector<uintptr_t> offsets = { 0x0, 0xC0, 0x318, 0x18 };

		//	//// 3. 读取Unicode字符串
		//	//std::wstring result = reader.readUnicodeStringFromPointerChain(baseAddress, offsets);

		//	//// 4. 输出结果
		//	//std::wcout << L"Read value: " << result << std::endl;
		//}
		//catch (const std::exception& e) {
		//	std::cerr << "Error: " << e.what() << std::endl;
		//	return 1;
		//}

		//pubg_main();

		cs2Monitor();
		//ThreadWrapper thread([&monitor]() {
		//	monitor.MonitorLoop();
		//	});

		///*monitor.StartMonitoring();*/

		//thread.Start();

		//std::this_thread::sleep_for(std::chrono::seconds(1));
		//
		//thread.Detach();
		LoLStateMonitor monitor;
		//ValStateMonitor valMonitor;

		// 运行线程安全测试
		//ThreadSafeLogger::GetInstance().TestThreadSafety(100); // 测试5秒
		// 
		//ThreadSafeLogger::GetInstance().Log(LogLevel::INFO, "测试完毕!");
		// 
		//// 在单独线程中运行监控
		std::thread lolMonitorThread([&monitor]() {
			monitor.MonitorLoop();
			});


		/*std::thread valMonitorThread([&valMonitor]() {
			valMonitor.MonitorLoop();
			});*/
		//monitorThread.detach();

		// 主线程可以做其他事情
		while (true) {
			//loadDriver();
			//GENERAL_CONSTRUCTION gc_in = GENERAL_CONSTRUCTION();
			//GENERAL_CONSTRUCTION gc_out;
			//driverUpdate(gc_in, gc_out);
			//driverGetPlayerName(gc_in, gc_out);
			//std::wstring playerName = gc_out.PlayerName;
			//LOG_IMMEDIATE("gc_out:::" + WStringToString(gc_out.PlayerName));
			std::this_thread::sleep_for(std::chrono::seconds(10));
		}

		lolMonitorThread.join();
		//valMonitorThread.join();

		//std::thread([&monitor]() {
		//	monitor.MonitorLoop();
		//	}).detach();  // 分离线程

		//MonitorGameProcess();
	}
	catch (const std::exception& e) {
		//std::cerr << "发生错误: " << e.what() << std::endl;
		 //(e.what());
		LOG_ERROR(e.what());
		return 1;
	}
	catch (...) {  // 捕获其他所有异常
		LOG_IMMEDIATE_ERROR("main :::Unknown exception occurred");
	}
	//调试时备注掉
	return 0;
}


// 监视入口
extern "C" __declspec(dllexport) const int monitorLOL() {
	return main();
}

