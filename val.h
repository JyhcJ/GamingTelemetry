#pragma once
#include <nlohmann/json.hpp>
#include <fstream>
#include <unordered_set>
#include "ThreadSafeLogger.h"
#include <processthreadsapi.h>
#include <WinBase.h>
#include "common.h"



class MitmDumpController {
private:
    // 单例实例
    static std::unique_ptr<MitmDumpController> instance;
    static std::mutex instanceMutex;

    // 进程和管道句柄
    HANDLE hProcess = NULL;
    HANDLE hReadPipe = NULL;
    HANDLE hWritePipe = NULL;

    // 输出处理
    std::thread readerThread;
    std::queue<std::string> outputQueue;
    std::mutex queueMutex;
    std::condition_variable queueCV;
    std::atomic<bool> stopRequested{ false };
    std::atomic<bool> isRunning{ false };

    // 私有构造函数
    MitmDumpController() = default;

    // 读取线程函数
    void readerLoop() {
        constexpr DWORD BUFFER_SIZE = 4096;
        char buffer[BUFFER_SIZE];
        DWORD bytesRead;

        while (!stopRequested) {
            if (!ReadFile(hReadPipe, buffer, BUFFER_SIZE, &bytesRead, NULL)) {
                DWORD err = GetLastError();
                if (err != ERROR_BROKEN_PIPE && err != ERROR_HANDLE_EOF) {
                    std::lock_guard<std::mutex> lock(queueMutex);
                    outputQueue.push("[ERROR] Failed to read from pipe");
                    queueCV.notify_one();
                }
                break;
            }

            if (bytesRead > 0) {
                std::lock_guard<std::mutex> lock(queueMutex);
                outputQueue.emplace(buffer, bytesRead);
                queueCV.notify_one();
            }
        }

        isRunning = false;
    }

    // 清理资源
    void cleanup() {
        stopRequested = true;

        if (readerThread.joinable()) {
            readerThread.join();
        }

        if (hProcess) {
            TerminateProcess(hProcess, 0);
            CloseHandle(hProcess);
            hProcess = NULL;
        }

        if (hReadPipe) {
            CloseHandle(hReadPipe);
            hReadPipe = NULL;
        }

        if (hWritePipe) {
            CloseHandle(hWritePipe);
            hWritePipe = NULL;
        }

        std::lock_guard<std::mutex> lock(queueMutex);
        while (!outputQueue.empty()) {
            outputQueue.pop();
        }
    }

public:
    // 删除拷贝构造函数和赋值运算符
    MitmDumpController(const MitmDumpController&) = delete;
    MitmDumpController& operator=(const MitmDumpController&) = delete;

    ~MitmDumpController() {
        cleanup();
    }

    // 辅助函数：去除字符串中的 b' 和 '
    std::string clean_byte_string(const std::string& s) {
        if (s.size() >= 3 && s.substr(0, 2) == "b'" && s.back() == '\'') {
            return s.substr(2, s.size() - 3);
        }
        return s;
    }

    // 解析 Python 风格的元组字符串
    void parse_python_headers(const std::string& input, nlohmann::json& result) {
        std::vector<std::pair<std::string, std::string>> headers;
        std::vector<std::string> cookies;

        size_t start = input.find('(');
        while (start != std::string::npos) {
            size_t end = input.find(')', start);
            if (end == std::string::npos) break;

            std::string tuple_str = input.substr(start + 1, end - start - 1);
            size_t comma = tuple_str.find(',');
            if (comma == std::string::npos) continue;

            std::string key = tuple_str.substr(0, comma);
            std::string value = tuple_str.substr(comma + 1);

            // 清理 b' 前缀和两边的空白/引号
            key = clean_byte_string(key);
            value = clean_byte_string(value);

            // 去除可能的空白
            key.erase(std::remove_if(key.begin(), key.end(), ::isspace), key.end());
            value.erase(std::remove_if(value.begin(), value.end(), ::isspace), value.end());

            if (key == "cookie") {
                cookies.push_back(value);
            }
            else {
                result["headers"][key] = value;
            }

            start = input.find('(', end);
        }

        if (!cookies.empty()) {
            result["headers"]["cookie"] = cookies;
        }
    }

    // 读取指定文件名的UTF-8文本到字符串
    static std::string readUtf8File(const std::string& filename) {
        std::ifstream file(filename, std::ios::binary);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open file: " + filename);
        }

