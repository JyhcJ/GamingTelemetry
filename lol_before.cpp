#include "pch.h"
#include "lol_before.h"

// 全局变量传到对局中吧
std::string BEFORE_NUMS_COUNT;
std::string BEFORE_REGION;
std::string BEFORE_RANK;

bool Game_Before::getParam() {
	std::string str = ExecuteCommandAsAdmin(L"wmic PROCESS WHERE name='LeagueClientUx.exe' GET commandline");
	if (str.size() < 100)
	{
		LOG_IMMEDIATE_ERROR("客户端未启动?");
		return false;
	}
	app_port = extractParamValue(str, "--app-port=");
	auth_token = extractParamValue(str, "--remoting-auth-token");
	rso_platform_id = extractParamValue(str, "--rso_platform_id=");
	rso_original_platform_id = extractParamValue(str, "--rso_original_platform_id=");

	url = "https://riot:" + auth_token + "@127.0.0.1:" + app_port;

	return true;
}

void Game_Before::getUserInfo() {
	std::string auth = "riot:" + auth_token;
	std::string auth_header = "Authorization: Basic " + base64_encode(auth);
	std::map<std::wstring, std::wstring> headers = {};
	headers.emplace(L"Authorization", L"Basic " + string2wstring(base64_encode(auth)));
	headers.emplace(L"Content-Type", L"application/json");

	std::string summoner_url = url + "/lol-summoner/v1/current-summoner";
	//std::string summoner_json = make_request(summoner_url);
	std::string response_summoner_json = http.SendRequest(
		string2wstring(summoner_url),
		L"GET",
		headers,
		"",
		true
	);
	std::cout << "Response: " << response_summoner_json << std::endl;
	//LOG_IMMEDIATE("Response: " + response);

	//std::cout << "Summoner Info: " << summoner_json << std::endl;

	//std::string ranked_url = "https://127.0.0.1:" + app_port + "/lol-ranked/v1/ranked-stats/123"; // Need summonerId
	std::string ranked_url = url + "/lol-ranked/v1/ranked-stats/123"; //  summonerId
	//std::string ranked_json = make_request(ranked_url, auth_header);
	std::string response_ranked_json = http.SendRequest(
		string2wstring(ranked_url),
		L"GET",
		headers,
		"",
		true

	);
	//std::cout << "Ranked Stats: " << response_ranked_json << std::endl;
	nlohmann::json rankData;
	nlohmann::json rank;
	try {
		rankData = nlohmann::json::parse(response_ranked_json);
	}
	catch (const nlohmann::json::parse_error& e) {
		std::cerr << "response_ranked_json 解析失败: " << e.what() << std::endl;
		std::cerr << "response_ranked_json : " << response_ranked_json << std::endl;
	}


	for (const auto& member : rankData["queues"]) {
		std::string compare = member["queueType"].get<std::string>();
		if (compare == "RANKED_SOLO_5x5") {
			rank["RANKED_SOLO_5x5"]["tier"] = member["tier"];
			rank["RANKED_SOLO_5x5"]["division"] = member["division"];
		}
		else if (compare == "RANKED_FLEX_SR") {
			rank["RANKED_FLEX_SR"]["tier"] = member["tier"];
			rank["RANKED_FLEX_SR"]["division"] = member["division"];
		}

	}



	std::string rankJsonStr = rankData.dump(0); // 缩进4的话，漂亮输出


	// Get lobby info
	//std::string lobby_url = "https://127.0.0.1:" + app_port + "/lol-lobby/v2/lobby";
	std::string lobby_url = url + "/lol-lobby/v2/lobby";
	//std::string lobby_json = make_request(lobby_url, auth_header);
	std::string response_lobby_json = http.SendRequest(
		string2wstring(lobby_url),
		L"GET",
		headers,
		"",
		true
	);

	nlohmann::json lobbyData;
	try {
		lobbyData = nlohmann::json::parse(response_lobby_json);
	}
	catch (const nlohmann::json::parse_error& e) {
		std::cerr << "response_lobby_json 解析失败: " << e.what() << std::endl;
		std::cerr << "response_lobby_json : " << response_lobby_json << std::endl;
	}
	//std::cout << lobbyData.dump() << std::endl;
	//std::cout << "Lobby Info: " << response_lobby_json << std::endl;
	size_t teamSize = lobbyData["members"].size();
	std::vector<uint64_t> summonerIds;
	//0 2 3 4 5
	if (teamSize > 1) {
		for (const auto& member : lobbyData["members"]) {
			summonerIds.push_back(member["summonerId"].get<uint64_t>());
		}
		std::cout << "Summoner IDs:" << std::endl;
		for (uint64_t id : summonerIds) {
			std::cout << "- " << id << std::endl;
		}
	}
	else {
		teamSize = 1;
	}


	std::string session_url = url + "/lol-gameflow/v1/session";
	//std::string lobby_json = make_request(lobby_url, auth_header);
	std::string response_session_json = http.SendRequest(
		string2wstring(session_url),
		L"GET",
		headers,
		"",
		true
	);
	nlohmann::json sessionData;
	try {
		sessionData = nlohmann::json::parse(response_session_json);
	}
	catch (const nlohmann::json::parse_error& e) {
		std::cerr << "response_session_json 解析失败: " << e.what() << std::endl;
		std::cerr << "response_session_json : " << response_session_json << std::endl;
	}
	//std::cout << lobbyData.dump() << std::endl;
	size_t gameId = sessionData["gameData"]["gameId"];
	std::string game_mode = sessionData["gameData"]["queue"]["type"];



	// Get region name
	std::string region = region_map.count(rso_original_platform_id) ?
		region_map[rso_original_platform_id] :
		"Unknown";

	//队伍人数:ONE=单人,TWO=双人,THREE=三人,FOUR=四人,OVER_FOUR=四人以上
	std::map<std::string, std::string> gameModMap = {
		{"NORMAL", "MATCH"},
		{"RANKED_SOLO_5x5", "SINGLE_AND_DOUBLE"},
		{"RANKED_FLEX_SR", "FREE_GROUP"},
		{"ARAM_UNRANKED_5x5", "SUPER_SMASH_BROTHERS"},
		{"URF", "UNLIMIT_FIRE"},
		{"ARURF", "UNLIMIT_FIRE"},
		{"NEXUS_BLITZ", "ULTIMATE_HIT"},
		{"BRAWL", "GOD_TREE"}
	};
	

	std::map<int, std::string> teamSizeMap = {
		{1, "ONE"},
		{2, "TWO"},
		{3, "THREE"},
		{4, "FOUR"},
		{5, "OVER_FOUR"}
	};

	BEFORE_RANK = rank.dump(4);
	BEFORE_NUMS_COUNT = teamSizeMap[static_cast<int>(teamSize)];
	//BEFORE_NUMS_COUNT = teamSizeMap[static_cast<int>(teamSize)];
	BEFORE_REGION = region;
	LOG_IMMEDIATE("gameID : " + std::to_string(gameId));
	LOG_IMMEDIATE("gameType : " + gameModMap[game_mode]);
	LOG_IMMEDIATE("所在区服 : " + BEFORE_REGION);
	LOG_IMMEDIATE("段位 : " + BEFORE_RANK);
	LOG_IMMEDIATE("队伍人数 : " + BEFORE_NUMS_COUNT);

}


bool Game_Before::before_main() {
	if (Game_Before::getParam()) {
		Game_Before::getUserInfo();
		return true;
	}
	return false;
}