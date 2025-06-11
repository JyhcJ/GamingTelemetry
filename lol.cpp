#include "pch.h"
#include "lol.h"
#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <map>
#include <mutex>
#include <sstream>
#include <thread>
#include <vector>
#include <windows.h>
#include <winhttp.h>
#include "HttpClient.h"
#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"
#include "common.h"
#include <unordered_set>
#include "ThreadSafeLogger.h"
#include <string>
#include <tchar.h>
#include <atlstr.h>
#include <wincrypt.h>
#include <algorithm>
#pragma comment(lib, "crypt32.lib")

#include <tlhelp32.h>  // ����PROCESSENTRY32����غ���
#include <wchar.h>     // ����_wcsicmp����


#include <codecvt>
#include <fstream>

#include <locale>

#include <comutil.h>
#include <http.h>


#include <wininet.h>
#include <shellapi.h>
#include "lol_before.h"


#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "comsuppw.lib")

// ȫ�ֱ�������
extern bool is_lol_game_running;
extern bool is_lol_running;
extern nlohmann::json g_infoBefore;

//extern std::string BEFORE_NUMS_COUNT;
//extern std::string BEFORE_REGION;
//extern std::string BEFORE_RANK;
//extern std::string BEFORE_STATE;
//extern std::string BEFORE_TYPE;

int MULTIKILL;
int DEATHS;
bool isCHAOSHEN;
int EVENTID;
std::string playerName;
std::map<std::string, std::vector<time_t>> kill_history;
std::mutex kill_mutex;
const int MULTIKILL_WINDOW = 10; // ��ɱʱ�䴰��(��)
std::unordered_set<int> processed_event_ids;


struct PostGameData {
	std::string game_uuid;
	std::string computer_no;
	std::string name;
	std::string game_mode;
	std::string team_size;
	std::string user_game_rank;
	std::string eventID;
	std::string type;
	std::string data;
}pgd;

void _sendHttp_LOL(PostGameData pgd);

// ���ó���
const int POLL_INTERVAL = 1;
//const std::wstring LCU_URL = L"https://127.0.0.1:2999/liveclientdata/eventdata";
const std::wstring LCU_URL = L"https://127.0.0.1:2999/liveclientdata/allgamedata";
const std::wstring NAME_URL = L"https://127.0.0.1:2999/liveclientdata/activeplayername";
const std::map<std::wstring, std::wstring> HEADERS = {
	/*	{ L"Content-Type", L"application/json" },
		{ L"User-Agent", L"Mozilla/5.0" },
		{ L"token", L"{{bToken}}" },*/

		{   L"language",L"ZH_CN" },
		{   L"merchantId",L"53" },
		{   L"sec-ch-ua-platform",L"\"Windows\""                                                                                                    },
		{   L"Referer",L"https://dev-asz.cjmofang.com/activity/activityManagement/createActivity/MODE_SIGN/0/add/0"                                 },
		{   L"sec-ch-ua",L"\"Chromium\";v=\"136\", \"Google Chrome\";v=\"136\", \"Not.A/Brand\";v=\"99\""                                           },
		{   L"sec-ch-ua-mobile",L"?0"                                                                                                               },
		{   L"barId",L"98"                                                                                                                          },
		{   L"Accept",L"application/json, text/plain, */*"                                                                                          },
		{   L"Content-Type",L"application/json"                                                                                                     },
		{   L"organizationType",L"\"BAR\""                                                                                                          },
		{   L"token",L"eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJSZW1vdGVJcCI6IiIsIkxvY2FsTG9naW4iOjAsIkNvbnRleHQiOnsidXNlcl9pZCI6MjQ3LCJ1c2VyX25hbWUiOiJ4eHgiLCJ1dWlkIjoiIiwicmlkIjoxOCwibWFudWZhY3R1cmVfaWQiOjUzLCJiYXJfaWQiOjk4LCJyb290X2lkIjowLCJvcmdhbml6YXRpb25fdHlwZSI6IiIsInBsYXRmb3JtIjoiYmFyY2xpZW50In0sImV4cCI6MTc1MDQ2ODMwMH0.21sEbRTirJggWvWlOygMOczAQWs8vQd0hh0ZKJuNTbs"                                                                                                                  },
		{   L"User-Agent",L"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/136.0.0.0 Safari/537.36\r\n"        },
		{   L"Accept-Encoding",L"gzip, deflate, br"                                                                                                 },
		{   L"Connection",L"keep-alive"                                                                                                             },
		{   L"Cache-Control",L"no-cache"                                                                                                            },
		{   L"Host",L"127.0.0.1:8000"                                                                                                                }
};


