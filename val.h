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
    // ����ʵ��
    static std::unique_ptr<MitmDumpController> instance;
    static std::mutex instanceMutex;

    // ���̺͹ܵ����
    HANDLE hProcess = NULL;
    HANDLE hReadPipe = NULL;
    HANDLE hWritePipe = NULL;

    // �������
    std::thread readerThread;
    std::queue<std::string> outputQueue;
    std::mutex queueMutex;
    std::condition_variable queueCV;
    std::atomic<bool> stopRequested{ false };
    std::atomic<bool> isRunning{ false };

    // ˽�й��캯��
    MitmDumpController() = default;

    // ��ȡ�̺߳���
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

    // ������Դ
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
    // ɾ���������캯���͸�ֵ�����
    MitmDumpController(const MitmDumpController&) = delete;
    MitmDumpController& operator=(const MitmDumpController&) = delete;

    ~MitmDumpController() {
        cleanup();
    }

    // ����������ȥ���ַ����е� b' �� '
    std::string clean_byte_string(const std::string& s) {
        if (s.size() >= 3 && s.substr(0, 2) == "b'" && s.back() == '\'') {
            return s.substr(2, s.size() - 3);
        }
        return s;
    }

    // ���� Python ����Ԫ���ַ���
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

            // ���� b' ǰ׺�����ߵĿհ�/����
            key = clean_byte_string(key);
            value = clean_byte_string(value);

            // ȥ�����ܵĿհ�
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

    // ��ȡָ���ļ�����UTF-8�ı����ַ���
    static std::string readUtf8File(const std::string& filename) {
        std::ifstream file(filename, std::ios::binary);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open file: " + filename);
        }

        // ��ȡ�ļ����ݵ��ַ���
        std::string content((std::istreambuf_iterator<char>(file)),
            std::istreambuf_iterator<char>());
        return content;
    }

    // ���ָ���ļ�������
    static void clearFileContent(const std::string& filename) {
        std::ofstream file(filename, std::ios::trunc | std::ios::binary);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open file for clearing: " + filename);
        }
        // ���ļ�ʱʹ��truncģʽ���Զ��������
    }

    // ��ȡ����ʵ��
    static MitmDumpController& getInstance() {
        std::lock_guard<std::mutex> lock(instanceMutex);
        if (!instance) {
            instance.reset(new MitmDumpController());
        }
        return *instance;
    }

    // ���� mitmdump
    bool start(int port, const std::string& filter = "") {
        if (isRunning) {
            return false;
        }

        SECURITY_ATTRIBUTES sa = { sizeof(sa), NULL, TRUE };

        // ���������ܵ�
        if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, 0)) {
            return false;
        }

        // ��������
        //--mode transparent --ssl - insecure --set upstream_cert = false  --allow - hosts "wegame.com.cn"
        std::string cmd = "mitmdump -p " + std::to_string(port) + " ";
        if (!filter.empty()) {
            cmd +=  filter ;
        }

        // ���ý���������Ϣ
        STARTUPINFOA si = { sizeof(si) };
        si.dwFlags = STARTF_USESTDHANDLES;
        si.hStdOutput = hWritePipe;
        si.hStdError = hWritePipe;

        PROCESS_INFORMATION pi;

        // ��������
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
        CloseHandle(hWritePipe); // �ӽ����Ѿ��̳У����Թر�д���
        hWritePipe = NULL;

        // ������ȡ�߳�
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

    // ����Ƿ���������
    bool running() const {
        return isRunning;
    }

    // ��ȡ����ID
    DWORD pid() const {
        return hProcess ? GetProcessId(hProcess) : 0;
    }
};

class LogAnalyzer {
private:
    std::unordered_set<std::string> processedTimestamps;
    std::string logFilePath;

public:
    //std::string valGamePath = "E:\\WeGameApps\\rail_apps\\��η��Լ(2001715)";
    std::string valGamePath = GetPath_REG(
        HKEY_LOCAL_MACHINE,
        L"SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\��η��Լ",
        L"InstallSource");
    LogAnalyzer() {
        logFilePath = valGamePath + "\\live\\ShooterGame\\Saved\\Logs\\ShooterGame.log";
    }

    bool checkMatchEnd(bool &isFirst) {
   

        if (isFirst)
        {
            if (std::remove(logFilePath.c_str()) != 0) {
                // �ļ������ڻ�ɾ��ʧ��
                if (errno == ENOENT) {
                    LOG_IMMEDIATE("�ļ������ڣ�����ɾ��");
                    //return true;  // �ļ������Ͳ����ڣ��������
                }
                else {
                    LOG_IMMEDIATE("��־�ļ�ɾ��ʧ��");
                    //return false;
                }
            }
            else {
                LOG_IMMEDIATE("ɾ����־�ļ�");
            }
            isFirst = false;
        }
        // ����־�ļ�
        std::ifstream logFile(logFilePath);
        if (!logFile.is_open()) {
            LOG_IMMEDIATE("�޷�����־�ļ�");
            return false;
        }

        // ��ȡ�ļ����ݵ��������Ա㷴�����
        std::vector<std::string> lines;
        std::string line;
        while (std::getline(logFile, line)) {
            lines.push_back(line);
        }
        logFile.close();

        // ��ĩβ��ʼ�������
        for (auto it = lines.rbegin(); it != lines.rend(); ++it) {
            const std::string& currentLine = *it;

            // ����Ƿ����ƥ����ַ���
            if (currentLine.find("LogShooterGameState: Match Ended: Completion State: 'Completed'") != std::string::npos) {

                // ��ȡʱ��� [2025.06.24-06.35.47:978]
                size_t timestampStart = currentLine.find('[');
                size_t timestampEnd = currentLine.find(']');

                if (timestampStart != std::string::npos && timestampEnd != std::string::npos) {
                    std::string timestamp = currentLine.substr(timestampStart, timestampEnd - timestampStart + 1);

                    // ���ʱ����Ƿ��Ѵ���
                    if (processedTimestamps.find(timestamp) != processedTimestamps.end()) {
                        return false;
                    }
                    else {
                        // �洢��ʱ���������true
                        processedTimestamps.insert(timestamp);
                        return true;
                    }
                }
            }
        }

        // û���ҵ�ƥ�����
        return false;
    }

  
};

void _sendHttp_Val(nlohmann::json jsonBody);

void _sendHttp_Val(std::string type, nlohmann::json data);

int startTempProxy();

std::string getValinfo2send();


