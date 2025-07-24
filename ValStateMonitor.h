#pragma once

#include <Windows.h>
#include <mutex>
#include <string>
#include <thread>

#include <iostream>
#include "common.h"
#include "val.h"
enum class ValState {
    WEGAME_STARTED,     // 客户端启动
    WEGAME_CLOSED,      // wegame关闭
    VAL_STARTED,        // 对局开始(客户端运行中)
    VAL_ENDED,          // 对局结束(客户端运行中)
    WEGAME_LOGIN,       // 登录
    VAL_OVER,           // 对局结束
    WEGAME_INLOGIN      // 登录中
};

class ValStateMonitor {
private:
    ValState currentState;
    std::mutex stateMutex;

    // 进程检测相关
    bool wasProcessRunning = false;
    const std::wstring wegameProcessName = L"wegame.exe";
    const std::wstring valProcessName = L"VALORANT-Win64-Shipping.exe";
    const std::wstring railProcessName = L"rail.exe";

    // 对局状态检测相关
    bool wasInGame = false;
    bool wasLogIn = false;
    std::string lastGameStatus;

public:
    ValStateMonitor() : currentState(ValState::WEGAME_CLOSED) {}

    // 主监控循环
    void MonitorLoop() {
        while (true) {
            CheckStateTransition();
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

    // 检测状态转换
    void CheckStateTransition() {
        bool isWegameRunning = IsProcessRunning(wegameProcessName);
        bool isGameRunning = IsProcessRunning(valProcessName);
        bool isLogIn = IsProcessRunning(railProcessName);
        //bool isGameOver = 单独控制?


        //bool isGameOver =  读文本
        
        //std::lock_guard<std::mutex> lock(stateMutex);

        // 状态转换逻辑
        if (!wasProcessRunning && isWegameRunning) {
            HandleStateChange(ValState::WEGAME_STARTED);
        }
        else if (wasProcessRunning && !isWegameRunning) {
            HandleStateChange(ValState::WEGAME_CLOSED);
        }
        else if (isWegameRunning) {
            // 客户端运行中，检测游戏状态变化
            if (!wasInGame && isGameRunning) {
                HandleStateChange(ValState::VAL_STARTED);
            }
            else if (wasInGame && !isGameRunning) {
                HandleStateChange(ValState::VAL_ENDED);
            }
            else if (!wasLogIn && isLogIn) {
                //延迟(1000)
                std::this_thread::sleep_for(std::chrono::seconds(3));
                HandleStateChange(ValState::WEGAME_LOGIN);
            }
            else if (wasLogIn && !isLogIn) {
                HandleStateChange(ValState::WEGAME_INLOGIN);
            }
        }

        // 更新状态记录
        wasProcessRunning = isWegameRunning;
        wasInGame = isGameRunning;
        wasLogIn = isLogIn;
    }


    // 状态变化处理
    void HandleStateChange(ValState newState) {
        ValState oldState = currentState;
        currentState = newState;

        // 记录状态转换
        std::cout << "Val:State changed from " << StateToString(oldState)
            << " to " << StateToString(newState) << std::endl;

        // 根据状态转换执行相应操作
        switch (newState) {
        case ValState::WEGAME_STARTED:
            OnClientStarted();
            break;
        case ValState::WEGAME_CLOSED:
            OnClientClosed();
            break;
        case ValState::VAL_STARTED:
            OnMatchStarted();
            break;
        case ValState::VAL_ENDED:
            OnMatchEnded();
            break;
        case ValState::WEGAME_LOGIN:
            OnWegameLogin();
            break;
        case ValState::VAL_OVER:
            OnMatchOver();
            break;
        case ValState::WEGAME_INLOGIN:
            OnInLogin();
            break;
        }
    }

    // 状态字符串表示
    std::string StateToString(ValState state) {
        switch (state) {
        case ValState::WEGAME_STARTED: return "WE Started";
        case ValState::WEGAME_CLOSED: return "WE Closed";
        case ValState::VAL_STARTED: return "Match Started";
        case ValState::VAL_ENDED: return "Match Ended";
        case ValState::WEGAME_LOGIN: return "WE LogIn";
        case ValState::VAL_OVER: return "Match GameOver";
        case ValState::WEGAME_INLOGIN: return "WE InLogin";
        default: return "Unknown";
        }
    }

    // 各个状态的处理函数
    void OnClientStarted();

    void OnClientClosed();

    void OnMatchStarted();

    void OnMatchEnded();

    void OnWegameLogin();

    void OnMatchOver();

    void OnNotRunning();

    void OnInLogin();
};

std::wstring getWGPath();
