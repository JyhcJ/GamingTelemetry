#pragma once
#include <map>
#include <string>
#include <mutex>
//#include <shared_mutex>
///token为默认token

//线上配置 
//const std::wstring IS_DEBUG = L"";
//const std::wstring token测试 = L"eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJSZW1vdGVJcCI6IiIsIkxvY2FsTG9naW4iOjAsIkNvbnRleHQiOnsidXNlcl9pZCI6MjcxLCJ1c2VyX25hbWUiOiLpqaznq4vlm70iLCJ1dWlkIjoiIiwicmlkIjoxOCwibWFudWZhY3R1cmVfaWQiOjUzLCJiYXJfaWQiOjk5LCJyb290X2lkIjowLCJvcmdhbml6YXRpb25fdHlwZSI6IiIsInBsYXRmb3JtIjoiYmFyY2xpZW50In0sImV4cCI6MTc1MzQ0MTQxMH0.IUm74RI2IjXRdxT6fUbNcUeTD1Q7SqJ1cgeiJdfgwW4";

//本地调试线下配置(如果要强制线下,要关闭从barclient中获取域名)
const std::wstring IS_DEBUG = L"dev-";
const std::wstring domain_ = L"https://" + IS_DEBUG + L"asz.cjmofang.com";
const std::wstring token测试 = L"eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJSZW1vdGVJcCI6IiIsIkxvY2FsTG9naW4iOjAsIkNvbnRleHQiOnsidXNlcl9pZCI6MjgyLCJ1c2VyX25hbWUiOiLnvZfmlofotoXmsojkuq4iLCJ1dWlkIjoiIiwicmlkIjoxOCwibWFudWZhY3R1cmVfaWQiOjUzLCJiYXJfaWQiOjk4LCJyb290X2lkIjowLCJvcmdhbml6YXRpb25fdHlwZSI6IiIsInBsYXRmb3JtIjoiYmFyY2xpZW50In0sImV4cCI6MTc1NTEzNTcwM30.jmZHp3LxsAibkpChPOBpAy0bIy0rWB8IbVcflwT8_d4";

extern std::wstring g_domain;
extern std::mutex g_domain_mutex;
// 读取 g_domain 的函数

//try {}
//catch (const std::exception& e) {
//	LOG_IMMEDIATE("NarakaStateMonitor::OnClientStarted():" + std::string(e.what()));
//	return;
//}
//catch (...) {
//	LOG_IMMEDIATE("NarakaStateMonitor::OnClientStarted():未知错误");
//	return;
//}



//PUBG API Key(10req/min) 可申请多个提高容量
static const std::string _PUBG_APIKEY = "Bearer eyJ0eXAiOiJKV1QiLCJhbGciOiJIUzI1NiJ9.eyJqdGkiOiJlZGNjMDYwMC0zYWNiLTAxM2UtMjBlZC02ZThjNzIzMTJmZDIiLCJpc3MiOiJnYW1lbG9ja2VyIiwiaWF0IjoxNzUxNjA5NjA5LCJwdWIiOiJibHVlaG9sZSIsInRpdGxlIjoicHViZyIsImFwcCI6Ii03YmFjMDQ2My1hMDQ4LTRiN2QtYjdhNy0wNmI1MjlhNWEwMjYifQ.V-GZIMArqvqOuQhYfQL61Jbv4NEnmJMLcXquQS_5bPQ";

static const std::wstring _AUTHCOM = L"wmic PROCESS WHERE name='LeagueClientUx.exe' GET commandline";
static const std::string _APPPORT = "--app-port=";
static const std::string _AUTHTOKEN = "--remoting-auth-token";
static const std::string _RSOPLATFORM = "--rso_platform_id=";
static const std::string _RSO_ORIPLATFORM = "--rso_original_platform_id=";
static const std::string _RSOPLATFORMID = "--rso_platform_id=";
static const std::wstring _TEMPFILE = L"C:\\output.txt";