// ��ȡ��ǰʱ���ַ���
std::string getCurrentTimeString() {
	auto now = std::chrono::system_clock::now();
	auto in_time_t = std::chrono::system_clock::to_time_t(now);
	std::tm tm;
	localtime_s(&tm, &in_time_t);

	std::stringstream ss;
	ss << std::put_time(&tm, "[%H:%M:%S]");
	return ss.str();
}

// ��ȡ��ɱ��ǩ
std::string getMultikillLabel(int killCount) {
	if (killCount == 2) return "����˫ɱ (Double Kill)";
	if (killCount == 3) return "������ɱ (Triple Kill)";
	if (killCount == 4) return "������ɱ (Quadra Kill)";
	if (killCount >= 5) return "������ɱ (Penta Kill)";
	return "";
}

std::string ExecuteCommand(const std::wstring& command) {
	SECURITY_ATTRIBUTES sa;
	sa.nLength = sizeof(sa);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = TRUE;

	HANDLE hReadPipe, hWritePipe;
	if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, 0)) {
		return "Error creating pipe";
	}

	STARTUPINFOW si;
	PROCESS_INFORMATION pi;
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
	si.hStdOutput = hWritePipe;
	si.hStdError = hWritePipe;
	si.wShowWindow = SW_HIDE;

	ZeroMemory(&pi, sizeof(pi));

	// ����cmd����
	std::wstring cmd = L"cmd.exe /c " + command;
	if (!CreateProcessW(NULL, &cmd[0], NULL, NULL, TRUE,
		CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
		CloseHandle(hReadPipe);
		CloseHandle(hWritePipe);
		return "Error creating process";
	}

	CloseHandle(hWritePipe);

	// ��ȡ���
	std::string result;
	char buffer[4096];
	DWORD bytesRead;
	while (true) {
		if (!ReadFile(hReadPipe, buffer, sizeof(buffer), &bytesRead, NULL) || bytesRead == 0) {
			break;
		}
		result.append(buffer, bytesRead);
	}

	CloseHandle(hReadPipe);
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	return result;
}

std::string extractParamValue(const std::string& commandLine, const std::string& paramName) {
	// ���Ҳ�������λ��
	size_t paramPos = commandLine.find(paramName);
	if (paramPos == std::string::npos) {
		return ""; // ����������
	}

	// �����������͵Ⱥ�
	size_t valueStart = paramPos + paramName.length();
	if (commandLine[valueStart] == '=') {
		valueStart++; // �����Ⱥ�
	}

	// ����ֵ�Ľ���λ�ã��ո���ַ�����β��
	size_t valueEnd = commandLine.find(' ', valueStart);
	if (valueEnd == std::string::npos) {
		valueEnd = commandLine.length();
	}

	// ��ȡֵ
	std::string value = commandLine.substr(valueStart, valueEnd - valueStart);

	// ȥ�����ܵ�����
	value.erase(std::remove(value.begin(), value.end(), '\"'), value.end());

	return value;
}


