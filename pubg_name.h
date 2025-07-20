
#pragma once
#include <iostream>
#include <string>
#include <memory>
#include <curl/curl.h>

typedef struct {
    ULONG64 base_address;  // 基址如0x7FF6D7DE7C30
    ULONG offset_count;    // 偏移量层级数
    ULONG offsets[8];      // 最多支持8级偏移
    SIZE_T buffer_size;    // 输出缓冲区大小
} MEMORY_REQUEST;

typedef struct _GENERAL_CONSTRUCTION {
    DWORD ProcessId;
    WCHAR FeatureCode[256]; // 足够大的缓冲区存储特征码
    DWORD FeatureCodeSize; // 足够大的缓冲区存储特征码
    WCHAR PlayerName[256];
    DWORD PlayerNameSize;
    MEMORY_REQUEST mr;

} GENERAL_CONSTRUCTION, * PGENERAL_CONSTRUCTION;


std::string driverGetPlayerName(GENERAL_CONSTRUCTION gc_in, GENERAL_CONSTRUCTION& gc_out,DWORD fun=0);

int loadDriver();

bool driverUpdate(GENERAL_CONSTRUCTION gc_in, GENERAL_CONSTRUCTION& gc_out , DWORD fun = 0);

std::string getPlayerNamePUBG();

class PUBGAPI {
public:
    PUBGAPI(const std::string& apiKey) : apiKey_(apiKey) {
        curl_global_init(CURL_GLOBAL_DEFAULT);
    }

    ~PUBGAPI() {
        curl_global_cleanup();
    }

    std::string getPlayerInfo(const std::string& wurl,const std::string& playerName) {
        std::string response;
        CURL* curl = curl_easy_init();

        if (curl) {
            // 设置请求URL
            std::string url = wurl + playerName;
            //std::string url = "https://api.pubg.com/shards/steam/players?filter[playerNames]=" + playerName;
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

            // 设置HTTP头
            struct curl_slist* headers = nullptr;
            headers = curl_slist_append(headers, "accept: application/vnd.api+json");
            std::string authHeader = "Authorization:" + apiKey_;
            headers = curl_slist_append(headers, authHeader.c_str());
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

            // 设置回调函数和响应缓冲区
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

            // 执行请求
            CURLcode res = curl_easy_perform(curl);

            // 检查错误
            if (res != CURLE_OK) {
                std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
            }

            // 清理
            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);
        }

        return response;
    }

private:
    std::string apiKey_;

    // libcurl 写回调函数
    static size_t writeCallback(void* contents, size_t size, size_t nmemb, void* userp) {
        ((std::string*)userp)->append((char*)contents, size * nmemb);
        return size * nmemb;
    }
};