static std::map<std::string, std::string> LOL_regionMap = {
	{"HN1", "艾欧尼亚"},
	{"HN2", "祖安"},
	{"HN3", "诺克萨斯"},
	{"HN4", "班德尔城"},
	{"HN5", "皮尔特沃夫"},
	{"HN6", "战争学院"},
	{"HN7", "巨神峰"},
	{"HN8", "雷瑟守备"},
	{"HN9", "裁决之地"},
	{"HN10", "黑色玫瑰"},
	{"HN11", "暗影岛"},
	{"HN12", "钢铁烈阳"},
	{"HN13", "水晶之痕"},
	{"HN14", "均衡教派"},
	{"HN15", "影流"},
	{"HN16", "守望之海"},
	{"HN17", "征服之海"},
	{"HN18", "卡拉曼达"},
	{"HN19", "皮城警备"},
	{"WT1_NEW", "比尔吉沃特"},
	{"WT2_NEW", "德玛西亚"},
	{"WT3_NEW", "弗雷尔卓德"},
	{"WT4_NEW", "无畏先锋"},
	{"WT5", "恕瑞玛"},
	{"WT6", "扭曲丛林"},
	{"WT7", "巨龙之巢"},
	{"EDU1", "教育网专区"},
	{"BGP1", "男爵领域"}
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
	//段位:UNLIMIT=不限制(空段位),ZUI_QIANG_WANG_ZHE=最强王者,AO_SHI_ZONG_SHI=傲世宗师,CHAO_FAN_DA_SHI=超凡大师,ZUAN_SHI=钻石,
	// FEI_CUI=翡翠,BO_JIN=铂金,HUANG_JIN=黄金,BAI_YIN=白银,QING_TONG=青铜,HEI_TIE=黑铁
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
	{"unrated", "BOMB"},//普通模式
	{"competitive", "RANK"},//竞技模式
	{"swiftplay", "SWIFTPLAY"},//快速模式
	{"spikerush", "QUICK_BOMB"},//爆能快攻
	{"deathmatch", "DEATHMATCH"},//死斗模式
	{"ggteam", "SWIFTPLAY"},//极速模式
	{"onefa", "ONE_FOR_ALL"},//复制战士
	{"snowball", "SNOWBALL_FIGHT"},//雪球大战
	{"premier","冠军赛"},//冠军赛
	{"custom","CUSTOM"},//自定义游戏
	{"practice","射击训练"},//射击训练
	{"newplayer","新手教程"},//新手教程
	{"hurms","HURM"},//团队死斗
	{"",""},//GUN_GAME武装升级

};

static std::map<int, std::string> VAL_rankMap = {
	{0, "UNLIMIT"},//未定级

	{1, "UNLIMIT"},//未定级
	{2, "UNLIMIT"},//未定级
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
	{"competitive", "NORMAL"},// 5e匹配
	{"casual", "QUICK"},// 官匹人机
	{"custom", "QUICK"},//TODO 练习
};

static std::map<std::string, std::string> PUBG_modeMap = {
	{"competitive", "RANK"},
	{"official", "MATCH"} //匹配
};

static std::map<std::string, std::string> PUBG_teamSize = {
	{"solo", "ONE"},
	{"solo-fpp", "ONE"},
	{"duo", "TWO"},
	{"duo-fpp", "TWO"},
	{"squad", "FOUR"},
	{"squad-fpp", "FOUR"},
	//{"competitive", "UNLIMIT"},
	//{"competitive", "OVER_FOUR"},
};

static std::map<int, std::string> NARAKA_modeMap = {
			{0 ,"0"},  //全部
			{1 ,"1"},  //天选单排
			{2 ,"2"},  //天选三排
			{3 ,"3"},  //无尽试炼
			{4 ,"4"},  //天人单排
			{5 ,"5"},  //天人三排
			{6 ,"6"},  //匹配单排
			{7 ,"7"},  //匹配三排
			{9 ,"9"},  //匹配双排
			{10,"10"},  //无尽试炼双排
			{11,"11"},  //无尽试炼三排
			{12,"12"},  //天选双排
			{13,"13" }  //天人双排
};
static std::map<int,int> NARAKA_teamSize = {
	{1, 1},  	//天选单排
	{2, 3},		//天选三排
	{3, 1},		//无尽试炼
	{4, 1},		//天人单排
	{5, 3},		//天人三排
	{6, 1},		//匹配单排
	{7, 3},		//匹配三排
	{9, 2},		//匹配双排
	{10, 2},	//无尽试炼双排
	{11, 3},	//无尽试炼三排
	{12, 2},	//天选双排
	{13, 2}		//天人双排
};				


static std::map<int, std::string> DELTA_rankMap = {
	{1, "QING_TONG"},
	{2, "QING_TONG"},
	{3, "QING_TONG"},
	{4, "BAI_YIN"},
	{5, "BAI_YIN"},
	{6, "BAI_YIN"},
	{7, "HUANG_JIN"},
	{8, "HUANG_JIN"},
	{9, "HUANG_JIN"},
	{10, "HUANG_JIN"},
	{11, "BO_JIN"},
	{12, "BO_JIN"},
	{13, "BO_JIN"},
	{14, "BO_JIN"},
	{15, "ZUAN_SHI"},
	{16, "ZUAN_SHI"},
	{17, "ZUAN_SHI"},
	{18, "ZUAN_SHI"},
	{19, "ZUAN_SHI"},
	{20, "HEI_YING"},
	{21, "HEI_YING"},
	{22, "HEI_YING"},
	{23, "HEI_YING"},
	{24, "HEI_YING"},
	{25, "SAN_JIAO_ZHOU_DIAN_FEN"},
	{0, "UNLIMIT"},
};


//BOMB=普通模式,DEATHMATCH=乱斗模式,GUN_GAME=武装升级,ONE_FOR_ALL=克隆模式,QUICK_BOMB=爆能快攻,SNOWBALL_FIGHT=雪球大战,SWIFTPLAY=极速模式,HURM=团队乱斗,CUSTOM=自定义,RANK=竞技模式
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