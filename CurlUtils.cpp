#include "pch.h"
#include "CurlUtils.h"
#include <curl/curl.h>
#include <sstream>
#include <iostream>

struct CurlData {
    std::string response;
};
bool CurlUtils::s_VerifySSL = true;

// 回调函数：接收响应数据
size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// 回调函数：设置请求头
struct curl_slist* buildHeaders(const std::map<std::string, std::string>& headers) {
    struct curl_slist* headerList = nullptr;
    for (const auto& header : headers) {
        std::ostringstream oss;
        oss << header.first << ": " << header.second;
        headerList = curl_slist_append(headerList, oss.str().c_str());
    }
    return headerList;
}

// 静态变量


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
    // 启用自动解压（关键！）
    curl_easy_setopt(curl, CURLOPT_ENCODING, "");

    // 设置超时（可选，避免卡死）
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);

    // 设置 cURL 选项抑制错误
    curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);  // 静默失败

    // 设置 URL
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

    // 设置请求方式
    if (method == Method::POST) {
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE_LARGE, (curl_off_t)body.size());
    }
    else {
        curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
    }

    // 设置请求头
    struct curl_slist* headerList = buildHeaders(headers);
    if (headerList) {
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerList);
    }

    // 接收响应
    std::string responseString;
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseString);

    // SSL 设置
    if (!s_VerifySSL) {
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    }

    // 执行请求
    CURLcode resCode = curl_easy_perform(curl);

    if (resCode != CURLE_OK) {
        std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(resCode) << std::endl;
    }
    else {
        // 获取状态码
        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        res.statusCode = static_cast<int>(http_code);
        res.body = responseString;
    }

    // 清理资源
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