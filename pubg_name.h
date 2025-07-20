
#pragma once
#include <iostream>
#include <string>
#include <memory>
#include <curl/curl.h>

typedef struct {
    ULONG64 base_address;  // ��ַ��0x7FF6D7DE7C30
    ULONG offset_count;    // ƫ�����㼶��
    ULONG offsets[8];      // ���֧��8��ƫ��
    SIZE_T buffer_size;    // �����������С
} MEMORY_REQUEST;

typedef struct _GENERAL_CONSTRUCTION {
    DWORD ProcessId;
    WCHAR FeatureCode[256]; // �㹻��Ļ������洢������
    DWORD FeatureCodeSize; // �㹻��Ļ������洢������
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
            // ��������URL
            std::string url = wurl + playerName;
            //std::string url = "https://api.pubg.com/shards/steam/players?filter[playerNames]=" + playerName;
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

            // ����HTTPͷ
            struct curl_slist* headers = nullptr;
            headers = curl_slist_append(headers, "accept: application/vnd.api+json");
            std::string authHeader = "Authorization:" + apiKey_;
            headers = curl_slist_append(headers, authHeader.c_str());
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

            // ���ûص���������Ӧ������
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

            // ִ������
            CURLcode res = curl_easy_perform(curl);

            // ������
            if (res != CURLE_OK) {
                std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
            }

            // ����
            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);
        }

        return response;
    }

private:
    std::string apiKey_;

    // libcurl д�ص�����
    static size_t writeCallback(void* contents, size_t size, size_t nmemb, void* userp) {
        ((std::string*)userp)->append((char*)contents, size * nmemb);
        return size * nmemb;
    }
};