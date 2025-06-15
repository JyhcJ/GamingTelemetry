#include "pch.h"
#include "HttpClient.h"
#include <sstream>
//
HttpClient::HttpClient() {
	// ��ʼ��WinHTTP�Ự
	m_hSession = WinHttpOpen(
		L"HttpClient/1.0",
		WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
		WINHTTP_NO_PROXY_NAME,
		WINHTTP_NO_PROXY_BYPASS,
		0
	);
}

HttpClient::~HttpClient() {
	if (m_hSession) {
		WinHttpCloseHandle(m_hSession);
	}
}

// ���ó�ʱʱ�䣨��λ�����룩 TODO����Ҫ�ر�...
void HttpClient::SetTimeout(DWORD resolveTimeout, DWORD connectTimeout,
	DWORD sendTimeout, DWORD receiveTimeout) {
	m_resolveTimeout = resolveTimeout;
	m_connectTimeout = connectTimeout;
	m_sendTimeout = sendTimeout;
	m_receiveTimeout = receiveTimeout;
}

// ����URL��֧��http/https��
bool HttpClient::CrackUrl(
	const std::wstring& url,
	std::wstring& scheme,
	std::wstring& host,
	std::wstring& path,
	INTERNET_PORT& port
) {
	URL_COMPONENTS urlComp = { 0 };
	urlComp.dwStructSize = sizeof(urlComp);

	// ������Ҫ�������ֶ�
	urlComp.dwSchemeLength = -1;
	urlComp.dwHostNameLength = -1;
	urlComp.dwUrlPathLength = -1;
	urlComp.dwExtraInfoLength = -1;

	if (!WinHttpCrackUrl(url.c_str(), (DWORD)url.length(), 0, &urlComp)) {
		return false;
	}

	scheme.assign(urlComp.lpszScheme, urlComp.dwSchemeLength);
	host.assign(urlComp.lpszHostName, urlComp.dwHostNameLength);
	path.assign(urlComp.lpszUrlPath, urlComp.dwUrlPathLength);
	port = urlComp.nPort;

	return true;
}
std::wstring extractPassword(const std::wstring& url) {
	// ���� "://" ��λ��
	size_t protocol_pos = url.find(L"://");
	if (protocol_pos == std::wstring::npos) {
		return L"";
	}

	// ���� "@" ��λ�ã��� protocol_pos ��ʼ����
	size_t at_pos = url.find(L'@', protocol_pos + 3);
	if (at_pos == std::wstring::npos) {
		return L"";
	}

	// ��ȡ��֤���� (username:password)
	std::wstring auth = url.substr(protocol_pos + 3, at_pos - (protocol_pos + 3));

	// ���� ":" �ָ��û���������
	size_t colon_pos = auth.find(L':');
	if (colon_pos == std::wstring::npos) {
		return L"";
	}

	// ��ȡ���벿��
	return auth.substr(colon_pos + 1);
}
// ����HTTP����
std::string HttpClient::SendRequest(
	const std::wstring& url,
	const std::wstring& method,
	const std::map<std::wstring, std::wstring>& headers,
	const std::string& body,
	bool userPass,
	const std::wstring& pathAdd
) {
	HINTERNET hConnect = NULL;
	HINTERNET hRequest = NULL;
	std::string response;

	try {
		// 1. ����URL
		std::wstring scheme, host, path;
		INTERNET_PORT port;
		if (!CrackUrl(url, scheme, host, path, port)) {
			throw std::runtime_error("Failed to parse URL");
		}
		path = path + pathAdd;
		// 2. ���ӵ�������
		hConnect = WinHttpConnect(m_hSession, host.c_str(), port, 0);
		DWORD err = GetLastError();

		if (!hConnect) {
			throw std::runtime_error("WinHttpConnect failed");
		}

		// 3. ��������
		hRequest = WinHttpOpenRequest(
			hConnect,
			method.c_str(),
			path.c_str(),
			NULL,
			WINHTTP_NO_REFERER,
			WINHTTP_DEFAULT_ACCEPT_TYPES,
			(scheme == L"https") ? WINHTTP_FLAG_SECURE : 0
		);

		//if (!WinHttpSetTimeouts(
		//	hRequest,
		//	m_resolveTimeout,  // DNS������ʱ
		//	m_connectTimeout,  // ���ӳ�ʱ
		//	m_sendTimeout,     // ���ͳ�ʱ
		//	m_receiveTimeout   // ���ճ�ʱ
		//)) {
		//	throw std::runtime_error("WinHttpSetTimeouts failed");
		//}

		if (!hRequest) {
			throw std::runtime_error("WinHttpOpenRequest failed");
		}

		// 4. �������ͷ
		if (!headers.empty()) {
			std::wstringstream headerStream;
			for (const auto& header : headers) {
				headerStream << header.first << L": " << header.second << L"\r\n";
			}
			std::wstring headerStr = headerStream.str();
			if (!WinHttpAddRequestHeaders(
				hRequest,
				headerStr.c_str(),
				(DWORD)headerStr.length(),
				WINHTTP_ADDREQ_FLAG_ADD
			)) {
				throw std::runtime_error("WinHttpAddRequestHeaders failed");
			}
		}
		DWORD dwFlags =
			SECURITY_FLAG_IGNORE_UNKNOWN_CA |             // ����δ֪ CA
			SECURITY_FLAG_IGNORE_CERT_DATE_INVALID |      // ���Թ���֤��
			SECURITY_FLAG_IGNORE_CERT_CN_INVALID |        // ���� CN ��ƥ��
			SECURITY_FLAG_IGNORE_CERT_WRONG_USAGE;        // ����֤����;����

		if (userPass == true) {
			std::wstring wstr = extractPassword(url);
			// ����Basic��֤
			WinHttpSetCredentials(
				hRequest,
				WINHTTP_AUTH_TARGET_SERVER,
				WINHTTP_AUTH_SCHEME_BASIC,
				L"riot",
				wstr.c_str(),
				NULL
			);
		}

		if (!WinHttpSetOption(hRequest, WINHTTP_OPTION_SECURITY_FLAGS, &dwFlags, sizeof(dwFlags))) {
			DWORD err = GetLastError();
			//std::cerr << "WinHttpSetOption (ignore cert error) failed. Error: " << err << std::endl;
			throw std::runtime_error("Failed to ignore certificate errors");
		}

		// 5. ��������
		if (!WinHttpSendRequest(
			hRequest,
			WINHTTP_NO_ADDITIONAL_HEADERS,
			0,
			//(LPVOID)body.empty() ? NULL : body.c_str(),
			(LPVOID)(body.empty() ? NULL : body.c_str()),  // Explicit cast to LPVOID
			(DWORD)body.size(),
			(DWORD)body.size(),
			0
		)) {
			DWORD error = GetLastError();
			throw std::runtime_error("WinHttpSendRequest failed");
		}

		// 6. ������Ӧ
		if (!WinHttpReceiveResponse(hRequest, NULL)) {
			throw std::runtime_error("WinHttpReceiveResponse failed");
		}

		// 7. ��ȡ��Ӧ����
		DWORD dwSize = 0;
		DWORD dwDownloaded = 0;
		LPSTR pszOutBuffer;

		do {
			// ��������Ƿ����
			if (!WinHttpQueryDataAvailable(hRequest, &dwSize)) {
				throw std::runtime_error("WinHttpQueryDataAvailable failed");
			}

			if (dwSize == 0) break;

			// ��ȡ���ݿ�
			pszOutBuffer = new char[dwSize + 1];
			ZeroMemory(pszOutBuffer, dwSize + 1);

			if (!WinHttpReadData(hRequest, pszOutBuffer, dwSize, &dwDownloaded)) {
				delete[] pszOutBuffer;
				throw std::runtime_error("WinHttpReadData failed");
			}

			response.append(pszOutBuffer, dwDownloaded);
			delete[] pszOutBuffer;
		} while (dwSize > 0);
	}
	catch (const std::exception& e) {
		if (hRequest) WinHttpCloseHandle(hRequest);
		if (hConnect) WinHttpCloseHandle(hConnect);
		throw e.what();  // �����׳��쳣
	}

	// ������Դ
	if (hRequest) WinHttpCloseHandle(hRequest);
	if (hConnect) WinHttpCloseHandle(hConnect);

	return response;
}


