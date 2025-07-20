#pragma once
#include "nloJson.h"
#include "constant.h"
#include <iostream>
#include <functional>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <chrono>
#include <atomic>

class TimedTaskQueue {
public:
	using Task = std::function<void()>;

	TimedTaskQueue() : running_(true) {
		worker_thread_ = std::thread(&TimedTaskQueue::monitor_loop, this);
	}

	~TimedTaskQueue() {
		stop();
	}

	// �������֧�� lambda��
	void add_task(Task task) {
		std::unique_lock<std::mutex> lock(mutex_);
		tasks_.push(std::move(task));
	}

	// �����������߳�
	void start() {
		if (!worker_thread_.joinable()) {
			worker_thread_ = std::thread(&TimedTaskQueue::monitor_loop, this);
		}
	}

	// ֹͣ�߳�
	void stop() {
		{
			std::unique_lock<std::mutex> lock(mutex_);
			running_ = false;
		}
		cv_.notify_all();
		if (worker_thread_.joinable()) {
			worker_thread_.join();
		}
	}

private:
	std::queue<Task> tasks_;
	std::mutex mutex_;
	std::condition_variable cv_;
	std::thread worker_thread_;
	std::atomic<bool> running_;

	void monitor_loop() {
		while (running_) {
			std::unique_lock<std::mutex> lock(mutex_);

			// �ȴ� 1 ���ӻ򱻻���
			cv_.wait_for(lock, std::chrono::minutes(1), [this] {
				return !tasks_.empty() || !running_;
				});

			if (!running_) break;

			// ������������
			while (!tasks_.empty()) {
				Task task = std::move(tasks_.front());
				tasks_.pop();
				lock.unlock();
				task(); // ִ������
				lock.lock();
			}
		}
	}
};

class ProcessMonitor_PUBG {
public:
	// �߳̿���
	std::atomic<bool> running_;
	std::thread monitor_thread_;
	std::thread request_thread_;
	std::thread periodic_thread_;

	// ����Ƶ�ʿ���
	std::mutex request_mutex_;
	int request_count_;
	std::chrono::steady_clock::time_point last_request_time_;

	// �����������
	TimedTaskQueue request_tasks_;
	std::mutex task_mutex_;
	std::condition_variable task_cond_;

	// ��ʱ�������
	std::mutex periodic_mutex_;
	std::condition_variable periodic_cond_;
	std::chrono::steady_clock::time_point last_periodic_time_;

	//��ʼ����:
	std::vector<std::string> initMatchIds = { "init" };

	// ����״̬
	bool isPUBG_Running = false;
	bool lastState = false;

	// ��־
	std::mutex log_mutex_;

	using Task = std::function<void()>;

	ProcessMonitor_PUBG()
		: running_(false),
		request_count_(0),
		last_request_time_(std::chrono::steady_clock::now()),
		last_periodic_time_(std::chrono::steady_clock::now()) {
	}

	~ProcessMonitor_PUBG() {
		stop();
	}

	void start() {
		if (running_) return;


		running_ = true;
		// �������������߳�
		monitor_thread_ = std::thread(&ProcessMonitor_PUBG::monitor_loop, this);

		//request_thread_ = std::thread(&ProcessMonitor_PUBG::request_processing_loop, this);
		//ÿ1����ִ��һ�εĶ�ʱ����
		periodic_thread_ = std::thread(&ProcessMonitor_PUBG::periodic_task_loop, this);


		monitor_thread_.detach();
		//request_thread_.detach();
		periodic_thread_.detach();
		/*	while (true) {
				std::this_thread::sleep_for(std::chrono::seconds(1));
			}*/
			//periodic_thread_.detach();

	}

