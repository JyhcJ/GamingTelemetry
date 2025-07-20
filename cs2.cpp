#include "pch.h" // Ԥ����ͷ�ļ�
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
#include "nloJson.h"

struct my_exception : virtual std::exception, virtual boost::exception {};

using errinfo_line = boost::error_info<struct tag_errinfo_line, int>;

using json = nlohmann::json;

using namespace std::chrono;

#pragma comment(lib, "ws2_32.lib")

// ȫ��״̬
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
// ����
const int GSI_PORT = 3000;
const int STREAK_TIMEOUT = 5; // ��ɱ��ʱ(��)
const int RECONNECT_TIMEOUT = 10; // ������ʱ(��)

std::chrono::system_clock::time_point matchStartTime;
// ��־��
//#define LOG(msg) std::cout << "[LOG] " << __FUNCTION__ << ": " << msg << std::endl
//#define LOG_ERROR(msg) std::cerr << "[ERROR] " << __FUNCTION__ << ": " << msg << " (Error: " << WSAGetLastError() << ")" << std::endl
void _sendHttp_cs2(nlohmann::json jsonBody) {
	HttpClient http;
	LOG_IMMEDIATE(jsonBody.dump());

	try {
		// 3. ����POST����
		g_mtx_header.lock();
		std::string response = http.SendRequest(
			L"https://"+ IS_DEBUG +L"asz.cjmofang.com/api/client/CsgoPostGameData",
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
	catch (...) {  // �������������쳣
		LOG_IMMEDIATE_ERROR("_sendHttp_LOL :::Unknown exception occurred");
	}
}

void _sendHttp_cs2(std::string type, nlohmann::json data) {
	nlohmann::json jsonBody;
	jsonBody["type"] = type;
	_sendHttp_cs2(jsonBody);
}
// �����ʼ��
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

// ״̬����/����
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

// ������Ϸ״̬
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

// ������ɱ�¼�
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
		LOG_IMMEDIATE("��ɱʱJSON:::" + data.dump(4));


		//jsonBody["game_uuid"] = std::to_string(g_state.startGameState["provider"]["timestamp"].get<int>());
		if (g_state.startGameState.contains("provider")) {
			const auto& provider = g_state.startGameState["provider"];
			if (provider.contains("timestamp") && !provider["timestamp"].is_null()) {
				jsonBody["game_uuid"] = std::to_string(provider["timestamp"].get<size_t>());
				std::cout << "Timestamp: " << provider["timestamp"] << std::endl;
			}
			else {
				std::cerr << "Timestamp not found or is null" << std::endl;
			}
		}
		else {
			jsonBody["game_uuid"] = "yhc" + GenerateUUID();
			std::cerr << "Provider object not found" << std::endl;
		}

		//jsonBody["game_uuid"]=g_state.startGameState["provider"].value("timestamp", GenerateUUID());

		jsonBody["event_id"] = std::to_string(data["provider"]["timestamp"].get<int>());
		jsonBody["computer_no"] = getComputerName();
		jsonBody["name"] = "";
		//jsonBody["game_mode"] = CS2_modeMap[data["map"]["mode"]];
		jsonBody["game_mode"] = data["map"]["mode"];
		jsonBody["team_size"] = "";
		jsonBody["user_game_rank"] = "";
		//jsonBody["data"] = "";
		jsonBody["remark"] = "";
		_sendHttp_cs2(jsonBody);

	}
}

