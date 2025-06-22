#include "pch.h"
#include "ThreadSafeLogger.h"
#include <iostream>
#include "ValStateMonitor.h"
#include "val.h"


void ValStateMonitor::OnNotRunning() {
	std::cout << "等待英雄联盟客户端运行" << std::endl;
	// 执行清理操作...
}
void ValStateMonitor::OnClientStarted() {
	// 客户端启动时的处理逻辑
	LOG_IMMEDIATE(" Wegame启动\n");
	std::cout << "处理客户端启动事件..." << std::endl;
	updateMatch();
}

void ValStateMonitor::OnClientClosed() {
	// 上报
	LOG_IMMEDIATE("客户端关闭");
	std::cout << "处理客户端关闭事件..." << std::endl;


	//std::cout << "[" << GetCurrentTimeString() << "] 英雄联盟已关闭\n";
	//_sendHttp_LOL("KILL", "");
}

void ValStateMonitor::OnMatchStarted() {
	// 1分钟
	LOG_IMMEDIATE("VAL开始\n");
	updateMatch();



}

void ValStateMonitor::OnMatchEnded() {
	// 对局结束时的处理逻辑
	LOG_IMMEDIATE(" Val结束\n");
	std::cout << "处理对局结束事件..." << std::endl;



	//gb.getAndSendInfo("END");


}