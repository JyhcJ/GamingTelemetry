#include "pch.h"
#include "ThreadSafeLogger.h"
#include <iostream>
#include "ValStateMonitor.h"
#include "val.h"

// 还是要hookwegame 使其无法关闭!
LogAnalyzer g_la;

std::atomic<bool> g_readlog(false);

static std::wstring wgPath;

void ValStateMonitor::OnNotRunning() {
	std::cout << "等待运行" << std::endl;
	// 执行清理操作...
}
void ValStateMonitor::OnClientStarted() {
	// 客户端启动时的处理逻辑
	LOG_IMMEDIATE(" Wegame启动");
	//injectDLL(L"wegame.exe", L"wgHelper.dll");
			// 开启代理
	//startTempProxy();
	//// 查战绩
	//getValinfo2send();
}

void ValStateMonitor::OnClientClosed() {
	LOG_IMMEDIATE("wegame关闭");
}

void ValStateMonitor::OnMatchStarted() {
	// 1分钟
	LOG_IMMEDIATE("VAL开始\n");
	_sendHttp_Val("RUN", "");

	////TODO 
	//startTempProxy();
	//// 查战绩
	//getValinfo2send();


	try {
		g_readlog.store(true);
		std::thread valMatchThread([this]() {
			bool isFirst = true;
			while (g_readlog) {
				if (g_la.checkMatchEnd(isFirst)) {
					ValStateMonitor::HandleStateChange(ValState::VAL_OVER);
				}
				std::this_thread::sleep_for(std::chrono::milliseconds(10000));
				LOG_IMMEDIATE_DEBUG("间隔检查...");
			}
			});
		valMatchThread.detach();

	}
	catch (...) {
		LOG_IMMEDIATE("OnClientStarted::::读取日志捕获错误");
	}

}

void ValStateMonitor::OnMatchEnded() {
	LOG_IMMEDIATE(" Val结束\n");
	g_readlog.store(false);
	_sendHttp_Val("KILL", "");
	//结束后确保代理关闭即可
}

void ValStateMonitor::OnWegameLogin() {
	//_sendHttp_Val("RUN", "");
	// 代理7秒 登录后10秒内?
	LOG_IMMEDIATE(" WEGAME登录后\n");
	wgPath = stringTOwstring( GetWGPath_REG());
	GetPath_REG(
		HKEY_CURRENT_USER,
		L"SOFTWARE\\Tencent\\valorant.live",
		L"InstallLocation");
	//injectDLL(L"notepad.exe", L"getInfo.dll");
	//injectDLL(L"wegame.exe", L"wgHelper.dll");
	//startTempProxy();
	//std::this_thread::sleep_for(std::chrono::seconds(10));
	////TODO测试后删除
	//getValinfo2send();
	//startTempProxy();
	//// 查战绩
	//getValinfo2send();

}

void ValStateMonitor::OnMatchOver() {
	LOG_IMMEDIATE(" 对局结束,战绩统计\n");

	//g_readlog.store(false);

	std::this_thread::sleep_for(std::chrono::seconds(10));

	startTempProxy();
	// 查战绩
	getValinfo2send();
	//结束后确保代理关闭即可
}


void ValStateMonitor::OnInLogin() {
	LOG_IMMEDIATE(" 注销,切换账号\n");
	//TODO确保关闭代理

}

std::wstring getWGPath() {

	return wgPath;
}