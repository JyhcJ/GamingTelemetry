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
#include "pubg_name.h"
#include "common.h"
#include <regex>

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
	std::unordered_set<std::string> matchIds = { };

	std::string player_name;

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
	
		running_ = true;
		// �������������߳�
		monitor_thread_ = std::thread(&ProcessMonitor_PUBG::monitor_loop, this);

		//request_thread_ = std::thread(&ProcessMonitor_PUBG::request_processing_loop, this);
		//ÿ1����ִ��һ�εĶ�ʱ����
		//periodic_thread_ = std::thread(&ProcessMonitor_PUBG::periodic_task_loop, this);
		//periodic_thread_.detach();

		monitor_thread_.detach();
		//request_thread_.detach();

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
		catch (...) {  // �������������쳣
			LOG_IMMEDIATE_ERROR("_sendHttp_pubg :::Unknown exception occurred");
		}
	}

	void _sendHttp_pubg(std::string type, nlohmann::json data) {
		nlohmann::json jsonBody;
		jsonBody["type"] = type;
		_sendHttp_pubg(jsonBody);
	}

	bool readPlayerNameFromFile(const std::string& filePath, std::string& playerName, std::string& matchId) {

		try {
			std::ifstream file(filePath, std::ios::binary); // �� binary ���⻻�з�ת��
			if (!file||!file.is_open()) {
				std::cerr << "Failed to open player name file: " << filePath << std::endl;
				return false;
			}


			// ��ȡ�ļ����ڴ滺����
			//file.seekg(0, std::ios::end);
			//std::streamsize size = file.tellg();
			//file.seekg(0, std::ios::beg);

			//std::vector<char> buffer(size);
			//if (file.read(buffer.data(), size)) {
			//	std::cout << "�ļ���С: " << size << " �ֽ�" << std::endl;
			//	std::cout << "ʮ����������:\n";
			//	for (size_t i = 0; i < buffer.size(); ++i) {
			//		std::cout << std::hex << std::setw(2) << std::setfill('0')
			//			<< (static_cast<unsigned int>(static_cast<unsigned char>(buffer[i]))) << " ";
			//		if ((i + 1) % 16 == 0) std::cout << "\n";
			//	}
			//}
			//else {
			//	std::cerr << "��ȡʧ��" << std::endl;
			//}




			// ��ȡ�ļ���С
			file.seekg(0, std::ios::end);
			std::streamsize size = file.tellg();
			file.seekg(0, std::ios::beg);

			// ��ȡ��������
			std::vector<char> buffer(size);
			if (!file.read(buffer.data(), size)) {
				std::cerr << "��ȡʧ��" << std::endl;
				return 1;
			}

			// ����������ͷ��ʾ������4�ֽڣ�
			const size_t BINARY_HEADER_SIZE = 4;
			if (size > BINARY_HEADER_SIZE) {
				buffer.erase(buffer.begin(), buffer.begin() + BINARY_HEADER_SIZE);
			}

			// ת��Ϊ�ַ���
			std::string text(buffer.begin(), buffer.end());
			std::cout << "��Ч����:\n" << text << std::endl;

			nlohmann::json content = nlohmann::json::parse(text);
			std::string friendlyName = getNestedValue<std::string>(content, { "FriendlyName" }, "error");
			playerName = getNestedValue<std::string>(content, { "RecordUserNickName" }, "error");



			size_t lastDotPos = friendlyName.rfind('.');  // �Ӻ���ǰ�������һ�� '.'
			if (lastDotPos == std::string::npos) {
				LOG_IMMEDIATE("����˳��Ծֺ�û���ҵ�pubg_matchID");
				return false;
			}
			matchId = friendlyName.substr(lastDotPos + 1);  // �������һ�������Ӵ�





			//std::string line;
			//const std::string key = "\"RecordUserNickName\": ";
			//const std::string key_ID = "\"FriendlyName\": ";

			//while (std::getline(file, line)) {
			//	// ȥ�����ܵ� BOM
			//	if (line.size() >= 3 &&
			//		(unsigned char)line[0] == 0xEF &&
			//		(unsigned char)line[1] == 0xBB &&
			//		(unsigned char)line[2] == 0xBF) {
			//		line.erase(0, 3);
			//	}

			//	// ȥ���س��� \r
			//	if (!line.empty() && line.back() == '\r') {
			//		line.pop_back();
			//	}
			//	//TODO Try
			//	size_t pos = line.find(key);
			//	if (pos == std::string::npos) {
			//		LOG_IMMEDIATE("����˳��Ծֺ�û���ҵ�pubg_playerName");
			//		return false;
			//	}
			//	size_t start = pos + key.length();
			//	size_t end = line.find(',', start);
			//	if (end == std::string::npos) {
			//		end = line.length();
			//	}
			//	playerName = line.substr(start, end - start);


			//	size_t pos_id = line.find(key_ID);
			//	if (pos != std::string::npos) {
			//		size_t start = pos + key_ID.length();
			//		size_t end = line.find(',', start);

			//		if (end == std::string::npos) {
			//			end = line.length();
			//		}

			//		std::string friendlyName = line.substr(start, end - start);
			//		size_t lastDotPos = friendlyName.rfind('.');  // �Ӻ���ǰ�������һ�� '.'
			//		if (lastDotPos == std::string::npos) {
			//			LOG_IMMEDIATE("����˳��Ծֺ�û���ҵ�pubg_matchID");
			//			return false;
			//		}
			//		matchId = friendlyName.substr(lastDotPos + 1);  // �������һ�������Ӵ�
			//	}
			//	return false;
			//}
		}
		catch (const std::exception& e) {
			LOG_IMMEDIATE_ERROR("pubg::readPlayerNameFromFile::" + std::string(e.what()));
			return false;
		}
		catch (...) {
			LOG_IMMEDIATE_ERROR("pubg::readPlayerNameFromFile::Unknown exception occurred while reading player name from file");
			return false;
		}
		return true;
	}

	// ����߳���ѭ��
	void monitor_loop() {
		//std::string player_name = getPlayerNamePUBG();
		//std::string player_name = "ppuubbggBOOMBOOM";

		//GENERAL_CONSTRUCTION gc_in = GENERAL_CONSTRUCTION();
		//GENERAL_CONSTRUCTION gc_out;
		//std::string response;
		//try {
		//	_sendHttp(L"/api/client/JuediqiushengGameConfig", "", response);
		//	if (json::accept(response)) {
		//		nlohmann::json jsonRes = nlohmann::json::parse(response);
		//		nlohmann::json jsonTest;
		//		jsonTest = jsonRes["metadata"];
		//		if (jsonTest["type"] == "off")
		//			return;
		//	}
		//}
		//catch (const std::exception& e) {
		//	LOG_IMMEDIATE("Exception in monitor_loop :::_sendHttp" + std::string(e.what()));
		//}
		//catch (...) {
		//	LOG_IMMEDIATE("Exception in monitor_loop :::_sendHttp:::δ֪�쳣");
		//}
		while (running_) {
			try {
				std::this_thread::sleep_for(std::chrono::seconds(1));
				isPUBG_Running = check_process_exists();


				if (lastState == true && isPUBG_Running == false)
				{
					lastState = false;
					player_name = "";
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
					//������־�߳�
					std::thread([this]() {
						std::unordered_set<std::string> recent_folders;
						std::string compUserName = GetEnvSafe("USERPROFILE");
						const std::string reportFolder = compUserName + "\\AppData\\Local\\TslGame\\Saved\\Demos\\";
						while (check_process_exists()) {
							std::this_thread::sleep_for(std::chrono::seconds(1));

							// C:\Users\Administrator\AppData\Local\TslGame\Saved\Demos
							recent_folders = get_recent_folders(reportFolder, 9);

							//recent_folders = get_recent_folders(reportFolder, 44444);
							if (!recent_folders.empty()) {
								std::string path = *recent_folders.begin();
								if (matchIds.find(path) != matchIds.end())
								{
									LOG_IMMEDIATE_DEBUG("pubg matchID�Ѿ�������. ����startMonitor");
									continue;
								}
								LOG_IMMEDIATE("pubg:����˳��˶Ծ�");
								matchIds.insert(recent_folders.begin(), recent_folders.end());

								// PUBG.replayinfo
								std::string name;
								const std::string newFile = path + "\\PUBG.replayinfo";  // �����õ�����
								std::string match_id;
								bool isGetName = readPlayerNameFromFile(newFile, name, match_id);

								if (isGetName)
								{
									std::thread([this, name, match_id]() {process_request(name, match_id);}).detach();
								}

							}
							//std::string log_data = generate_monitoring_data();
							//LOG_IMMEDIATE(log_data);
						}
						}).detach();


					//initMatchIds.clear();
					//initMatchIds.emplace_back("init");
					//std::this_thread::sleep_for(std::chrono::seconds(35));//���ض���
					//int ret = loadDriver(); //��������ʱ��
					//if (ret >= 0) {
					//	std::this_thread::sleep_for(std::chrono::seconds(3));
					//	WCHAR tempPlayerName[50];
					//	std::wstring tempStr;

					//	//Ҫ��ŵĻ� ʹ��ocr��ʽ(���浥�˿��ܻ���)
					//	while (true) {
					//		LOG_IMMEDIATE_ERROR("��ȡ��ɫ��...");
					//		driverUpdate(gc_in, gc_out, tempPlayerName);
					//	/*	size_t length = wcslen(tempPlayerName);
					//		if (tempPlayerName[0] != L'\0' || length < 3 || length>16) {
					//			break;
					//		}*/
					//	
					//		tempStr.assign(tempPlayerName);        // ʹ�� assign ����
					//		if (isValidPubgName(WStringToString(tempStr))) {
					//			break;
					//		}
					//		std::this_thread::sleep_for(std::chrono::seconds(5));
						//}



						//ocr ��ͼ��ʽ
						//player_name = getPlayerNamePUBG();

					//	player_name = WStringToString(tempStr);
					//	StopDriver(L"KMDFDriver2");
					//	RemoveDriver(L"KMDFDriver2");
					//	LOG_IMMEDIATE_ERROR("����ж��");
					//}
					//else {
					//	LOG_IMMEDIATE_ERROR("��������ʧ�ܣ����������Ƿ�װ��ȷ��");
					//	break;
					//}



				}
				//if (isPUBG_Running) { //��Ϸ��
				//	lastState = true;
				//	/*periodic_thread_ = std::thread(&ProcessMonitor_PUBG::periodic_task_loop, this);
				//	periodic_thread_.detach();*/
				//	if (player_name == "")
				//	{
				//		continue;
				//	}
				//	LOG_IMMEDIATE("���ڲ�ս��gameid...");
				//	bool isReq = process_request(player_name);
				//	if (!isReq) {
				//		// �������Ƶ�����ƣ�������������  (�����߳̿�������???)
				//		add_request_task([this] {
				//			process_request(player_name);
				//			});
				//	}
				//	std::this_thread::sleep_for(std::chrono::seconds(60));
				//}

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
	//void periodic_task_loop() {
	//	while (running_) {
	//		try {
	//			// ÿ1����ִ��һ��
	//			std::unique_lock<std::mutex> lock(periodic_mutex_);
	//			periodic_cond_.wait_for(lock, std::chrono::minutes(1), [this] {
	//				return !running_;
	//				});

	//			if (running_) {
	//				execute_periodic_task();
	//				last_periodic_time_ = std::chrono::steady_clock::now();
	//			}
	//		}
	//		catch (const std::exception& e) {
	//			LOG_IMMEDIATE("Exception in periodic_task_loop" + std::string(e.what()));
	//		}
	//		catch (...) {
	//			LOG_IMMEDIATE("Unknown exception in periodic_task_loop");
	//		}
	//	}
	//}

	// ���Դ������󣨷�������
	//bool try_process_request(const std::string& data) {
	//	std::unique_lock<std::mutex> lock(request_mutex_, std::try_to_lock);
	//	if (!lock.owns_lock()) {
	//		return false;
	//	}

	//	auto now = std::chrono::steady_clock::now();
	//	auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - last_request_time_).count();

	//	if (elapsed >= 60) {
	//		// ����1���ӣ����ü�����
	//		request_count_ = 0;
	//		last_request_time_ = now;
	//	}

	//	if (request_count_ < 10) {
	//		request_count_++;
	//		lock.unlock(); // ��ǰ�ͷ���

	//		// ʵ�ʴ�������
	//		process_request(data);
	//		return true;
	//	}

	//	return false;
	//}

	// ��������ʵ�ʹ�����
	bool process_request(const std::string& player_name, const std::string& match_id) {
		try {
			//��ȷ�������˳��� ս������ô���� ������Ϸ������Ż���
		/*	std::vector<std::string> matchInfo = getIdForName(player_name);
			if (matchInfo.empty()) {
				return false;
			}*/

			//std::string player_name111 = "L1ke-S";
			//matchInfo.emplace_back("901ee802-af98-4dbf-abd1-582e7dd4cc23");
			// 2. ���ݵ�һ����Ӧ����ڶ�������
			getDetailInfo(player_name, match_id);

			// 3. �����ڶ�����Ӧ��JSON
			//parse_json_response(second_response);
			//std::this_thread::sleep_for(std::chrono::seconds(120));

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

	//// ִ�ж�ʱ����
	//void execute_periodic_task() {
	//	try {

	//		if (player_name == "")
	//		{
	//			return;
	//		}

	//		bool isReq = process_request(player_name);

	//		if (!isReq) {
	//			// �������Ƶ�����ƣ�������������  (�����߳̿�������???)
	//			add_request_task([this] {
	//				process_request(player_name);
	//				});
	//		}

	//	}
	//	catch (const std::exception& e) {
	//		LOG_IMMEDIATE("Exception in periodic task" + std::string(e.what()));
	//	}
	//}

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
				LOG_IMMEDIATE("pubg�µĶԾ�id:" + s);
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

	bool getDetailInfo(std::string player_name, const std::vector<std::string>& newIDS) {
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
			return true; // mock JSON response
		}
		catch (const std::exception& e) {
			LOG_IMMEDIATE("Exception in make_second_request" + std::string(e.what()));
			return false; // mock JSON response
		}
	}

	bool getDetailInfo(std::string player_name, const std::string& newID) {
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
					LOG_IMMEDIATE_DEBUG("�ȴ��Ծֽ���" + jsonResponse.dump() + player_name +";;;" + newID);
					continue;
				}

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

				//return R"({"status": "success", "data": {"value": 42}})"; // mock JSON response
				return true; // mock JSON response
			
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


