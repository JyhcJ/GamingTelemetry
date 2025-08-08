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
	//��ȡsession 

	std::string session = "2gYhJUz6USFQV5Lx3iG7q1XktvCHWmzMJT_QepHr";

	try {
		std::string ret;
		_sendHttp(L"/api/client/GetGameConfig", "", ret);
		nlohmann::json jsonData1 = nlohmann::json::parse(ret);
		nlohmann::json jsonData2 = nlohmann::json::parse(remove_escape_chars(trim_quotes(jsonData1["metadata"]["value"].dump())));
		//LOG_IMMEDIATE("ȡ����value: " + jsonData1["metadata"]["value"].dump());
		//LOG_IMMEDIATE("ȥ����βvalue: " + remove_escape_chars(trim_quotes(jsonData1["metadata"]["value"].dump())));
		LOG_IMMEDIATE(jsonData2.dump() + "cjmofang.com.");
		session=jsonData2["yongjiewujian"]["token"];
		//LOG_IMMEDIATE(generate_md5(jsonData2.dump() + "cjmofang.com."));
	}
	catch (const std::exception& e) {
		LOG_IMMEDIATE("NarakaStateMonitor::OnClientStarted():" + std::string(e.what()));
		return;
	}
	catch (...) {
		LOG_IMMEDIATE("NarakaStateMonitor::OnClientStarted():δ֪����");
		return;
	}




	std::string compUserName = GetEnvSafe("USERPROFILE");
	std::string playerNameFile = compUserName + "\\AppData\\LocalLow\\24Entertainment\\Naraka\\player_prefs.txt";
	if (std::remove(playerNameFile.c_str()) != 0) {
		// �ļ������ڻ�ɾ��ʧ��
		if (errno == ENOENT) {
			LOG_IMMEDIATE("NARAKA �ļ������ڣ�����ɾ��");
			//return true;  // �ļ������Ͳ����ڣ��������
		}
		else {
			LOG_IMMEDIATE("NARAKA ��־�ļ�ɾ��ʧ��");
			//return false;
		}
	}
	else {
		LOG_IMMEDIATE("NARAKA ɾ����־�ļ�");
	}

	std::string lastPlayerName;
	while (true) {
		std::this_thread::sleep_for(std::chrono::milliseconds(5000));

		if (IsProcessRunning(L"NarakaBladepoint.exe")) {
			//��ȡname;


			tracker.playerData->readPlayerNameFromFileDESC(playerNameFile, playerName);
			tracker.playerData->setPlayerName(playerName);
			lastPlayerName = playerName;
			if (playerName == "") {
				continue;
			}
			else{

				LOG_IMMEDIATE("Naraka:�������: " + UTF8ToGBK(playerName));
			}
			break;
		}
		else {
			return;
		}

		//LOG_IMMEDIATE_DEBUG("������...");
	}
	//});

	
	while (IsProcessRunning(L"NarakaBladepoint.exe")) {
		tracker.playerData->readPlayerNameFromFileDESC(playerNameFile, playerName);
		if (lastPlayerName != playerName) {
			LOG_IMMEDIATE("Naraka:������Ʊ��: " + UTF8ToGBK(playerName));
			lastPlayerName = playerName;
			isInit = true;
		}
		bool isNew = tracker.checkNew(playerName, isInit);
		std::this_thread::sleep_for(std::chrono::milliseconds(1000 * 30));
		isInit = false;
	}


}

void NarakaStateMonitor::OnClientClosed() {
	LOG_IMMEDIATE("�����޼�ر�");
}
