#include "pch.h"
#include <windows.h>
#include <winhttp.h>
#include <string>
#include <iostream>

#pragma comment(lib, "winhttp.lib")
// ����?
void SendPostRequest() {
	HINTERNET hSession = NULL;
	HINTERNET hConnect = NULL;
	HINTERNET hRequest = NULL;

	try {
		// ��ʼ��WinHTTP�Ự
		hSession = WinHttpOpen(L"WinHTTP Example/1.0",
			WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
			WINHTTP_NO_PROXY_NAME,
			WINHTTP_NO_PROXY_BYPASS, 0);
		if (!hSession) throw "WinHttpOpen failed";

		// ���ӵ�������
		hConnect = WinHttpConnect(hSession, L"127.0.0.1", 8000, 0);
		if (!hConnect) throw "WinHttpConnect failed";

		// ����HTTP����
		hRequest = WinHttpOpenRequest(hConnect, L"POST",
			L"/api/client/PostGameData",
			NULL, WINHTTP_NO_REFERER,
			WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
		if (!hRequest) throw "WinHttpOpenRequest failed";
		// ��װһ��http�࣬����Ϊurl�� get����post�����Զ����ͷ��body�����ؽ�����Ӧ
		// ��������ͷ
		const std::wstring headers =
			L"language: ZH_CN\r\n"
			L"merchantId: 53\r\n"
			L"sec-ch-ua-platform: \"Windows\"\r\n"
			L"Referer: https://dev-asz.cjmofang.com/activity/activityManagement/createActivity/MODE_SIGN/0/add/0\r\n"
			L"sec-ch-ua: \"Chromium\";v=\"136\", \"Google Chrome\";v=\"136\", \"Not.A/Brand\";v=\"99\"\r\n"
			L"sec-ch-ua-mobile: ?0\r\n"
			L"barId: 98\r\n"
			L"Accept: application/json, text/plain, */*\r\n"
			L"Content-Type: application/json\r\n"
			L"organizationType: \"BAR\"\r\n"
			L"token: {{bToken}}\r\n"
			L"User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/136.0.0.0 Safari/537.36\r\n"
			L"Accept-Encoding: gzip, deflate, br\r\n"
			L"Connection: keep-alive\r\n"
			L"Cache-Control: no-cache\r\n"
			L"Host: 127.0.0.1:8000";

		if (!WinHttpAddRequestHeaders(hRequest, headers.c_str(), (DWORD)headers.length(), WINHTTP_ADDREQ_FLAG_ADD)) {
			throw "WinHttpAddRequestHeaders failed";
		}

		// JSON������
		const std::string jsonBody = R"({
            "computer_no":"A046",
            "name": "test",
            "game_mode": "MATCH",
            "team_size": "ONE",
            "type": "SERIES_KILLS",
            "type_count": "3"
        })";

		// ��������
		if (!WinHttpSendRequest(hRequest,
			WINHTTP_NO_ADDITIONAL_HEADERS, 0,
			(LPVOID)jsonBody.c_str(),
			(DWORD)jsonBody.size(),
			(DWORD)jsonBody.size(), 0)) {
			throw "WinHttpSendRequest failed";
		}

		// ������Ӧ
		if (!WinHttpReceiveResponse(hRequest, NULL)) {
			throw "WinHttpReceiveResponse failed";
		}

		// ��ȡ��Ӧ���ݣ�ʾ����
		DWORD dwSize = 0;
		DWORD dwDownloaded = 0;
		LPSTR pszOutBuffer;

		do {
			dwSize = 0;
			if (!WinHttpQueryDataAvailable(hRequest, &dwSize)) {
				throw "WinHttpQueryDataAvailable failed";
			}

			pszOutBuffer = new char[dwSize + 1];
			ZeroMemory(pszOutBuffer, dwSize + 1);

			if (!WinHttpReadData(hRequest, (LPVOID)pszOutBuffer, dwSize, &dwDownloaded)) {
				delete[] pszOutBuffer;
				throw "WinHttpReadData failed";
			}

			std::cout << pszOutBuffer;
			delete[] pszOutBuffer;
		} while (dwSize > 0);

		std::cout << "Request completed successfully.\n";
	}
	catch (const char* msg) {
		std::cerr << "Error: " << msg << " (Error code: " << GetLastError() << ")\n";
	}

	// ������Դ
	if (hRequest) WinHttpCloseHandle(hRequest);
	if (hConnect) WinHttpCloseHandle(hConnect);
	if (hSession) WinHttpCloseHandle(hSession);
}

void sendHttp(std::string sendtype, std::string jsonBody, const std::wstring headers) {}

int main1() {
	SendPostRequest();
	return 0;
}

extern "C" __declspec(dllexport) void test1() {
	main1();
}