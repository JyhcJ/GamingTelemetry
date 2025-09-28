#pragma once
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <memory>
#include "constant.h"
#include "nloJson.h"
#include <regex>
#include <unordered_set>

// ϵͳ���ͷ�ļ�
#ifdef _WIN32
#include <windows.h>
#include <tlhelp32.h>
#endif
#include "common.h"
#include "ThreadSafeLogger.h"
#include "HttpClient.h"
#include "Naraka.h"
#include "CurlUtils.h"


// ��Ϸ���̼����
class GameMonitor {
public:
	bool isGameRunning() {
#ifdef _WIN32
		HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		if (hSnapshot == INVALID_HANDLE_VALUE) {
			return false;
		}

		PROCESSENTRY32 pe;
		pe.dwSize = sizeof(PROCESSENTRY32);

		if (!Process32First(hSnapshot, &pe)) {
			CloseHandle(hSnapshot);
			return false;
		}

		do {
			std::wstring processName(pe.szExeFile);
			if (processName == L"NarakaBladepoint.exe") {
				CloseHandle(hSnapshot);
				return true;
			}
		} while (Process32Next(hSnapshot, &pe));

		CloseHandle(hSnapshot);
		return false;
#else
		// Linux/Macʵ��
		FILE* pipe = popen("ps -A | grep NarakaBladepoint", "r");
		if (!pipe) return false;

		char buffer[128];
		bool found = false;

		while (!feof(pipe)) {
			if (fgets(buffer, 128, pipe) != nullptr) {
				if (strstr(buffer, "NarakaBladepoint")) {
					found = true;
					break;
				}
			}
		}

		pclose(pipe);
		return found;
#endif
	}
};

// ������ݹ�����
class PlayerDataManager {
public:
	bool readPlayerNameFromFile(const std::string& filePath, std::string& playerName) {
		std::ifstream file(filePath, std::ios::binary); // �� binary ���⻻�з�ת��
		if (!file.is_open()) {
			std::cerr << "Failed to open player name file: " << filePath << std::endl;
			return false;
		}

		std::string line;
		const std::string key = "player_name,";

		while (std::getline(file, line)) {
			// ȥ�����ܵ� BOM
			if (line.size() >= 3 &&
				(unsigned char)line[0] == 0xEF &&
				(unsigned char)line[1] == 0xBB &&
				(unsigned char)line[2] == 0xBF) {
				line.erase(0, 3);
			}

			// ȥ���س��� \r
			if (!line.empty() && line.back() == '\r') {
				line.pop_back();
			}
			//TODO Try
			size_t pos = line.find(key);
			if (pos != std::string::npos) {
				size_t start = pos + key.length();
				size_t end = line.find(';', start);
				if (end == std::string::npos) {
					end = line.length();
				}
				playerName = line.substr(start, end - start);

				return true;
			}
		}

		LOG_IMMEDIATE_DEBUG("NARAKAδ���ļ����ҵ� player_name");
		return false;
	}
	bool readPlayerNameFromFileDESC(const std::string& filePath, std::string& playerName) {
		try {
			const std::string key = "player_name,";
			const size_t chunkSize = 4096; // ÿ�ζ�ȡ�Ŀ��С���ɸ����ļ���С������
			char buffer[chunkSize];

			std::ifstream file(filePath, std::ios::binary | std::ios::ate); // ֱ�Ӷ�λ���ļ�ĩβ
			if (!file.is_open()) {
				std::cerr << "Failed to open player name file: " << filePath << std::endl;
				return false;
			}

			std::streampos fileSize = file.tellg();
			std::streampos readPos = std::min<std::streampos>(fileSize, chunkSize * 10); // �����ǰ���10����

			while (readPos > 0) {
				// ����ʵ�ʶ�ȡλ�úʹ�С
				size_t thisChunkSize = static_cast<size_t>(std::min<std::streampos>(chunkSize, readPos));
				readPos -= thisChunkSize;
				file.seekg(readPos, std::ios::beg);
				file.read(buffer, thisChunkSize);

				// ���������ؼ��ֶ�
				std::string chunk(buffer, thisChunkSize);
				size_t pos = chunk.rfind(key); // �������

				if (pos != std::string::npos) {
					size_t valueStart = pos + key.length();
					size_t lineEnd = chunk.find(';', valueStart);
					if (lineEnd == std::string::npos) {
						lineEnd = chunk.length();
					}

					// ��ȡ�������������ܵĻ��з���
					playerName = chunk.substr(valueStart, lineEnd - valueStart);
					playerName.erase(std::remove(playerName.begin(), playerName.end(), '\r'), playerName.end());
					playerName.erase(std::remove(playerName.begin(), playerName.end(), '\n'), playerName.end());

					return true;
				}

				// ���û�ҵ����Ѿ�������ļ���ͷ�����˳�
				if (readPos <= 0) break;
			}

			LOG_IMMEDIATE_DEBUG("NARAKAδ���ļ����ҵ� player_name");
			return false;
		}
		catch (const std::exception& e) {
			LOG_EXCEPTION_WITH_STACK(e);
			//LOG_IMMEDIATE("NarakaStateMonitor::OnClientStarted():" + std::string(e.what()));
			return false;
		}
		catch (...) {
			LOG_IMMEDIATE("Naraka.h::readPlayerNameFromFileDESC:δ֪����");
			return false;
		}
	}
	const std::string& getPlayerName() const {
		return playerName;
	}

