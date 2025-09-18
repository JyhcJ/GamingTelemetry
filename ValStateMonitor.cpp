#include "pch.h"
#include "ThreadSafeLogger.h"
#include <iostream>
#include "ValStateMonitor.h"
#include "val.h"

// ����Ҫhookwegame ʹ���޷��ر�!
LogAnalyzer g_la;

std::atomic<bool> g_readlog(false);

static std::wstring wgPath;

void ValStateMonitor::OnNotRunning() {
	std::cout << "�ȴ�����" << std::endl;
	// ִ���������...
}
void ValStateMonitor::OnClientStarted() {
	// �ͻ�������ʱ�Ĵ����߼�
	LOG_IMMEDIATE(" Wegame����");
	//injectDLL(L"wegame.exe", L"wgHelper.dll");
			// ��������
	//startTempProxy();
	//// ��ս��
	//getValinfo2send();
}

void ValStateMonitor::OnClientClosed() {
	LOG_IMMEDIATE("wegame�ر�");
}

void ValStateMonitor::OnMatchStarted() {
	// 1����
	LOG_IMMEDIATE("VAL��ʼ\n");
	_sendHttp_Val("RUN", "");

	////TODO 
	//startTempProxy();
	//// ��ս��
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
				LOG_IMMEDIATE_DEBUG("������...");
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
	g_readlog.store(false);
	_sendHttp_Val("KILL", "");
	//������ȷ������رռ���
}

void ValStateMonitor::OnWegameLogin() {
	//_sendHttp_Val("RUN", "");
	// ����7�� ��¼��10����?
	LOG_IMMEDIATE(" WEGAME��¼��\n");
	wgPath = stringTOwstring( GetWGPath_REG());
	GetPath_REG(
		HKEY_CURRENT_USER,
		L"SOFTWARE\\Tencent\\valorant.live",
		L"InstallLocation");
	//injectDLL(L"notepad.exe", L"getInfo.dll");
	//injectDLL(L"wegame.exe", L"wgHelper.dll");
	//startTempProxy();
	//std::this_thread::sleep_for(std::chrono::seconds(10));
	////TODO���Ժ�ɾ��
	//getValinfo2send();
	//startTempProxy();
	//// ��ս��
	//getValinfo2send();

}

void ValStateMonitor::OnMatchOver() {
	LOG_IMMEDIATE(" �Ծֽ���,ս��ͳ��\n");

	//g_readlog.store(false);

	std::this_thread::sleep_for(std::chrono::seconds(10));

	startTempProxy();
	// ��ս��
	getValinfo2send();
	//������ȷ������رռ���
}


void ValStateMonitor::OnInLogin() {
	LOG_IMMEDIATE(" ע��,�л��˺�\n");
	//TODOȷ���رմ���

}

std::wstring getWGPath() {

	return wgPath;
}