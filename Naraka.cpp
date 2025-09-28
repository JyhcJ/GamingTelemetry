#include "Naraka.h"


int main_Naraka() {
	NarakaStatsTracker tracker;

	// 初始化确认用
	std::string compUserName = GetEnvSafe("USERPROFILE");
	if (compUserName == "") {
		LOG_IMMEDIATE("永劫无间初始化失败.也许游戏运行且账号登录后能完成初始化");
	}
	std::string playerNameFile = compUserName + "\\AppData\\LocalLow\\24Entertainment\\Naraka\\Player-prev.log";

	if (!tracker.initialize(playerNameFile)) {
		LOG_IMMEDIATE("永劫无间初始化失败");
		return 1;
	}

	tracker.run();

	return 0;
}