	void setPlayerName(std::string p_name) {
		playerName = p_name;
	}

	void setRegion(const std::string& reg) {
		region = reg;
	}

	const std::string& getRegion() const {
		return region;
	}

	void addToExclusionList(const std::string& matchId) {
		exclusionList.insert(matchId);
	}

	bool isExcluded(const std::string& matchId) const {
		return exclusionList.find(matchId) != exclusionList.end();
	}

	bool hasInit() const {	
		return exclusionList.empty();
	}

private:
	std::string playerName;
	std::string region = "CN"; // Ĭ������
	std::unordered_set<std::string> exclusionList;

};

// ս����ѯ����
class NarakaStatsTracker {
public:
	bool isInit = true;
	std::unique_ptr<GameMonitor> gameMonitor;
	std::unique_ptr<PlayerDataManager> playerData;

	//std::map<std::wstring, std::wstring> m_header = {
	//	{L":method",L"GET"},
	//	{L":authority", L"record.uu.163.com"},
	//	{L":scheme", L"https"},
	//	{L":path", L"/api/naraka/battles?page=1&page_size=10"},
	//	{L"sec-ch-ua-platform", L"Windows"},
	//	{L"user-agent", L"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/138.0.0.0 Safari/537.36"},
	//	{L"accept", L"application/json, text/plain, */*"},
	//	{L"sec-ch-ua", L"\"Not)A;Brand\";v=\"8\", \"Chromium\";v=\"138\", \"Google Chrome\";v=\"138\""},
	//	{L"sec-ch-ua-mobile", L"?0"},
	//	{L"sec-fetch-site", L"same-origin"},
	//	{L"sec-fetch-mode", L"cors"},
	//	{L"sec-fetch-dest", L"empty"},
	//	{L"referer", L"https://record.uu.163.com/naraka/"},
	//	{L"accept-encoding", L"gzip, deflate, br, zstd"},
	//	{L"accept-language", L"zh-CN,zh;q=0.9"},
	//	{L"cookie", L"session=2gYhJUz6USFQV5Lx3iG7q1XktvCHWmzMJT_QepHr"},
	//	{L"priority", L"u=1, i"}
	//};
	std::string session;