// ������Ϸ����
void HandleMatchEnd(const json& lastMyData, const json& Data) {
	try {

		LOG_IMMEDIATE("����ʱdump lastMyself :" + lastMyData.dump(4));
		LOG_IMMEDIATE("����ʱdump :" + Data.dump(4));

		int ctScore = Data["map"]["team_ct"]["score"];
		int tScore = Data["map"]["team_t"]["score"];
		std::string playerTeam = lastMyData["player"]["team"];

		bool victory = (playerTeam == "CT" && ctScore > tScore) ||
			(playerTeam == "T" && tScore > ctScore);

		json jsonBody;
		jsonBody["type"] = "END";
		//if (g_state.startGameState["provider"]["timestamp"]!=NULL)
		//{
		//	jsonBody["game_uuid"] = std::to_string(g_state.startGameState["provider"]["timestamp"].get<int>());
		//}
		//else {
		//	// ������ ���ϲ��ᴥ��
		//	jsonBody["game_uuid"] = GenerateUUID();
		//}
		  // ��ȫ���� provider.timestamp
		if (g_state.startGameState.contains("provider")) {
			const auto& provider = g_state.startGameState["provider"];
			if (provider.contains("timestamp") && !provider["timestamp"].is_null()) {
				jsonBody["game_uuid"] = std::to_string(provider["timestamp"].get<size_t>());
				std::cout << "Timestamp: " << provider["timestamp"] << std::endl;
			}
			else {
				std::cerr << "Timestamp not found or is null" << std::endl;
			}
		}
		else {
			std::cerr << "Provider object not found" << std::endl;
		}

		//jsonBody["game_uuid"] = g_state.startGameState["provider"].value("timestamp", GenerateUUID());
		jsonBody["event_id"] = std::to_string(lastMyData["provider"]["timestamp"].get<int>());
		jsonBody["computer_no"] = getComputerName();
		jsonBody["name"] = "";
		//jsonBody["game_mode"] = CS2_modeMap[lastMyData["map"]["mode"]];
		jsonBody["game_mode"] = lastMyData["map"]["mode"];
		jsonBody["team_size"] = "";
		jsonBody["user_game_rank"] = "";
		nlohmann::json data1;
		data1["ren_tou_shu"] = lastMyData["player"]["match_stats"]["kills"];
		data1["win"] = victory == false ? 0 : 1;

		LOG_IMMEDIATE("win_team�ֶ�ʤ��:" + Data["round"]["win_team"].dump());

		auto duration = std::chrono::duration_cast<std::chrono::seconds>(
			std::chrono::system_clock::now() - matchStartTime
		);
		data1["time"] = duration.count();
		jsonBody["data"] = data1;
		nlohmann::json member;
		member["role"] = "self";
		member["id"] = lastMyData["provider"]["steamid"];
		//TODO �ҹ������ǲ��Ե�jsonBody["data"]�ᱻ���ǵİ�
		jsonBody["data"]["member"].push_back(member);
		jsonBody["remark"] = "";
		_sendHttp_cs2(jsonBody);
		//_sendHttp_cs2("KILL", "");

		LOG_IMMEDIATE("Match ended: " + std::string(victory ? "VICTORY" : "DEFEAT"));
		// ������ӷ��ͽ���Ĵ���

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
	// ����1����һ״̬����live�ҵ�ǰ��live
	LOG_IMMEDIATE("IsNewMatchStarting::" + current.dump(4) + " last:" + last.dump(4));
	/*if (!(last["map"]["phase"] == "warmup" && current["map"]["phase"] == "live")) {
		return false;
	}*/
	// ��Ĭ��ֵ

	std::string ts = getNestedValue<std::string>(current, { "provider", "timestamp" }, "unknown");

	//int id = getNestedValue<int>(data, { "provider", "id" }, -1);
	//bool active = getNestedValue<bool>(data, { "provider", "active" }, false);

	int score = getNestedValue<int>(current, { "map", "round" }, -1);
	std::string mode = getNestedValue<std::string>(current, {"map", "phase"}, "");

	if (score > 0 || mode == "warmup") {
		return false;
	}
	
	// ����2������������һ���
	return true;
		// ���A���غ�������
		//current["map"]["round"].get<int>() < last["map"]["round"].get<int>() ||

		//// ���B���ȷ�����
		//(current["map"]["team_ct"]["score"].get<int>() == 0 &&
		//	current["map"]["team_t"]["score"].get<int>() == 0) ||

		//// ���C����Ϸģʽ�ı�
		//current["map"]["mode"] != last["map"]["mode"]

		//||
		//// ���D��provider.timestamp���䣨����10���ӣ�
		//(current["provider"]["timestamp"] - last["provider"]["timestamp"] > 600)
		;
	;
}

// ����GSI����
void ProcessGSIData(const std::string& rawData) {
	try {
		// ��ȡJSON����
		size_t headerEnd = rawData.find("\r\n\r\n");
		if (headerEnd == std::string::npos) {
			LOG_IMMEDIATE_ERROR("Invalid HTTP format");
			return;
		}

		std::string jsonStr = rawData.substr(headerEnd + 4);
		json data;
		if (json::accept(jsonStr)) {
			data = json::parse(jsonStr); // ��ȫ����Ϊ�Ѽ����Ч��
			//std::cout << "Parsed JSON: " << data.dump(4) << std::endl;
		}
		else {
			std::cerr << "Invalid JSON string!" << std::endl;
			return;
		}
		//LOG_IMMEDIATE(data.dump(4));
		// ��������Ծʱ��
		g_state.lastUpdateTime = time(nullptr);

		// �����ұ��
		std::string steamID = data["provider"]["steamid"];
		if (!g_state.currentSteamID.empty() && g_state.currentSteamID != steamID) {
			LOG_IMMEDIATE("Player changed from " + g_state.currentSteamID + " to " + steamID);
			ResetGameState(true);
		}
		g_state.currentSteamID = steamID;

		// ��Ϸ״̬���
		bool isInGame = data["map"]["phase"] == "live";
		bool wasInGame = g_state.lastGameState["map"]["phase"] == "live";

		// ��Ϸ��ʼ TODO BUG�غ�ת��ʱ,���ж�Ϊ��ʼ
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
		// ��Ϸ����
		else if (wasInGame && !isInGame) {
			if (data["map"]["phase"] == "gameover") {
				_sendHttp_cs2("KILL", "");
				HandleMatchEnd(myLastData, data);
			}
		} 

		// ��ɱ��� (������һ�Ծʱ) //��ƥ��ϰʱ observer_slot ���ܲ�Ϊ0
		if (isInGame && data["player"]["activity"] == "playing" && (data["player"]["steamid"] == data["provider"]["steamid"])) {
			myLastData = data;
			int currentKills = data["player"]["match_stats"]["kills"];
			time_t now = time(nullptr);

			// ��ɱ��ʱ���
			if (g_state.killStreak > 0 && now - g_state.lastKillTime > STREAK_TIMEOUT) {
				//LOG_IMMEDIATE(g_state.startGameState.dump(4));
				HandleKillStreak(g_state.killStreak, data);
				g_state.killStreak = 0;
			}

			// �»�ɱ����
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
		// ���� JSON �����쳣
		std::cerr << "JSON parse error: " << e.what() << std::endl;
		// ��������緵��Ĭ��ֵ����־��¼�ȣ�
	}
	catch (const std::exception& e) {
		LOG_IMMEDIATE_ERROR("Processing failed: " + std::string(e.what()));
	}
}

// ��������߳�
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

// �������߳�
void GSIServer() {
	//LoadState();

	std::thread(HeartbeatMonitor).detach();

	while (g_running) {
		std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // �������ѭ��

		bool wuE = IsProcessRunning(L"5EClient.exe");
		bool wanmei = IsProcessRunning(L"�������羺��ƽ̨.exe");

		if (!wuE && !wanmei) {
			continue;
		}

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

// ����
void cs2Cleanup() {
	g_running = false;
	closesocket(g_serverSocket);
	WSACleanup();
}

std::string getCS2_location() {
	std::string relativePath = "\\Counter-Strike Global Offensive\\game\\csgo\\cfg";
	std::string fullPath = FindGamePath(relativePath);

	if (!fullPath.empty()) {
		std::cout << "����·��: " << fullPath << std::endl;
	}
	else {
		std::cout << "δ�ҵ�ָ��·��" << std::endl;
	}
	return fullPath;
}

bool CreateGameStateIntegrationFile(const std::string& directoryPath) {
	// ���������ļ�·��
	std::string filePath = directoryPath + "\\gamestate_integration_custom123456.cfg";

	// ����ļ��Ƿ��Ѵ���
	std::ifstream testFile(filePath);
	if (testFile.good()) {
		std::cout << "�ļ��Ѵ��ڣ��������κβ�����" << std::endl;
		testFile.close();
		return false;
	}
	testFile.close();

	// ����Ŀ¼����������ڣ�
	if (CreateDirectoryA(directoryPath.c_str(), NULL) ||
		GetLastError() == ERROR_ALREADY_EXISTS) {

		// ������д���ļ�
		std::ofstream outFile(filePath);
		if (!outFile.is_open()) {
			std::cerr << "�޷������ļ�: " << filePath << std::endl;
			return false;
		}

		// д���ļ�����
		outFile << "\"Console Sample v.1\"\n";
		outFile << "{\n";
		outFile << " \"uri\" \"http://127.0.0.1:3000\"\n";
		outFile << " \"timeout\" \"5.0\"\n";
		outFile << " \"buffer\"  \"0.1\"\n";
		outFile << " \"throttle\" \"0.5\"\n";
		outFile << " \"heartbeat\" \"60.0\"\n";
		outFile << " \"output\"\n";
		outFile << " {\n";
		outFile << "   \"precision_time\" \"3\"\n";
		outFile << "   \"precision_position\" \"1\"\n";
		outFile << "   \"precision_vector\" \"3\"\n";
		outFile << " }\n";
		outFile << " \"data\"\n";
		outFile << " {\n";
		outFile << "   \"provider\"            \"1\"      // general info about client being listened to: game name, appid, client steamid, etc.\n";
		outFile << "   \"map\"                 \"1\"      // map, gamemode, and current match phase ('warmup', 'intermission', 'gameover', 'live') and current score\n";
		outFile << "   \"round\"               \"1\"      // round phase ('freezetime', 'over', 'live'), bomb state ('planted', 'exploded', 'defused'), and round winner (if any)\n";
		outFile << "   \"player_id\"           \"1\"      // player name, clan tag, observer slot (ie key to press to observe this player) and team\n";
		outFile << "   \"player_state\"        \"1\"      // player state for this current round such as health, armor, kills this round, etc.\n";
		outFile << "   \"player_match_stats\"  \"1\"      // player stats this match such as kill, assists, score, deaths and MVPs\n";
		outFile << " }\n";
		outFile << "}\n";

		outFile.close();
		std::cout << "�ļ������ɹ�: " << filePath << std::endl;
		return true;
	}
	else {
		std::cerr << "�޷�����Ŀ¼: " << directoryPath << std::endl;
		return false;
	}
}



BOOL cs2Monitor() {
	try {
		std::string str = getCS2_location();
		if (str=="" ||str.empty())
		{
			LOG_IMMEDIATE_ERROR("û���ҵ�CS2��Ϸ·��.");
			return FALSE;
		}

		CreateGameStateIntegrationFile(str);
		if (!InitNetwork()) return FALSE;
		std::thread(GSIServer).detach();
	}
	catch (const std::exception& e) {
		LOG_IMMEDIATE_ERROR(std::string("Exception in DLL_PROCESS_ATTACH: ") + e.what());
		return FALSE;
	}
	return TRUE;
}
