#pragma once

#include <Windows.h>
#include <mutex>
#include <string>
#include <thread>

#include <iostream>
#include "common.h"
#include "val.h"
enum class ValState {
    WEGAME_STARTED,     // �ͻ�������
    WEGAME_CLOSED,      // wegame�ر�
    VAL_STARTED,        // �Ծֿ�ʼ(�ͻ���������)
    VAL_ENDED,          // �Ծֽ���(�ͻ���������)
    WEGAME_LOGIN,       // ��¼
    VAL_OVER,           // �Ծֽ���
    WEGAME_INLOGIN      // ��¼��
};

class ValStateMonitor {
private:
    ValState currentState;
    std::mutex stateMutex;

    // ���̼�����
    bool wasProcessRunning = false;
    const std::wstring wegameProcessName = L"wegame.exe";
    const std::wstring valProcessName = L"VALORANT-Win64-Shipping.exe";
    const std::wstring railProcessName = L"rail.exe";

    // �Ծ�״̬������
    bool wasInGame = false;
    bool wasLogIn = false;
    std::string lastGameStatus;

public:
    ValStateMonitor() : currentState(ValState::WEGAME_CLOSED) {}

    // �����ѭ��
    void MonitorLoop() {
        while (true) {
            CheckStateTransition();
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

    // ���״̬ת��
    void CheckStateTransition() {
        bool isWegameRunning = IsProcessRunning(wegameProcessName);
        bool isGameRunning = IsProcessRunning(valProcessName);
        bool isLogIn = IsProcessRunning(railProcessName);
        //bool isGameOver = ��������?


        //bool isGameOver =  ���ı�
        
        //std::lock_guard<std::mutex> lock(stateMutex);

        // ״̬ת���߼�
        if (!wasProcessRunning && isWegameRunning) {
            HandleStateChange(ValState::WEGAME_STARTED);
        }
        else if (wasProcessRunning && !isWegameRunning) {
            HandleStateChange(ValState::WEGAME_CLOSED);
        }
        else if (isWegameRunning) {
            // �ͻ��������У������Ϸ״̬�仯
            if (!wasInGame && isGameRunning) {
                HandleStateChange(ValState::VAL_STARTED);
            }
            else if (wasInGame && !isGameRunning) {
                HandleStateChange(ValState::VAL_ENDED);
            }
            else if (!wasLogIn && isLogIn) {
                //�ӳ�(1000)
                std::this_thread::sleep_for(std::chrono::seconds(3));
                HandleStateChange(ValState::WEGAME_LOGIN);
            }
            else if (wasLogIn && !isLogIn) {
                HandleStateChange(ValState::WEGAME_INLOGIN);
            }
        }

        // ����״̬��¼
        wasProcessRunning = isWegameRunning;
        wasInGame = isGameRunning;
        wasLogIn = isLogIn;
    }


    // ״̬�仯����
    void HandleStateChange(ValState newState) {
        ValState oldState = currentState;
        currentState = newState;

        // ��¼״̬ת��
        std::cout << "Val:State changed from " << StateToString(oldState)
            << " to " << StateToString(newState) << std::endl;

        // ����״̬ת��ִ����Ӧ����
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

    // ״̬�ַ�����ʾ
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

    // ����״̬�Ĵ�����
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
