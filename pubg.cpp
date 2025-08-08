#include "pubg.h"
#include "HttpClient.h"
#include "ThreadSafeLogger.h"

TimedTaskQueue::TimedTaskQueue() : running_(true) {
    worker_thread_ = std::thread(&TimedTaskQueue::monitor_loop, this);
}

TimedTaskQueue::~TimedTaskQueue() {
    stop();
}

void TimedTaskQueue::add_task(Task task) {
    std::unique_lock<std::mutex> lock(mutex_);
    tasks_.push(std::move(task));
}

void TimedTaskQueue::start() {
    if (!worker_thread_.joinable()) {
        worker_thread_ = std::thread(&TimedTaskQueue::monitor_loop, this);
    }
}

void TimedTaskQueue::stop() {
    {
        std::unique_lock<std::mutex> lock(mutex_);
        running_ = false;
    }
    cv_.notify_all();
    if (worker_thread_.joinable()) {
        worker_thread_.join();
    }
}

void TimedTaskQueue::monitor_loop() {
    while (running_) {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait_for(lock, std::chrono::minutes(1), [this] {
            return !tasks_.empty() || !running_;
            });

        if (!running_) break;

        while (!tasks_.empty()) {
            Task task = std::move(tasks_.front());
            tasks_.pop();
            lock.unlock();
            task();
            lock.lock();
        }
    }
}

ProcessMonitor_PUBG::ProcessMonitor_PUBG()
    : running_(false),
    request_count_(0),
    last_request_time_(std::chrono::steady_clock::now()),
    last_periodic_time_(std::chrono::steady_clock::now()) {
}

ProcessMonitor_PUBG::~ProcessMonitor_PUBG() {
    stop();
}

void ProcessMonitor_PUBG::start() {
    running_ = true;
    monitor_thread_ = std::thread(&ProcessMonitor_PUBG::monitor_loop, this);
    monitor_thread_.detach();
}

void ProcessMonitor_PUBG::stop() {
    running_ = false;

    {
        std::lock_guard<std::mutex> lock(task_mutex_);
        task_cond_.notify_all();
    }
    {
        std::lock_guard<std::mutex> lock(periodic_mutex_);
        periodic_cond_.notify_all();
    }

    if (monitor_thread_.joinable()) monitor_thread_.join();
    if (request_thread_.joinable()) request_thread_.join();
    if (periodic_thread_.joinable()) periodic_thread_.join();
}

void ProcessMonitor_PUBG::_sendHttp_pubg(nlohmann::json jsonBody) {
    HttpClient http;
    LOG_IMMEDIATE(jsonBody.dump());

    try {
        std::string response = http.SendRequest(
            get_g_domain() + L"/api/client/JuediqiushengPostGameData",
            L"POST",
            getHeader(),
            jsonBody.dump()
        );

        LOG_IMMEDIATE("pubg:Response: " + UTF8ToGBK(response));
    }
    catch (const std::exception& e) {
        LOG_IMMEDIATE_ERROR("_sendHttp_pubg:::");
        LOG_IMMEDIATE_ERROR(e.what());
    }
    catch (...) {
        LOG_IMMEDIATE_ERROR("_sendHttp_pubg :::Unknown exception occurred");
    }
}

void ProcessMonitor_PUBG::_sendHttp_pubg(std::string type, nlohmann::json data) {
    nlohmann::json jsonBody;
    jsonBody["type"] = type;
    _sendHttp_pubg(jsonBody);
}

