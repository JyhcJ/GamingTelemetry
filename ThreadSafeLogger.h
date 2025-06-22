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

// �޸�����Ĵ��룺�� "ERROR" �滻Ϊ��ȷ��ö��ֵ����
enum class LogLevel {
	DEBUG1,
	INFO,
	WARNING,
	ERR
};

class ThreadSafeLogger {
public:
	// ��ȡ����ʵ��
	static ThreadSafeLogger& GetInstance();

	// ������־���Ŀ�꣨Ĭ�Ͻ�����̨��
	void SetOutputFile(const std::string& filePath);

	// ���������־���𣨵��ڴ˼������־�������
	void SetMinLogLevel(LogLevel level);

	// д����־���̰߳�ȫ��
	void Log(LogLevel level, const std::string& message);

	void LogImmediate(LogLevel level, const std::string& message);

	// �ֶ�ˢ�»�����
	void Flush();

	// ֹͣ��־�̣߳�������У�
	void Stop();

	// �����ķ���(�ֶ�)
	void TestThreadSafety(int testDurationSeconds = 5);

	// ��־�ӿں꣨�򻯵��ã�
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

	// ��ֹ����
	ThreadSafeLogger(const ThreadSafeLogger&) = delete;
	ThreadSafeLogger& operator=(const ThreadSafeLogger&) = delete;

	// ��־��Ŀ�ṹ
	struct LogEntry {
		std::chrono::system_clock::time_point time;
		LogLevel level;
		std::string message;
	};

	// ��־�����̺߳���
	void ProcessLogs();

	std::ofstream m_logFile;                     // ��־�ļ�
	std::atomic<LogLevel> m_minLogLevel;         // �����־����
	std::queue<LogEntry> m_logQueue;             // ��־����
	std::mutex m_queueMutex;                    // ���л�����
	std::condition_variable m_queueCV;           // ������������
	std::atomic<bool> m_running;                 // ��־�߳����б�־
	std::unique_ptr<std::thread> m_logThread;    // ��־�߳�

	// ������Ա(�ֶ�)
	std::chrono::system_clock::time_point m_lastRotationTime;
	std::string m_currentLogFilePath;
	void RotateLogFileIfNeeded();
	static std::string GenerateTimestampedFilename(const std::string& basePath);
};

std::string LogGetCurrentTimeString();
