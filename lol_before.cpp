#include "pch.h"
#include "lol_before.h"
#include "constant.h"
#include "lol.h"

extern std::string g_hostName;
extern bool is_lol_running ;
extern bool is_lol_game_running;
nlohmann::json g_infoBefore;

std::mutex g_mtx;
static std::map<size_t, size_t> HISTORY_GAMES;//gameid _ userid

bool Game_Before::getParam() {
	std::string str = ExecuteCommandAsAdmin(L"wmic PROCESS WHERE name='LeagueClientUx.exe' GET commandline");
	if (str.size() < 100)
	{
		LOG_IMMEDIATE_ERROR("�ͻ���δ����? �ȴ��ͻ�����ȫ����");
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
	nlohmann::json p_responseJson;
	//std::string summoner_json = make_request(summoner_url);
	try {
		std::string response_json = http.SendRequest(
			string2wstring(allUrl),
			L"GET",
			headers,
			"",
			true,
			string2wstring(param)
		);

		p_responseJson = nlohmann::json::parse(response_json);
	}
	catch (const nlohmann::json::parse_error& e) {
		std::string error = e.what();
		LOG_IMMEDIATE_DEBUG("p_responseJson ����ʧ��: " + error);
		LOG_IMMEDIATE_DEBUG("p_responseJson : " + p_responseJson);
	}
	catch (const std::exception& e) {
		LOG_IMMEDIATE_ERROR("httpAuthSend:::");
		LOG_IMMEDIATE_ERROR(e.what());
	}
	catch (...) {  // �������������쳣
		LOG_IMMEDIATE_ERROR("httpAuthSend :::Unknown exception occurred");
	}

	// ����ֶ��Ƿ����
	if (p_responseJson.contains("errorCode")) {
		LOG_IMMEDIATE_DEBUG(allUrl + "::errorCode::" + p_responseJson["errorCode"].dump());
		if (p_responseJson.contains("message")) {
			LOG_IMMEDIATE_DEBUG(allUrl + "::message::" + p_responseJson["message"].dump());
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

	//-------------------self
	nlohmann::json summonerData;
	uint64_t myAccountId;
	if (httpAuthSend("/lol-summoner/v1/current-summoner", summonerData))
	{
		if (summonerData.contains("accountId")) {
			myAccountId = summonerData["accountId"];
		}
	
		//myAccountId = summonerData["accountId"].get<uint64_t>();
	}




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

	//-------------------gameID,gamemode,phase(��Ϸ����ʱ,����׼��,����ʱ��������)
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


	// -------------------�������� ��ҳ

	nlohmann::json	matchesData;
	std::string		endOfGameResult;
	uint64_t		participantId_u64;
	uint64_t matchAccountId_u64;
	uint64_t matchGameId_u64;

	nlohmann::json jsonBody;
	nlohmann::json data;


	bool last_is_end = false;
	if (httpAuthSend("/lol-match-history/v1/products/lol/current-summoner/matches", matchesData, "?begIndex=0&endIndex=0"))
	{
		matchAccountId_u64 = matchesData["accountId"].get<uint64_t>();
		data["member"] = nlohmann::json::array();

		//jsonBody
		for (const auto& game : matchesData["games"]["games"]) {
			matchGameId_u64 = game["gameId"];
			if (HISTORY_GAMES[matchGameId_u64] == myAccountId && "GameComplete" == game["endOfGameResult"])
			{
				for (const auto& participantIdentitie : game["participantIdentities"]) {
					nlohmann::json member;
					uint64_t id;
					id = participantIdentitie["player"]["summonerId"].get<uint64_t>();
					member["id"] = std::to_string(id);
					member["role"] = "other";
					// ��� participantId �Ƿ�ƥ��
					if (id == matchAccountId_u64) {
						participantId_u64 = participantIdentitie["participantId"].get<uint64_t>();
						member["role"] = "self";
						//continue;
					}
					bool isContains = (std::find(lobbySummonerIds.begin(), lobbySummonerIds.end(), member["id"]) != lobbySummonerIds.end());
					if (isContains)
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
				last_is_end = true;
				g_mtx.lock();
				//is_lol_running = true;
				is_lol_game_running = false;
				g_mtx.unlock();
			}
			else {
				
			}

		}
	}


	// -------------------ranked data
	nlohmann::json	ranksData;
	std::string		highestTier;
	if (httpAuthSend("/lol-ranked/v1/current-ranked-stats", ranksData, ""))
	{
		highestTier = ranksData["highestRankedEntrySR"]["highestTier"];
	}

	std::string region = LOL_regionMap.count(rso_original_platform_id) ?
		LOL_regionMap[rso_original_platform_id] :
		"Unknown";

	if (teamSize != 0)
	{
		jsonBody["team_size"] = LOL_teamSizeMap[static_cast<int>(teamSize)];  //�Ծ���û��
	}
	if (LOL_gameModMap[game_mode] !="")
	{
	jsonBody["game_mode"] = LOL_gameModMap[game_mode];					//�Ծ���û��
	}

	if (gameId != 0) {
		jsonBody["game_uuid"] = std::to_string(gameId);							//�Ծ�����
	}
	jsonBody["name"] = "LOL";													//�̶�
	jsonBody["user_game_rank"] = LOL_rankAPIMap[highestTier];				//һֱ����?
	jsonBody["type"] = sendType;
	jsonBody["data"] = data;												//������ƥ��gameid��ѯ
	jsonBody["remark"] = phase;
	g_mtx.lock();
	jsonBody["computer_no"] = g_hostName;	
	g_infoBefore.update(jsonBody);//�Ծ�����	
	if (sendType == "update") {
		//LOG_IMMEDIATE(g_infoBefore.dump(4));
		//g_infoBefore = jsonBody;
	}

	else if (sendType == "END" && last_is_end) {
		LOG_IMMEDIATE(" Ӣ�����˶Ծֽ���?/ ����? / �ؿ�?");
		g_infoBefore["event_id"] = "end";
		_sendHttp_LOL(g_infoBefore);
	}
	g_mtx.unlock();
}


bool Game_Before::before_main(std::string sendType) {
	if (Game_Before::getParam()) {
		Game_Before::getAndSendInfo(sendType);
		return true;
	}
	return false;
}