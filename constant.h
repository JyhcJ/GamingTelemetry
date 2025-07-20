#pragma once
#include <map>
#include <string>

///token为默认token

//线上配置 
//const std::wstring IS_DEBUG = L"";
//const std::wstring token测试 = L"eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJSZW1vdGVJcCI6IiIsIkxvY2FsTG9naW4iOjAsIkNvbnRleHQiOnsidXNlcl9pZCI6MjcxLCJ1c2VyX25hbWUiOiLpqaznq4vlm70iLCJ1dWlkIjoiIiwicmlkIjoxOCwibWFudWZhY3R1cmVfaWQiOjUzLCJiYXJfaWQiOjk5LCJyb290X2lkIjowLCJvcmdhbml6YXRpb25fdHlwZSI6IiIsInBsYXRmb3JtIjoiYmFyY2xpZW50In0sImV4cCI6MTc1MzQ0MTQxMH0.IUm74RI2IjXRdxT6fUbNcUeTD1Q7SqJ1cgeiJdfgwW4";
//线下配置
const std::wstring IS_DEBUG = L"dev-";
const std::wstring token测试 = L"eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJSZW1vdGVJcCI6IiIsIkxvY2FsTG9naW4iOjAsIkNvbnRleHQiOnsidXNlcl9pZCI6MjQ3LCJ1c2VyX25hbWUiOiJ4eHgiLCJ1dWlkIjoiIiwicmlkIjoxOCwibWFudWZhY3R1cmVfaWQiOjUzLCJiYXJfaWQiOjk4LCJyb290X2lkIjowLCJvcmdhbml6YXRpb25fdHlwZSI6IiIsInBsYXRmb3JtIjoiYmFyY2xpZW50In0sImV4cCI6MTc1MjEzODc4N30.OxuSFEDQOq31KK9Vh-uwL9phsuV5zovluBptoNC3eXw";
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
	{"competitive", "QUICK"},// 5e匹配
	{"casual", "NORMAL"},// 官匹人机
	{"custom", "QUICK"},//TODO 练习
};

static std::map<std::string, std::string> PUBG_modeMap = {
	{"competitive", "RANK"}
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