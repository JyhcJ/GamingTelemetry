#include "pch.h"
#include "ThreadSafeLogger.h"
#include <iostream>
#include <algorithm>
#include <string>
ThreadSafeLogger::ThreadSafeLogger()
	: m_minLogLevel(LogLevel::DEBUG1), m_running(true) {
	m_logThread = std::make_unique<std::thread>(&ThreadSafeLogger::ProcessLogs, this);
}

ThreadSafeLogger::~ThreadSafeLogger() {
	Stop();
	if (m_logFile.is_open()) {
		m_logFile.close();
	}
}

ThreadSafeLogger& ThreadSafeLogger::GetInstance() {
	static ThreadSafeLogger instance;
	return instance;
}

void ThreadSafeLogger::SetOutputFile(const std::string& filePath) {
	std::lock_guard<std::mutex> lock(m_queueMutex);
	if (m_logFile.is_open()) {
		m_logFile.close();
	}
	m_logFile.open(filePath, std::ios::app);
}

void ThreadSafeLogger::SetMinLogLevel(LogLevel level) {
	m_minLogLevel.store(level);
}

void OutputDebugInfo(const char* pszFormat, ...)
{
#ifdef _DEBUG
	// ����̶�ǰ׺�ַ���
	const char* prefix = "[������Ϣ] ";
	char szbufFormat[0x1000];
	char szbufFormat_Game[0x1100] = "";
	// ���̶�ǰ׺ƴ�ӵ� szbufFormat_Game
	strcat_s(szbufFormat_Game, prefix);
	va_list argList;
	va_start(argList, pszFormat);//�����б��ʼ��
	vsprintf_s(szbufFormat, pszFormat, argList);
	strcat_s(szbufFormat_Game, szbufFormat);
	OutputDebugStringA(szbufFormat_Game);
	va_end(argList);

#endif
}

void ThreadSafeLogger::Log(LogLevel level, const std::string& message) {
	if (level < m_minLogLevel.load()) return;

	LogEntry entry{ std::chrono::system_clock::now(), level, message };
	{
		std::lock_guard<std::mutex> lock(m_queueMutex);
		m_logQueue.push(std::move(entry));
	}
	m_queueCV.notify_one();
}

void ThreadSafeLogger::LogImmediate(LogLevel level, const std::string& message) {
	if (level < m_minLogLevel.load()) return;

	auto now = std::chrono::system_clock::now();
	auto time = std::chrono::system_clock::to_time_t(now);
	std::tm tm;
	localtime_s(&tm, &time);
	char timeStr[20];
	strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &tm);

	const char* levelStr = "";
	switch (level) {
	case LogLevel::DEBUG1:   levelStr = "DEBUG";   break;
	case LogLevel::INFO:    levelStr = "INFO";    break;
	case LogLevel::WARNING: levelStr = "WARNING"; break;
	case LogLevel::ERR:   levelStr = "ERROR";   break;
	}

	std::string logLine = std::string("[") + timeStr + "] [" + levelStr + "] " + message + "\n";

	// ���������̨
	std::cout << logLine;
	std::cout.flush();

	// ������ļ�
	std::lock_guard<std::mutex> lock(m_queueMutex);
	if (m_logFile.is_open()) {
		m_logFile << logLine;
		m_logFile.flush();
		if (logLine.length() <= 0x1000)
		{
			OutputDebugInfo("%s", logLine.c_str());
		}
	}
}

void ThreadSafeLogger::Flush() {
	if (m_logFile.is_open()) {
		m_logFile.flush();
	}
	std::cout.flush();
}

void ThreadSafeLogger::Stop() {
	m_running = false;
	m_queueCV.notify_all();
	if (m_logThread && m_logThread->joinable()) {
		m_logThread->join();
	}
}
// ��ȡ��ǰʱ���ַ���
std::string LogGetCurrentTimeString() {
	auto now = std::chrono::system_clock::now();
	auto in_time_t = std::chrono::system_clock::to_time_t(now);
	std::tm tm;
	localtime_s(&tm, &in_time_t);

	char buffer[80];
	strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &tm);
	return std::string(buffer);
}

void ThreadSafeLogger::ProcessLogs() {
	while (m_running || !m_logQueue.empty()) {
		std::unique_lock<std::mutex> lock(m_queueMutex);
		m_queueCV.wait(lock, [this] {
			return !m_logQueue.empty() || !m_running;
			});

		while (!m_logQueue.empty()) {
			auto entry = std::move(m_logQueue.front());
			m_logQueue.pop();
			lock.unlock();

			// ��ʽ��ʱ��
			auto time = std::chrono::system_clock::to_time_t(entry.time);
			std::tm tm;
			localtime_s(&tm, &time);
			char timeStr[20];
			strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &tm);

			// ��־�����ַ���
			const char* levelStr = "";
			switch (entry.level) {
			case LogLevel::DEBUG1:   levelStr = "DEBUG";   break;
			case LogLevel::INFO:    levelStr = "INFO";    break;
			case LogLevel::WARNING: levelStr = "WARNING"; break;
			case LogLevel::ERR:   levelStr = "ERROR";   break;
			}

			// ��ʽ����־��
			std::string logLine = std::string("[") + timeStr + "] [" + levelStr + "] " + entry.message + "\n";

			// ���������̨
			std::cout << logLine;

			// ������ļ�
			if (m_logFile.is_open()) {
				m_logFile << logLine;
			}

			lock.lock();
		}
	}
}