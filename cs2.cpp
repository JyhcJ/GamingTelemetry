#include "pch.h" // 预编译头文件
#include <windows.h>
#include <tlhelp32.h>
#include <iostream>
#include <thread>
#include <atomic>
#include <chrono>
#include <mutex>
#include <string>
#include <fstream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <nlohmann/json.hpp>
#include "ThreadSafeLogger.h"
#include "common.h"
#include "constant.h"
#include "HttpClient.h"
#include "cs2.h"

#include <boost/throw_exception.hpp>
#include <boost/exception/all.hpp>
#include <boost/exception/info.hpp>
#include <boost/exception/diagnostic_information.hpp>

struct my_exception : virtual std::exception, virtual boost::exception {};

using errinfo_line = boost::error_info<struct tag_errinfo_line, int>;

using json = nlohmann::json;

using namespace std::chrono;

#pragma comment(lib, "ws2_32.lib")

// 全局状态
struct GameState {
	int lastKills = 0;
	int killStreak = 0;
	time_t lastKillTime = 0;
	time_t lastUpdateTime = 0;
	json lastGameState;
	json startGameState;
	std::string currentSteamID;
};

std::mutex g_stateMutex;
GameState g_state;
json myLastData;
std::atomic<bool> g_running(true);
SOCKET g_serverSocket = INVALID_SOCKET;
static std::mutex g_mtx_header;
// 配置
const int GSI_PORT = 3000;
const int STREAK_TIMEOUT = 5; // 连杀超时(秒)
const int RECONNECT_TIMEOUT = 10; // 重连超时(秒)

std::chrono::system_clock::time_point matchStartTime;
// 日志宏
//#define LOG(msg) std::cout << "[LOG] " << __FUNCTION__ << ": " << msg << std::endl
//#define LOG_ERROR(msg) std::cerr << "[ERROR] " << __FUNCTION__ << ": " << msg << " (Error: " << WSAGetLastError() << ")" << std::endl
void _sendHttp_cs2(nlohmann::json jsonBody) {
	HttpClient http;
	LOG_IMMEDIATE(jsonBody.dump());

	try {
		// 3. 发送POST请求
		g_mtx_header.lock();
		std::string response = http.SendRequest(
			L"https://dev-asz.cjmofang.com/api/client/CsgoPostGameData",
			L"POST",
			getHeader(),
			jsonBody.dump()
		);
		g_mtx_header.unlock();
		LOG_IMMEDIATE("Response: " + UTF8ToGBK(response));
	}
	catch (const std::exception& e) {
		LOG_IMMEDIATE_ERROR("_sendHttp_LOL:::");
		LOG_IMMEDIATE_ERROR(e.what());
		g_mtx_header.unlock();
	}
	catch (...) {  // 捕获其他所有异常
		LOG_IMMEDIATE_ERROR("_sendHttp_LOL :::Unknown exception occurred");
	}
}

void _sendHttp_cs2(std::string type, nlohmann::json data) {
	nlohmann::json jsonBody;
	jsonBody["type"] = type;
	_sendHttp_cs2(jsonBody);
}
// 网络初始化
bool InitNetwork() {
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		LOG_IMMEDIATE_ERROR("WSAStartup failed");
		return false;

	}

	g_serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (g_serverSocket == INVALID_SOCKET) {
		LOG_IMMEDIATE_ERROR("Socket creation failed");
		return false;
	}

	sockaddr_in serverAddr{};
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = INADDR_ANY;
	serverAddr.sin_port = htons(GSI_PORT);

	if (bind(g_serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
		LOG_IMMEDIATE_ERROR("Bind failed");
		closesocket(g_serverSocket);
		return false;
	}

	if (listen(g_serverSocket, SOMAXCONN) == SOCKET_ERROR) {
		LOG_IMMEDIATE_ERROR("Listen failed");
		closesocket(g_serverSocket);
		return false;
	}

	LOG_IMMEDIATE("GSI server started on port " + std::to_string(GSI_PORT));
	return true;
}

