#include "pch.h"
#include "ThreadWrapper.h"
#include "ThreadSafeLogger.h"


ThreadWrapper::ThreadWrapper(std::function<void()> threadFunc)
	: m_state(ThreadState::IDLE), m_threadFunc(std::move(threadFunc)) {
	if (!m_threadFunc) {
		throw std::invalid_argument("线程函数不能为空");
	}
}

ThreadWrapper::~ThreadWrapper() {
	if (m_state == ThreadState::RUNNING) {
		if (m_thread.joinable()) {
			// 如果线程正在运行且未被分离，尝试优雅停止
			m_state = ThreadState::STOPPED;
			m_thread.join();
		}
	}
	// 如果是DETACHED状态，不进行任何操作
}

void ThreadWrapper::Start() {
	if (m_state == ThreadState::RUNNING) {
		Log("线程已在运行中");
		return;
	}
	m_state = ThreadState::RUNNING;
	m_thread = std::thread(&ThreadWrapper::ThreadMain, this);
	Log("线程启动");
}

void ThreadWrapper::Stop() {
	if (m_state != ThreadState::RUNNING) {
		return;
	}
	m_state = ThreadState::STOPPED;
	if (m_thread.joinable()) {
		m_thread.join();
	}
	Log("线程停止");
}

void ThreadWrapper::Join() {
	if (m_state != ThreadState::RUNNING) {
		Log("警告: 尝试Join非运行状态的线程");
		return;
	}
	if (m_thread.joinable()) {
		m_thread.join();
		m_state = ThreadState::STOPPED;
	}
}

void ThreadWrapper::Detach() {
	if (m_state != ThreadState::RUNNING) {
		Log("警告: 尝试Detach非运行状态的线程");
		return;
	}
	if (m_thread.joinable()) {
		m_thread.detach();
		m_state = ThreadState::DETACHED;
		Log("线程已分离");
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
	m_logCV.notify_one(); // 通知日志输出
}

void ThreadWrapper::ThreadMain() {
	try {
		m_threadFunc();
	}
	catch (const std::exception& e) {
		//Log("线程异常: " + std::string(e.what()));
		LOG_IMMEDIATE_ERROR("线程异常: " + std::string(e.what()));
	}

	// 自动状态转换
	if (m_state == ThreadState::RUNNING) {
		m_state = ThreadState::STOPPED;
	}

	// 处理剩余日志
	std::unique_lock<std::mutex> lock(m_logMutex);
	while (!m_logQueue.empty()) {
		std::cout << "[Thread] " << m_logQueue.front() << std::endl;
		LOG_INFO("[Thread] " + m_logQueue.front());
		m_logQueue.pop();
	}
}