#include "Naraka.h"


int main_Naraka() {
	NarakaStatsTracker tracker;

	// ��ʼ��ȷ����
	std::string compUserName = GetEnvSafe("USERPROFILE");
	if (compUserName == "") {
		LOG_IMMEDIATE("�����޼��ʼ��ʧ��.Ҳ����Ϸ�������˺ŵ�¼������ɳ�ʼ��");
	}
	std::string playerNameFile = compUserName + "\\AppData\\LocalLow\\24Entertainment\\Naraka\\Player-prev.log";

	if (!tracker.initialize(playerNameFile)) {
		LOG_IMMEDIATE("�����޼��ʼ��ʧ��");
		return 1;
	}

	tracker.run();

	return 0;
}