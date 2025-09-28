#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <stdexcept>
#include <memory>
#include <windows.h>
#include <locale>
#include <codecvt>
#include <TlHelp32.h>

class ProcessMemoryReader {
public:
    ProcessMemoryReader(const std::wstring& processName)
        : processName_(processName), hProcess_(NULL), processId_(0) {
        openProcess();
    }

    ~ProcessMemoryReader() {
        if (hProcess_ != NULL) {
            CloseHandle(hProcess_);
        }
    }

    std::wstring readUnicodeStringFromPointerChain(uintptr_t baseAddress, const std::vector<uintptr_t>& offsets) {
        try {
            // 1. 遍历指针链获取最终地址
            uintptr_t currentAddress = baseAddress;

            for (size_t i = 0; i < offsets.size(); ++i) {
                currentAddress = readPointer(currentAddress);
                if (currentAddress == 0) {
                    throw std::runtime_error("Null pointer encountered at offset " + std::to_string(i));
                }
                currentAddress += offsets[i];
            }

            // 2. 读取Unicode字符串
            return readUnicodeString(currentAddress);
        }
        catch (const std::exception& e) {
            throw std::runtime_error(std::string("Failed to read Unicode string from pointer chain: ") + e.what());
        }
    }

private:
    void openProcess() {
        processId_ = findProcessId(processName_);

        DWORD pid = 0;
        HWND hWnd = FindWindowW(L"UnrealWindow", nullptr);

        GetWindowThreadProcessId(hWnd, (DWORD*)(&pid));
        processId_ = pid;
        if (processId_ == 0) {
            throw std::runtime_error("Process '" + wideToUtf8(processName_) + "' not found");
        }

        // 尝试使用更高权限打开进程
        hProcess_ = OpenProcess(PROCESS_ALL_ACCESS,
            FALSE,
            processId_);
        if (hProcess_ == NULL) {
            DWORD error = GetLastError();
            throw std::runtime_error("Failed to open process (Error " + std::to_string(error) + ")");
        }
    }

    DWORD findProcessId(const std::wstring& processName) {
        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hSnapshot == INVALID_HANDLE_VALUE) {
            return 0;
        }

        PROCESSENTRY32W pe32;
        pe32.dwSize = sizeof(PROCESSENTRY32W);

        if (!Process32FirstW(hSnapshot, &pe32)) {
            CloseHandle(hSnapshot);
            return 0;
        }

        do {
            if (_wcsicmp(pe32.szExeFile, processName.c_str()) == 0) {
                CloseHandle(hSnapshot);
                return pe32.th32ProcessID;
            }
        } while (Process32NextW(hSnapshot, &pe32));

        CloseHandle(hSnapshot);
        return 0;
    }

    uintptr_t readPointer(uintptr_t address) {
        uintptr_t value = 0;
        SIZE_T bytesRead = 0;

        if (!ReadProcessMemory(hProcess_, reinterpret_cast<LPCVOID>(address), &value, sizeof(value), &bytesRead)) {
            DWORD error = GetLastError();
            throw std::runtime_error("Failed to read pointer at address 0x" + toHexString(address) +
                " (Error " + std::to_string(error) + ")");
        }

        if (bytesRead != sizeof(value)) {
            throw std::runtime_error("Incomplete read of pointer at address 0x" + toHexString(address));
        }

        return value;
    }

    std::wstring readUnicodeString(uintptr_t address) {
        // 1. 先读取字符串长度(假设是前缀长度的Unicode字符串)
        const int maxLength = 4096; // 安全限制
        wchar_t buffer[maxLength] = { 0 };
        SIZE_T bytesRead = 0;

        // 2. 读取字符串内容
        if (!ReadProcessMemory(hProcess_, reinterpret_cast<LPCVOID>(address), buffer, maxLength * sizeof(wchar_t), &bytesRead)) {
            DWORD error = GetLastError();
            throw std::runtime_error("Failed to read Unicode string at address 0x" + toHexString(address) +
                " (Error " + std::to_string(error) + ")");
        }

        // 3. 确保以null结尾
        buffer[maxLength - 1] = L'\0';
        return std::wstring(buffer);
    }

    std::string toHexString(uintptr_t value) {
        char buf[32];
        sprintf_s(buf, "%llX", value);
        return buf;
    }

    std::string wideToUtf8(const std::wstring& wstr) {
        std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
        return converter.to_bytes(wstr);
    }

    std::wstring processName_;
    HANDLE hProcess_;
    DWORD processId_;
};

int main_memory_reader() {
    try {
        // 1. 创建读取器实例
        ProcessMemoryReader reader(L"TslGame.exe");

        // 2. 定义指针链 [[[[1F1804A0060+0]+C0]+318]+18
        uintptr_t baseAddress = 0x1F1804A0060;
        std::vector<uintptr_t> offsets = { 0x0, 0xC0, 0x318, 0x18 };

        // 3. 读取Unicode字符串
        std::wstring result = reader.readUnicodeStringFromPointerChain(baseAddress, offsets);

        // 4. 输出结果
        std::wcout << L"Read value: " << result << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}