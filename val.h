#pragma once
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

int updateMatch();