	void stop() {
		running_ = false;

		// �������п��ܵȴ����߳�
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

private:
	void _sendHttp_pubg(nlohmann::json jsonBody) {
		HttpClient http;
		LOG_IMMEDIATE(jsonBody.dump());

		try {
			// 3. ����POST����
			std::string response = http.SendRequest(
				L"https://" + IS_DEBUG + L"asz.cjmofang.com/api/client/JuediqiushengPostGameData",
				L"POST",
				getHeader(),
				jsonBody.dump()
			);

			LOG_IMMEDIATE("Response: " + UTF8ToGBK(response));
		}
		catch (const std::exception& e) {
			LOG_IMMEDIATE_ERROR("_sendHttp_pubg:::");
			LOG_IMMEDIATE_ERROR(e.what());

		}
		catch (...) {  // �������������쳣
			LOG_IMMEDIATE_ERROR("_sendHttp_pubg :::Unknown exception occurred");
		}
	}

	void _sendHttp_pubg(std::string type, nlohmann::json data) {
		nlohmann::json jsonBody;
		jsonBody["type"] = type;
		_sendHttp_pubg(jsonBody);
	}
	// ����߳���ѭ��
	void monitor_loop() {
		//std::string player_name = getPlayerNamePUBG();
		std::string player_name = "ppuubbggBOOMBOOM";

		GENERAL_CONSTRUCTION gc_in = GENERAL_CONSTRUCTION();
		GENERAL_CONSTRUCTION gc_out;

		while (running_) {
			std::this_thread::sleep_for(std::chrono::seconds(1));
			try {
				//isPUBG_Running = check_process_exists();
				isPUBG_Running = true;

				if (lastState == true && isPUBG_Running == false)
				{
					lastState = false;
					_sendHttp_pubg("KILL", "");
					LOG_IMMEDIATE("pubg��Ϸ�˳�");
					////���ݽ�ɫ���Ʋ�matid
					//std::vector<std::string> newIDS = getIdForName(player_name);

					////����matid��ս��
					//std::string second_response = getDetailInfo(player_name, newIDS);
				
				}

				// ��������Ϸ
				if (lastState == false && isPUBG_Running) {
					lastState = true;
					_sendHttp_pubg("RUN", "");
					initMatchIds.clear();
					initMatchIds.emplace_back("init");

					int ret = loadDriver();
					if (ret >= 0) {
						std::this_thread::sleep_for(std::chrono::seconds(1));//���ض���
						//std::this_thread::sleep_for(std::chrono::seconds(20));//���ض���
						if (driverUpdate(gc_in, gc_out)) {
							player_name = driverGetPlayerName(gc_in, gc_out);
						}
					}
					else {
						LOG_IMMEDIATE_ERROR("��������ʧ�ܣ����������Ƿ�װ��ȷ��");
						break;
					}
			


				}
				if (isPUBG_Running) {
					lastState = true;

					if (player_name == "")
					{
						// ����Ҫ���ڴ���
						continue;
					}

					bool isReq = process_request(player_name);

					if (!isReq) {
						// �������Ƶ�����ƣ�������������  (�����߳̿�������???)
						add_request_task([this, player_name] {
							process_request(player_name);
							});
					}

				}

			}
			catch (const std::exception& e) {
				LOG_IMMEDIATE("Exception in monitor_loop" + std::string(e.what()));
			}
			catch (...) {
				//log_error("Unknown exception in monitor_loop");
			}
		}
	}

	//// �������߳���ѭ��
	//void request_processing_loop() {
	//	while (running_) {
	//		std::this_thread::sleep_for(std::chrono::seconds(1));
	//		try {
	//			Task task;
	//			if (request_tasks_.pop(task)) {
	//				task();
	//			}
	//		}
	//		catch (const std::exception& e) {
	//			LOG_IMMEDIATE("Exception in request_processing_loop" + std::string(e.what()));
	//		}
	//		catch (...) {
	//			LOG_IMMEDIATE("Unknown exception in request_processing_loop");
	//		}
	//	}
	//}

	// ��ʱ�����߳���ѭ��
	void periodic_task_loop() {
		while (running_) {
			try {
				// ÿ1����ִ��һ��
				std::unique_lock<std::mutex> lock(periodic_mutex_);
				periodic_cond_.wait_for(lock, std::chrono::minutes(1), [this] {
					return !running_;
					});

				if (running_) {
					execute_periodic_task();
					last_periodic_time_ = std::chrono::steady_clock::now();
				}
			}
			catch (const std::exception& e) {
				LOG_IMMEDIATE("Exception in periodic_task_loop" + std::string(e.what()));
			}
			catch (...) {
				LOG_IMMEDIATE("Unknown exception in periodic_task_loop");
			}
		}
	}

	// ���Դ������󣨷�������
	bool try_process_request(const std::string& data) {
		std::unique_lock<std::mutex> lock(request_mutex_, std::try_to_lock);
		if (!lock.owns_lock()) {
			return false;
		}

		auto now = std::chrono::steady_clock::now();
		auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - last_request_time_).count();

		if (elapsed >= 60) {
			// ����1���ӣ����ü�����
			request_count_ = 0;
			last_request_time_ = now;
		}

		if (request_count_ < 10) {
			request_count_++;
			lock.unlock(); // ��ǰ�ͷ���

			// ʵ�ʴ�������
			process_request(data);
			return true;
		}

		return false;
	}

	// ��������ʵ�ʹ�����
	bool process_request(const std::string& player_name) {
		try {

			std::vector<std::string> matchInfo = getIdForName(player_name);
			if (matchInfo.empty()) {
				return false;
			}

			std::string player_name111 = "L1ke-S";
			//matchInfo.emplace_back("901ee802-af98-4dbf-abd1-582e7dd4cc23");
			// 2. ���ݵ�һ����Ӧ����ڶ�������
			std::string second_response = getDetailInfo(player_name111, matchInfo);

			// 3. �����ڶ�����Ӧ��JSON
			parse_json_response(second_response);

			std::this_thread::sleep_for(std::chrono::seconds(120));

			return true;
		}
		catch (const std::exception& e) {
			return false;
			LOG_IMMEDIATE("Exception processing request: " + std::string(e.what()));
		}
	}

	// ����������񵽶���
	void add_request_task(Task&& task) {
		request_tasks_.add_task(std::move(task));
		//log_message("Request task added to queue (queue size: " +
		//    std::to_string(request_tasks_.size()) + ")";
	}