// 状态保存/加载
void SaveState() {
	std::lock_guard<std::mutex> lock(g_stateMutex);
	std::ofstream file("cs2_state.json");
	if (file) {
		json state;
		state["lastKills"] = g_state.lastKills;
		state["killStreak"] = g_state.killStreak;
		state["lastKillTime"] = g_state.lastKillTime;
		state["currentSteamID"] = g_state.currentSteamID;
		file << state.dump(4);
		LOG_IMMEDIATE("State saved successfully: " + state.dump(4));
	}
}

void LoadState() {
	std::lock_guard<std::mutex> lock(g_stateMutex);
	std::ifstream file("cs2_state.json");
	if (file) {
		try {
			json state = json::parse(file);
			g_state.lastKills = state["lastKills"];
			g_state.killStreak = state["killStreak"];
			g_state.lastKillTime = state["lastKillTime"];
			g_state.currentSteamID = state["currentSteamID"];
			LOG_IMMEDIATE("State loaded successfully: " + state.dump(4));
		}
		catch (...) {
			LOG_IMMEDIATE("Failed to load state, using defaults");
		}
	}
}

// 重置游戏状态
void ResetGameState(bool fullReset = false) {
	std::lock_guard<std::mutex> lock(g_stateMutex);
	g_state.lastKills = 0;
	if (fullReset) {
		g_state.killStreak = 0;
		g_state.currentSteamID.clear();
		LOG_IMMEDIATE("game ALL state reset");
	}
	//LOG_IMMEDIATE("Game state reset" + std::string(fullReset ? " (full)" : ""));
}

// 处理连杀事件
void HandleKillStreak(int streak, json data) {
	if (streak >= 3 && streak <= 5) {
		nlohmann::json jsonBody;

		switch (streak) {
		case 3:
			jsonBody["type"] = "SAN_LIAN_SHA_SHU";
			break;
		case 4:
			jsonBody["type"] = "SI_LIAN_SHA_SHU";
			break;
		case 5:
			jsonBody["type"] = "WU_LIAN_SHA_SHU";
			break;
		}
		LOG_IMMEDIATE("连杀时JSON:::" + data.dump(4));
		jsonBody["game_uuid"] = std::to_string(g_state.startGameState["provider"]["timestamp"].get<int>());
		jsonBody["event_id"] = std::to_string(data["provider"]["timestamp"].get<int>());
		jsonBody["computer_no"] = getComputerName();
		jsonBody["name"] = "";
		jsonBody["game_mode"] = CS2_modeMap[data["map"]["mode"]];
		jsonBody["team_size"] = "";
		jsonBody["user_game_rank"] = "";
		//jsonBody["data"] = "";
		jsonBody["remark"] = "";
		_sendHttp_cs2(jsonBody);

	}
}

