#pragma once
#include <string>
#include <map>
#include <vector>

class CurlUtils {
public:
   
    // HTTP ��������
    enum class Method {
        GET,
        POST
    };

    struct Response {
        int statusCode = 0;
        std::string body;
    };

    // ͬ����������
    static Response request(
        Method method,
        const std::string& url,
        const std::map<std::string, std::string>& headers = {},
        const std::string& body = ""
    );

    // ��ݷ�����GET ����
    static Response get(
        const std::string& url,
        const std::map<std::string, std::string>& headers = {}
    );

    // ��ݷ�����POST ����
    static Response post(
        const std::string& url,
        const std::map<std::string, std::string>& headers = {},
        const std::string& body = ""
    );

    // �����Ƿ���� SSL ֤����֤��Ĭ�� false��
    static void setVerifySSL(bool verify);
private:
    static bool s_VerifySSL;  // ������̬��Ա����
};
