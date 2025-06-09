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

	// 发送HTTP请求
	std::string SendRequest(
		const std::wstring& url,
		const std::wstring& method,               // "GET" 或 "POST"
		const std::map<std::wstring, std::wstring>& headers,  // 请求头
		const std::string& body = "",               // 请求体（POST用）
		bool userPass = false
	);

private:
	HINTERNET m_hSession;  // WinHTTP会话句柄

	// 解析URL
	bool CrackUrl(
		const std::wstring& url,
		std::wstring& scheme,
		std::wstring& host,
		std::wstring& path,
		INTERNET_PORT& port
	);

	DWORD m_resolveTimeout = 3000;   // DNS解析超时
	DWORD m_connectTimeout = 5000;   // 连接超时
	DWORD m_sendTimeout = 5000;      // 发送超时
	DWORD m_receiveTimeout = 5000;   // 接收超时
};