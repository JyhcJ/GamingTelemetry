#include "pch.h"
#include "ThreadSafeLogger.h"
#include <iostream>
#include "NarakaStateMonitor.h"
#include "Naraka.h"

NarakaStatsTracker tracker;

void NarakaStateMonitor::OnClientStarted() {
	std::string playerName;
	//PlayerDataManager playerDataManager;

	//std::thread valMatchThread([this]() {
	//获取session 

	std::string session;
	std::map<std::string, std::string> m_headers;
	try {
		std::string ret;
		nlohmann::json jsonData;
		jsonData["type"] = "RUN";
		_sendHttp(L"/api/client/YongjiewujianPostGameData", jsonData.dump(), ret);

		_sendHttp(L"/api/client/GetGameConfig", "", ret);




	
		if (ret != "")
		{

			nlohmann::json jsonData1 = nlohmann::json::parse(ret);
			nlohmann::json jsonData2 = nlohmann::json::parse(remove_escape_chars(trim_quotes(jsonData1["metadata"]["value"].dump())));
			//LOG_IMMEDIATE("取到的value: " + jsonData1["metadata"]["value"].dump());
			//LOG_IMMEDIATE("去除首尾value: " + remove_escape_chars(trim_quotes(jsonData1["metadata"]["value"].dump())));
			LOG_IMMEDIATE(jsonData2.dump() + "cjmofang.com.");
			session = jsonData2["yongjiewujian"]["token"];
			if (session == "") {
				session = "L2DgCvsWBq2p3df9J0U2fvqq4PhPDO4qAbBnkdTZ";
			}
		}
		else {
			session = "L2DgCvsWBq2p3df9J0U2fvqq4PhPDO4qAbBnkdTZ";
		}
		m_headers = {
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
	  {"cookie", "session=" + session},
	  {"priority", "u=1, i"}
		};
		//LOG_IMMEDIATE(generate_md5(jsonData2.dump() + "cjmofang.com."));
	}
	catch (const std::exception& e) {
		LOG_IMMEDIATE("NarakaStateMonitor::OnClientStarted():" + std::string(e.what()));
		return ;
	}
	catch (...) {
		LOG_IMMEDIATE("NarakaStateMonitor::OnClientStarted():未知错误");
		return ;
	}




	std::string compUserName = GetEnvSafe("USERPROFILE");
	std::string playerNameFile = compUserName + "\\AppData\\LocalLow\\24Entertainment\\Naraka\\player_prefs.txt";
	if (std::remove(playerNameFile.c_str()) != 0) {
		// 文件不存在或删除失败
		if (errno == ENOENT) {
			LOG_IMMEDIATE("NARAKA 文件不存在，无需删除");
			//return true;  // 文件本来就不存在，不算错误
		}
		else {
			LOG_IMMEDIATE("NARAKA 日志文件删除失败");
			//return false;
		}
	}
	else {
		LOG_IMMEDIATE("NARAKA 删除日志文件");
	}

	std::string lastPlayerName;
	while (true) {
		std::this_thread::sleep_for(std::chrono::milliseconds(5000));

		if (IsProcessRunning(L"NarakaBladepoint.exe")) {
			//获取name;


			tracker.playerData->readPlayerNameFromFileDESC(playerNameFile, playerName);
			tracker.playerData->setPlayerName(playerName);
			lastPlayerName = playerName;
			if (playerName == "") {
				continue;
			}
			else{
			
				LOG_IMMEDIATE("Naraka:玩家名称: " + UTF8ToGBK(playerName));
			}
			break;
		}
		else {
			return;
		}

		//LOG_IMMEDIATE_DEBUG("间隔检查...");
	}
	//});

	bool ret_init = false;
	bool isInit = true;
	while (IsProcessRunning(L"NarakaBladepoint.exe")) {
		tracker.playerData->readPlayerNameFromFileDESC(playerNameFile, playerName);
		if (lastPlayerName != playerName) {
			LOG_IMMEDIATE("Naraka:玩家名称变更: " + UTF8ToGBK(playerName));
			lastPlayerName = playerName;
			isInit = true;
		}
		bool isNew = tracker.checkNew(playerName, isInit, m_headers, &ret_init);
		std::this_thread::sleep_for(std::chrono::milliseconds(1000 * 30)); // 30 秒
	
		if (!ret_init) {
			isInit = false;
		}
			
		
	}


}

void NarakaStateMonitor::OnClientClosed() {
	nlohmann::json jsonData;
	std::string ret;
	jsonData["type"] = "KILL";
	_sendHttp(L"/api/client/YongjiewujianPostGameData", jsonData.dump(), ret);
	LOG_IMMEDIATE("永劫无间关闭");
}
