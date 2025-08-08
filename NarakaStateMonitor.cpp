#include "pch.h"
#include "ThreadSafeLogger.h"
#include <iostream>
#include "NarakaStateMonitor.h"
#include "Naraka.h"

NarakaStatsTracker tracker;

void NarakaStateMonitor::OnClientStarted() {
	std::string playerName;
	//PlayerDataManager playerDataManager;
	bool isInit = false;
	//std::thread valMatchThread([this]() {
	//获取session 

	std::string session = "2gYhJUz6USFQV5Lx3iG7q1XktvCHWmzMJT_QepHr";

	try {
		std::string ret;
		_sendHttp(L"/api/client/GetGameConfig", "", ret);
		nlohmann::json jsonData1 = nlohmann::json::parse(ret);
		nlohmann::json jsonData2 = nlohmann::json::parse(remove_escape_chars(trim_quotes(jsonData1["metadata"]["value"].dump())));
		//LOG_IMMEDIATE("取到的value: " + jsonData1["metadata"]["value"].dump());
		//LOG_IMMEDIATE("去除首尾value: " + remove_escape_chars(trim_quotes(jsonData1["metadata"]["value"].dump())));
		LOG_IMMEDIATE(jsonData2.dump() + "cjmofang.com.");
		session=jsonData2["yongjiewujian"]["token"];
		//LOG_IMMEDIATE(generate_md5(jsonData2.dump() + "cjmofang.com."));
	}
	catch (const std::exception& e) {
		LOG_IMMEDIATE("NarakaStateMonitor::OnClientStarted():" + std::string(e.what()));
		return;
	}
	catch (...) {
		LOG_IMMEDIATE("NarakaStateMonitor::OnClientStarted():未知错误");
		return;
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

	
	while (IsProcessRunning(L"NarakaBladepoint.exe")) {
		tracker.playerData->readPlayerNameFromFileDESC(playerNameFile, playerName);
		if (lastPlayerName != playerName) {
			LOG_IMMEDIATE("Naraka:玩家名称变更: " + UTF8ToGBK(playerName));
			lastPlayerName = playerName;
			isInit = true;
		}
		bool isNew = tracker.checkNew(playerName, isInit);
		std::this_thread::sleep_for(std::chrono::milliseconds(1000 * 30));
		isInit = false;
	}


}

void NarakaStateMonitor::OnClientClosed() {
	LOG_IMMEDIATE("永劫无间关闭");
}
