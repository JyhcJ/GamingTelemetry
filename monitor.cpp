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

//extern bool lol_running;
//extern std::string BEFORE_STATE;
// 配置部分
const std::wstring LOL_PROCESS_NAME = L"LeagueClient.exe";
const std::wstring LOL_GAME_PROCESS_NAME = L"League of Legends.exe";
const int CHECK_INTERVAL_MS = 5000; // 检查间隔5秒

// 全局变量
std::chrono::system_clock::time_point lol_start_time;
bool is_lol_running = false;
bool is_lol_game_running = false;
double total_lol_time = 0.0;

// 检查指定进程是否在运行
bool IsProcessRunning(const std::wstring& processName) {
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnapshot == INVALID_HANDLE_VALUE) {
		return false;
	}

	PROCESSENTRY32W pe32;
	pe32.dwSize = sizeof(PROCESSENTRY32W);

	if (!Process32FirstW(hSnapshot, &pe32)) {
		CloseHandle(hSnapshot);
		return false;
	}

	bool found = false;
	do {
		if (std::wstring(pe32.szExeFile) == processName) {
			found = true;
			break;
		}
	} while (Process32NextW(hSnapshot, &pe32));

	CloseHandle(hSnapshot);
	return found;
}

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



// 主监控函数(线程)
void MonitorGameProcess() {
	while (true) {
		bool currently_running = IsProcessRunning(LOL_PROCESS_NAME);
		bool currently_game_running = IsProcessRunning(LOL_GAME_PROCESS_NAME);

		if (currently_running && !is_lol_running && !is_lol_game_running) {
			// 英雄联盟刚启动
			//lol_start_time = std::chrono::system_clock::now();
			is_lol_running = true;
			is_lol_game_running = false;
			LOG_IMMEDIATE(" 英雄联盟已启动\n");

			_sendHttp_LOL("RUN", "");
			//// 启动线程
			ThreadWrapper thread(pollRankNum);
			thread.Start();
			std::this_thread::sleep_for(std::chrono::seconds(1));
			thread.Detach();

			// 客户端启动后不断更新 段位和队伍人数(队伍人数要测试)
			// 对局结束后更新连胜并发送(发送总胜利数,连胜数)
			// 对局中发送信息(段位,队伍人数,对局ID,连杀累杀,对局模式)

		}
		else if (!currently_running && is_lol_running) {
			// 英雄联盟客户端关闭
			is_lol_running = false;
			is_lol_game_running = false;
			//std::cout << "[" << GetCurrentTimeString() << "] 英雄联盟已关闭\n";
			LOG_IMMEDIATE(" 英雄联盟已关闭\n");
			_sendHttp_LOL("KILL", "");
			// LOG_IMMEDIATE(std::string("本次游戏时长") + std::to_string(hours));
			// 如果需要上传时间
			
		}
		else if (currently_game_running && !is_lol_game_running) {
			// 英雄联盟对局开始
			LOG_IMMEDIATE(" 英雄联盟对局已启动,开始监视对局信息\n");
			is_lol_running = false;
			is_lol_game_running = true;
			
			//// 启动线程
			ThreadWrapper thread(pollEvents);
			thread.Start();
			std::this_thread::sleep_for(std::chrono::seconds(1));
			thread.Detach();
		
		}
		else if (!currently_game_running && is_lol_game_running) {
			is_lol_running = false;
			is_lol_game_running = false;

		/*	auto end_time = std::chrono::system_clock::now();
			std::chrono::duration<double> elapsed = end_time - lol_start_time;
			total_lol_time += elapsed.count();
		

			int hours = static_cast<int>(elapsed.count() / 3600);
			int minutes = static_cast<int>(fmod(elapsed.count(), 3600) / 60);
			int seconds = static_cast<int>(fmod(elapsed.count(), 60));*/

			LOG_IMMEDIATE(" 英雄联盟对局结束?/ 掉线? / 重开?");
			//LOG_IMMEDIATE(" 英雄联盟对局状态:" + BEFORE_STATE);
			_sendHttp_LOL("END","");
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
	LOG_IMMEDIATE("DLL监视程序已启动");

	try {
		//// 启动线程
		ThreadWrapper thread(MonitorGameProcess);
		thread.Start();
		std::this_thread::sleep_for(std::chrono::seconds(1));
		thread.Detach();

		//MonitorGameProcess();
	}
	catch (const std::exception& e) {
		//std::cerr << "发生错误: " << e.what() << std::endl;
		 //(e.what());
		LOG_ERROR(e.what());
		return 1;;
	}
	catch (...) {  // 捕获其他所有异常
		LOG_IMMEDIATE_ERROR("main :::Unknown exception occurred");
	}
	return 0;
}


// 监视入口
extern "C" __declspec(dllexport) const int monitorLOL() {
	return main();
}