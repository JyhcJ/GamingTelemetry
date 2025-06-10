#include "pch.h"
#include "lol_before.h"
#include "constant.h"
#include "lol.h"

// ȫ�ֱ��������Ծ��а�
//std::string BEFORE_NUMS_COUNT;
//std::string BEFORE_REGION;
//std::string BEFORE_RANK;
//std::string BEFORE_STATE;
//std::string BEFORE_TYPE;
//std::string BEFORE_RANK_H;

nlohmann::json g_infoBefore;

static std::map<size_t, size_t> HISTORY_GAMES;//gameid _ userid

bool Game_Before::getParam() {
	std::string str = ExecuteCommandAsAdmin(L"wmic PROCESS WHERE name='LeagueClientUx.exe' GET commandline");
	if (str.size() < 100)
	{
		LOG_IMMEDIATE_ERROR("�ͻ���δ����?");
		return false;
	}
	app_port = extractParamValue(str, "--app-port=");
	auth_token = extractParamValue(str, "--remoting-auth-token");
	rso_platform_id = extractParamValue(str, "--rso_platform_id=");
	rso_original_platform_id = extractParamValue(str, "--rso_original_platform_id=");

	if (rso_original_platform_id == "") {
		rso_original_platform_id = extractParamValue(str, "--rso_platform_id=");
	}


	url = "https://riot:" + auth_token + "@127.0.0.1:" + app_port;

	return true;
}