// ����HTTPS����
std::string makeHttpsRequest(const std::wstring& url) {
	HINTERNET hSession = WinHttpOpen(
		L"Valorant Multikill Detector/1.0",
		WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
		WINHTTP_NO_PROXY_NAME,
		WINHTTP_NO_PROXY_BYPASS,
		0
	);

	if (!hSession) {
		return "";
	}

	URL_COMPONENTS urlComp;
	memset(&urlComp, 0, sizeof(urlComp));
	urlComp.dwStructSize = sizeof(urlComp);
	urlComp.dwSchemeLength = (DWORD)-1;
	urlComp.dwHostNameLength = (DWORD)-1;
	urlComp.dwUrlPathLength = (DWORD)-1;
	//
	if (!WinHttpCrackUrl(url.c_str(), (DWORD)url.length(), 0, &urlComp)) {
		WinHttpCloseHandle(hSession);
		return "";
	}

	std::wstring host(urlComp.lpszHostName, urlComp.dwHostNameLength);
	std::wstring path(urlComp.lpszUrlPath, urlComp.dwUrlPathLength);

	HINTERNET hConnect = WinHttpConnect(hSession, host.c_str(), urlComp.nPort, 0);
	if (!hConnect) {
		WinHttpCloseHandle(hSession);
		return "";
	}

	HINTERNET hRequest = WinHttpOpenRequest(
		hConnect,
		L"GET",
		path.c_str(),
		NULL,
		WINHTTP_NO_REFERER,
		WINHTTP_DEFAULT_ACCEPT_TYPES,
		WINHTTP_FLAG_SECURE
	);

	if (!hRequest) {
		WinHttpCloseHandle(hConnect);
		WinHttpCloseHandle(hSession);
		return "";
	}

	// ����SSL֤����֤(������Python�е�verify=False)
	DWORD dwFlags = SECURITY_FLAG_IGNORE_UNKNOWN_CA |
		SECURITY_FLAG_IGNORE_CERT_WRONG_USAGE |
		SECURITY_FLAG_IGNORE_CERT_CN_INVALID |
		SECURITY_FLAG_IGNORE_CERT_DATE_INVALID;

	WinHttpSetOption(hRequest, WINHTTP_OPTION_SECURITY_FLAGS, &dwFlags, sizeof(dwFlags));

	if (!WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0)) {
		WinHttpCloseHandle(hRequest);
		WinHttpCloseHandle(hConnect);
		WinHttpCloseHandle(hSession);
		return "";
	}

	if (!WinHttpReceiveResponse(hRequest, NULL)) {
		WinHttpCloseHandle(hRequest);
		WinHttpCloseHandle(hConnect);
		WinHttpCloseHandle(hSession);
		return "";
	}

	std::string response;
	DWORD dwSize = 0;
	DWORD dwDownloaded = 0;
	LPSTR pszOutBuffer;

	do {
		dwSize = 0;
		if (!WinHttpQueryDataAvailable(hRequest, &dwSize)) {
			break;
		}

		pszOutBuffer = new char[dwSize + 1];
		if (!pszOutBuffer) {
			break;
		}

		ZeroMemory(pszOutBuffer, dwSize + 1);
		if (!WinHttpReadData(hRequest, (LPVOID)pszOutBuffer, dwSize, &dwDownloaded)) {
			delete[] pszOutBuffer;
			break;
		}

		response.append(pszOutBuffer, dwSize);
		delete[] pszOutBuffer;
	} while (dwSize > 0);

	WinHttpCloseHandle(hRequest);
	WinHttpCloseHandle(hConnect);
	WinHttpCloseHandle(hSession);

	return response;
}

