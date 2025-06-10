#pragma once

#include <string>
#include <atomic>

#include<nlohmann/json.hpp>

// 全局变量声明
//bool lol_running;
//std::string playerName;

// 函数声明
__declspec(dllexport) const char* GetPlayerName();
void pollEvents();
void pollRankNum();
std::string getPlayerName();
std::string makeHttpsRequest(const std::wstring& url);
void _sendHttp_LOL(std::string type, std::string data);
void _sendHttp_LOL(nlohmann::json jsonBody);