// ��Ҫ��get ��֤��װ
bool Game_Before::httpAuthSend(std::string endUrl, nlohmann::json& responseJson, std::string param) {
	std::string auth = "riot:" + auth_token;
	//std::string auth_header = "Authorization: Basic " + base64_encode(auth);
	std::map<std::wstring, std::wstring> headers = {};
	headers.emplace(L"Authorization", L"Basic " + string2wstring(base64_encode(auth)));
	headers.emplace(L"Content-Type", L"application/json");

	std::string allUrl = url + endUrl;
	//std::string summoner_json = make_request(summoner_url);
	std::string response_json = http.SendRequest(
		string2wstring(allUrl),
		L"GET",
		headers,
		"",
		true,
		string2wstring(param)
	);
	nlohmann::json p_responseJson;
	try {
		p_responseJson = nlohmann::json::parse(response_json);
	}
	catch (const nlohmann::json::parse_error& e) {
		std::string error = e.what();
		LOG_IMMEDIATE_WARNING("p_responseJson ����ʧ��: " + error);
		LOG_IMMEDIATE_WARNING("p_responseJson : " + p_responseJson);
	}
	// ����ֶ��Ƿ����
	if (p_responseJson.contains("errorCode")) {
		LOG_IMMEDIATE_WARNING(allUrl + "::errorCode::" + p_responseJson["errorCode"].dump());
		if (p_responseJson.contains("message")) {
			LOG_IMMEDIATE_WARNING(allUrl + "::message::" + p_responseJson["message"].dump());
		}
		responseJson = p_responseJson;
		return false;
	}

	responseJson = p_responseJson;
	return true;


}
// ��Ϸ��:SAN_SHA_SHU = ��ɱ��, SI_SHA_SHU = ��ɱ��, WU_SHA_SHU = ��ɱ��, CHAO_SHEN_SHU = ������,
// ��Ϸ����ǰ/��:RUN = �ͻ��˴�, KILL = �ͻ��˹ر� / END = ��Ϸ����
void Game_Before::getAndSendInfo(std::string sendType) {
	if (sendType == "RUN" || sendType == "KILL") {
		nlohmann::json	jsonbody;
		jsonbody[sendType] = sendType;
		_sendHttp_LOL(jsonbody);
		return;
	}

	/*std::string auth = "riot:" + auth_token;
	std::string auth_header = "Authorization: Basic " + base64_encode(auth);*/
	//std::map<std::wstring, std::wstring> headers = {};

	//headers.emplace(L"Authorization", L"Basic " + string2wstring(base64_encode(auth)));
	//headers.emplace(L"Content-Type", L"application/json");
	//-------------------������Ϣ
	//std::string summoner_url = url + "/lol-summoner/v1/current-summoner";
	////std::string summoner_json = make_request(summoner_url);
	//std::string response_summoner_json = http.SendRequest(
	//	string2wstring(summoner_url),
	//	L"GET",
	//	headers,
	//	"",
	//	true
	//);
	//std::cout << "Response: " << response_summoner_json << std::endl;

	nlohmann::json summonerData;
	uint64_t myAccountId;
	if (httpAuthSend("/lol-summoner/v1/current-summoner", summonerData))
	{
		myAccountId = summonerData["accountId"];
		//myAccountId = summonerData["accountId"].get<uint64_t>();
	}


	//LOG_IMMEDIATE("Response: " + response);

	//std::cout << "Summoner Info: " << summoner_json << std::endl;

	//std::string ranked_url = "https://127.0.0.1:" + app_port + "/lol-ranked/v1/ranked-stats/123"; // Need summonerId
	//-------------------��λ��Ϣ ����ӿڲ���ô������
	//std::string ranked_url = url + "/lol-ranked/v1/ranked-stats/123"; //  summonerId
	////std::string ranked_json = make_request(ranked_url, auth_header);
	//std::string response_ranked_json = http.SendRequest(
	//	string2wstring(ranked_url),
	//	L"GET",
	//	headers,
	//	"",
	//	true

	//);

	////std::cout << "Ranked Stats: " << response_ranked_json << std::endl;
	//nlohmann::json rankData;
	//nlohmann::json rank;
	//try {
	//	rankData = nlohmann::json::parse(response_ranked_json);
	//}
	//catch (const nlohmann::json::parse_error& e) {
	//	std::cerr << "response_ranked_json ����ʧ��: " << e.what() << std::endl;
	//	std::cerr << "response_ranked_json : " << response_ranked_json << std::endl;
	//}


	//for (const auto& member : rankData["queues"]) {
	//	std::string compare = member["queueType"].get<std::string>();
	//	if (compare == "RANKED_SOLO_5x5") {
	//		rank["RANKED_SOLO_5x5"]["tier"] = member["tier"];
	//		rank["RANKED_SOLO_5x5"]["division"] = member["division"];
	//	}
	//	else if (compare == "RANKED_FLEX_SR") {
	//		rank["RANKED_FLEX_SR"]["tier"] = member["tier"];
	//		rank["RANKED_FLEX_SR"]["division"] = member["division"];
	//	}

	//}

	//std::string rankJsonStr = rankData.dump(0); // ����4�Ļ�

	//-------------------�����Ϣ
	//std::string lobby_url = url + "/lol-lobby/v2/lobby";
	//std::string response_lobby_json = http.SendRequest(
	//	string2wstring(lobby_url),
	//	L"GET",
	//	headers,
	//	"",
	//	true
	//);

	//nlohmann::json lobbyData;
	//try {
	//	lobbyData = nlohmann::json::parse(response_lobby_json);
	//}
	//catch (const nlohmann::json::parse_error& e) {
	//	std::cerr << "response_lobby_json ����ʧ��: " << e.what() << std::endl;
	//	std::cerr << "response_lobby_json : " << response_lobby_json << std::endl;
	//}

	nlohmann::json lobbyData;
	size_t teamSize = 0;
	std::vector<uint64_t> lobbySummonerIds;
	if (httpAuthSend("/lol-lobby/v2/lobby", lobbyData))
	{
		teamSize = lobbyData["members"].size();
		//myAccountId = summonerData["accountId"].get<uint64_t>();

		//0 2 3 4 5
		if (teamSize > 1) {
			for (const auto& member : lobbyData["members"]) {
				lobbySummonerIds.push_back(member["summonerId"].get<uint64_t>());
			}
			std::cout << "Summoner IDs:" << std::endl;
			for (uint64_t id : lobbySummonerIds) {
				std::cout << "- " << id << std::endl;
			}
		}
		else {
			teamSize = 1;
		}
	}

	//std::cout << lobbyData.dump() << std::endl;
	//std::cout << "Lobby Info: " << response_lobby_json << std::endl;



	//-------------------gameID,gamemode,phase(��Ϸ����ʱ,����׼��,����ʱ��������)
	//std::string session_url = url + "/lol-gameflow/v1/session";
	////std::string lobby_json = make_request(lobby_url, auth_header);
	//std::string response_session_json = http.SendRequest(
	//	string2wstring(session_url),
	//	L"GET",
	//	headers,
	//	"",
	//	true
	//);
	//nlohmann::json sessionData;
	//try {
	//	sessionData = nlohmann::json::parse(response_session_json);
	//}
	//catch (const nlohmann::json::parse_error& e) {
	//	std::cerr << "response_session_json ����ʧ��: " << e.what() << std::endl;
	//	std::cerr << "response_session_json : " << response_session_json << std::endl;
	//}
	nlohmann::json sessionData;
	size_t gameId = 0;
	std::string	game_mode = "init";
	std::string phase = "init";
	if (httpAuthSend("/lol-gameflow/v1/session", sessionData))
	{
		gameId = sessionData["gameData"]["gameId"]; //�˴� �� sj�Ծ��е�gameid
		if (gameId != 0) {
			HISTORY_GAMES.emplace(gameId, myAccountId);
		}
		game_mode = sessionData["gameData"]["queue"]["type"];
		phase = sessionData["phase"];
	}


	// -------------------�����������
	//std::string matches_url = url + "/lol-match-history/v1/products/lol/current-summoner/matches";
	////std::string lobby_json = make_request(lobby_url, auth_header);
	//std::string response_matches_json = http.SendRequest(
	//	string2wstring(matches_url),
	//	L"GET",
	//	headers,
	//	"",
	//	true,
	//	L"?begIndex=0&endIndex=0"
	//);
	//nlohmann::json matchesData;
	//try {
	//	matchesData = nlohmann::json::parse(response_matches_json);
	//}
	//catch (const nlohmann::json::parse_error& e) {
	//	std::cerr << "response_matches_json ����ʧ��: " << e.what() << std::endl;
	//	std::cerr << "response_matches_json : " << response_matches_json << std::endl;
	//}

	nlohmann::json	matchesData;

	std::string		endOfGameResult;
	uint64_t		participantId_u64;
	uint64_t matchAccountId_u64;
	uint64_t matchGameId_u64;

	nlohmann::json jsonBody;
	nlohmann::json data;
	nlohmann::json member;

	//
	if (HISTORY_GAMES[gameId] == myAccountId)
	{

	}

	if (httpAuthSend("/lol-match-history/v1/products/lol/current-summoner/matches", matchesData, "?begIndex=0&endIndex=0"))
	{
		matchAccountId_u64 = matchesData["accountId"].get<uint64_t>();
		data["member"] = nlohmann::json::array();

		//jsonBody
		for (const auto& game : matchesData["games"]["games"]) {
			matchGameId_u64 = game["gameId"];
			if (HISTORY_GAMES[matchGameId_u64]== myAccountId && "GameComplete" == game["endOfGameResult"])
			{
				for (const auto& participantIdentitie : game["participantIdentities"]) {
					member["id"] = participantIdentitie["player"]["summonerId"];
					member["role"] = "other";
					// ��� participantId �Ƿ�ƥ��
					if (member["id"] == matchAccountId_u64) {
						participantId_u64 = participantIdentitie["participantId"].get<uint64_t>();
						member["role"] = "self";

						continue;
					}
					if (lobbyData.contains(member["id"]))
					{
						member["role"] = "team";
					}

					data["member"].push_back(member);
				}

				for (const auto& participant : game["participants"]) {
					if (participant["participantId"] == participantId_u64) {
						data["ren_tou_shu"] = participant["stats"]["kills"];
						data["zhu_gong_shu"] = participant["stats"]["assists"];
						// neutralMinionsKilled �ֶ� С��+Ұ��.
						data["bu_bing_shu"] = participant["stats"]["totalMinionsKilled"];
						data["pai_yan_shu"] = participant["stats"]["wardsKilled"];
						data["win"] = participant["stats"]["win"] == false ? 0 : 1;
						//data["time"] = participant["stats"]["assists"];
							//data["time"] = matchesData["games"]["games"]["gameDuration"];
						break;
					}
				}
				data["time"] = game["gameDuration"];
			}
			
		}
	}

	nlohmann::json	ranksData;
	std::string		highestTier;
	if (httpAuthSend("/lol-ranked/v1/current-ranked-stats", ranksData, ""))
	{
		highestTier = ranksData["highestRankedEntrySR"]["highestTier"];
	}

	//for (const auto& participant : matchesData["games"]["games"]["participants"]) {
	//	// ��� participantId �Ƿ�ƥ��
	//	if (participant["participantId"] == participantId) {

	//		data["ren_tou_shu"] = participant["stats"]["kills"];
	//		data["zhu_gong_shu"] = participant["stats"]["assists"];
	//		data["bu_bing_shu"] = participant["stats"]["totalMinionsKilled"];
	//		data["pai_yan_shu"] = participant["stats"]["wardsKilled"];
	//		data["win"] = participant["stats"]["win"];
	//		//data["time"] = participant["stats"]["assists"];
	//		break;
	//	}
	//}


	std::string region = LOL_regionMap.count(rso_original_platform_id) ?
		LOL_regionMap[rso_original_platform_id] :
		"Unknown";

	//BEFORE_RANK = rank.dump(4);
	//BEFORE_RANK = LOL_rankAPIMap[highestTier];

	//BEFORE_NUMS_COUNT = LOL_teamSizeMap[static_cast<int>(teamSize)];
	////BEFORE_NUMS_COUNT = teamSizeMap[static_cast<int>(teamSize)];
	//BEFORE_REGION = region;
	//BEFORE_STATE = phase;
	//BEFORE_TYPE = LOL_gameModMap[game_mode];

	//remark
	jsonBody["game_uuid"] = std::to_string(gameId);
	jsonBody["computer_no"] = "8888888888";
	jsonBody["name"] = "LOL";
	jsonBody["game_mode"] = LOL_gameModMap[game_mode];
	jsonBody["team_size"] = LOL_teamSizeMap[static_cast<int>(teamSize)];
	jsonBody["user_game_rank"] = LOL_rankAPIMap[highestTier];
	jsonBody["type"] = sendType;
	jsonBody["data"] = data;
	jsonBody["remark"] = phase;
	if (sendType == "update") {
		//LOG_IMMEDIATE(jsonBody.dump(4));
		g_infoBefore = jsonBody;
	}
	else if (sendType == "END") {
		_sendHttp_LOL(jsonBody);
	}

	//std::string ds_str = rank["RANKED_SOLO_5x5"]["division"];
	//std::string lh_str = rank["RANKED_FLEX_SR"]["division"];
	//int ds = rankLevel[ds_str];
	//int lh = rankLevel[lh_str];

	//if (ds >= lh) {
	//	BEFORE_RANK_H = rankAPIMap[ds_str];
	//}
	//else {
	//	BEFORE_RANK_H = rankAPIMap[lh_str];
	//}

	//LOG_IMMEDIATE("data : " + data.dump());
	//LOG_IMMEDIATE("gameID : " + std::to_string(gameId));
	//LOG_IMMEDIATE("gameState : " + phase);
	//LOG_IMMEDIATE("gameType : " + LOL_gameModMap[game_mode]);
	//LOG_IMMEDIATE("�������� : " + BEFORE_REGION);
	//LOG_IMMEDIATE("��� : " + BEFORE_RANK);
	////LOG_IMMEDIATE("��߶�λ : " + BEFORE_RANK_H);
	//LOG_IMMEDIATE("�������� : " + BEFORE_NUMS_COUNT);

}


bool Game_Before::before_main(std::string sendType) {
	if (Game_Before::getParam()) {
		Game_Before::getAndSendInfo(sendType);
		return true;
	}
	return false;
}