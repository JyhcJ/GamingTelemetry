#pragma once
#include <Windows.h>

#include <mutex>
#include <string>
#include <thread>

#include <iostream>
#include "common.h"
enum class LoLClientState {
    CLIENT_STARTED,     // 客户端刚启动
    CLIENT_CLOSED,      // 客户端刚关闭
    MATCH_STARTED,      // 对局开始(客户端运行中)
    MATCH_ENDED         // 对局结束(客户端运行中)
};

class LoLStateMonitor {
private:
    LoLClientState currentState;
    std::mutex stateMutex;

    // 进程检测相关
    bool wasProcessRunning = false;
    const std::wstring lolProcessName = L"LeagueClient.exe";
    const std::wstring lolGameProcessName = L"League of Legends.exe";

    // 对局状态检测相关
    bool wasInGame = false;
    std::string lastGameStatus;

public:
    LoLStateMonitor() : currentState(LoLClientState::CLIENT_CLOSED) {}

    // 主监控循环
    void MonitorLoop() {
        while (true) {
            CheckStateTransition();
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

    // 检测状态转换
    void CheckStateTransition() {
        bool isProcessRunning = IsProcessRunning(lolProcessName);
        bool isGameRunning = IsProcessRunning(lolGameProcessName);

        std::lock_guard<std::mutex> lock(stateMutex);

        // 状态转换逻辑
        if (!wasProcessRunning && isProcessRunning) {
            HandleStateChange(LoLClientState::CLIENT_STARTED);
        }
        else if (wasProcessRunning && !isProcessRunning) {
            HandleStateChange(LoLClientState::CLIENT_CLOSED);
        }
        else if (isProcessRunning) {
            // 客户端运行中，检测游戏状态变化
            if (!wasInGame && isGameRunning) {
                HandleStateChange(LoLClientState::MATCH_STARTED);
            }
            else if (wasInGame && !isGameRunning) {
                HandleStateChange(LoLClientState::MATCH_ENDED);
            }
        }

        // 更新状态记录
        wasProcessRunning = isProcessRunning;
        wasInGame = isGameRunning;
    }

    // 进程检测函数 已经并入common 待删除?
    //bool IsProcessRunning(const std::wstring& processName) {
    //    // Windows实现
    //    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    //    if (snapshot == INVALID_HANDLE_VALUE) return false;

    //    PROCESSENTRY32W entry;
    //    entry.dwSize = sizeof(PROCESSENTRY32W);

    //    bool found = false;
    //    if (Process32FirstW(snapshot, &entry)) {
    //        do {
    //            if (std::wstring(entry.szExeFile) == processName) {
    //                found = true;
    //                break;
    //            }
    //        } while (Process32NextW(snapshot, &entry));
    //    }

    //    CloseHandle(snapshot);
    //    return found;
    //}

    // 状态变化处理
    void HandleStateChange(LoLClientState newState) {
        LoLClientState oldState = currentState;
        currentState = newState;

        // 记录状态转换
        std::cout << "State changed from " << StateToString(oldState)
            << " to " << StateToString(newState) << std::endl;

        // 根据状态转换执行相应操作
        switch (newState) {
        case LoLClientState::CLIENT_STARTED:
            OnClientStarted();
            break;
        case LoLClientState::CLIENT_CLOSED:
            OnClientClosed();
            break;
        case LoLClientState::MATCH_STARTED:
            OnMatchStarted();
            break;
        case LoLClientState::MATCH_ENDED:
            OnMatchEnded();
            break;
        }
    }

    // 状态字符串表示
    std::string StateToString(LoLClientState state) {
        switch (state) {
        case LoLClientState::CLIENT_STARTED: return "Client Started";
        case LoLClientState::CLIENT_CLOSED: return "Client Closed";
        case LoLClientState::MATCH_STARTED: return "Match Started";
        case LoLClientState::MATCH_ENDED: return "Match Ended";
        default: return "Unknown";
        }
    }

    // 各个状态的处理函数
    void OnClientStarted();

    void OnClientClosed();

    void OnMatchStarted();

    void OnMatchEnded();

    void OnNotRunning();
};