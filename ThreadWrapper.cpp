#include "pch.h"
#include "ThreadWrapper.h"
#include "ThreadSafeLogger.h"


ThreadWrapper::ThreadWrapper(std::function<void()> threadFunc)
	: m_state(ThreadState::IDLE), m_threadFunc(std::move(threadFunc)) {
	if (!m_threadFunc) {
		throw std::invalid_argument("�̺߳�������Ϊ��");
	}
}

ThreadWrapper::~ThreadWrapper() {
	if (m_state == ThreadState::RUNNING) {
		if (m_thread.joinable()) {
			// ����߳�����������δ�����룬��������ֹͣ
			m_state = ThreadState::STOPPED;
			m_thread.join();
		}
	}
	// �����DETACHED״̬���������κβ���
}

void ThreadWrapper::Start() {
	if (m_state == ThreadState::RUNNING) {
		Log("�߳�����������");
		return;
	}
	m_state = ThreadState::RUNNING;
	m_thread = std::thread(&ThreadWrapper::ThreadMain, this);
	Log("�߳�����");
}

void ThreadWrapper::Stop() {
	if (m_state != ThreadState::RUNNING) {
		return;
	}
	m_state = ThreadState::STOPPED;
	if (m_thread.joinable()) {
		m_thread.join();
	}
	Log("�߳�ֹͣ");
}

void ThreadWrapper::Join() {
	if (m_state != ThreadState::RUNNING) {
		Log("����: ����Join������״̬���߳�");
		return;
	}
	if (m_thread.joinable()) {
		m_thread.join();
		m_state = ThreadState::STOPPED;
	}
}

void ThreadWrapper::Detach() {
	if (m_state != ThreadState::RUNNING) {
		Log("����: ����Detach������״̬���߳�");
		return;
	}
	if (m_thread.joinable()) {
		m_thread.detach();
		m_state = ThreadState::DETACHED;
		Log("�߳��ѷ���");
	}
}

ThreadWrapper::ThreadState ThreadWrapper::GetState() const {
	return m_state;
}

void ThreadWrapper::Log(const std::string& message) {
	{
		std::lock_guard<std::mutex> lock(m_logMutex);
		m_logQueue.push(message);
	}
	m_logCV.notify_one(); // ֪ͨ��־���
}

void ThreadWrapper::ThreadMain() {
	try {
		m_threadFunc();
	}
	catch (const std::exception& e) {
		//Log("�߳��쳣: " + std::string(e.what()));
		LOG_IMMEDIATE_ERROR("�߳��쳣: " + std::string(e.what()));
	}

	// �Զ�״̬ת��
	if (m_state == ThreadState::RUNNING) {
		m_state = ThreadState::STOPPED;
	}

	// ����ʣ����־
	std::unique_lock<std::mutex> lock(m_logMutex);
	while (!m_logQueue.empty()) {
		std::cout << "[Thread] " << m_logQueue.front() << std::endl;
		LOG_INFO("[Thread] " + m_logQueue.front());
		m_logQueue.pop();
	}
}