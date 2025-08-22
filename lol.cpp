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

#include <tlhelp32.h>  // 包含PROCESSENTRY32和相关函数
#include <wchar.h>     // 用于_wcsicmp函数


#include <codecvt>
#include <fstream>

#include <locale>

#include <comutil.h>
#include <http.h>


#include <wininet.h>
#include <shellapi.h>
#include "lol_before.h"
#include "constant.h"


#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "comsuppw.lib")

// 全局变量定义
extern bool is_lol_game_running;
extern bool is_lol_running;
extern nlohmann::json g_infoBefore;
extern std::mutex g_mtx;
//extern std::string BEFORE_NUMS_COUNT;
//extern std::string BEFORE_REGION;
//extern std::string BEFORE_RANK;
//extern std::string BEFORE_STATE;
//extern std::string BEFORE_TYPE;

int g_multkill;
int g_deaths = -1;
bool g_is_chaoshen;

std::mutex g_mtx_header;

std::string playerName;
std::map<std::string, std::vector<time_t>> kill_history;
//std::mutex kill_mutex;
//const int MULTIKILL_WINDOW = 10; // 连杀时间窗口(秒)
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

// 配置常量
const int POLL_INTERVAL = 5;
//const std::wstring LCU_URL = L"https://127.0.0.1:2999/liveclientdata/eventdata";
const std::wstring LCU_URL = L"https://127.0.0.1:2999/liveclientdata/allgamedata";
const std::wstring NAME_URL = L"https://127.0.0.1:2999/liveclientdata/activeplayername";

std::map<std::wstring, std::wstring> HEADERS = {
	/*	{ L"Content-Type", L"application/json" },
		{ L"User-Agent", L"Mozilla/5.0" },
		{ L"token", L"{{bToken}}" },*/

		{   L"organizationType",L"\"BAR\""                                                                                                          },
		{   L"merchantId",L"53" },
		{   L"barId",L"98"                                                                                                                          },
		//{   L"token",L"eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJSZW1vdGVJcCI6IiIsIkxvY2FsTG9naW4iOjAsIkNvbnRleHQiOnsidXNlcl9pZCI6MjQ3LCJ1c2VyX25hbWUiOiJ4eHgiLCJ1dWlkIjoiIiwicmlkIjoxOCwibWFudWZhY3R1cmVfaWQiOjUzLCJiYXJfaWQiOjk4LCJyb290X2lkIjowLCJvcmdhbml6YXRpb25fdHlwZSI6IiIsInBsYXRmb3JtIjoiYmFyY2xpZW50In0sImV4cCI6MTc1MjEzODc4N30.OxuSFEDQOq31KK9Vh-uwL9phsuV5zovluBptoNC3eXw"                                                                                                                  },
		{   L"token",token测试,                                                                                                                  },
		{   L"User-Agent",L"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/136.0.0.0 Safari/537.36\r\n"        },
		{   L"language",L"ZH_CN" },
		{   L"sec-ch-ua-platform",L"\"Windows\""                                                                                                    },
		{   L"sec-ch-ua-mobile",L"?0"                                                                                                               },
		{   L"Accept",L"application/json, text/plain, */*"                                                                                          },
		{   L"Content-Type",L"application/json"                                                                                                     },
		//	{   L"Referer",L"https://dev-asz.cjmofang.com/activity/activityManagement/createActivity/MODE_SIGN/0/add/0"                                 },
			{   L"sec-ch-ua",L"\"Chromium\";v=\"136\", \"Google Chrome\";v=\"136\", \"Not.A/Brand\";v=\"99\""                                           },
			{   L"Accept-Encoding",L"gzip, deflate, br"                                                                                                 },
			{   L"Connection",L"keep-alive"                                                                                                             },
			{   L"Cache-Control",L"no-cache"                                                                                                            },
			{   L"Host",L"127.0.0.1:8000"                                                                                                                }
};



// 获取当前时间字符串
std::string getCurrentTimeString() {
	auto now = std::chrono::system_clock::now();
	auto in_time_t = std::chrono::system_clock::to_time_t(now);
	std::tm tm;
	localtime_s(&tm, &in_time_t);

	std::stringstream ss;
	ss << std::put_time(&tm, "[%H:%M:%S]");
	return ss.str();
}

