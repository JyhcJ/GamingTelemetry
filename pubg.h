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

    TimedTaskQueue();
    ~TimedTaskQueue();

    void add_task(Task task);
    void start();
    void stop();

private:
    std::queue<Task> tasks_;
    std::mutex mutex_;
    std::condition_variable cv_;
    std::thread worker_thread_;
    std::atomic<bool> running_;

    void monitor_loop();
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
    std::unordered_set<std::string> matchIds = {};

    std::string player_name;

    // ����״̬
    bool isPUBG_Running = false;
    bool lastState = false;

    // ��־
    std::mutex log_mutex_;

    using Task = std::function<void()>;

    ProcessMonitor_PUBG();
    ~ProcessMonitor_PUBG();

    void start();
    void stop();

private:
    void _sendHttp_pubg(nlohmann::json jsonBody);
    void _sendHttp_pubg(std::string type, nlohmann::json data);
    bool readPlayerNameFromFile(const std::string& filePath, std::string& playerName, std::string& matchId);
    void monitor_loop();
    bool process_request(const std::string& player_name, const std::string& match_id);
    void add_request_task(Task&& task);
    std::string generate_monitoring_data();
    bool check_process_exists();
    std::vector<std::string> getIdForName(const std::string& player_name);
    bool getDetailInfo(std::string player_name, const std::vector<std::string>& newIDS);
    bool getDetailInfo(std::string player_name, const std::string& newID);
    std::string make_periodic_request();
    void parse_json_response(const std::string& json_response);
};