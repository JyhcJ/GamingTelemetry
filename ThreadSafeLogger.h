#pragma once
#include <string>
#include <fstream>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <memory>
#include <chrono>
#include <iomanip>

// 修复问题的代码：将 "ERROR" 替换为正确的枚举值定义
enum class LogLevel {
	DEBUG1,
	INFO,
	WARNING,
	ERR
};

class ThreadSafeLogger {
public:
	// 获取单例实例
	static ThreadSafeLogger& GetInstance();

	// 设置日志输出目标（默认仅控制台）
	void SetOutputFile(const std::string& filePath);

	// 设置最低日志级别（低于此级别的日志不输出）
	void SetMinLogLevel(LogLevel level);

	// 写入日志（线程安全）
	void Log(LogLevel level, const std::string& message);

	void LogImmediate(LogLevel level, const std::string& message);

	// 手动刷新缓冲区
	void Flush();

	// 停止日志线程（如果运行）
	void Stop();

	// 新增的方法(分段)
	void TestThreadSafety(int testDurationSeconds = 5);

	// 日志接口宏（简化调用）
#define LOG_DEBUG(msg)    ThreadSafeLogger::GetInstance().Log(LogLevel::DEBUG1, msg)
#define LOG_IMMEDIATE_DEBUG(msg)    ThreadSafeLogger::GetInstance().LogImmediate(LogLevel::DEBUG1, msg)
#define LOG_INFO(msg)     ThreadSafeLogger::GetInstance().Log(LogLevel::INFO, msg)
#define LOG_IMMEDIATE(msg)     ThreadSafeLogger::GetInstance().LogImmediate(LogLevel::INFO, msg)
#define LOG_IMMEDIATE_WARNING(msg)     ThreadSafeLogger::GetInstance().LogImmediate(LogLevel::WARNING, msg)
#define LOG_WARNING(msg)  ThreadSafeLogger::GetInstance().Log(LogLevel::WARNING, msg)
#define LOG_ERROR(msg)    ThreadSafeLogger::GetInstance().Log(LogLevel::ERR, msg)
#define LOG_IMMEDIATE_ERROR(msg)    ThreadSafeLogger::GetInstance().LogImmediate(LogLevel::ERR, msg)

private:
	ThreadSafeLogger();
	~ThreadSafeLogger();

	// 禁止拷贝
	ThreadSafeLogger(const ThreadSafeLogger&) = delete;
	ThreadSafeLogger& operator=(const ThreadSafeLogger&) = delete;

	// 日志条目结构
	struct LogEntry {
		std::chrono::system_clock::time_point time;
		LogLevel level;
		std::string message;
	};

	// 日志工作线程函数
	void ProcessLogs();

	std::ofstream m_logFile;                     // 日志文件
	std::atomic<LogLevel> m_minLogLevel;         // 最低日志级别
	std::queue<LogEntry> m_logQueue;             // 日志队列
	std::mutex m_queueMutex;                    // 队列互斥锁
	std::condition_variable m_queueCV;           // 队列条件变量
	std::atomic<bool> m_running;                 // 日志线程运行标志
	std::unique_ptr<std::thread> m_logThread;    // 日志线程

	// 新增成员(分段)
	std::chrono::system_clock::time_point m_lastRotationTime;
	std::string m_currentLogFilePath;
	void RotateLogFileIfNeeded();
	static std::string GenerateTimestampedFilename(const std::string& basePath);
};

std::string LogGetCurrentTimeString();
