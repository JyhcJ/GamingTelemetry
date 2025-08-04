#include "pch.h"
#include "CurlUtils.h"
#include <curl/curl.h>
#include <sstream>
#include <iostream>

struct CurlData {
    std::string response;
};
bool CurlUtils::s_VerifySSL = true;

// �ص�������������Ӧ����
size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// �ص���������������ͷ
struct curl_slist* buildHeaders(const std::map<std::string, std::string>& headers) {
    struct curl_slist* headerList = nullptr;
    for (const auto& header : headers) {
        std::ostringstream oss;
        oss << header.first << ": " << header.second;
        headerList = curl_slist_append(headerList, oss.str().c_str());
    }
    return headerList;
}

// ��̬����


CurlUtils::Response CurlUtils::request(
    Method method,
    const std::string& url,
    const std::map<std::string, std::string>& headers,
    const std::string& body
) {
    CURL* curl = curl_easy_init();
    Response res;

    if (!curl) {
        std::cerr << "Failed to initialize CURL" << std::endl;
        return res;
    }
    // �����Զ���ѹ���ؼ�����
    curl_easy_setopt(curl, CURLOPT_ENCODING, "");

    // ���ó�ʱ����ѡ�����⿨����
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);

    // ���� cURL ѡ�����ƴ���
    curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);  // ��Ĭʧ��

    // ���� URL
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

    // ��������ʽ
    if (method == Method::POST) {
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE_LARGE, (curl_off_t)body.size());
    }
    else {
        curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
    }

    // ��������ͷ
    struct curl_slist* headerList = buildHeaders(headers);
    if (headerList) {
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerList);
    }

    // ������Ӧ
    std::string responseString;
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseString);

    // SSL ����
    if (!s_VerifySSL) {
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    }

    // ִ������
    CURLcode resCode = curl_easy_perform(curl);

    if (resCode != CURLE_OK) {
        std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(resCode) << std::endl;
    }
    else {
        // ��ȡ״̬��
        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        res.statusCode = static_cast<int>(http_code);
        res.body = responseString;
    }

    // ������Դ
    if (headerList) curl_slist_free_all(headerList);
    curl_easy_cleanup(curl);

    return res;
}

CurlUtils::Response CurlUtils::get(
    const std::string& url,
    const std::map<std::string, std::string>& headers
) {
    return request(Method::GET, url, headers);
}

CurlUtils::Response CurlUtils::post(
    const std::string& url,
    const std::map<std::string, std::string>& headers,
    const std::string& body
) {
    return request(Method::POST, url, headers, body);
}

void CurlUtils::setVerifySSL(bool verify) {
    s_VerifySSL = verify;
}