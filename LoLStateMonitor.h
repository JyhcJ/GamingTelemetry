#pragma once
#include <Windows.h>

#include <mutex>
#include <string>
#include <thread>

#include <iostream>
#include "common.h"
enum class LoLClientState {
    CLIENT_STARTED,     // �ͻ��˸�����
    CLIENT_CLOSED,      // �ͻ��˸չر�
    MATCH_STARTED,      // �Ծֿ�ʼ(�ͻ���������)
    MATCH_ENDED         // �Ծֽ���(�ͻ���������)
};

class LoLStateMonitor {
private:
    LoLClientState currentState;
    std::mutex stateMutex;

    // ���̼�����
    bool wasProcessRunning = false;
    const std::wstring lolProcessName = L"LeagueClient.exe";
    const std::wstring lolGameProcessName = L"League of Legends.exe";

    // �Ծ�״̬������
    bool wasInGame = false;
    std::string lastGameStatus;

public:
    LoLStateMonitor() : currentState(LoLClientState::CLIENT_CLOSED) {}

    // �����ѭ��
    void MonitorLoop() {
        while (true) {
            CheckStateTransition();
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

    // ���״̬ת��
    void CheckStateTransition() {
        bool isProcessRunning = IsProcessRunning(lolProcessName);
        bool isGameRunning = IsProcessRunning(lolGameProcessName);

        std::lock_guard<std::mutex> lock(stateMutex);

        // ״̬ת���߼�
        if (!wasProcessRunning && isProcessRunning) {
            HandleStateChange(LoLClientState::CLIENT_STARTED);
        }
        else if (wasProcessRunning && !isProcessRunning) {
            HandleStateChange(LoLClientState::CLIENT_CLOSED);
        }
        else if (isProcessRunning) {
            // �ͻ��������У������Ϸ״̬�仯
            if (!wasInGame && isGameRunning) {
                HandleStateChange(LoLClientState::MATCH_STARTED);
            }
            else if (wasInGame && !isGameRunning) {
                HandleStateChange(LoLClientState::MATCH_ENDED);
            }
        }

        // ����״̬��¼
        wasProcessRunning = isProcessRunning;
        wasInGame = isGameRunning;
    }

    // ���̼�⺯�� �Ѿ�����common ��ɾ��?
    //bool IsProcessRunning(const std::wstring& processName) {
    //    // Windowsʵ��
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

    // ״̬�仯����
    void HandleStateChange(LoLClientState newState) {
        LoLClientState oldState = currentState;
        currentState = newState;

        // ��¼״̬ת��
        std::cout << "State changed from " << StateToString(oldState)
            << " to " << StateToString(newState) << std::endl;

        // ����״̬ת��ִ����Ӧ����
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

    // ״̬�ַ�����ʾ
    std::string StateToString(LoLClientState state) {
        switch (state) {
        case LoLClientState::CLIENT_STARTED: return "Client Started";
        case LoLClientState::CLIENT_CLOSED: return "Client Closed";
        case LoLClientState::MATCH_STARTED: return "Match Started";
        case LoLClientState::MATCH_ENDED: return "Match Ended";
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