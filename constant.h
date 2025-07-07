#pragma once
#include <map>
#include <string>

static const std::wstring _AUTHCOM = L"wmic PROCESS WHERE name='LeagueClientUx.exe' GET commandline";
static const std::string _APPPORT = "--app-port=";
static const std::string _AUTHTOKEN = "--remoting-auth-token";
static const std::string _RSOPLATFORM = "--rso_platform_id=";
static const std::string _RSO_ORIPLATFORM = "--rso_original_platform_id=";
static const std::string _RSOPLATFORMID = "--rso_platform_id=";
static const std::wstring _TEMPFILE = L"C:\\output.txt";


static std::map<std::string, std::string> LOL_regionMap = {
	{"HN1", "��ŷ����"},
	{"HN2", "�氲"},
	{"HN3", "ŵ����˹"},
	{"HN4", "��¶���"},
	{"HN5", "Ƥ�����ַ�"},
	{"HN6", "ս��ѧԺ"},
	{"HN7", "�����"},
	{"HN8", "��ɪ�ر�"},
	{"HN9", "�þ�֮��"},
	{"HN10", "��ɫõ��"},
	{"HN11", "��Ӱ��"},
	{"HN12", "��������"},
	{"HN13", "ˮ��֮��"},
	{"HN14", "�������"},
	{"HN15", "Ӱ��"},
	{"HN16", "����֮��"},
	{"HN17", "����֮��"},
	{"HN18", "��������"},
	{"HN19", "Ƥ�Ǿ���"},
	{"WT1_NEW", "�ȶ�������"},
	{"WT2_NEW", "��������"},
	{"WT3_NEW", "���׶�׿��"},
	{"WT4_NEW", "��η�ȷ�"},
	{"WT5", "ˡ����"},
	{"WT6", "Ť������"},
	{"WT7", "����֮��"},
	{"EDU1", "������ר��"},
	{"BGP1", "�о�����"}
};

static std::map<std::string, std::string> LOL_gameModMap = {
	{"NORMAL", "MATCH"},
	{"RANKED_SOLO_5x5", "SINGLE_AND_DOUBLE"},
	{"RANKED_FLEX_SR", "FREE_GROUP"},
	{"ARAM_UNRANKED_5x5", "SUPER_SMASH_BROTHERS"},
	{"URF", "UNLIMIT_FIRE"},
	{"ARURF", "UNLIMIT_FIRE"},
	{"NEXUS_BLITZ", "ULTIMATE_HIT"},
	{"BRAWL", "GOD_TREE"},
	//{"NONE","NULL"},
	{"NONE","MATCH"},
	{"",""},

};

static std::map<int, std::string> LOL_teamSizeMap = {
	{0, ""},
	{1, "ONE"},
	{2, "TWO"},
	{3, "THREE"},
	{4, "FOUR"},
	{5, "OVER_FOUR"}
};

static std::map<std::string, std::string> LOL_rankAPIMap = {
	//��λ:UNLIMIT=������(�ն�λ),ZUI_QIANG_WANG_ZHE=��ǿ����,AO_SHI_ZONG_SHI=������ʦ,CHAO_FAN_DA_SHI=������ʦ,ZUAN_SHI=��ʯ,
	// FEI_CUI=���,BO_JIN=����,HUANG_JIN=�ƽ�,BAI_YIN=����,QING_TONG=��ͭ,HEI_TIE=����
		{"", "UNLIMIT"},
		{"NA", "UNLIMIT"},
		{"IRON", "HEI_TIE"},
		{"BRONZE", "QING_TONG"},
		{"SILVER", "BAI_YIN"},
		{"GOLD", "HUANG_JIN"},
		{"PLATINUM", "BO_JIN"},
		{"EMERALD", "FEI_CUI"},
		{"DIAMOND", "ZUAN_SHI"},
		{"MASTER", "CHAO_FAN_DA_SHI"},
		{"GRANDMASTER", "AO_SHI_ZONG_SHI"},
		{"CHALLENGER", "ZUI_QIANG_WANG_ZHE"}
};

static std::map<std::string, std::string> VAL_gameModMap = {
	{"unrated", "BOMB"},//��ͨģʽ
	{"competitive", "RANK"},//����ģʽ
	{"swiftplay", "SWIFTPLAY"},//����ģʽ
	{"spikerush", "QUICK_BOMB"},//���ܿ칥
	{"deathmatch", "DEATHMATCH"},//����ģʽ
	{"ggteam", "SWIFTPLAY"},//����ģʽ
	{"onefa", "ONE_FOR_ALL"},//����սʿ
	{"snowball", "SNOWBALL_FIGHT"},//ѩ���ս
	{"premier","�ھ���"},//�ھ���
	{"custom","CUSTOM"},//�Զ�����Ϸ
	{"practice","���ѵ��"},//���ѵ��
	{"newplayer","���ֽ̳�"},//���ֽ̳�
	{"hurms","HURM"},//�Ŷ�����
	{"",""},//GUN_GAME��װ����

};

static std::map<int, std::string> VAL_rankMap = {
	{0, "UNLIMIT"},//δ����

	{1, "UNLIMIT"},//δ����
	{2, "UNLIMIT"},//δ����
	{3, "HEI_TIE"},//
	{4, "HEI_TIE"},//
	{5, "HEI_TIE"},//
	{6, "QING_TONG"},//
	{7, "QING_TONG"},//
	{8,"QING_TONG"},//
	{9,"BAI_YIN"},//
	{10,"BAI_YIN"},//
	{11,"BAI_YIN"},//
	{12,"HUANG_JIN"},//
	{13,"HUANG_JIN"},//
	{14, "HUANG_JIN"},//
	{15, "BO_JIN"},//
	{16, "BO_JIN"},//
	{17, "BO_JIN"},//
	{18, "ZUAN_SHI"},//
	{19, "ZUAN_SHI"},//
	{20, "ZUAN_SHI"},//
	{21, "CHAO_FAN"},//
	{22,"CHAO_FAN"},//
	{23,"CHAO_FAN"},//
	{24,"SHEN_HUA"},//b 
	{25,"SHEN_HUA"},//
	{26,"SHEN_HUA"},//
	{27,"WU_WEI_ZHAN_HUN"},//

};

static std::map<std::string, std::string> CS2_modeMap = {
	{"competitive", "QUICK"},// 5eƥ��
	{"casual", "NORMAL"},// ��ƥ�˻�
	{"custom", "QUICK"},//TODO ��ϰ
};


//BOMB=��ͨģʽ,DEATHMATCH=�Ҷ�ģʽ,GUN_GAME=��װ����,ONE_FOR_ALL=��¡ģʽ,QUICK_BOMB=���ܿ칥,SNOWBALL_FIGHT=ѩ���ս,SWIFTPLAY=����ģʽ,HURM=�Ŷ��Ҷ�,CUSTOM=�Զ���,RANK=����ģʽ
//std::unordered_map<std::string, int> rankLevel = {
//{"NA", 0},
//{"IRON", 1},
//{"BRONZE", 2},
//{"SILVER", 3},
//{"GOLD", 4},
//{"PLATINUM", 5},
//{"EMERALD", 6},
//{"DIAMOND", 7},
//{"MASTER", 8},
//{"GRANDMASTER", 9},
//{"CHALLENGER", 10}
//};