// ����JSON�¼�����
void processEvents(const std::string& jsonData) {
	//LOG_IMMEDIATE("!!!:" + jsonData);

	const char* jsonDataCStr = jsonData.c_str();

	rapidjson::Document data1;

	nlohmann::json jsonbody = g_infoBefore;

	data1.Parse(jsonDataCStr);

	if (data1.HasMember("gameData"))
	{
		const rapidjson::Value& temp = data1["gameData"];
		if (temp.HasMember("gameMode")) {
			pgd.game_mode = temp["gameMode"].GetString();
		}
		else {
			LOG_IMMEDIATE_ERROR("json����:" + jsonData + "\n" + "û���ҵ�gameMode�ֶ�");
			return;
		}
	}
	else {
		LOG_IMMEDIATE_ERROR("json����:" + jsonData + "\n" + "û���ҵ�gameData�ֶ�");
		return;
	}
	//pgd.game_mode = data1["gameData"]["gameMode"].GetString();
	//pgd.team_size = "1";
	// ���� JSON �ַ���
	/*if (doc.Parse(jsonDataCStr).HasParseError()) {
		std::cerr << "JSON ��������" << std::endl;
		return;
	}
	std::cout << getCurrentTimeString() << " �������¼�����: " << jsonData << std::endl;*/
	const rapidjson::Value& data = data1["events"];

	// ���Events�����Ƿ����
	if (!data.HasMember("Events") || !data["Events"].IsArray()) {
		LOG_IMMEDIATE_ERROR("json����:" + jsonData + "\n" + "û����ȷʶ��Events�ֶ�");
		return;
	}
	//TODO ���computer_no Ҫ��barclient�в�?
	//pgd.computer_no = "ttt";
	//playerName = "����������s";
	playerName = getPlayerName();
	const rapidjson::Value& events = data["Events"];
	time_t current_time = time(nullptr);

	// TODO�����������Ϊ����
	//pgd.name = "LOL";
	for (rapidjson::SizeType i = 0; i < events.Size(); i++) {
		const rapidjson::Value& event = events[i];

		// ����¼�ID�Ƿ��Ѵ���
		if (!event.HasMember("EventID") || !event["EventID"].IsInt()) {

			//TODO ����ID���������ﴦ��
			continue;
		}

		int event_id = event["EventID"].GetInt();
		if (processed_event_ids.find(event_id) != processed_event_ids.end()) {
			continue; // �Ѵ���
		}

		// ����: kill>7 ��ɱ�͵�ɱʱ count++  ������+1ʱ, count = 0   ,����Ҳ��eventid
		// TODO ��û��֤����
		int die;
		const rapidjson::Value& players = data1["allPlayers"];
		for (rapidjson::SizeType i = 0; i < players.Size(); i++) {
			const rapidjson::Value& player = players[i];
			if (player["riotIdGameName"].GetString() != playerName) {
				continue;
			}
			else {
				die = player["scores"]["deaths"].GetInt();
				if (die != DEATHS && die != 0) {
					MULTIKILL = 0;
					DEATHS = die;
					isCHAOSHEN = false;
				}
				else if (MULTIKILL == 7) {
					isCHAOSHEN = true;
					//���ͳ���
					jsonbody["event_id"] = std::to_string(event_id) + "_chaoshen";
					jsonbody["type"] = "CHAO_SHEN_SHU";
					_sendHttp_LOL(jsonbody);
				}


			}
		}

		// ����Ƿ�ΪMultikill�¼�
		if (event.HasMember("EventName") && event["EventName"].IsString() &&
			std::string(event["EventName"].GetString()) == "Multikill") {
			// ��ȡ��ɱ������
			if (!event.HasMember("KillerName") || !event["KillerName"].IsString()) {
				continue;
			}

			std::string killer = event["KillerName"].GetString();
			if (killer.empty() || killer != playerName) {
				continue;
			}

			// ��ӵ�ǰ��ɱʱ��
			kill_history[killer].push_back(current_time);

			// ��ȡ��ɱ��
			int kill_count = 0;
			if (event.HasMember("KillStreak") && event["KillStreak"].IsInt()) {
				kill_count = event["KillStreak"].GetInt();
				jsonbody["event_id"] = std::to_string(event["EventID"].GetInt());
				MULTIKILL += kill_count;
				switch (kill_count) {
					/*	case 2:
							pgd.type = "2_SHU";
							break;*/
				case 3:
					jsonbody["type"] = "SAN_SHA_SHU";
					break;
				case 4:
					jsonbody["type"] = "SI_SHA_SHU";
					break;
				case 5:
					jsonbody["type"] = "WU_SHA_SHU";
					break;
				}
				if (kill_count >= 3) {
					_sendHttp_LOL(jsonbody);
				}

				
			}

		}
		else {
			//LOG_IMMEDIATE_WARNING("û����ȷʶ��EventID�ֶ�");
		}
		// TODO �����Ż�������if��
		if (event.HasMember("EventName") && event["EventName"].IsString() &&
			std::string(event["EventName"].GetString()) == "ChampionKill") {
			// ��ȡ��ɱ������
			if (!event.HasMember("KillerName") || !event["KillerName"].IsString()) {
				continue;
			}
			std::string killer = event["KillerName"].GetString();
			const rapidjson::Value& players = data1["allPlayers"];
			if (killer.empty() || killer != playerName) {
				//���˻�ɱ �ж������Ƿ�ı� (���Ҫʵʱ�ϴ�����)
				//for (rapidjson::SizeType i = 0; i < players.Size(); i++) {
				//	const rapidjson::Value& player = players[i];
				//	if (player["riotIdGameName"].GetString() != playerName) {
				//		continue;
				//	}
				//	else {
				//		pgd.type_count = std::to_string(player["scores"]["assists"].GetInt());
				//		//_sendHttp_LOL(pgd);
				//	}
				//}

				continue;
			}
			//pgd.type = "CUMULATIVE_KILLS";
			//data["allPlayers"]["riotIdGameName"].GetString();
			for (rapidjson::SizeType i = 0; i < players.Size(); i++) {
				const rapidjson::Value& player = players[i];
				if (player["riotIdGameName"].GetString() != playerName) {
					continue;
				}
				else {
					// �ۼ�ɱ��
					MULTIKILL += 1;
					// �½ӿڷ�������
					//pgd.type = 
					//pgd.type_count = std::to_string(player["scores"]["kills"].GetInt());
					//_sendHttp_LOL(pgd);
				}
			}
		}
		else {
			//LOG_IMMEDIATE_WARNING("û����ȷʶ��EventName�ֶ�");
		}

		// ����¼�Ϊ�Ѵ���
		processed_event_ids.insert(event_id);
	}
}



