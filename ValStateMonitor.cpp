#include "pch.h"
#include "ThreadSafeLogger.h"
#include <iostream>
#include "ValStateMonitor.h"
#include "val.h"

// ����Ҫhookwegame ʹ���޷��ر�!
LogAnalyzer g_la;

std::atomic<bool> g_readlog(false);
void ValStateMonitor::OnNotRunning() {
	std::cout << "�ȴ�����" << std::endl;
	// ִ���������...
}
void ValStateMonitor::OnClientStarted() {
	// �ͻ�������ʱ�Ĵ����߼�
	LOG_IMMEDIATE(" Wegame����");
	//injectDLL(L"wegame.exe", L"wgHelper.dll");
}

void ValStateMonitor::OnClientClosed() {
	LOG_IMMEDIATE("wegame�ر�");
}

void ValStateMonitor::OnMatchStarted() {
	// 1����
	LOG_IMMEDIATE("VAL��ʼ\n");
	_sendHttp_Val("RUN", "");
	try {
		g_readlog.store(true);
		std::thread valMatchThread([this]() {
			while (g_readlog) {

				if (g_la.checkMatchEnd()) {
					ValStateMonitor::HandleStateChange(ValState::VAL_OVER);
				}
				std::this_thread::sleep_for(std::chrono::milliseconds(10000));
			}
			});
		valMatchThread.detach();
	}
	catch (...) {
		LOG_IMMEDIATE("OnClientStarted::::��ȡ��־�������");
	}

}

void ValStateMonitor::OnMatchEnded() {
	LOG_IMMEDIATE(" Val����\n");
	_sendHttp_Val("KILL", "");
	//������ȷ������رռ���
}

void ValStateMonitor::OnWegameLogin() {
	_sendHttp_Val("RUN", "");
	// ����7�� ��¼��10����?
	LOG_IMMEDIATE(" WEGAME��¼��\n");
	//injectDLL(L"notepad.exe", L"getInfo.dll");
	//injectDLL(L"wegame.exe", L"wgHelper.dll");
	startTempProxy();

	std::this_thread::sleep_for(std::chrono::seconds(10));
	////TODO���Ժ�ɾ��
	getValinfo2send();
}

void ValStateMonitor::OnMatchOver() {
	LOG_IMMEDIATE(" �Ծֽ���,ս��ͳ��\n");
	g_readlog.store(false);
	getValinfo2send();
	//������ȷ������رռ���
}


void ValStateMonitor::OnInLogin() {
	LOG_IMMEDIATE(" ע��,�л��˺�\n");
	//TODOȷ���رմ���

}