bool ProcessMonitor_PUBG::readPlayerNameFromFile(const std::string& filePath, std::string& playerName, std::string& matchId) {
    try {
        std::ifstream file(filePath, std::ios::binary);
        if (!file || !file.is_open()) {
            std::cerr << "Failed to open player name file: " << filePath << std::endl;
            return false;
        }

        file.seekg(0, std::ios::end);
        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);

        std::vector<char> buffer(size);
        if (!file.read(buffer.data(), size)) {
            std::cerr << "读取失败" << std::endl;
            return false;
        }

        const size_t BINARY_HEADER_SIZE = 4;
        if (size > BINARY_HEADER_SIZE) {
            buffer.erase(buffer.begin(), buffer.begin() + BINARY_HEADER_SIZE);
        }

        std::string text(buffer.begin(), buffer.end());
        nlohmann::json content = nlohmann::json::parse(text);
        std::string friendlyName = getNestedValue<std::string>(content, { "FriendlyName" }, "error");
        playerName = getNestedValue<std::string>(content, { "RecordUserNickName" }, "error");

        size_t lastDotPos = friendlyName.rfind('.');
        if (lastDotPos == std::string::npos) {
            LOG_IMMEDIATE("玩家退出对局后没有找到pubg_matchID");
            return false;
        }
        matchId = friendlyName.substr(lastDotPos + 1);

        return true;
    }
    catch (const std::exception& e) {
        LOG_IMMEDIATE_ERROR("pubg::readPlayerNameFromFile::" + std::string(e.what()));
        return false;
    }
    catch (...) {
        LOG_IMMEDIATE_ERROR("pubg::readPlayerNameFromFile::Unknown exception occurred while reading player name from file");
        return false;
    }
}

void ProcessMonitor_PUBG::monitor_loop() {
    while (running_) {
        try {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            isPUBG_Running = check_process_exists();

            if (lastState == true && isPUBG_Running == false) {
                lastState = false;
                player_name = "";
                _sendHttp_pubg("KILL", "");
                LOG_IMMEDIATE("pubg游戏退出");
            }

            if (lastState == false && isPUBG_Running) {
                lastState = true;
                _sendHttp_pubg("RUN", "");
                std::thread([this]() {
                    std::unordered_set<std::string> recent_folders;
                    std::string compUserName = GetEnvSafe("USERPROFILE");
                    const std::string reportFolder = compUserName + "\\AppData\\Local\\TslGame\\Saved\\Demos\\";
                    while (check_process_exists()) {
                        std::this_thread::sleep_for(std::chrono::seconds(1));
                        recent_folders = get_recent_folders(reportFolder, 9);

                        if (!recent_folders.empty()) {
                            std::string path = *recent_folders.begin();
                            if (matchIds.find(path) != matchIds.end()) {
                                LOG_IMMEDIATE_DEBUG("pubg matchID已经存在了. 跳过startMonitor");
                                continue;
                            }
                            LOG_IMMEDIATE("pubg:玩家退出了对局");
                            matchIds.insert(recent_folders.begin(), recent_folders.end());

                            std::string name;
                            const std::string newFile = path + "\\PUBG.replayinfo";
                            std::string match_id;
                            bool isGetName = readPlayerNameFromFile(newFile, name, match_id);

                            if (isGetName) {
                                std::thread([this, name, match_id]() { process_request(name, match_id); }).detach();
                            }
                        }
                    }
                    }).detach();
            }
        }
        catch (const std::exception& e) {
            LOG_IMMEDIATE("Exception in monitor_loop" + std::string(e.what()));
        }
        catch (...) {
        }
    }
}

bool ProcessMonitor_PUBG::process_request(const std::string& player_name, const std::string& match_id) {
    try {
        getDetailInfo(player_name, match_id);
        return true;
    }
    catch (const std::exception& e) {
        return false;
        LOG_IMMEDIATE("Exception processing request: " + std::string(e.what()));
    }
}

void ProcessMonitor_PUBG::add_request_task(Task&& task) {
    request_tasks_.add_task(std::move(task));
}

std::string ProcessMonitor_PUBG::generate_monitoring_data() {
    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    return "MonitorData_" + std::to_string(now_time_t);
}

bool ProcessMonitor_PUBG::check_process_exists() {
    try {
        return IsProcessRunning(L"TslGame.exe");
    }
    catch (const std::exception& e) {
        LOG_IMMEDIATE("Exception in check_process_exists" + std::string(e.what()));
        return false;
    }
}

