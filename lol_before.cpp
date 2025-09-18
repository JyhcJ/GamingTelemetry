#include "pch.h"
#include "lol_before.h"
#include "constant.h"
#include "lol.h"
#include <unordered_set>
#include "common.h"
#include "nloJson.h"

// 外部变量声明
extern std::string g_hostName;
extern bool is_lol_running;
extern bool is_lol_game_running;
extern std::unordered_set<int> processed_event_ids;
extern int g_multkill;
extern int g_deaths;
extern bool g_is_chaoshen;

// 全局变量
nlohmann::json g_infoBefore;
std::unordered_set<uint64_t> g_lobbySummonerIds;
std::mutex g_mtx;
std::mutex g_m;
std::mutex g_mtx_cv;
std::condition_variable g_cv;

// 历史游戏记录：gameId -> userId
static std::map<size_t, size_t> HISTORY_GAMES;

// ==================== 工具函数 ====================

/**
 * @brief 读取命令行输出结果
 */
std::string Game_Before::getUserPass(const std::wstring& command) {
    std::wstring tempFile = _TEMPFILE;
    std::wstring cmdLine = L"/c " + command + L" > \"" + tempFile + L"\"";

    SHELLEXECUTEINFOW sei = { 0 };
    sei.cbSize = sizeof(sei);
    sei.fMask = SEE_MASK_NOCLOSEPROCESS;
    sei.lpVerb = L"runas";  // 管理员权限
    sei.lpFile = L"cmd.exe";
    sei.lpParameters = cmdLine.c_str();
    sei.nShow = SW_HIDE;

    if (!ShellExecuteExW(&sei)) {
        return "Error: Failed to launch process with admin rights.";
    }

    // 等待命令执行完成
    WaitForSingleObject(sei.hProcess, INFINITE);
    CloseHandle(sei.hProcess);

    return ReadTxtFileForceUtf8(L"C:\\output.txt");
}

/**
 * @brief 发送HTTP认证请求
 */
bool Game_Before::httpAuthSend(const std::string& endUrl, nlohmann::json& responseJson,  std::string param) {
    std::string auth = "riot:" + auth_token;
    std::map<std::wstring, std::wstring> headers = {
        {L"Authorization", L"Basic " + string2wstring(base64_encode(auth))},
        {L"Content-Type", L"application/json"}
    };

    std::string fullUrl = url + endUrl;

    try {
        std::string response = http.SendRequest(
            string2wstring(fullUrl),
            L"GET",
            headers,
            "",
            true,
            string2wstring(param)
        );

        nlohmann::json p_responseJson = nlohmann::json::parse(response);

        // 检查错误码
        if (p_responseJson.contains("errorCode")) {
            LOG_IMMEDIATE_DEBUG(fullUrl + "::errorCode::" + p_responseJson["errorCode"].dump());
            if (p_responseJson.contains("message")) {
                LOG_IMMEDIATE_DEBUG(fullUrl + "::message::" + p_responseJson["message"].dump());
            }
            responseJson = p_responseJson;
            return false;
        }

        responseJson = p_responseJson;
        return true;
    }
    catch (const nlohmann::json::parse_error& e) {
        LOG_IMMEDIATE_DEBUG("JSON解析失败: " + std::string(e.what()));
        return false;
    }
    catch (const std::exception& e) {
        LOG_IMMEDIATE_ERROR("httpAuthSend异常: " + std::string(e.what()));
        return false;
    }
    catch (...) {
        LOG_IMMEDIATE_ERROR("httpAuthSend未知异常");
        return false;
    }
}

// ==================== 主要功能函数 ====================

/**
 * @brief 获取认证参数
 */
bool Game_Before::getParam() {
    try {
        std::lock_guard<std::mutex> lock(g_m);

        std::string str = getUserPass(_AUTHCOM);
        if (str.size() < 100) {
            LOG_IMMEDIATE("客户端未启动? 等待客户端完全启动");
            return false;
        }

        // 提取各项参数
        app_port = extractParamValue(str, _APPPORT);
        auth_token = extractParamValue(str, _AUTHTOKEN);
        rso_platform_id = extractParamValue(str, _RSOPLATFORM);
        rso_original_platform_id = extractParamValue(str, _RSO_ORIPLATFORM);

        if (rso_original_platform_id.empty()) {
            rso_original_platform_id = extractParamValue(str, _RSOPLATFORMID);
        }

        url = "https://riot:" + auth_token + "@127.0.0.1:" + app_port;
        return true;
    }
    catch (const std::exception& e) {
        LOG_EXCEPTION_WITH_STACK(e);
        return false;
    }
    catch (...) {
        LOG_IMMEDIATE("getParam未知错误");
        return false;
    }
}

/**
 * @brief 处理TFT游戏结束数据
 */
