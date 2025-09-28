#pragma once
#include <mutex>
#include <string>
#include <thread>
#include <iostream>
#include "common.h"

enum class NarakaState {
	NARAKA_START,     // �ͻ�������
	NARAKA_END,      // wegame�ر�
	NARAKA_NAME,        // �Ծֿ�ʼ(�ͻ���������)
};

class NarakaStateMonitor {
private:
	NarakaState currentState;
	std::mutex stateMutex;

	// ���̼�����
	bool wasProcessRunning = false;
	const std::wstring narakaProcessName = L"NarakaBladepoint.exe";


public:
	NarakaStateMonitor() {}
		//: currentState(NarakaState::WEGAME_CLOSED) {}
		;
	// �����ѭ��
	void MonitorLoop() {
		while (true) {
			CheckStateTransition();
			std::this_thread::sleep_for(std::chrono::seconds(1));
		}
	}

	// ���״̬ת��
	void CheckStateTransition() {
		bool isNarakaRunning = IsProcessRunning(narakaProcessName);

		// ״̬ת���߼�
		if (!wasProcessRunning && isNarakaRunning) {
		//if (!isNarakaRunning && !wasProcessRunning) {
			HandleStateChange(NarakaState::NARAKA_START);
		}
		else if (wasProcessRunning && !isNarakaRunning) {
			HandleStateChange(NarakaState::NARAKA_END);
		}
		
		// ����״̬��¼
		wasProcessRunning = isNarakaRunning;
	
	
	}


	// ״̬�仯����
	void HandleStateChange(NarakaState newState) {
		NarakaState oldState = currentState;
		currentState = newState;

		// ��¼״̬ת��
		std::cout << "Val:State changed from " << StateToString(oldState)
			<< " to " << StateToString(newState) << std::endl;

		// ����״̬ת��ִ����Ӧ����
		switch (newState) {
		case NarakaState::NARAKA_START:
			OnClientStarted();
			break;
		case NarakaState::NARAKA_END:
			OnClientClosed();
			break;

		}
	}

	// ״̬�ַ�����ʾ
	std::string StateToString(NarakaState state) {
		switch (state) {
		case NarakaState::NARAKA_START: return "�����޼���Ϸ����";
		case NarakaState::NARAKA_END: return "�����޼���Ϸ�ر�";
		default: return "Unknown";
		}
	}

	// ����״̬�Ĵ�����
	void OnClientStarted();

	void OnClientClosed();

};