std::vector<std::string> ProcessMonitor_PUBG::getIdForName(const std::string& player_name) {
    try {
        PUBGAPI pubgApi(_PUBG_APIKEY);
        std::string playerInfo = pubgApi.getPlayerInfo("https://api.pubg.com/shards/steam/players?filter[playerNames]=", player_name);
        nlohmann::json jsonResponse;
        if (playerInfo == "") {
            return std::vector<std::string>();
        }

        jsonResponse = nlohmann::json::parse(playerInfo);
        std::vector<std::string> matchIds;

        json::array_t matches;
        if (jsonResponse.contains("data") && !jsonResponse["data"].empty()) {
            auto& playerData = jsonResponse["data"][0];

            if (playerData.contains("relationships") &&
                playerData["relationships"].contains("matches") &&
                playerData["relationships"]["matches"].contains("data")) {

                json::array_t matches = playerData["relationships"]["matches"]["data"];
                for (const auto& match : matches) {
                    std::string id = getNestedValue<std::string>(match, { "id" }, "");
                    if (!id.empty()) {
                        matchIds.push_back(id);
                    }
                }
            }
        }

        for (const std::string& id : initMatchIds) {
            if (id == "init") {
                initMatchIds = matchIds;
            }
        }

        matchIds.push_back("901ee802-af98-4dbf-abd1-582e7dd4cc23");
        std::vector<std::string> symmetric_diff;

        std::sort(matchIds.begin(), matchIds.end());
        std::sort(initMatchIds.begin(), initMatchIds.end());

        std::set_symmetric_difference(
            matchIds.begin(), matchIds.end(),
            initMatchIds.begin(), initMatchIds.end(),
            std::back_inserter(symmetric_diff)
        );

        for (const auto& s : symmetric_diff) {
            LOG_IMMEDIATE("pubg新的对局id:" + s);
            initMatchIds.emplace_back(s);
        }

        return symmetric_diff;
    }
    catch (const std::exception& e) {
        LOG_IMMEDIATE("Exception in make_first_request" + std::string(e.what()));
        throw;
    }
}

bool ProcessMonitor_PUBG::getDetailInfo(std::string player_name, const std::vector<std::string>& newIDS) {
    try {
        PUBGAPI pubgApi(_PUBG_APIKEY);
        for (const std::string& newID : newIDS) {
            std::string matchInfo = pubgApi.getPlayerInfo("https://api.pubg.com/shards/steam/matches/", newID);
            nlohmann::json jsonResponse = nlohmann::json::parse(matchInfo);
            nlohmann::json jsonBody;
            std::string id = getNestedValue<std::string>(jsonResponse, { "data","id" }, "");
            std::string uuid = GenerateUUID();
            jsonBody["game_uuid"] = uuid;
            jsonBody["event_id"] = uuid;
            jsonBody["computer_no"] = getComputerName();
            jsonBody["name"] = "";
            jsonBody["game_mode"] = PUBG_modeMap[getNestedValue<std::string>(jsonResponse, { "data","attributes","matchType" }, "error")];
            jsonBody["team_size"] = PUBG_teamSize[getNestedValue<std::string>(jsonResponse, { "data","attributes","gameMode" }, "error")];
            jsonBody["user_game_rank"] = "";
            jsonBody["type"] = "END";
            jsonBody["remark"] = getNestedValue<std::string>(jsonResponse, { "data","id" }, "error");

            nlohmann::json data;

            for (const auto& obj : jsonResponse["included"]) {
                std::string name = getNestedValue<std::string>(obj, { "attributes","stats","name" }, "error");
                if (name == player_name) {
                    data["ren_tou_shu"] = getNestedValue<int>(obj, { "attributes","stats","kills" }, -1);
                    data["time"] = getNestedValue<int>(obj, { "attributes","stats","timeSurvived" }, -1);
                    int rank = getNestedValue<int>(obj, { "attributes","stats","winPlace" }, -1);
                    data["rank"] = rank;
                    data["win"] = rank == 1 ? 1 : 0;
                }
            }
            jsonBody["data"] = data;
            nlohmann::json member;
            member["role"] = "self";
            member["id"] = player_name;
            jsonBody["data"]["member"].push_back(member);
            _sendHttp_pubg(jsonBody);
        }

        return true;
    }
    catch (const std::exception& e) {
        LOG_IMMEDIATE("Exception in make_second_request" + std::string(e.what()));
        return false;
    }
}