// 处理游戏结束
void HandleMatchEnd(const json& lastMyData, const json& Data) {
	try {

		LOG_IMMEDIATE("结束时dump lastMyself :" + lastMyData.dump(4));
		LOG_IMMEDIATE("结束时dump :" + Data.dump(4));

		int ctScore = Data["map"]["team_ct"]["score"];
		int tScore = Data["map"]["team_t"]["score"];
		std::string playerTeam = lastMyData["player"]["team"];

		bool victory = (playerTeam == "CT" && ctScore > tScore) ||
			(playerTeam == "T" && tScore > ctScore);

		json jsonBody;
		jsonBody["type"] = "END";
		jsonBody["game_uuid"] = std::to_string(g_state.startGameState["provider"]["timestamp"].get<int>());
		jsonBody["event_id"] = std::to_string(lastMyData["provider"]["timestamp"].get<int>());
		jsonBody["computer_no"] = getComputerName();
		jsonBody["name"] = "";
		jsonBody["game_mode"] = CS2_modeMap[lastMyData["map"]["mode"]];
		jsonBody["team_size"] = "";
		jsonBody["user_game_rank"] = "";
		nlohmann::json member;
		member["role"] = "self";
		member["id"] = lastMyData["provider"]["steamid"];
		jsonBody["data"]["member"].push_back(member);
		nlohmann::json data1;
		data1["ren_tou_shu"] = lastMyData["player"]["match_stats"]["kills"];
		data1["win"] = victory == false ? 0 : 1;

		LOG_IMMEDIATE("win_team字段胜负:" + Data["round"]["win_team"].dump());

		auto duration = std::chrono::duration_cast<std::chrono::seconds>(
			std::chrono::system_clock::now() - matchStartTime
		);
		data1["time"] = duration.count();
		jsonBody["data"] = data1;
		jsonBody["remark"] = "";
		_sendHttp_cs2(jsonBody);
		_sendHttp_cs2("KILL", "");

		LOG_IMMEDIATE("Match ended: " + std::string(victory ? "VICTORY" : "DEFEAT"));
		// 这里添加发送结果的代码

		ResetGameState(true);
	}
	catch (const boost::exception& e) {
		if (auto file = boost::get_error_info<boost::errinfo_file_name>(e)) {
			int line = *boost::get_error_info<errinfo_line>(e);
			std::cout << "Boost Exception thrown at " << *file << ":" << line << std::endl;
			//LOG_IMMEDIATE("Boost Exception thrown at " + std::string(file) + ":" + std::to_string(line));
		}
		LOG_IMMEDIATE("Boost Exception: " + std::string(diagnostic_information(e)));
	}
	catch (const json::exception& e) {
		LOG_IMMEDIATE_ERROR("Failed to parse match end: " + std::string(e.what()));
	}
}

bool IsNewMatchStarting(const json& current, const json& last) {
	// 条件1：上一状态不是live且当前是live
	LOG_IMMEDIATE("IsNewMatchStarting::" + current.dump(4) + " last:" + last.dump(4));
	if (!(last["map"]["phase"] == "warmup" && current["map"]["phase"] == "live")) {
		return false;
	}

	// 条件2：满足以下任一情况
	return
		// 情况A：回合数归零
		current["map"]["round"] < last["map"]["round"] ||

		// 情况B：比分重置
		(current["map"]["team_ct"]["score"] == 0 &&
			current["map"]["team_t"]["score"] == 0) ||

		// 情况C：游戏模式改变
		current["map"]["mode"] != last["map"]["mode"]

		//||
		//// 情况D：provider.timestamp跳变（超过10分钟）
		//(current["provider"]["timestamp"] - last["provider"]["timestamp"] > 600)
		;
	;
}

