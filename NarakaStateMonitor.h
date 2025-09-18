#pragma once
#include <mutex>
#include <string>
#include <thread>
#include <iostream>
#include "common.h"

enum class NarakaState {
	NARAKA_START,     // 客户端启动
	NARAKA_END,      // wegame关闭
	NARAKA_NAME,        // 对局开始(客户端运行中)
};

class NarakaStateMonitor {
private:
	NarakaState currentState;
	std::mutex stateMutex;

	// 进程检测相关
	bool wasProcessRunning = false;
	const std::wstring narakaProcessName = L"NarakaBladepoint.exe";


public:
	NarakaStateMonitor() {}
		//: currentState(NarakaState::WEGAME_CLOSED) {}
		;
	// 主监控循环
	void MonitorLoop() {
		while (true) {
			CheckStateTransition();
			std::this_thread::sleep_for(std::chrono::seconds(1));
		}
	}

	// 检测状态转换
	void CheckStateTransition() {
		bool isNarakaRunning = IsProcessRunning(narakaProcessName);

		// 状态转换逻辑
		if (!wasProcessRunning && isNarakaRunning) {
		//if (!isNarakaRunning && !wasProcessRunning) {
			HandleStateChange(NarakaState::NARAKA_START);
		}
		else if (wasProcessRunning && !isNarakaRunning) {
			HandleStateChange(NarakaState::NARAKA_END);
		}
		
		// 更新状态记录
		wasProcessRunning = isNarakaRunning;
	
	
	}


	// 状态变化处理
	void HandleStateChange(NarakaState newState) {
		NarakaState oldState = currentState;
		currentState = newState;

		// 记录状态转换
		std::cout << "Val:State changed from " << StateToString(oldState)
			<< " to " << StateToString(newState) << std::endl;

		// 根据状态转换执行相应操作
		switch (newState) {
		case NarakaState::NARAKA_START:
			OnClientStarted();
			break;
		case NarakaState::NARAKA_END:
			OnClientClosed();
			break;

		}
	}

	// 状态字符串表示
	std::string StateToString(NarakaState state) {
		switch (state) {
		case NarakaState::NARAKA_START: return "永劫无间游戏运行";
		case NarakaState::NARAKA_END: return "永劫无间游戏关闭";
		default: return "Unknown";
		}
	}

	// 各个状态的处理函数
	void OnClientStarted();

	void OnClientClosed();

};