bool ProcessMonitor_PUBG::getDetailInfo(std::string player_name, const std::string& newID) {
    try {
        PUBGAPI pubgApi(_PUBG_APIKEY);
        while (true) {
            std::string matchInfo = pubgApi.getPlayerInfo("https://api.pubg.com/shards/steam/matches/", newID);
            if (matchInfo == "") {
                std::this_thread::sleep_for(std::chrono::seconds(10));
                continue;
            }

            nlohmann::json jsonResponse = nlohmann::json::parse(matchInfo);

            if (jsonResponse.contains("errors")) {
                std::this_thread::sleep_for(std::chrono::seconds(15));
                LOG_IMMEDIATE_DEBUG("等待对局结束" + jsonResponse.dump() + player_name + ";;;" + newID);
                continue;
            }

            nlohmann::json jsonBody;
            std::string id = getNestedValue<std::string>(jsonResponse, { "data","id" }, "");
            std::string uuid = GenerateUUID();
            jsonBody["game_uuid"] = uuid;
            jsonBody["event_id"] = uuid;
            jsonBody["computer_no"] = getComputerName();
            jsonBody["name"] = "";
            jsonBody["game_mode"] = PUBG_modeMap[getNestedValue<std::string>(jsonResponse, { "data","attributes","matchType" }, "error")];
            jsonBody["team_size"] = PUBG_teamSize[getNestedValue<std::string>(jsonResponse, { "data","attributes","gameMode" }, "error")];
            jsonBody["user_game_rank"] = "";
            jsonBody["type"] = "END";
            jsonBody["remark"] = getNestedValue<std::string>(jsonResponse, { "data","id" }, "error");

            nlohmann::json data;

            for (const auto& obj : jsonResponse["included"]) {
                std::string name = getNestedValue<std::string>(obj, { "attributes","stats","name" }, "error");
                if (name == player_name) {
                    data["ren_tou_shu"] = getNestedValue<int>(obj, { "attributes","stats","kills" }, -1);
                    data["time"] = getNestedValue<int>(obj, { "attributes","stats","timeSurvived" }, -1);
                    int rank = getNestedValue<int>(obj, { "attributes","stats","winPlace" }, -1);
                    data["rank"] = rank;
                    data["win"] = rank == 1 ? 1 : 0;
                }
            }
            jsonBody["data"] = data;
            nlohmann::json member;
            member["role"] = "self";
            member["id"] = player_name;
            jsonBody["data"]["member"].push_back(member);
            _sendHttp_pubg(jsonBody);

            return true;
        }
    }
    catch (const std::exception& e) {
        LOG_IMMEDIATE("Exception in getDetailInfo" + std::string(e.what()));
        return false;
    }
    catch (...) {
        LOG_IMMEDIATE("Exception in getDetailInfo ...");
        return false;
    }
}

std::string ProcessMonitor_PUBG::make_periodic_request() {
    try {
        return R"({"status": "periodic", "data": {"timestamp": )" +
            std::to_string(std::chrono::system_clock::to_time_t(std::chrono::system_clock::now())) +
            "}}";
    }
    catch (const std::exception& e) {
        LOG_IMMEDIATE("Exception in make_periodic_request" + std::string(e.what()));
        throw;
    }
}

void ProcessMonitor_PUBG::parse_json_response(const std::string& json_response) {
    try {
    }
    catch (const std::exception& e) {
        LOG_IMMEDIATE("Exception in parse_json_response" + std::string(e.what()));
        throw;
    }
}