	std::map<std::string, std::string> m_headers = {
		  {"sec-ch-ua-platform", "\"Windows\""},
		  {"user-agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/138.0.0.0 Safari/537.36"},
		  {"accept", "application/json, text/plain, */*"},
		  {"sec-ch-ua", "\"Not)A;Brand\";v=\"8\", \"Chromium\";v=\"138\", \"Google Chrome\";v=\"138\""},
		  {"sec-ch-ua-mobile", "?0"},
		  {"sec-fetch-site", "same-origin"},
		  {"sec-fetch-mode", "cors"},
		  {"sec-fetch-dest", "empty"},
		  {"referer", "https://record.uu.163.com/naraka/"},
		  {"accept-encoding", "gzip, deflate, br, zstd"},
		  {"accept-language", "zh-CN,zh;q=0.9"},
		  {"cookie", "session="+ session},
		  {"priority", "u=1, i"}
	};
	NarakaStatsTracker()
		:gameMonitor(std::make_unique<GameMonitor>()),
		playerData(std::make_unique<PlayerDataManager>()) {

		//std::string session = "2gYhJUz6USFQV5Lx3iG7q1XktvCHWmzMJT_QepHr";
		//try {
		//	std::string ret;
		//	_sendHttp(L"/api/client/GetGameConfig", "", ret);
		//	nlohmann::json jsonData1 = nlohmann::json::parse(ret);
		//	nlohmann::json jsonData2 = nlohmann::json::parse(remove_escape_chars(trim_quotes(jsonData1["metadata"]["value"].dump())));
		//	//LOG_IMMEDIATE("ȡ����value: " + jsonData1["metadata"]["value"].dump());
		//	//LOG_IMMEDIATE("ȥ����βvalue: " + remove_escape_chars(trim_quotes(jsonData1["metadata"]["value"].dump())));
		//	LOG_IMMEDIATE(jsonData2.dump() + "cjmofang.com.");
		//	session = jsonData2["yongjiewujian"]["token"];
		//	if (session == "") {
		//		session = "L2DgCvsWBq2p3df9J0U2fvqq4PhPDO4qAbBnkdTZ";
		//	}
		//	//LOG_IMMEDIATE(generate_md5(jsonData2.dump() + "cjmofang.com."));
		//}
		//catch (const std::exception& e) {
		//	LOG_IMMEDIATE("NarakaStateMonitor::OnClientStarted():" + std::string(e.what()));
		//	return;
		//}
		//catch (...) {
		//	LOG_IMMEDIATE("NarakaStateMonitor::OnClientStarted():δ֪����");
		//	return;
		//}


	}

	bool initialize(const std::string& playerNameFile) {
		// 1. ��ȡ�������
		std::string playerName;
		if (!playerData->readPlayerNameFromFile(playerNameFile, playerName)) {
			std::cerr << "Failed to read player name from file." << std::endl;
			return false;
		}

		//// 3. ��ȡ��ʼս���б�
		//if (!checkNew()) {
		//	std::cerr << "Failed to fetch initial stats." << std::endl;
		//	return false;
		//}
			


		return true;
	}

	void run() {
		while (true) {
			// �����Ϸ�Ƿ�������
			if (!gameMonitor->isGameRunning()) {
				std::cout << "Game is not running. Waiting..." << std::endl;
				std::this_thread::sleep_for(std::chrono::seconds(30));
				continue;
			}

			// ÿ���Ӳ�ѯһ��
			std::this_thread::sleep_for(std::chrono::minutes(1));

			//// ��ѯ��ս��
			//if (!fetchNewStats()) {
			//	std::cerr << "Failed to fetch new stats." << std::endl;
			//}
		}
	}

