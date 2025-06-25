#include "pch.h"
#include "ThreadSafeLogger.h"
#include <iostream>
#include "ValStateMonitor.h"
#include "val.h"

void ValStateMonitor::OnNotRunning() {
	std::cout << "�ȴ�����" << std::endl;
	// ִ���������...
}
void ValStateMonitor::OnClientStarted() {
	// �ͻ�������ʱ�Ĵ����߼�
	LOG_IMMEDIATE(" Wegame����\n");
	std::cout << "����ͻ��������¼�..." << std::endl;
	//updateHeader();
}

void ValStateMonitor::OnClientClosed() {
	// �ϱ�
	LOG_IMMEDIATE("�ͻ��˹ر�");
	std::cout << "����ͻ��˹ر��¼�..." << std::endl;


	//std::cout << "[" << GetCurrentTimeString() << "] Ӣ�������ѹر�\n";
	//_sendHttp_LOL("KILL", "");
}

void ValStateMonitor::OnMatchStarted() {
	// 1����
	LOG_IMMEDIATE("VAL��ʼ\n");
	_sendHttp_Val("RUN", "");

	// 
	updateHeader();



}

void ValStateMonitor::OnMatchEnded() {
	// �Ծֽ���ʱ�Ĵ����߼�
	LOG_IMMEDIATE(" Val����\n");
	_sendHttp_Val("KILL", "");
	//������ȷ������رռ���
}

void ValStateMonitor::OnWegameLogin() {
	//
	LOG_IMMEDIATE("wegame��¼\n");
	updateHeader();
	std::this_thread::sleep_for(std::chrono::seconds(5));
	getValinfo2send();
}

void ValStateMonitor::OnMatchOver() {
	// �Ծֽ���ʱ�Ĵ����߼�
	LOG_IMMEDIATE(" �Ծֽ���,ս��ͳ��\n");
	getValinfo2send();
	//������ȷ������رռ���
}

