#pragma once

#include <string>
#include <atomic>

// ȫ�ֱ�������
//bool lol_running;
//std::string playerName;

// ��������
extern "C" __declspec(dllexport) void StartDetection();
extern "C" __declspec(dllexport) void StopDetection();
extern "C" __declspec(dllexport) const char* GetPlayerName();
void pollEvents();
void pollRankNum();
std::string getPlayerName();
std::string makeHttpsRequest(const std::wstring& url);
void _sendHttp_LOL(std::string type, std::string data);
