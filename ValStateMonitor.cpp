#include "pch.h"
#include "ThreadSafeLogger.h"
#include <iostream>
#include "ValStateMonitor.h"
#include "val.h"


void ValStateMonitor::OnNotRunning() {
	std::cout << "�ȴ�Ӣ�����˿ͻ�������" << std::endl;
	// ִ���������...
}
void ValStateMonitor::OnClientStarted() {
	// �ͻ�������ʱ�Ĵ����߼�
	LOG_IMMEDIATE(" Wegame����\n");
	std::cout << "����ͻ��������¼�..." << std::endl;
	updateMatch();
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
	updateMatch();



}

void ValStateMonitor::OnMatchEnded() {
	// �Ծֽ���ʱ�Ĵ����߼�
	LOG_IMMEDIATE(" Val����\n");
	std::cout << "����Ծֽ����¼�..." << std::endl;



	//gb.getAndSendInfo("END");


}