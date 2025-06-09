#pragma once
#pragma once
#include <windows.h>
#include <winhttp.h>
#include <string>
#include <map>
#include <vector>

#pragma comment(lib, "winhttp.lib")

class HttpClient {
public:
	HttpClient();
	~HttpClient();

	void SetTimeout(DWORD resolveTimeout = 3000,
		DWORD connectTimeout = 5000,
		DWORD sendTimeout = 5000,
		DWORD receiveTimeout = 5000);

	// ����HTTP����
	std::string SendRequest(
		const std::wstring& url,
		const std::wstring& method,               // "GET" �� "POST"
		const std::map<std::wstring, std::wstring>& headers,  // ����ͷ
		const std::string& body = "",               // �����壨POST�ã�
		bool userPass = false
	);

private:
	HINTERNET m_hSession;  // WinHTTP�Ự���

	// ����URL
	bool CrackUrl(
		const std::wstring& url,
		std::wstring& scheme,
		std::wstring& host,
		std::wstring& path,
		INTERNET_PORT& port
	);

	DWORD m_resolveTimeout = 3000;   // DNS������ʱ
	DWORD m_connectTimeout = 5000;   // ���ӳ�ʱ
	DWORD m_sendTimeout = 5000;      // ���ͳ�ʱ
	DWORD m_receiveTimeout = 5000;   // ���ճ�ʱ
};