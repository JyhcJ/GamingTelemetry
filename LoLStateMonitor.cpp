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
    std::cout << "�ȴ�Ӣ�����˿ͻ�������" << std::endl;
    // ִ���������...
}
void LoLStateMonitor::OnClientStarted() {
	// �ͻ�������ʱ�Ĵ����߼�
	LOG_IMMEDIATE(" Ӣ�����˸�����\n");
	std::cout << "����ͻ��������¼�..." << std::endl;

	g_mtx.lock();
	is_lol_running = true;
	is_lol_game_running = false;
	g_mtx.unlock();
	_sendHttp_LOL("RUN", "");
	//// �����߳�

	std::thread t(pollRankNum);
	t.detach();
	//ThreadWrapper thread(pollRankNum);
	//thread.Start();
	// �ȴ��̳߳�ʼ����ɣ�����ʵ���������������
	//while (thread.GetState() != ThreadWrapper::ThreadState::RUNNING) {
	//std::this_thread::sleep_for(std::chrono::seconds(1));
	//}

	//thread.Detach();
}

void LoLStateMonitor::OnClientClosed() {
	// �ͻ��˹ر�ʱ�Ĵ����߼�
	LOG_IMMEDIATE("�ͻ��˹ر�");
	std::cout << "����ͻ��˹ر��¼�..." << std::endl;
	g_mtx.lock();
	is_lol_running = false;
	is_lol_game_running = false;
	g_mtx.unlock();

	//std::cout << "[" << GetCurrentTimeString() << "] Ӣ�������ѹر�\n";
	LOG_IMMEDIATE(" Ӣ�������ѹر�\n");
	_sendHttp_LOL("KILL", "");
}

void LoLStateMonitor::OnMatchStarted() {
	// �Ծֿ�ʼʱ�Ĵ����߼�
	LOG_IMMEDIATE(" Ӣ�����˶Ծֿ�ʼ,��ʼ���ӶԾ���Ϣ\n");

	g_mtx.lock();
	//is_lol_running = false;
	is_lol_game_running = true;
	g_mtx.unlock();

	//// �����߳�
	//ThreadWrapper thread(pollEvents);
	//thread.Start();
	//std::this_thread::sleep_for(std::chrono::seconds(1));
	//thread.Detach();

	std::thread poll(pollEvents);
	poll.detach();

	std::cout << "����Ծֿ�ʼ�¼�..." << std::endl;
}

void LoLStateMonitor::OnMatchEnded() {
	// �Ծֽ���ʱ�Ĵ����߼�
	LOG_IMMEDIATE(" Ӣ�����˶Ծֽ���\n");
	std::cout << "����Ծֽ����¼�..." << std::endl;

	g_mtx.lock();
	is_lol_running = true;
	is_lol_game_running = false;
	g_mtx.unlock();

	Game_Before gb;
	// ��ȡ data
	gb.before_main("END");

	//gb.getAndSendInfo("END");


}