#include "pch.h"
#include "lol_before.h"
#include "constant.h"
#include "lol.h"
#include <unordered_set>
#include "common.h"
#include "nloJson.h"

extern std::string g_hostName;
extern bool is_lol_running;
extern bool is_lol_game_running;
extern std::unordered_set<int> processed_event_ids;
extern int g_multkill;
extern int g_deaths;
extern bool g_is_chaoshen;
nlohmann::json g_infoBefore;
std::unordered_set<uint64_t> g_lobbySummonerIds;
std::mutex g_mtx;
std::mutex g_m;


std::mutex g_mtx_cv;
std::condition_variable g_cv;

static std::map<size_t, size_t> HISTORY_GAMES;//gameid _ userid

bool Game_Before::getParam() {
	try {
		std::lock_guard<std::mutex> lock(g_m); // 自动加锁/解锁
		std::string str = getUserPass(_AUTHCOM);
		if (str.size() < 100)
		{
			LOG_IMMEDIATE("客户端未启动? 等待客户端完全启动");
			return false;
		}
		app_port = extractParamValue(str, _APPPORT);
		auth_token = extractParamValue(str, _AUTHTOKEN);
		rso_platform_id = extractParamValue(str, _RSOPLATFORM);
		rso_original_platform_id = extractParamValue(str, _RSO_ORIPLATFORM);

		if (rso_original_platform_id == "") {
			rso_original_platform_id = extractParamValue(str, _RSOPLATFORMID);
		}
		//std::cout << "auth_token: " << auth_token << std::endl;
		//std::cout << "app_port: " << app_port << std::endl;
		url = "https://riot:" + auth_token + "@127.0.0.1:" + app_port;

		return true;
	}
	catch (const std::exception& e) {
		LOG_EXCEPTION_WITH_STACK(e);
		//LOG_IMMEDIATE("NarakaStateMonitor::OnClientStarted():" + std::string(e.what()));
		return false;
	}
	catch (...) {
		LOG_IMMEDIATE("lol_before.cpp::getParam:未知错误");
		return false;
	}
}