bool Game_Before::processTftEndGameData(nlohmann::json& data, const uint64_t& myAccountId) {
    nlohmann::json matchesData;

    // 轮询获取TFT结束游戏数据
    for (int retry = 0; retry < 10; retry++) {
        std::this_thread::sleep_for(std::chrono::seconds(3));

        if (!httpAuthSend("/lol-end-of-game/v1/tft-eog-stats", matchesData)) {
            continue;
        }

        if (matchesData.contains("errorCode") && matchesData["errorCode"] == "RPC_ERROR") {
            continue;
        }

        // 解析游戏数据
        int health = getNestedValue<int>(matchesData, { "localPlayer", "health" }, -1);
        int rank = getNestedValue<int>(matchesData, { "localPlayer", "rank" }, -1);
        int gameTime = getNestedValue<int>(matchesData, { "gameLength" }, -1);
        nlohmann::json participants = getNestedValue(matchesData, { "players" }, nlohmann::json());

        // 填充数据
        data["ren_tou_shu"] = 0;
        data["win"] = (rank > 4) ? 0 : 1;
        data["time"] = gameTime;

        // 处理参与者信息
        for (auto& participant : participants) {
            nlohmann::json member;
            uint64_t summonerId = getNestedValue<uint64_t>(participant, { "summonerId" }, -1);

            // 确定角色类型
            std::string role = "other";
            if (summonerId == myAccountId) {
                role = "self";
            }
            else if (g_lobbySummonerIds.find(summonerId) != g_lobbySummonerIds.end()) {
                role = "team";
            }

            member["id"] = std::to_string(summonerId);
            member["role"] = role;
            data["member"].push_back(member);
        }

        return true;
    }

    return false;
}

/**
 * @brief 处理普通游戏结束数据
 */
bool Game_Before::processNormalEndGameData(nlohmann::json& data, uint64_t myAccountId) {
    nlohmann::json matchesData;

    // 轮询获取结束游戏数据
    for (int retry = 0; retry < 10; retry++) {
        std::this_thread::sleep_for(std::chrono::seconds(3));

        if (!httpAuthSend("/lol-end-of-game/v1/eog-stats-block", matchesData)) {
            continue;
        }

        if (matchesData.contains("errorCode") && matchesData["errorCode"] == "RPC_ERROR") {
            continue;
        }

        // 解析玩家数据
        uint64_t matchAccountId = getNestedValue<uint64_t>(matchesData, { "localPlayer", "summonerId" }, 0);
        size_t matchGameId = getNestedValue<size_t>(matchesData, { "gameId" }, 0);

        // 验证游戏ID匹配
        if (HISTORY_GAMES[matchGameId] != myAccountId) {
            continue;
        }

        data["member"] = nlohmann::json::array();

        // 处理队伍玩家
        for (const auto& team : matchesData["teams"]) {
            for (const auto& player : team["players"]) {
                nlohmann::json member;
                uint64_t playerId = getNestedValue<uint64_t>(player, { "summonerId" }, 0);

                // 确定角色类型
                std::string role = "other";
                if (playerId == matchAccountId) {
                    role = "self";
                }
                else if (g_lobbySummonerIds.find(playerId) != g_lobbySummonerIds.end()) {
                    role = "team";
                }

                member["id"] = std::to_string(playerId);
                member["role"] = role;
                data["member"].push_back(member);
            }
        }

        // 填充统计数据
        data["ren_tou_shu"] = getNestedValue<int>(matchesData, { "localPlayer", "stats", "CHAMPIONS_KILLED" }, 0);
        data["zhu_gong_shu"] = getNestedValue<int>(matchesData, { "localPlayer", "stats", "ASSISTS" }, 0);
        data["bu_bing_shu"] = getNestedValue<int>(matchesData, { "localPlayer", "stats", "MINIONS_KILLED" }, 0);
        data["pai_yan_shu"] = getNestedValue<int>(matchesData, { "localPlayer", "stats", "WARD_KILLED" }, 0);
        data["win"] = getNestedValue<bool>(matchesData, { "localPlayer", "stats", "WIN" }, false) ? 1 : 0;
        data["time"] = getNestedValue<int>(matchesData, { "gameLength" }, 0);

        return true;
    }

    return false;
}

/**
 * @brief 获取并发送游戏信息
 */
