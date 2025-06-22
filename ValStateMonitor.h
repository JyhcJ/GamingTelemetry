#pragma once

#include <Windows.h>
#include <mutex>
#include <string>
#include <thread>

#include <iostream>
#include "common.h"
enum class ValState {
    WEGAME_STARTED,     // �ͻ�������
    WEGAME_CLOSED,      // wegame�ر�
    VAL_STARTED,      // �Ծֿ�ʼ(�ͻ���������)
    VAL_ENDED         // �Ծֽ���(�ͻ���������)
};

class ValStateMonitor {
private:
    ValState currentState;
    std::mutex stateMutex;

    // ���̼�����
    bool wasProcessRunning = false;
    const std::wstring wegameProcessName = L"wegame.exe";
    const std::wstring valProcessName = L"VALORANT-Win64-Shipping.exe";

    // �Ծ�״̬������
    bool wasInGame = false;
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

        std::lock_guard<std::mutex> lock(stateMutex);

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
        }

        // ����״̬��¼
        wasProcessRunning = isWegameRunning;
        wasInGame = isGameRunning;
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
        }
    }

    // ״̬�ַ�����ʾ
    std::string StateToString(ValState state) {
        switch (state) {
        case ValState::WEGAME_STARTED: return "WE Started";
        case ValState::WEGAME_CLOSED: return "WE Closed";
        case ValState::VAL_STARTED: return "Match Started";
        case ValState::VAL_ENDED: return "Match Ended";
        default: return "Unknown";
        }
    }

    // ����״̬�Ĵ�����
    void OnClientStarted();

    void OnClientClosed();

    void OnMatchStarted();

    void OnMatchEnded();

    void OnNotRunning();
};