	bool checkNew(std::string playerName, bool isInit, std::map<std::string, std::string> p_headers, bool ret_isInit) {
		// ��ȡ��ʼս���б�����ų��б�
		CurlUtils::setVerifySSL(false);
		std::string str;
		std::string room_id;
		try {
			nlohmann::json response;
			m_headers = p_headers;
			std::string url = "https://record.uu.163.com/api/login/status";
			auto response1 = CurlUtils::get(url, m_headers);
			//ʧ�ܿ��ܷ����ǿ�  ��ȷ��
			if (response1.statusCode == 200) {
				//std::cout << "Success! Body length: " << response1.body.size() << std::endl;
				//std::cout << response1.body << std::endl;

			}
			else {
				std::cerr << "Request failed with status: " << response1.statusCode << std::endl;
			}

			std::string encoded = UrlEncode(playerName);
			url = "https://record.uu.163.com/api/naraka/auth/" + encoded + "/163";
			response1 = CurlUtils::get(url, m_headers);
			if (response1.statusCode == 200) {
				//std::cout << "Success! Body length: " << response1.body.size() << std::endl;
				//std::cout << response1.body << std::endl;

			}


			url = "https://record.uu.163.com/api/naraka/battles?page=1&page_size=2";
			response1 = CurlUtils::get(url, m_headers);
			if (response1.statusCode == 200) {
				//std::cout << "Success! Body length: " << response1.body.size() << std::endl;
				//std::cout << response1.body << std::endl;
				response = nlohmann::json::parse(response1.body);

				std::vector<PathItem> path = {
				PathItem::makeKey("data"),
				PathItem::makeIndex(0),
				PathItem::makeKey("room_id")
				};
				room_id = getNestedValuePlus<std::string>(response, path, "error");
		
				if (isInit && room_id !="error") {
					//time_t currentTimestamp = time(nullptr);
					//std::vector<PathItem> path_begin_time = {
					//PathItem::makeKey("data"),
					//PathItem::makeIndex(0),
					//PathItem::makeKey("begin_time")
					//};
					//time_t begin_time = getNestedValuePlus<time_t>(response, path_begin_time, currentTimestamp);
					//// �����ֵ���룩
					//long long secondsElapsed = static_cast<long long>(currentTimestamp - begin_time);
					//LOG_IMMEDIATE("Naraka:It has been " + std::to_string(secondsElapsed) + " seconds since the last game.");
				 //   if (secondsElapsed > 60 * 25) {}
					ret_isInit = false;
					playerData->addToExclusionList(room_id);
					return false;
				}

				ret_isInit = true;
				if (!playerData->isExcluded(room_id) && room_id != "error")
				{
					LOG_IMMEDIATE("�����޼��µĶԾ�:" + room_id);
					playerData->addToExclusionList(room_id);
				}
				else {
					return false;
				}

			}
			else {
				std::cerr << "Request failed with status: " << response1.statusCode << std::endl;
			}

			url = "https://record.uu.163.com/api/naraka/battle/detail/" + room_id;
			response1 = CurlUtils::get(url, m_headers);
			if (response1.statusCode == 200) {
				//std::cout << "Success! Body length: " << response1.body.size() << std::endl;
				//std::cout << response1.body << std::endl;
				nlohmann::json obj0 = nlohmann::json::parse(response1.body);
			    obj0 = obj0["data"];
				nlohmann::json jsonBody;

				jsonBody["type"] = "END";
				jsonBody["game_uuid"] = room_id;
				jsonBody["event_id"] = getNestedValue<std::string>(obj0, { "_id" }, "error");
				jsonBody["computer_no"] = getComputerName();
				jsonBody["name"] = "";
	

				jsonBody["game_mode"] = mapLookupOrDefaultPlus(
					NARAKA_modeMap,
					obj0["game_mode"].get<int>(),
					//std::to_string(obj0["game_mode"].get<int>()) 
					"UNLIMIT"
				);

				jsonBody["team_size"] = mapLookupOrDefaultPlus(
					NARAKA_teamSize,
					obj0["game_mode"].get<int>(),
					std::to_string(obj0["game_mode"].get<int>())
				);
				jsonBody["data"]["ren_tou_shu"]= getNestedValue<int>(obj0, { "kill" }, -1);
				jsonBody["data"]["rank"]= getNestedValue<int>(obj0, { "rank" }, -1);
				nlohmann::json member;
				member["role"] = "self";
				member["id"] = utf8ToUnicodeEscape(playerName);
				jsonBody["data"]["member"].push_back(member);
				for (const auto teammate : obj0["teammates"]) {
					member["role"] = "other";
					member["id"] = utf8ToUnicodeEscape(teammate["role_name"]);
					/*		LOG_IMMEDIATE(nlohmann::json::string_t(teammate["role_name"].dump()));
							LOG_IMMEDIATE(teammate["role_name"].dump());*/

					jsonBody["data"]["member"].push_back(member);
				}
				jsonBody["remark"] = "";


				std::string season_id;
				std::string user_game_rank;
			
				url = "https://record.uu.163.com/api/naraka/season";
				response1 = CurlUtils::get(url, m_headers);
				if (response1.statusCode == 200) {
					nlohmann::json season = nlohmann::json::parse(response1.body);
					std::vector<PathItem> path = {
					PathItem::makeKey("data"),
					PathItem::makeIndex(0),
					PathItem::makeKey("id")
					};
					season_id =  getNestedValuePlus<std::string>(season, path, "error");

					url = "https://record.uu.163.com/api/naraka/career?game_mode=" + std::to_string(obj0["game_mode"].get<int>()) + "&season_id=" + season_id;
					//url = "https://record.uu.163.com/api/naraka/career?game_mode=1&season_id=" + season_id;
					response1 = CurlUtils::get(url, m_headers);
					if (response1.statusCode == 200) {
						nlohmann::json career = nlohmann::json::parse(response1.body);
						//LOG_IMMEDIATE(career.dump(4));
						user_game_rank = UTF8ToGBK(getNestedValue<std::string>(career, { "data","grade_name" }, "error")) ;
					}
				}
				if (user_game_rank.find("��ͭ") != std::string::npos)
				{
					user_game_rank = "QING_TONG";
				}
				else if (user_game_rank.find("����") != std::string::npos) {
					user_game_rank = "BAI_YIN";
				}
				else if (user_game_rank.find("�ƽ�") != std::string::npos) {
					user_game_rank = "HUANG_JIN";
				}
				else if (user_game_rank.find("����") != std::string::npos) {
					user_game_rank = "BO_JIN";
				}
				else if (user_game_rank.find("����") != std::string::npos) {
					user_game_rank = "YUN_XING";
				}
				else if (user_game_rank.find("ʴ��") != std::string::npos) {
					user_game_rank = "SHI_YUE";
				}
				else if (user_game_rank.find("׹��") != std::string::npos) {
					user_game_rank = "ZHUI_RI";
				}
				else if (user_game_rank.find("�޼�����") != std::string::npos) {
					user_game_rank = "WU_JIAN_XIU_LUO";
				}
				else {
					user_game_rank = "UNLIMIT";
				}
				jsonBody["user_game_rank"] = user_game_rank;
				



				std::string response;
				_sendHttp(L"/api/client/YongjiewujianPostGameData", jsonBody.dump(), response);

				//LOG_IMMEDIATE(jsonBody.dump(4, ' ', true));

			
			}
			else {
				std::cerr << "Request failed with status: " << response1.statusCode << std::endl;
			}
		}
		catch (std::exception& e) {
			LOG_IMMEDIATE_ERROR("Error: " + std::string(e.what()));
		}
		//LOG_IMMEDIATE_WARNING("Naraka ���� ��¼״̬: " + UTF8ToGBK( str));

		//Դ�ļ����뱣��Ϊ UTF-8 ����
	/*	std::string encoded = UrlEncode(playerName);
		std::wstring check = stringTOwstring(encoded);
		str = http.SendRequest(
			L"https://record.uu.163.com/api/naraka/auth/" + check + L"/163",
			L"GET",
			m_headers,
			""
		);*/




		//nlohmann::json response = nlohmann::json::parse(str);

		//std::string msg = getNestedValue<std::string>(response, { "msg" }, "error");

		//if (msg != "ok")
		//{
		//	LOG_IMMEDIATE_WARNING("��ȡս��ʧ��: " + response.dump());
		//	return false;
		//}


		//for (const auto& obj : response["data"]) {
		//	std::string room_id = getNestedValue<std::string>(obj, { "room_id" }, "error");
		//	if (isInit) {
		//		playerData->addToExclusionList(room_id);
		//		break;
		//	}
		//	else if (playerData->isExcluded(room_id)) {
		//		return false;
		//	}
		//	else {
		//		LOG_IMMEDIATE("�µĶԾֲ���.�ϱ���Ϣ");


		//		return true;
		//	}


		//	break;
		//}



		return true;
	}
private:

	//bool fetchNewStats() {
	//	// ��ȡ��ս��
	//	std::string response = chromeController->fillAndQuery(
	//		playerData->getPlayerName(),
	//		playerData->getRegion()
	//	);

	//	// ������Ӧ�������ս��
	//	std::regex matchIdRegex("\"matchId\":\"([^\"]+)\"");
	//	std::smatch matches;

	//	bool newStatsFound = false;
	//	std::string::const_iterator searchStart(response.cbegin());
	//	while (std::regex_search(searchStart, response.cend(), matches, matchIdRegex)) {
	//		if (matches.size() > 1) {
	//			std::string matchId = matches[1].str();
	//			if (!playerData->isExcluded(matchId)) {
	//				std::cout << "New match found: " << matchId << std::endl;
	//				newStatsFound = true;
	//				// ������ս��...

	//				// ��ӵ��ų��б�
	//				playerData->addToExclusionList(matchId);
	//			}
	//		}
	//		searchStart = matches.suffix().first;
	//	}

	//	return !response.empty();
	//}


};