// 需要先get 验证封装
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
		LOG_IMMEDIATE_DEBUG("p_responseJson 解析失败: " + error);
		LOG_IMMEDIATE_DEBUG("p_responseJson : " + p_responseJson);
	}
	catch (const std::exception& e) {
		LOG_IMMEDIATE_ERROR("httpAuthSend:::exception");
		LOG_IMMEDIATE_ERROR(e.what());
	}
	catch (...) {  // 捕获其他所有异常
		LOG_IMMEDIATE_ERROR("httpAuthSend :::Unknown exception occurred");
	}

	// 检查字段是否存在
	if (p_responseJson.contains("errorCode")) {
		//LOG_IMMEDIATE_DEBUG(allUrl + "::errorCode::" + p_responseJson["errorCode"].dump());
		if (p_responseJson.contains("message")) {
			//LOG_IMMEDIATE_DEBUG(allUrl + "::message::" + p_responseJson["message"].dump());
		}
		responseJson = p_responseJson;
		return false;
	}

	responseJson = p_responseJson;
	return true;


}
// 游戏中:SAN_SHA_SHU = 三杀数, SI_SHA_SHU = 四杀数, WU_SHA_SHU = 五杀数, CHAO_SHEN_SHU = 超神数,
// 游戏结束前/后:RUN = 客户端打开, KILL = 客户端关闭 / END = 游戏结束
void Game_Before::getAndSendInfo(std::string sendType, std::string uuid) {
	try {
		if (sendType == "RUN" || sendType == "KILL") {
			nlohmann::json	jsonbody;
			jsonbody[sendType] = sendType;
			_sendHttp_LOL(jsonbody);
			return;
		}


		//-------------------self
		nlohmann::json summonerData;
		uint64_t myAccountId;
		std::string puuid;
		if (httpAuthSend("/lol-summoner/v1/current-summoner", summonerData))
		{
			if (summonerData.contains("accountId")) {
				myAccountId = summonerData["accountId"];
				puuid = summonerData["puuid"];
			}

			//myAccountId = summonerData["accountId"].get<uint64_t>();
		}




		nlohmann::json lobbyData;
		size_t teamSize = 0;

		if (httpAuthSend("/lol-lobby/v2/lobby", lobbyData))
		{
			std::unordered_set<uint64_t> lobbySummonerIds;
			//LOG_IMMEDIATE(lobbyData);
			teamSize = lobbyData["members"].size();
			//myAccountId = summonerData["accountId"].get<uint64_t>();

			//0 2 3 4 5
			if (teamSize >= 1) {
				for (const auto& member : lobbyData["members"]) {
					lobbySummonerIds.insert(member["summonerId"].get<uint64_t>());
				}
				g_mtx.lock();
				g_lobbySummonerIds = lobbySummonerIds;
				g_mtx.unlock();
				//std::cout << "Summoner IDs:" << std::endl;
				LOG_IMMEDIATE("Summoner IDs:");
				for (uint64_t id : lobbySummonerIds) {
					LOG_IMMEDIATE(std::to_string(id));
					//std::cout << "- " << id << std::endl;
				}
			}
			else {
				teamSize = 1;
			}
		}



		// -------------------ranked data
		nlohmann::json	ranksData;
		std::string	highestTierSR; //bobao
		std::string	ranks[7];
		std::string judeRank;
		if (httpAuthSend("/lol-ranked/v1/current-ranked-stats", ranksData))
		{
			//highestTierSR = ranksData["highestRankedEntrySR"]["highestTier"];
			highestTierSR = getNestedValue<std::string>(ranksData, { "highestRankedEntrySR","highestTier" }, "error");

			ranks[0] = "error";
			ranks[1] = getNestedValue<std::string>(ranksData, { "queueMap","RANKED_FLEX_SR","tier" }, "error");
			ranks[2] = getNestedValue<std::string>(ranksData, { "queueMap","RANKED_SOLO_5x5","tier" }, "error");
			ranks[3] = getNestedValue<std::string>(ranksData, { "queueMap","RANKED_TFT","tier" }, "error");
			ranks[4] = getNestedValue<std::string>(ranksData, { "queueMap","RANKED_TFT_DOUBLE_UP","tier" }, "error");
			ranks[5] = getNestedValue<std::string>(ranksData, { "queueMap","RANKED_TFT_TURBO","tier" }, "error");
			ranks[6] = "";




		}

		// -------------------比赛数据 分页

		nlohmann::json	matchesData;
		std::string		endOfGameResult;
		uint64_t		participantId_u64;
		uint64_t matchAccountId_u64;
		uint64_t matchGameId_u64;

		nlohmann::json jsonBody;
		nlohmann::json data;


		bool last_is_end = false;

		//-------------------gameID,gamemode,phase(游戏进行时,或者准备,结算时才有数据)
		nlohmann::json sessionData;
		size_t gameId = 0;
		std::string	game_mode = "init";
		std::string phase = "init";
		std::string type = "init";
		if (httpAuthSend("/lol-gameflow/v1/session", sessionData))
		{
			gameId = sessionData["gameData"]["gameId"]; //此处 是 sj对局中的gameid
			game_mode = sessionData["gameData"]["queue"]["type"];
			phase = sessionData["phase"];
			type = sessionData["gameData"]["queue"]["gameMode"];
		}

		if (gameId != 0) {
			HISTORY_GAMES.emplace(gameId, myAccountId);
		}

		size_t gameId_tft;
		if (type == "TFT") {
			if (sendType == "END") {
				/*		std::unique_lock<std::mutex> lock(g_mtx_cv);
						auto timeout = std::chrono::seconds(60 * 20);

						g_cv.wait_for(lock, timeout, [this,puuid, &matchesData] {*/
				LOG_INFO("等待新的LOLTFT对局出现");

				while (true) {
					if (httpAuthSend("/lol-match-history/v1/products/tft/" + puuid + "/matches", matchesData)) {
						std::vector<PathItem> path = {
						PathItem::makeKey("games"),
						PathItem::makeIndex(0),
						PathItem::makeKey("json"),
						PathItem::makeKey("gameId")
						};
						size_t p_gameId_tft = getNestedValuePlus<size_t>(matchesData, path, -1);

						if (HISTORY_GAMES[p_gameId_tft] != 0) {
							path = {
							PathItem::makeKey("games"),
							PathItem::makeIndex(0),
							PathItem::makeKey("json"),
							PathItem::makeKey("game_datetime") };
							std::string game_datetime = getNestedValuePlus<std::string>(matchesData, path, "-1");
							//	结束时间在25s内
								// 1. 将字符串时间戳转换为 long long (毫秒)
							long long timestamp_ms;
							try {
								timestamp_ms = std::stoll(game_datetime);
							}
							catch (const std::exception& e) {
								std::cerr << "Error converting string to number: " << e.what() << std::endl;
							}

							auto now = std::chrono::system_clock::now();

							auto now_ms_duration = std::chrono::time_point_cast<std::chrono::milliseconds>(now).time_since_epoch();

							long long now_ms = now_ms_duration.count();


							long long diff_ms = std::abs(now_ms - timestamp_ms);

							const long long threshold_ms = 10 * 60 * 1000; // 25 seconds in milliseconds
							bool is_within_25_seconds = (diff_ms <= threshold_ms);
							if (is_within_25_seconds) {
								LOG_INFO("新的LOLTFT对局出现");
								break;
							}

						}

					}
					std::this_thread::sleep_for(std::chrono::milliseconds(5000));
				}


				std::vector<PathItem> path = {
				PathItem::makeKey("games"),
				PathItem::makeIndex(0),
				PathItem::makeKey("json"),
				PathItem::makeKey("gameId")
				};
				gameId_tft = getNestedValuePlus<size_t>(matchesData, path, -1);



				//胜利


				path = {
				PathItem::makeKey("games"),
				PathItem::makeIndex(0),
				PathItem::makeKey("json"),
				PathItem::makeKey("participants")
				};
				nlohmann::json participants = getNestedValuePlus<nlohmann::json>(matchesData, path, nlohmann::json());

				path = {
				PathItem::makeKey("games"),
				PathItem::makeIndex(0),
				PathItem::makeKey("json"),
				PathItem::makeKey("game_length")
				};
				data["time"] = (int)getNestedValuePlus<double>(matchesData, path, -1.0);

				for (auto& participant : participants) {
					nlohmann::json member;
					std::string p_puuid = getNestedValue<std::string>(participant, { "puuid" }, "error");
					if (puuid == p_puuid) {
						bool win = getNestedValue<bool>(participant, { "win" }, false);
						int gold_left = getNestedValue<int>(participant, { "gold_left" }, -1);
						int last_round = getNestedValue<int>(participant, { "last_round" }, -1);
						int placement = getNestedValue<int>(participant, { "placement" }, -1);
						data["ren_tou_shu"] = getNestedValue<int>(participant, { "players_eliminated" }, -1);
						data["win"] = getNestedValue<bool>(participant, { "win" }, false) == false ? 0 : 1;

						if (sendType == "END") {
							member["id"] = std::to_string(myAccountId);
							member["role"] = "self";
							data["member"].push_back(member);

							if (g_lobbySummonerIds.size() > 1)
							{
								for (uint64_t lobby : g_lobbySummonerIds) {
									if (lobby == myAccountId) {
										continue;
									}
									else {
										member["id"] = std::to_string(lobby);
										member["role"] = "team";
										data["member"].push_back(member);
									}
								}
								//g_lobbySummonerIds.clear();
							}
						}
						break;
					}

				}
				last_is_end = true;


				//return true; });

			}



		}
		else {

			if (sendType == "END") {
				std::this_thread::sleep_for(std::chrono::seconds(5));
				// 这个接口本来可用的  不知道后来为什么对局结束后延迟开始长了起来.
				//if (httpAuthSend("/lol-match-history/v1/products/lol/current-summoner/matches", matchesData, "?begIndex=0&endIndex=0"))
				//if (httpAuthSend("/lol-end-of-game/v1/eog-stats-block", matchesData, "?begIndex=0&endIndex=0"))
				//{

				//	if (matchesData.contains("localPlayer")) {
				//		matchAccountId_u64 = matchesData["accountId"].get<uint64_t>();
				//		data["member"] = nlohmann::json::array();
				//		//jsonBody
				//		for (const auto& game : matchesData["games"]["games"]) {

				//			matchGameId_u64 = game["gameId"];
				//			//if (HISTORY_GAMES[matchGameId_u64] == myAccountId && "GameComplete" == game["endOfGameResult"])
				//			if ("GameComplete" == game["endOfGameResult"]) {
				//				/*	if (HISTORY_GAMES[matchGameId_u64] == myAccountId)
				//					{*/
				//					/*		if (sendType == "END") {
				//								LOG_INFO("END 第3个if");
				//							}*/
				//				for (const auto& participantIdentitie : game["participantIdentities"]) {
				//					nlohmann::json member;
				//					uint64_t id;
				//					id = participantIdentitie["player"]["summonerId"].get<uint64_t>();
				//					member["id"] = std::to_string(id);
				//					member["role"] = "other";
				//					//这里有问题 接口只能查询到自己的信息==========
				//		/*			bool isContains = (std::find(lobbySummonerIds.begin(), lobbySummonerIds.end(), member["id"]) != lobbySummonerIds.end());
				//					if (isContains)
				//					{
				//						member["role"] = "team";
				//					}*/
				//					//==========================================
				//					// 检查 participantId 是否匹配
				//					if (id == matchAccountId_u64) {
				//						participantId_u64 = participantIdentitie["participantId"].get<uint64_t>();
				//						member["role"] = "self";
				//						data["member"].push_back(member);
				//						//continue;
				//					}
				//					if (g_lobbySummonerIds.size() > 1)
				//					{
				//						for (uint64_t lobby : g_lobbySummonerIds) {
				//							if (lobby == id) {
				//								continue;
				//							}
				//							else {
				//								member["id"] = std::to_string(lobby);
				//								member["role"] = "team";
				//								data["member"].push_back(member);
				//							}
				//						}
				//						//g_lobbySummonerIds.clear();
				//					}
				//				}
				//				for (const auto& participant : game["participants"]) {
				//					if (participant["participantId"] == participantId_u64) {
				//						data["ren_tou_shu"] = participant["stats"]["kills"];
				//						data["zhu_gong_shu"] = participant["stats"]["assists"];
				//						// neutralMinionsKilled 字段 小兵+野怪.
				//						data["bu_bing_shu"] = participant["stats"]["totalMinionsKilled"];
				//						data["pai_yan_shu"] = participant["stats"]["wardsKilled"];
				//						data["win"] = participant["stats"]["win"] == false ? 0 : 1;
				//						//data["time"] = participant["stats"]["assists"];
				//							//data["time"] = matchesData["games"]["games"]["gameDuration"];
				//						break;
				//					}
				//				}
				//				data["time"] = game["gameDuration"];
				//				last_is_end = true;
				//				g_mtx.lock();
				//				//is_lol_running = true;
				//				//is_lol_game_running = false;  //注释后会出现异常问题吗
				//				g_mtx.unlock();
				//			}
				//			else {
				//			}
				//		}
				//	}
				//}
				if (httpAuthSend("/lol-end-of-game/v1/eog-stats-block", matchesData))
				{
					matchAccountId_u64 = matchesData["localPlayer"]["summonerId"].get<uint64_t>();
					data["member"] = nlohmann::json::array();
					//jsonBody
					for (const auto& team : matchesData["teams"]) {
						matchGameId_u64 = matchesData["gameId"];
						if (HISTORY_GAMES[matchGameId_u64] == myAccountId)
						{
							for (const auto& player : team["players"]) {
								nlohmann::json member;
								uint64_t id;
								id = player["summonerId"].get<uint64_t>();
								member["id"] = std::to_string(id);
								member["role"] = "other";
								// ��� participantId �Ƿ�ƥ��

								bool isContains = (std::find(g_lobbySummonerIds.begin(), g_lobbySummonerIds.end(), id) != g_lobbySummonerIds.end());
								if (isContains)
								{
									member["role"] = "team";
								}
								if (id == matchAccountId_u64) {
									participantId_u64 = player["summonerId"].get<uint64_t>();
									member["role"] = "self";
									//continue;
								}
								data["member"].push_back(member);
							}
							//for (const auto& participant : team["participants"]) {
							if (participantId_u64 == matchAccountId_u64) {
								data["ren_tou_shu"] = matchesData["localPlayer"]["stats"]["CHAMPIONS_KILLED"];
								data["zhu_gong_shu"] = matchesData["localPlayer"]["stats"]["ASSISTS"];
								// NEUTRAL_MINIONS_KILLED �ֶ� С��+Ұ��.
								data["bu_bing_shu"] = matchesData["localPlayer"]["stats"]["MINIONS_KILLED"];
								data["pai_yan_shu"] = matchesData["localPlayer"]["stats"]["WARD_KILLED"];
								data["win"] = matchesData["localPlayer"]["stats"]["WIN"] == false ? 0 : 1;
								//data["time"] = participant["stats"]["assists"];
								data["time"] = matchesData["gameLength"];
								last_is_end = true;
								break;
							}
							//}
							//data["time"] = team["gameDuration"];
					
							g_mtx.lock();
							//is_lol_running = true;
							//is_lol_game_running = false;
							g_mtx.unlock();
						}
						else {
						}
					}
				}



			}





		}


		std::string region = LOL_regionMap.count(rso_original_platform_id) ?
			LOL_regionMap[rso_original_platform_id] :
			"Unknown";

		if (teamSize != 0)
		{
			jsonBody["team_size"] = LOL_teamSizeMap[static_cast<int>(teamSize)];  //对局中没有
		}
		if (LOL_gameModMap[game_mode] != "")
		{
			jsonBody["game_mode"] = LOL_gameModMap[game_mode];					//对局中没有
		}

		if (gameId != 0) {
			jsonBody["game_uuid"] = g_hostName+"yhc" + std::to_string(gameId);							//对局中有
		}
		//LOL_gameMod2RankIndexMap[judeRank]
		judeRank = ranks[LOL_gameMod2RankIndexMap[game_mode]];
		jsonBody["name"] = "LOL";													//固定
		jsonBody["user_game_rank"] = LOL_rankAPIMap[judeRank];				//一直都有?
		jsonBody["type"] = sendType;
		jsonBody["data"] = data;												//结束后匹配gameid查询
		jsonBody["remark"] = type;   //remark
		g_mtx.lock();
		jsonBody["computer_no"] = g_hostName;
		if (type != "TFT") {
			g_infoBefore.update(jsonBody);//对局中有
		}
		if (sendType == "update") {
			//LOG_IMMEDIATE(g_infoBefore.dump(4));
			//g_infoBefore = jsonBody;
		}

		else if (sendType == "END" && last_is_end) {
			LOG_IMMEDIATE(" 英雄联盟对局结束?/ 掉线? / 重开?");
			g_infoBefore["event_id"] = "end";
			jsonBody["event_id"] = "end";
			if (type == "TFT") {
				_sendHttp_LOL(jsonBody);
			}
			else {
				_sendHttp_LOL(g_infoBefore);
			}

			processed_event_ids.clear();
			g_multkill = 0;
			g_deaths = 0;
			g_is_chaoshen = false;

		}
		g_mtx.unlock();
	}




	catch (const std::exception& e) {
		LOG_EXCEPTION_WITH_STACK(e);
		//LOG_IMMEDIATE("NarakaStateMonitor::OnClientStarted():" + std::string(e.what()));
		return;
	}
	catch (...) {
		LOG_IMMEDIATE("lol_before.cpp::getAndSendInfo:未知错误");
		return;
	}
}

std::string Game_Before::getUserPass(const std::wstring& command) {
	std::wstring tempFile = _TEMPFILE;
	std::wstring cmdLine = L"/c " + command + L" > \"" + tempFile + L"\"";

	SHELLEXECUTEINFOW sei = { 0 };
	sei.cbSize = sizeof(sei);
	sei.fMask = SEE_MASK_NOCLOSEPROCESS;
	sei.lpVerb = L"runas";  // 管理员
	sei.lpFile = L"cmd.exe";
	sei.lpParameters = cmdLine.c_str();
	sei.nShow = SW_HIDE;

	if (!ShellExecuteExW(&sei)) {
		return  "Error: Failed to launch process with admin rights.";
	}

	// 等待命令执行完成
	WaitForSingleObject(sei.hProcess, INFINITE);
	CloseHandle(sei.hProcess);

	std::string str = ReadTxtFileForceUtf8(L"C:\\output.txt");

	// 读取输出文件

	return str;
}
bool Game_Before::before_main(std::string sendType, std::string uuid) {
	if (Game_Before::getParam()) {
		Game_Before::getAndSendInfo(sendType, uuid);
		return true;
	}
	return false;
}