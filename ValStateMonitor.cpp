#include "pch.h"
#include "ThreadSafeLogger.h"
#include <iostream>
#include "ValStateMonitor.h"
#include "val.h"

void ValStateMonitor::OnNotRunning() {
	std::cout << "等待运行" << std::endl;
	// 执行清理操作...
}
void ValStateMonitor::OnClientStarted() {
	// 客户端启动时的处理逻辑
	LOG_IMMEDIATE(" Wegame启动\n");
	std::cout << "处理客户端启动事件..." << std::endl;
	//updateHeader();
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
	_sendHttp_Val("RUN", "");

	// 
	updateHeader();



}

void ValStateMonitor::OnMatchEnded() {
	// 对局结束时的处理逻辑
	LOG_IMMEDIATE(" Val结束\n");
	_sendHttp_Val("KILL", "");
	//结束后确保代理关闭即可
}

void ValStateMonitor::OnWegameLogin() {
	//
	LOG_IMMEDIATE("wegame登录\n");
	updateHeader();
	std::this_thread::sleep_for(std::chrono::seconds(5));
	getValinfo2send();
}

void ValStateMonitor::OnMatchOver() {
	// 对局结束时的处理逻辑
	LOG_IMMEDIATE(" 对局结束,战绩统计\n");
	getValinfo2send();
	//结束后确保代理关闭即可
}

