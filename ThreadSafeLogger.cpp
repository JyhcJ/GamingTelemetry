#include "pch.h"
#include "ThreadSafeLogger.h"
#include <iostream>
#include <algorithm>
#include <string>
ThreadSafeLogger::ThreadSafeLogger()
	: m_minLogLevel(LogLevel::DEBUG1), m_running(true),
	m_lastRotationTime(std::chrono::system_clock::now()) {
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
	m_currentLogFilePath = filePath;
	RotateLogFileIfNeeded();
}


//新增(分段)
void ThreadSafeLogger::RotateLogFileIfNeeded() {
	auto now = std::chrono::system_clock::now();
	auto hoursSinceLastRotation = std::chrono::duration_cast<std::chrono::milliseconds>(
		now - m_lastRotationTime).count();
	/*auto hoursSinceLastRotation = std::chrono::duration_cast<std::chrono::hours>(
		now - m_lastRotationTime).count();*/

	if (hoursSinceLastRotation >= 24) {
		if (m_logFile.is_open()) {
			m_logFile.close();
		}

		// 重命名当前日志文件
		std::string newFilename = GenerateTimestampedFilename(m_currentLogFilePath);
		std::rename(m_currentLogFilePath.c_str(), newFilename.c_str());

		// 创建新日志文件
		m_logFile.open(m_currentLogFilePath, std::ios::app);
		m_lastRotationTime = now;
	}
	else if (!m_logFile.is_open()) {
		m_logFile.open(m_currentLogFilePath, std::ios::app);
	}
}
//新增(分段)
std::string ThreadSafeLogger::GenerateTimestampedFilename(const std::string& basePath) {
	auto now = std::chrono::system_clock::now();
	auto in_time_t = std::chrono::system_clock::to_time_t(now);
	std::tm tm;
	localtime_s(&tm, &in_time_t);

	char buffer[80];
	strftime(buffer, sizeof(buffer), "%Y%m%d_%H%M%S", &tm);

	size_t lastDot = basePath.find_last_of('.');
	if (lastDot != std::string::npos) {
		return basePath.substr(0, lastDot) + "_" + buffer + basePath.substr(lastDot);
	}
	return basePath + "_" + buffer;
}

void ThreadSafeLogger::SetMinLogLevel(LogLevel level) {
	m_minLogLevel.store(level);
}



// 新增(分段)
void ThreadSafeLogger::TestThreadSafety(int testDurationSeconds) {
	auto startTime = std::chrono::steady_clock::now();
	auto endTime = startTime + std::chrono::seconds(testDurationSeconds);

	const int numThreads = 2;
	std::vector<std::thread> threads;

	// 创建多个线程同时写入日志
	for (int i = 0; i < numThreads; ++i) {
		threads.emplace_back([this, i, endTime]() {
			int count = 0;
			while (std::chrono::steady_clock::now() < endTime) {
				std::string message = "Thread " + std::to_string(i) + " log entry " + std::to_string(count++);
				this->Log(LogLevel::INFO, message);

				// 随机间隔写入
				std::this_thread::sleep_for(std::chrono::milliseconds(10 + rand() % 50));
			}
			});
	}

	// 在主线程中频繁修改日志级别和文件路径
	int rotationCount = 0;
	while (std::chrono::steady_clock::now() < endTime) {
		this->SetMinLogLevel(static_cast<LogLevel>((rotationCount % 4) + 1));

		if (rotationCount % 5 == 0) {
			this->SetOutputFile("test_log_" + std::to_string(rotationCount / 5) + ".txt");
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		rotationCount++;
	}

	// 等待所有线程完成
	for (auto& thread : threads) {
		if (thread.joinable()) {
			thread.join();
		}
	}

	// 验证日志完整性
	Log(LogLevel::INFO, "Thread safety test completed successfully");
}

void OutputDebugInfo(const char* pszFormat, ...)
{
#ifdef _DEBUG
	// 定义固定前缀字符串
	const char* prefix = "[调试信息] ";
	char szbufFormat[0x1000];
	char szbufFormat_Game[0x1100] = "";
	// 将固定前缀拼接到 szbufFormat_Game
	strcat_s(szbufFormat_Game, prefix);
	va_list argList;
	va_start(argList, pszFormat);//参数列表初始化
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

	// 输出到控制台
	std::cout << logLine;
	std::cout.flush();

	// 输出到文件
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
// 获取当前时间字符串
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

		// 检查是否需要轮转日志文件
		if (!m_currentLogFilePath.empty()) {
			RotateLogFileIfNeeded();
		}

		while (!m_logQueue.empty()) {
			auto entry = std::move(m_logQueue.front());
			m_logQueue.pop();
			lock.unlock();

			// 格式化时间
			auto time = std::chrono::system_clock::to_time_t(entry.time);
			std::tm tm;
			localtime_s(&tm, &time);
			char timeStr[20];
			strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &tm);

			// 日志级别字符串
			const char* levelStr = "";
			switch (entry.level) {
			case LogLevel::DEBUG1:   levelStr = "DEBUG";   break;
			case LogLevel::INFO:    levelStr = "INFO";    break;
			case LogLevel::WARNING: levelStr = "WARNING"; break;
			case LogLevel::ERR:   levelStr = "ERROR";   break;
			}

			// 格式化日志行
			std::string logLine = std::string("[") + timeStr + "] [" + levelStr + "] " + entry.message + "\n";

			// 输出到控制台
			std::cout << logLine;

			// 输出到文件
			if (m_logFile.is_open()) {
				m_logFile << logLine;
				m_logFile.flush();
			}
		
			lock.lock();
		}
	}
}