        // 读取文件内容到字符串
        std::string content((std::istreambuf_iterator<char>(file)),
            std::istreambuf_iterator<char>());
        return content;
    }

    // 清空指定文件的内容
    static void clearFileContent(const std::string& filename) {
        std::ofstream file(filename, std::ios::trunc | std::ios::binary);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open file for clearing: " + filename);
        }
        // 打开文件时使用trunc模式会自动清空内容
    }

    // 获取单例实例
    static MitmDumpController& getInstance() {
        std::lock_guard<std::mutex> lock(instanceMutex);
        if (!instance) {
            instance.reset(new MitmDumpController());
        }
        return *instance;
    }

    // 启动 mitmdump
    bool start(int port, const std::string& filter = "") {
        if (isRunning) {
            return false;
        }

        SECURITY_ATTRIBUTES sa = { sizeof(sa), NULL, TRUE };

        // 创建匿名管道
        if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, 0)) {
            return false;
        }

        // 构建命令
        //--mode transparent --ssl - insecure --set upstream_cert = false  --allow - hosts "wegame.com.cn"
        std::string cmd = "mitmdump -p " + std::to_string(port) + " ";
        if (!filter.empty()) {
            cmd +=  filter ;
        }

        // 设置进程启动信息
        STARTUPINFOA si = { sizeof(si) };
        si.dwFlags = STARTF_USESTDHANDLES;
        si.hStdOutput = hWritePipe;
        si.hStdError = hWritePipe;

        PROCESS_INFORMATION pi;

        // 创建进程
        if (!CreateProcessA(
            NULL,
            const_cast<LPSTR>(cmd.c_str()),
            NULL, NULL, TRUE,
            CREATE_NO_WINDOW,
            NULL, NULL, &si, &pi)) {
            cleanup();
            return false;
        }

        hProcess = pi.hProcess;
        CloseHandle(pi.hThread);
        CloseHandle(hWritePipe); // 子进程已经继承，可以关闭写入端
        hWritePipe = NULL;

        // 启动读取线程
        stopRequested = false;
        isRunning = true;
        readerThread = std::thread(&MitmDumpController::readerLoop, this);

        return true;
    }

    // 
    void stop() {
        if (!isRunning) return;
        cleanup();
    }


    std::string getOutput(int timeoutMs = 100) {
        std::unique_lock<std::mutex> lock(queueMutex);
        if (queueCV.wait_for(lock, std::chrono::milliseconds(timeoutMs),
            [this] { return !outputQueue.empty(); })) {
            std::string output = outputQueue.front();
            outputQueue.pop();
            return output;
        }
        return "";
    }

    // 检查是否正在运行
    bool running() const {
        return isRunning;
    }

    // 获取进程ID
    DWORD pid() const {
        return hProcess ? GetProcessId(hProcess) : 0;
    }
};

class LogAnalyzer {
private:
    std::unordered_set<std::string> processedTimestamps;
    std::string logFilePath;

public:
    //std::string valGamePath = "E:\\WeGameApps\\rail_apps\\无畏契约(2001715)";
    std::string valGamePath = GetPath_REG(
        HKEY_LOCAL_MACHINE,
        L"SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\无畏契约",
        L"InstallSource");
    LogAnalyzer() {
        logFilePath = valGamePath + "\\live\\ShooterGame\\Saved\\Logs\\ShooterGame.log";
    }

    bool checkMatchEnd(bool &isFirst) {
   

        if (isFirst)
        {
            if (std::remove(logFilePath.c_str()) != 0) {
                // 文件不存在或删除失败
                if (errno == ENOENT) {
                    LOG_IMMEDIATE("文件不存在，无需删除");
                    //return true;  // 文件本来就不存在，不算错误
                }
                else {
                    LOG_IMMEDIATE("日志文件删除失败");
                    //return false;
                }
            }
            else {
                LOG_IMMEDIATE("删除日志文件");
            }
            isFirst = false;
        }
        // 打开日志文件
        std::ifstream logFile(logFilePath);
        if (!logFile.is_open()) {
            LOG_IMMEDIATE("无法打开日志文件");
            return false;
        }

        // 读取文件内容到向量中以便反向遍历
        std::vector<std::string> lines;
        std::string line;
        while (std::getline(logFile, line)) {
            lines.push_back(line);
        }
        logFile.close();

        // 从末尾开始反向遍历
        for (auto it = lines.rbegin(); it != lines.rend(); ++it) {
            const std::string& currentLine = *it;

            // 检查是否包含匹配的字符串
            if (currentLine.find("LogShooterGameState: Match Ended: Completion State: 'Completed'") != std::string::npos) {

                // 提取时间戳 [2025.06.24-06.35.47:978]
                size_t timestampStart = currentLine.find('[');
                size_t timestampEnd = currentLine.find(']');

                if (timestampStart != std::string::npos && timestampEnd != std::string::npos) {
                    std::string timestamp = currentLine.substr(timestampStart, timestampEnd - timestampStart + 1);

                    // 检查时间戳是否已存在
                    if (processedTimestamps.find(timestamp) != processedTimestamps.end()) {
                        return false;
                    }
                    else {
                        // 存储新时间戳并返回true
                        processedTimestamps.insert(timestamp);
                        return true;
                    }
                }
            }
        }

        // 没有找到匹配的行
        return false;
    }

  
};

void _sendHttp_Val(nlohmann::json jsonBody);

void _sendHttp_Val(std::string type, nlohmann::json data);

int startTempProxy();

std::string getValinfo2send();


