#pragma once
#include <thread>
#include <atomic>
#include <string>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <iostream>

class ThreadWrapper {
public:
	// �߳�״̬
	enum class ThreadState {
		IDLE,       // ����
		RUNNING,    // ������
		STOPPED,     // ��ֹͣ
		DETACHED    // �ѷ���
	};

	// ���캯���������̺߳�����
	ThreadWrapper(std::function<void()> threadFunc);

	// �����������Զ�ֹͣ�̣߳�
	~ThreadWrapper();

	// �����߳�
	void Start();

	// ֹͣ�̣߳������ȴ��߳̽�����
	void Stop();

	void Join();

	void Detach();
	// ��ȡ�߳�״̬
	ThreadState GetState() const;

	// �̰߳�ȫ��־���
	//void Log(const std::string& message);

private:
	std::thread m_thread;                // �̶߳���
	std::atomic<ThreadState> m_state;    // �߳�״̬��ԭ�Ӳ�����
	std::function<void()> m_threadFunc;  // �߳�ִ�к���

	//// �̰߳�ȫ��־����
	//std::queue<std::string> m_logQueue;
	//std::mutex m_logMutex;
	//std::condition_variable m_logCV;

	// �ڲ��߳�ִ�к���
	void ThreadMain();
};