// 获取连杀标签
std::string getMultikillLabel(int killCount) {
	if (killCount == 2) return "播报双杀 (Double Kill)";
	if (killCount == 3) return "播报三杀 (Triple Kill)";
	if (killCount == 4) return "播报四杀 (Quadra Kill)";
	if (killCount >= 5) return "播报五杀 (Penta Kill)";
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

	// 创建cmd进程
	std::wstring cmd = L"cmd.exe /c " + command;
	if (!CreateProcessW(NULL, &cmd[0], NULL, NULL, TRUE,
		CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
		CloseHandle(hReadPipe);
		CloseHandle(hWritePipe);
		return "Error creating process";
	}

	CloseHandle(hWritePipe);

	// 读取输出
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
	// 查找参数名的位置
	size_t paramPos = commandLine.find(paramName);
	if (paramPos == std::string::npos) {
		return ""; // 参数不存在
	}

	// 跳过参数名和等号
	size_t valueStart = paramPos + paramName.length();
	if (commandLine[valueStart] == '=') {
		valueStart++; // 跳过等号
	}

	// 查找值的结束位置（空格或字符串结尾）
	size_t valueEnd = commandLine.find(' ', valueStart);
	if (valueEnd == std::string::npos) {
		valueEnd = commandLine.length();
	}

	// 提取值
	std::string value = commandLine.substr(valueStart, valueEnd - valueStart);

	// 去除可能的引号
	value.erase(std::remove(value.begin(), value.end(), '\"'), value.end());

	return value;
}


// 发起HTTPS请求
std::string makeHttpsRequest(const std::wstring& url) {
	try {

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

		// 禁用SSL证书验证(类似于Python中的verify=False)
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
	catch (const std::exception& e) {
		LOG_EXCEPTION_WITH_STACK(e);
		//LOG_IMMEDIATE("NarakaStateMonitor::OnClientStarted():" + std::string(e.what()));
		return "";
	}
	catch (...) {
		LOG_IMMEDIATE("lol.cpp::makeHttpsRequest:未知错误");
		return "";
	}

}

// 解析JSON事件数据
void processEvents(const std::string& jsonData) {
	//LOG_IMMEDIATE("!!!:" + jsonData);
	try {
		const char* jsonDataCStr = jsonData.c_str();

		rapidjson::Document data1;

		g_mtx.lock();
		nlohmann::json jsonbody;
		if (!g_infoBefore.empty())
		{
			jsonbody.update(g_infoBefore);
		}

		g_mtx.unlock();


		data1.Parse(jsonDataCStr);

		if (data1.HasMember("gameData"))
		{
			const rapidjson::Value& temp = data1["gameData"];
			if (temp.HasMember("gameMode")) {
				std::string mode = temp["gameMode"].GetString();


				pgd.game_mode = mode;

				if (mode == "TFT") {
					return;
				}


				//if (mode == "TFT_RANKED") {
				//	//LOG_IMMEDIATE_DEBUG("云顶模式排位");
				//	//g_lol_gameMode = 2;
				//	return;
				//}

	/*			if (mode == "RANKED_FLEX_SR") {
					g_lol_gameMode = 0;
				}

				if (mode == "RANKED_SOLO_5x5") {
					g_lol_gameMode = 1;
				}


				if (mode == "TFT_HYPERROLL") {
					g_lol_gameMode = 4;
				}*/
			}
			else {
				LOG_IMMEDIATE_ERROR("json数据:" + jsonData + "\n" + "没有找到gameMode字段,等待对局开始");
				return;
			}
		}
		else {
			LOG_IMMEDIATE_ERROR("json数据:" + jsonData + "\n" + "没有找到gameData字段,,等待对局开始");
			return;
		}

		const rapidjson::Value& data = data1["events"];
		Game_Before gb;
		// 获取 gameid 和gamemod
		gb.before_main("update");

		// 检查Events数组是否存在
		if (!data.HasMember("Events") || !data["Events"].IsArray()) {
			LOG_IMMEDIATE_ERROR("json数据:" + jsonData + "\n" + "没有正确识别Events字段");
			return;
		}

		playerName = getPlayerName();
		const rapidjson::Value& events = data["Events"];
		time_t current_time = time(nullptr);

		// TODO后面可以设置为常量
		//pgd.name = "LOL";
		for (rapidjson::SizeType i = 0; i < events.Size(); i++) {
			const rapidjson::Value& event = events[i];

			// 检查事件ID是否已处理
			if (!event.HasMember("EventID") || !event["EventID"].IsInt()) {

				//TODO 掉线ID可以在这里处理
				continue;
			}

			int event_id = event["EventID"].GetInt();
			if (processed_event_ids.find(event_id) != processed_event_ids.end()) {
				continue; // 已处理
			}

			// 超神: kill>7 连杀和单杀时 count++  当死亡+1时, count = 0   ,超神也有eventid
			// TODO 还没验证测试
			int die;
			const rapidjson::Value& players = data1["allPlayers"];
			for (rapidjson::SizeType i = 0; i < players.Size(); i++) {
				const rapidjson::Value& player = players[i];
				if (player["riotIdGameName"].GetString() != playerName) {
					continue;
				}
				else {
					die = player["scores"]["deaths"].GetInt();

					if (die != g_deaths && die != 0) {
						g_multkill = 0;
						LOG_IMMEDIATE("g_multkill清空");
						LOG_IMMEDIATE(std::to_string(g_multkill));
						//g_deaths = die;
						g_is_chaoshen = false;
					}
					else if (g_multkill >= 7 && !g_is_chaoshen) {
						LOG_IMMEDIATE("g_multkill>=7");
						LOG_IMMEDIATE(std::to_string(g_multkill));
						g_is_chaoshen = true;

						//发送超神
						jsonbody["event_id"] = std::to_string(event_id) + "_chaoshen";
						jsonbody["type"] = "CHAO_SHEN_SHU";
						_sendHttp_LOL(jsonbody);
					}
					g_deaths = die;
				}
			}

			// 检查是否为Multikill事件
			if (event.HasMember("EventName") && event["EventName"].IsString() &&
				std::string(event["EventName"].GetString()) == "Multikill") {
				// 获取击杀者名称
				if (!event.HasMember("KillerName") || !event["KillerName"].IsString()) {
					processed_event_ids.insert(event_id);
					continue;
				}

				std::string killer = event["KillerName"].GetString();
				if (killer.empty() || killer != playerName) {
					processed_event_ids.insert(event_id);
					continue;
				}

				// 添加当前击杀时间
				kill_history[killer].push_back(current_time);

				// 获取连杀数
				int kill_count = 0;
				if (event.HasMember("KillStreak") && event["KillStreak"].IsInt()) {
					kill_count = event["KillStreak"].GetInt();
					jsonbody["event_id"] = std::to_string(event["EventID"].GetInt());
					//g_multkill += kill_count;
					//LOG_IMMEDIATE("g_multkill 累杀"+ std::to_string(kill_count));
					//LOG_IMMEDIATE(std::to_string(g_multkill));
					switch (kill_count) {
						/*	case 2:
								pgd.type = "2_SHU";
								break;*/
					case 3:
						jsonbody["type"] = "SAN_SHA_SHU";
						_sendHttp_LOL(jsonbody);
						break;
					case 4:
						jsonbody["type"] = "SI_SHA_SHU";
						_sendHttp_LOL(jsonbody);
						break;
					case 5:
						jsonbody["type"] = "WU_SHA_SHU";
						_sendHttp_LOL(jsonbody);
						break;
					}


				}

			}
			else {
				//LOG_IMMEDIATE_WARNING("没有正确识别EventID字段");
			}
			// TODO 可以优化到上面if中
			if (event.HasMember("EventName") && event["EventName"].IsString() &&
				std::string(event["EventName"].GetString()) == "ChampionKill") {
				// 获取击杀者名称
				if (!event.HasMember("KillerName") || !event["KillerName"].IsString()) {
					//continue;
				}

				else {
					std::string killer = event["KillerName"].GetString();
					const rapidjson::Value& players = data1["allPlayers"];
					if (killer.empty() || killer != playerName) {
						//他人击杀 判断助攻是否改变 (如果要实时上传助攻)
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

						//continue;
					}
					else {
						g_multkill += 1;
						LOG_IMMEDIATE("连杀数:" + std::to_string(g_multkill));
						LOG_IMMEDIATE("event_id:" + std::to_string(event_id));
					}
				}
				//for (rapidjson::SizeType i = 0; i < players.Size(); i++) {
				//	const rapidjson::Value& player = players[i];
				//	if (player["riotIdGameName"].GetString() != playerName) {
				//		//continue;
				//	}
				//	else {
				//		// 累计杀敌
				//		
				//	}
				//}




			}
			else {
				//LOG_IMMEDIATE_WARNING("没有正确识别EventName字段");
			}

			// 标记事件为已处理
			processed_event_ids.insert(event_id);
		}
	}
	catch (const std::exception& e) {
		LOG_EXCEPTION_WITH_STACK(e);
		//LOG_IMMEDIATE("NarakaStateMonitor::OnClientStarted():" + std::string(e.what()));
		return ;
	}
	catch (...) {
		LOG_IMMEDIATE("lol.cpp::processEvents::未知错误");
		return ;
	}
}


void pollEvents() {

		while (is_lol_game_running) {
			std::this_thread::sleep_for(std::chrono::seconds(POLL_INTERVAL));

			std::string response = makeHttpsRequest(LCU_URL);

			if (!response.empty()) {
				processEvents(response);
			}
			else {

				//g_mtx.lock();  
				//is_lol_game_running = false;
				//g_mtx.unlock();  

				LOG_IMMEDIATE("LOL:等待对局开始");

				//return; 这个return????
			}
			// 1 秒轮询 可更改
		
		}
	}


void pollRankNum() {

	Game_Before gb;

	while (is_lol_running) {

		//可以拆分...调高性能?
		gb.before_main("update");

		//std::cout << response << std::endl;
		//TODO  以UTF-8對比
		/*if (!response.empty()) {
			processEvents(response);
		}
		else {
			is_lol_game_running = false;
			LOG_IMMEDIATE("LOL:等待对局开始");
			return;
		}*/
		//std::this_thread::sleep_for(std::chrono::seconds(POLL_INTERVAL));
		// 
		// 10 秒检测队伍人数 应该可以更大点
		std::this_thread::sleep_for(std::chrono::seconds(11));
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

		// 清理字符串
		name.erase(std::remove(name.begin(), name.end(), '\"'), name.end());
		name.erase(std::remove(name.begin(), name.end(), '\r'), name.end());
		name.erase(std::remove(name.begin(), name.end(), '\n'), name.end());
		name.erase(0, name.find_first_not_of(" \t"));
		name.erase(name.find_last_not_of(" \t") + 1);
	}

	return name;
}

// 向发送信息 (主要用于对局中实时信息)
void _sendHttp_LOL(PostGameData pgd) {
	HttpClient http;
	//json jsonBody = {
	//{"computer_no", computer_no},
	//{"name", name},
	//{"game_mode", game_mode},
	//{"team_size", team_size},
	//{"type", type},
	//{"type_count", std::to_string(type_count)}  // 如果服务器期望是字符串
	//};


	std::string jsonBody =
		"{\n"
		"\"computer_no\": \"" + pgd.computer_no + "\",\n"
		"\"name\": \"" + pgd.name + "\",\n"
		"\"game_mode\": \"" + "TODO" + "\",\n"
		"\"user_game_rank\": \"" + +"\",\n"
		//"\"team_size\": \"" + teamSizeMap[pgd.team_size] + "\",\n"
		"\"type\": \"" + pgd.type + "\",\n"
		"\"eventID\": \"" + pgd.eventID + "\"\n"
		"\"data\": \"" + pgd.data + "\"\n"
		"}";
	LOG_IMMEDIATE(jsonBody);

	/*	//JSON请求体
		jsonBody = R"({
		"computer_no":"A046",
		"name": "test",
		"game_mode": "MATCH",
		"team_size": "ONE",
		"type": "SERIES_KILLS",
		"type_count": "3"
	})";*/

	try {
		// 3. 发送POST请求
		g_mtx_header.lock();
		std::string response = http.SendRequest(
			get_g_domain() + L"/api/client/PostGameData",
			L"POST",
			getHeader(),
			jsonBody
		);
		g_mtx_header.unlock();
		LOG_IMMEDIATE("lol:Response: " + response);
	}
	catch (const std::exception& e) {
		std::cerr << "Error: " << e.what() << std::endl;
		LOG_ERROR(e.what());
		g_mtx_header.unlock();
	}

}
// 向发送信息 (主要用于客户端启动结束)

// 向发送信息 (主要用于对局结束后 对局中)
void _sendHttp_LOL(nlohmann::json jsonBody) {
	HttpClient http;

	LOG_IMMEDIATE(jsonBody.dump());

	try {
		g_mtx_header.lock();
		std::wstring url;
		if (jsonBody["remark"] == "TFT") {
			url = L"/api/client/YundingzhiyiPostGameData";

		
		
		}else{
			url = L"/api/client/PostGameData";

		}

		std::string response = http.SendRequest(
			get_g_domain() + url,
			L"POST",
			getHeader(),
			jsonBody.dump()
		);
		// 3. 发送POST请求
		
	
		g_mtx_header.unlock();

		//使用 range-based for loop 遍历
	   //for (const auto& pair : getHeader()) {
	   //	const std::wstring& key = pair.first;
	   //	const std::wstring& value = pair.second;
	   //	LOG_IMMEDIATE( WStringToString( key )+ ": " + WStringToString(value));
	   //	//std::wcout  << key << L": " << value << std::endl;
	   //}

		LOG_IMMEDIATE("lol:Response: " + UTF8ToGBK(response));
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

void _sendHttp_LOL(std::string type, nlohmann::json data) {
	// data留着吧
	nlohmann::json jsonBody;
	jsonBody["type"] = type;
	jsonBody["name"] = "LOL";
	_sendHttp_LOL(jsonBody);

}
//extern "C" __declspec(dllexport) const char* GetPlayerName() {
//	return playerName.c_str();
//}

// 测试连通
extern "C" __declspec(dllexport) int Add(int a, int b) {
	return a + b;
	//_sendHttp_LOL("");
}


extern "C" __declspec(dllexport) const int setHeader(const char* token1, const char* manufactureId1, const char* barId1, const char* domain1) {

	std::lock_guard<std::mutex> lock(g_mtx_header);
	std::string token = (token1 != nullptr) ? token1 : "";
	std::string manufactureId = (manufactureId1 != nullptr) ? manufactureId1 : "";
	std::string barId = (barId1 != nullptr) ? barId1 : "";
	std::string domain = (domain1 != nullptr) ? domain1 : "";
	/*if (token.empty() || manufactureId.empty() || barId.empty()) {
		LOG_IMMEDIATE_ERROR("setHeader接收的为空");
		return 0;
	}*/

	//LOG_IMMEDIATE_DEBUG("DLL:Token:" + token);
	LOG_IMMEDIATE_DEBUG("DLL:domain:" + domain);
	LOG_IMMEDIATE_DEBUG("DLL:manufactureId:" + manufactureId);
	LOG_IMMEDIATE_DEBUG("DLL:barId:" + barId);
	try {

	
		if (domain != "" && get_g_domain() != stringTOwstring(domain))
		{
			set_g_domain(stringTOwstring(domain));
			LOG_IMMEDIATE("更改g_domain: " + WStringToString(get_g_domain()));
		}

		HEADERS = {
			/*	{ L"Content-Type", L"application/json" },
				{ L"User-Agent", L"Mozilla/5.0" },
				{ L"token", L"{{bToken}}" },*/
				{   L"organizationType",L"\"BAR\""                                                                                                          },
				{   L"merchantId",	stringTOwstring(manufactureId) },
				{   L"barId",	stringTOwstring(barId)                                                                                                                          },
				//{   L"token",L"eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJSZW1vdGVJcCI6IiIsIkxvY2FsTG9naW4iOjAsIkNvbnRleHQiOnsidXNlcl9pZCI6MjQ3LCJ1c2VyX25hbWUiOiJ4eHgiLCJ1dWlkIjoiIiwicmlkIjoxOCwibWFudWZhY3R1cmVfaWQiOjUzLCJiYXJfaWQiOjk4LCJyb290X2lkIjowLCJvcmdhbml6YXRpb25fdHlwZSI6IiIsInBsYXRmb3JtIjoiYmFyY2xpZW50In0sImV4cCI6MTc1MDQ2ODMwMH0.21sEbRTirJggWvWlOygMOczAQWs8vQd0hh0ZKJuNTbs"                                                                                                                  },
				{   L"token",       stringTOwstring(token)                                                                                                             },
				{   L"User-Agent",L"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/136.0.0.0 Safari/537.36"        },
				{   L"language",L"ZH_CN" },
				{   L"sec-ch-ua-platform",L"\"Windows\""                                                                                                    },
				{   L"sec-ch-ua-mobile",L"?0"                                                                                                               },
				{   L"Accept",L"application/json, text/plain, */*"                                                                                          },
				{   L"Content-Type",L"application/json"                                                                                                     },
				//	{   L"Referer",L"https://dev-asz.cjmofang.com/activity/activityManagement/createActivity/MODE_SIGN/0/add/0"                                 },
					{   L"sec-ch-ua",L"\"Chromium\";v=\"136\", \"Google Chrome\";v=\"136\", \"Not.A/Brand\";v=\"99\""                                           },
					{   L"Accept-Encoding",L"gzip, deflate, br"                                                                                                 },
					{   L"Connection",L"keep-alive"                                                                                                             },
					{   L"Cache-Control",L"no-cache"                                                                                                            },
					{   L"Host",L"127.0.0.1:8000"                                                                                                                }
		};
	}
	catch (const std::exception& e) {
		LOG_IMMEDIATE_ERROR("setHeader 没有设置:" + std::string(e.what()));
		return 0;
	}

	return 1;
}