void Game_Before::getAndSendInfo(const std::string& sendType, const std::string& uuid) {
    try {
        // 处理简单的RUN/KILL类型
        if (sendType == "RUN" || sendType == "KILL") {
            nlohmann::json jsonbody;
            jsonbody[sendType] = sendType;
            _sendHttp_LOL(jsonbody);
            return;
        }

        // ==================== 获取基础信息 ====================

        // 获取召唤师信息
        nlohmann::json summonerData;
        uint64_t myAccountId = 0;
        std::string puuid;

        if (httpAuthSend("/lol-summoner/v1/current-summoner", summonerData)) {
            myAccountId = getNestedValue<uint64_t>(summonerData, { "accountId" }, 0);
            puuid = getNestedValue<std::string>(summonerData, { "puuid" }, "");
        }

        // 获取大厅信息
        nlohmann::json lobbyData;
        size_t teamSize = 0;

        if (httpAuthSend("/lol-lobby/v2/lobby", lobbyData)) {
            std::unordered_set<uint64_t> lobbySummonerIds;
            teamSize = lobbyData["members"].size();

            if (teamSize >= 1) {
                for (const auto& member : lobbyData["members"]) {
                    lobbySummonerIds.insert(getNestedValue<uint64_t>(member, { "summonerId" }, 0));
                }

                {
                    std::lock_guard<std::mutex> lock(g_mtx);
                    g_lobbySummonerIds = lobbySummonerIds;
                }

                LOG_IMMEDIATE("Summoner IDs:");
                for (uint64_t id : lobbySummonerIds) {
                    LOG_IMMEDIATE(std::to_string(id));
                }
            }
            else {
                teamSize = 1;
            }
        }

        // 获取排位数据
        nlohmann::json ranksData;
        std::string highestTierSR;
        std::array<std::string, 7> ranks = { "error" };

        if (httpAuthSend("/lol-ranked/v1/current-ranked-stats", ranksData)) {
            highestTierSR = getNestedValue<std::string>(ranksData, { "highestRankedEntrySR", "highestTier" }, "error");

            ranks[1] = getNestedValue<std::string>(ranksData, { "queueMap", "RANKED_FLEX_SR", "tier" }, "error");
            ranks[2] = getNestedValue<std::string>(ranksData, { "queueMap", "RANKED_SOLO_5x5", "tier" }, "error");
            ranks[3] = getNestedValue<std::string>(ranksData, { "queueMap", "RANKED_TFT", "tier" }, "error");
            ranks[4] = getNestedValue<std::string>(ranksData, { "queueMap", "RANKED_TFT_DOUBLE_UP", "tier" }, "error");
            ranks[5] = getNestedValue<std::string>(ranksData, { "queueMap", "RANKED_TFT_TURBO", "tier" }, "error");
        }

        // 获取游戏会话信息
        nlohmann::json sessionData;
        size_t gameId = 0;
        std::string game_mode = "init";
        std::string phase = "init";
        std::string gameType = "init";

        if (httpAuthSend("/lol-gameflow/v1/session", sessionData)) {
            gameId = getNestedValue<size_t>(sessionData, { "gameData", "gameId" }, 0);
            game_mode = getNestedValue<std::string>(sessionData, { "gameData", "queue", "type" }, "init");
            phase = getNestedValue<std::string>(sessionData, { "phase" }, "init");
            gameType = getNestedValue<std::string>(sessionData, { "gameData", "queue", "gameMode" }, "init");
        }

        if (gameId != 0) {
            HISTORY_GAMES[gameId] = myAccountId;
        }

        // ==================== 处理游戏数据 ====================

        nlohmann::json data;
        bool isEndDataProcessed = false;

        if (sendType == "END") {
            if (gameType == "TFT") {
                isEndDataProcessed = processTftEndGameData(data, myAccountId);
            }
            else {
                isEndDataProcessed = processNormalEndGameData(data, myAccountId);
            }
        }

        // ==================== 构建发送数据 ====================

        std::string region = LOL_regionMap.count(rso_original_platform_id) ?
            LOL_regionMap[rso_original_platform_id] : "Unknown";

        nlohmann::json jsonBody;
        jsonBody["name"] = "LOL";
        jsonBody["type"] = sendType;
        jsonBody["computer_no"] = g_hostName;
        jsonBody["remark"] = gameType;

        if (gameId != 0) {
            jsonBody["game_uuid"] = g_hostName + "yhc" + std::to_string(gameId);
        }

        if (teamSize != 0) {
            jsonBody["team_size"] = LOL_teamSizeMap[static_cast<int>(teamSize)];
        }

        if (LOL_gameModMap.count(game_mode)) {
            jsonBody["game_mode"] = LOL_gameModMap[game_mode];
        }

        std::string judeRank = ranks[LOL_gameMod2RankIndexMap[game_mode]];
        if (LOL_rankAPIMap.count(judeRank)) {
            jsonBody["user_game_rank"] = LOL_rankAPIMap[judeRank];
        }

        jsonBody["data"] = data;

        // ==================== 处理发送逻辑 ====================

        {
            std::lock_guard<std::mutex> lock(g_mtx);

            if (gameType != "TFT") {
                g_infoBefore.update(jsonBody);
            }

            if (sendType == "END" && isEndDataProcessed) {
                LOG_IMMEDIATE("英雄联盟对局结束/掉线/重开");

                g_infoBefore["event_id"] = "end";
                jsonBody["event_id"] = "end";

                if (gameType == "TFT") {
                    _sendHttp_LOL(jsonBody);
                }
                else {
                    _sendHttp_LOL(g_infoBefore);
                }

                // 重置全局状态
                processed_event_ids.clear();
                g_multkill = 0;
                g_deaths = 0;
                g_is_chaoshen = false;
            }
        }
    }
    catch (const std::exception& e) {
        LOG_EXCEPTION_WITH_STACK(e);
    }
    catch (...) {
        LOG_IMMEDIATE("getAndSendInfo未知错误");
    }
}

/**
 * @brief 主执行函数
 */
bool Game_Before::before_main(const std::string& sendType,  std::string uuid) {
    if (getParam()) {
        getAndSendInfo(sendType, uuid);
        return true;
    }
    return false;
}