	// ִ�ж�ʱ����
	void execute_periodic_task() {
		try {
			//log_message("Executing periodic task");

			std::string response = make_periodic_request();
			parse_json_response(response);

		}
		catch (const std::exception& e) {
			LOG_IMMEDIATE("Exception in periodic task" + std::string(e.what()));
		}
	}

	// ���ɼ������
	std::string generate_monitoring_data() {
		auto now = std::chrono::system_clock::now();
		auto now_time_t = std::chrono::system_clock::to_time_t(now);
		return "MonitorData_" + std::to_string(now_time_t);
	}

	bool check_process_exists() {
		try {
			return  IsProcessRunning(L"TslGame.exe");
		}
		catch (const std::exception& e) {
			LOG_IMMEDIATE("Exception in check_process_exists" + std::string(e.what()));
			return false;
		}
	}

	std::vector<std::string> getIdForName(const std::string& player_name) {
		try {
			// ʵ�ֵ�һ��HTTP����
			PUBGAPI pubgApi(_PUBG_APIKEY);
			//		"createdAt": "2025-07-10T02:42:20Z",
			//		"createdAt": "2025-07-09T10:02:46Z",
			// ��ȡ�����Ϣ
			std::string playerInfo = pubgApi.getPlayerInfo("https://api.pubg.com/shards/steam/players?filter[playerNames]=", player_name);
			nlohmann::json jsonResponse;
			if (playerInfo == "") {
				return std::vector<std::string>();
			}


			//getNestedValue<>(jsonResponse, { "data", "relationships","matches"}, );

			jsonResponse = nlohmann::json::parse(playerInfo);
			std::vector<std::string> matchIds;


			/*		auto matches = getNestedValue<json::array_t>(jsonResponse,
						{ "data", "0", "relationships", "matches", "data" },
						json::array()
					);*/
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

			/*		for (const auto& match : matches) {
						std::string id = getNestedValue<std::string>(match, { "id" }, "");
						if (!id.empty()) {
							matchIds.push_back(id);
						}
					}*/

			for (const std::string& id : initMatchIds) {
				if (id == "init") {
					initMatchIds = matchIds;
				}
			}

			//TODO DEL
			matchIds.push_back("901ee802-af98-4dbf-abd1-582e7dd4cc23");
			std::vector<std::string> symmetric_diff;

			std::sort(matchIds.begin(), matchIds.end());
			std::sort(initMatchIds.begin(), initMatchIds.end());

			std::set_symmetric_difference(
				matchIds.begin(), matchIds.end(),
				initMatchIds.begin(), initMatchIds.end(),
				std::back_inserter(symmetric_diff)
			);

			//std::cout << "�ԳƲ:\n";
			for (const auto& s : symmetric_diff) {
				std::cout << "�µĶԾ�id:" << s << "\n";
				initMatchIds.emplace_back(s);
			}
			//https://api.pubg.com/shards/steam/players?filter[playerNames]=ppuubbggBOOMBOOM
				// return response;
			return symmetric_diff;
		}
		catch (const std::exception& e) {
			LOG_IMMEDIATE("Exception in make_first_request" + std::string(e.what()));
			throw;
		}
	}

	std::string getDetailInfo(std::string player_name, const std::vector<std::string>& newIDS) {
		try {
			PUBGAPI pubgApi(_PUBG_APIKEY);
			for (const std::string& newID : newIDS)
			{
				std::string matchInfo = pubgApi.getPlayerInfo("https://api.pubg.com/shards/steam/matches/", newID);
				nlohmann::json jsonResponse = nlohmann::json::parse(matchInfo);



				nlohmann::json jsonBody;
				std::string id = getNestedValue<std::string>(jsonResponse, { "data","id" }, "");
				std::string uuid = GenerateUUID();
				jsonBody["game_uuid"] = uuid;
				jsonBody["event_id"] = uuid;
				jsonBody["computer_no"] = getComputerName();
				jsonBody["name"] = "";
				//TODO ��Ҫ��λ����gameMode
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


			//return R"({"status": "success", "data": {"value": 42}})"; // mock JSON response
			return ""; // mock JSON response
		}
		catch (const std::exception& e) {
			LOG_IMMEDIATE("Exception in make_second_request" + std::string(e.what()));
			throw;
		}
	}

	std::string make_periodic_request() {
		try {
			// ʵ�ֶ�ʱHTTP����
			// std::string response = http_client::get("http://example.com/api/periodic");

			// return response;
			return R"({"status": "periodic", "data": {"timestamp": )" +
				std::to_string(std::chrono::system_clock::to_time_t(std::chrono::system_clock::now())) +
				"}}";
		}
		catch (const std::exception& e) {
			LOG_IMMEDIATE("Exception in make_periodic_request" + std::string(e.what()));
			throw;
		}
	}

	void parse_json_response(const std::string& json_response) {
		try {
			// ʵ��JSON�����߼�
			//log_message("Successfully parsed JSON response: " + json_response);
		}
		catch (const std::exception& e) {
			LOG_IMMEDIATE("Exception in parse_json_response" + std::string(e.what()));
			throw;
		}
	}


};


