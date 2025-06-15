#include "pch.h"


#include "LoLStateMonitor.h"
#include "ThreadSafeLogger.h"
#include "lol.h"
#include "ThreadWrapper.h"
#include "lol_before.h"
#include <TlHelp32.h>


extern bool is_lol_running;
extern bool is_lol_game_running;
extern std::mutex g_mtx;
void LoLStateMonitor::OnNotRunning() {
    std::cout << "等待英雄联盟客户端运行" << std::endl;
    // 执行清理操作...
}
void LoLStateMonitor::OnClientStarted() {
	// 客户端启动时的处理逻辑
	LOG_IMMEDIATE(" 英雄联盟刚启动\n");
	std::cout << "处理客户端启动事件..." << std::endl;

	g_mtx.lock();
	is_lol_running = true;
	is_lol_game_running = false;
	g_mtx.unlock();
	_sendHttp_LOL("RUN", "");
	//// 启动线程

	std::thread t(pollRankNum);
	t.detach();
	//ThreadWrapper thread(pollRankNum);
	//thread.Start();
	// 等待线程初始化完成（根据实际情况调整条件）
	//while (thread.GetState() != ThreadWrapper::ThreadState::RUNNING) {
	//std::this_thread::sleep_for(std::chrono::seconds(1));
	//}

	//thread.Detach();
}

void LoLStateMonitor::OnClientClosed() {
	// 客户端关闭时的处理逻辑
	LOG_IMMEDIATE("客户端关闭");
	std::cout << "处理客户端关闭事件..." << std::endl;
	g_mtx.lock();
	is_lol_running = false;
	is_lol_game_running = false;
	g_mtx.unlock();

	//std::cout << "[" << GetCurrentTimeString() << "] 英雄联盟已关闭\n";
	LOG_IMMEDIATE(" 英雄联盟已关闭\n");
	_sendHttp_LOL("KILL", "");
}

void LoLStateMonitor::OnMatchStarted() {
	// 对局开始时的处理逻辑
	LOG_IMMEDIATE(" 英雄联盟对局开始,开始监视对局信息\n");

	g_mtx.lock();
	//is_lol_running = false;
	is_lol_game_running = true;
	g_mtx.unlock();

	//// 启动线程
	//ThreadWrapper thread(pollEvents);
	//thread.Start();
	//std::this_thread::sleep_for(std::chrono::seconds(1));
	//thread.Detach();

	std::thread poll(pollEvents);
	poll.detach();

	std::cout << "处理对局开始事件..." << std::endl;
}

void LoLStateMonitor::OnMatchEnded() {
	// 对局结束时的处理逻辑
	LOG_IMMEDIATE(" 英雄联盟对局结束\n");
	std::cout << "处理对局结束事件..." << std::endl;

	g_mtx.lock();
	is_lol_running = true;
	is_lol_game_running = false;
	g_mtx.unlock();

	Game_Before gb;
	// 获取 data
	gb.before_main("END");

	//gb.getAndSendInfo("END");


}