#pragma once
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

int updateMatch();