void pollEvents() {

	while (is_lol_game_running) {
		std::string response = makeHttpsRequest(LCU_URL);

		//TODO ���ﴦ��gameID
		//std::string response_gamedata = makeHttpsRequest(LCU_URL);

		//std::cout << response << std::endl;
		//TODO  ��UTF-8����
		if (!response.empty()) {
			processEvents(response);
		}
		else {
			is_lol_game_running = false;
			LOG_IMMEDIATE("LOL:�ȴ��Ծֿ�ʼ");
			return;
		}
		// 1 ����ѯ �ɸ���
		std::this_thread::sleep_for(std::chrono::seconds(POLL_INTERVAL));
	}
}

void pollRankNum() {
	
	Game_Before gb;

	while (is_lol_running) {

		//���Բ��...��������?
		gb.before_main("update");

		//std::cout << response << std::endl;
		//TODO  ��UTF-8����
		/*if (!response.empty()) {
			processEvents(response);
		}
		else {
			is_lol_game_running = false;
			LOG_IMMEDIATE("LOL:�ȴ��Ծֿ�ʼ");
			return;
		}*/
		//std::this_thread::sleep_for(std::chrono::seconds(POLL_INTERVAL));
		// 
		// 5 ����������� Ӧ�ÿ��Ը����
		std::this_thread::sleep_for(std::chrono::seconds(5));
	}

}

std::string getPlayerName() {
	std::string name = "-1";
	std::string response = makeHttpsRequest(NAME_URL);

	if (!response.empty()) {
		size_t pos = response.find('#');
		if (pos != std::string::npos) {
			name = response.substr(0, pos);
		}
		else {
			name = response;
		}

		// �����ַ���
		name.erase(std::remove(name.begin(), name.end(), '\"'), name.end());
		name.erase(std::remove(name.begin(), name.end(), '\r'), name.end());
		name.erase(std::remove(name.begin(), name.end(), '\n'), name.end());
		name.erase(0, name.find_first_not_of(" \t"));
		name.erase(name.find_last_not_of(" \t") + 1);
	}

	return name;
}