// 处理GSI数据
void ProcessGSIData(const std::string& rawData) {
	try {
		// 提取JSON部分
		size_t headerEnd = rawData.find("\r\n\r\n");
		if (headerEnd == std::string::npos) {
			LOG_IMMEDIATE_ERROR("Invalid HTTP format");
			return;
		}

		std::string jsonStr = rawData.substr(headerEnd + 4);
		json data;
		if (json::accept(jsonStr)) {
			data = json::parse(jsonStr); // 安全，因为已检查有效性
			//std::cout << "Parsed JSON: " << data.dump(4) << std::endl;
		}
		else {
			std::cerr << "Invalid JSON string!" << std::endl;
			return;
		}

		// 更新最后活跃时间
		g_state.lastUpdateTime = time(nullptr);

		// 检查玩家变更
		std::string steamID = data["provider"]["steamid"];
		if (!g_state.currentSteamID.empty() && g_state.currentSteamID != steamID) {
			LOG_IMMEDIATE("Player changed from " + g_state.currentSteamID + " to " + steamID);
			ResetGameState(true);
		}
		g_state.currentSteamID = steamID;

		// 游戏状态检测
		bool isInGame = data["map"]["phase"] == "live";
		bool wasInGame = g_state.lastGameState["map"]["phase"] == "live";

		// 游戏开始 TODO BUG回合转换时,会判断为开始
		if (!wasInGame && isInGame) {
			bool isNewMatch = IsNewMatchStarting(data, g_state.lastGameState);
			if (isNewMatch)
			{
				_sendHttp_cs2("RUN", data);
				LOG_IMMEDIATE("Match started");
				//LOG_IMMEDIATE(data.dump(4));
				g_state.startGameState = data;
				matchStartTime = std::chrono::system_clock::now();
				ResetGameState();
			}
		}
		// 游戏结束
		else if (wasInGame && !isInGame) {
			if (data["map"]["phase"] == "gameover") {
				_sendHttp_cs2("KILL", "");
				HandleMatchEnd(myLastData, data);
			}
		}

		// 连杀检测 (仅当玩家活跃时)
		if (isInGame && data["player"]["activity"] == "playing" && data["player"]["observer_slot"] == 0) {
			myLastData = data;
			int currentKills = data["player"]["match_stats"]["kills"];
			time_t now = time(nullptr);

			// 连杀超时检测
			if (g_state.killStreak > 0 && now - g_state.lastKillTime > STREAK_TIMEOUT) {
				//LOG_IMMEDIATE(g_state.startGameState.dump(4));
				HandleKillStreak(g_state.killStreak, data);
				g_state.killStreak = 0;
			}

			// 新击杀处理
			if (currentKills > g_state.lastKills) {
				int killDiff = currentKills - g_state.lastKills;

				if (now - g_state.lastKillTime <= STREAK_TIMEOUT) {
					g_state.killStreak += killDiff;
				}
				else {
					g_state.killStreak = killDiff;
				}

				g_state.lastKillTime = now;
				g_state.lastKills = currentKills;
				SaveState();
			}
		}

		g_state.lastGameState = data;
	}
	catch (const json::parse_error& e) {
		// 捕获 JSON 解析异常
		std::cerr << "JSON parse error: " << e.what() << std::endl;
		// 处理错误（如返回默认值、日志记录等）
	}
	catch (const std::exception& e) {
		LOG_IMMEDIATE_ERROR("Processing failed: " + std::string(e.what()));
	}
}

// 心跳检测线程
void HeartbeatMonitor() {
	while (g_running) {
		time_t now = time(nullptr);

		if (now - g_state.lastUpdateTime > RECONNECT_TIMEOUT) {
			//LOG_IMMEDIATE("Connection timeout detected");
			ResetGameState();
		}

		std::this_thread::sleep_for(seconds(5));
	}
}

// 主服务线程
void GSIServer() {
	//LoadState();

	std::thread(HeartbeatMonitor).detach();

	while (g_running) {
		std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // 避免过快循环
		sockaddr_in clientAddr;
		int addrLen = sizeof(clientAddr);
		SOCKET clientSocket = accept(g_serverSocket, (sockaddr*)&clientAddr, &addrLen);

		if (clientSocket == INVALID_SOCKET) {
			if (g_running) LOG_IMMEDIATE_ERROR("Accept failed");
			continue;
		}

		char buffer[4096];
		int bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);

		if (bytesRead > 0) {
			ProcessGSIData(std::string(buffer, bytesRead));
		}

		closesocket(clientSocket);
	}

	//SaveState();
}

// 清理
void cs2Cleanup() {
	g_running = false;
	closesocket(g_serverSocket);
	WSACleanup();
}

BOOL cs2Monitor() {
	try {
		if (!InitNetwork()) return FALSE;
		std::thread(GSIServer).detach();
	}
	catch (const std::exception& e) {
		LOG_IMMEDIATE_ERROR(std::string("Exception in DLL_PROCESS_ATTACH: ") + e.what());
		return FALSE;
	}
	return TRUE;
}
