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
	// 线程状态
	enum class ThreadState {
		IDLE,       // 空闲
		RUNNING,    // 运行中
		STOPPED,     // 已停止
		DETACHED    // 已分离
	};

	// 构造函数（传入线程函数）
	ThreadWrapper(std::function<void()> threadFunc);

	// 析构函数（自动停止线程）
	~ThreadWrapper();

	// 启动线程
	void Start();

	// 停止线程（阻塞等待线程结束）
	void Stop();

	void Join();

	void Detach();
	// 获取线程状态
	ThreadState GetState() const;

	// 线程安全日志输出
	//void Log(const std::string& message);

private:
	std::thread m_thread;                // 线程对象
	std::atomic<ThreadState> m_state;    // 线程状态（原子操作）
	std::function<void()> m_threadFunc;  // 线程执行函数

	//// 线程安全日志队列
	//std::queue<std::string> m_logQueue;
	//std::mutex m_logMutex;
	//std::condition_variable m_logCV;

	// 内部线程执行函数
	void ThreadMain();
};