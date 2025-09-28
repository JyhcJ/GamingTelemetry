#pragma once
#include <string>
#include <map>
#include <vector>

class CurlUtils {
public:
   
    // HTTP 方法类型
    enum class Method {
        GET,
        POST
    };

    struct Response {
        int statusCode = 0;
        std::string body;
    };

    // 同步发送请求
    static Response request(
        Method method,
        const std::string& url,
        const std::map<std::string, std::string>& headers = {},
        const std::string& body = ""
    );

    // 快捷方法：GET 请求
    static Response get(
        const std::string& url,
        const std::map<std::string, std::string>& headers = {}
    );

    // 快捷方法：POST 请求
    static Response post(
        const std::string& url,
        const std::map<std::string, std::string>& headers = {},
        const std::string& body = ""
    );

    // 设置是否忽略 SSL 证书验证（默认 false）
    static void setVerifySSL(bool verify);
private:
    static bool s_VerifySSL;  // 声明静态成员变量
};