// ������Ϣ (��Ҫ���ڶԾ���ʵʱ��Ϣ)
void _sendHttp_LOL(PostGameData pgd) {
	HttpClient http;
	//json jsonBody = {
	//{"computer_no", computer_no},
	//{"name", name},
	//{"game_mode", game_mode},
	//{"team_size", team_size},
	//{"type", type},
	//{"type_count", std::to_string(type_count)}  // ����������������ַ���
	//};


	std::string jsonBody =
		"{\n"
		"\"computer_no\": \"" + pgd.computer_no + "\",\n"
		"\"name\": \"" + pgd.name + "\",\n"
		"\"game_mode\": \"" + "TODO" + "\",\n"
		"\"user_game_rank\": \"" +  + "\",\n"
		//"\"team_size\": \"" + teamSizeMap[pgd.team_size] + "\",\n"
		"\"type\": \"" + pgd.type + "\",\n"
		"\"eventID\": \"" + pgd.eventID + "\"\n"
		"\"data\": \"" + pgd.data + "\"\n"
		"}";
	LOG_IMMEDIATE(jsonBody);

	/*	//JSON������
		jsonBody = R"({
		"computer_no":"A046",
		"name": "test",
		"game_mode": "MATCH",
		"team_size": "ONE",
		"type": "SERIES_KILLS",
		"type_count": "3"
	})";*/

	try {
		// 3. ����POST����
		std::string response = http.SendRequest(
			L"https://dev-asz.cjmofang.com/api/client/PostGameData",
			L"POST",
			HEADERS,
			jsonBody
		);

		LOG_IMMEDIATE("Response: " + response);
	}
	catch (const std::exception& e) {
		std::cerr << "Error: " << e.what() << std::endl;
		LOG_ERROR(e.what());
	}
}
// ������Ϣ (��Ҫ���ڿͻ�����������)
void _sendHttp_LOL(std::string type,std::string data) {
	HttpClient http;
	nlohmann::json jsonBody;
	jsonBody["type"] = type;
	LOG_IMMEDIATE(jsonBody.dump(4));
	try {
		// 3. ����POST����
		std::string response = http.SendRequest(
			L"https://dev-asz.cjmofang.com/api/client/PostGameData",
			L"POST",
			HEADERS,
			jsonBody.dump()
		);

		LOG_IMMEDIATE("Response: " + UTF8ToGBK(response));
	}
	catch (const std::exception& e) {
		//std::cerr << "Error: " << e.what() << std::endl;
		LOG_IMMEDIATE_ERROR("_sendHttp_LOL:::");
		LOG_IMMEDIATE_ERROR(e.what());
	}

	catch (...) {  // �������������쳣
		LOG_IMMEDIATE_ERROR("_sendHttp_LOL :::Unknown exception occurred");
	}
}

// ������Ϣ (��Ҫ���ڶԾֽ�����)
void _sendHttp_LOL(nlohmann::json jsonBody) {
	HttpClient http;

	LOG_IMMEDIATE(jsonBody.dump(4));

	try {
		// 3. ����POST����
		std::string response = http.SendRequest(
			L"https://dev-asz.cjmofang.com/api/client/PostGameData",
			L"POST",
			HEADERS,
			jsonBody.dump()
		);
	
		LOG_IMMEDIATE("Response: " + UTF8ToGBK(response));
	}
	catch (const std::exception& e) {
		LOG_IMMEDIATE_ERROR("_sendHttp_LOL:::");
		LOG_IMMEDIATE_ERROR(e.what());
	}
	catch (...) {  // �������������쳣
		LOG_IMMEDIATE_ERROR("_sendHttp_LOL :::Unknown exception occurred");
	}
}

//extern "C" __declspec(dllexport) const char* GetPlayerName() {
//	return playerName.c_str();
//}

// ������ͨ
extern "C" __declspec(dllexport) int Add(int a, int b) {
	return a + b;
	//_sendHttp_LOL("");
}