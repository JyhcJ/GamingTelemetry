#pragma once
#include "pch.h"
#include "HttpClient.h"
#include "lol_before.h"
#include <comutil.h>
#include <iostream>
#include <string>
#include <windows.h>

#include <algorithm>
#include <codecvt>
#include <fstream>

#include <locale>
#include <map>
#include <sstream>
#include <vector>

#include <atlstr.h>

#include <http.h>
#include <iomanip>

#include <tchar.h>

#include <wincrypt.h>

#include <shellapi.h>
#include <wininet.h>

#include "ThreadSafeLogger.h"

#include<nlohmann/json.hpp>
#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "crypt32.lib")
#pragma comment(lib, "comsuppw.lib")


extern bool is_lol_running;
extern std::mutex g_mtx;

class Game_Before {
private:
	HttpClient http;
	std::string url;
	std::string app_port;
	std::string auth_token;
	std::string rso_platform_id;
	std::string rso_original_platform_id;

    

	std::string wstring2string(const std::wstring& ws) {
		_bstr_t t = ws.c_str();
		char* pchar = (char*)t;
		std::string result = pchar;
		return result;
	}

	std::wstring string2wstring(const std::string& s) {
		_bstr_t t = s.c_str();
		wchar_t* pwchar = (wchar_t*)t;
		std::wstring result = pwchar;
		return result;
	}
	std::string ReadTxtFileForceUtf8(const std::wstring& filePath) {
		std::wifstream file(filePath, std::ios::binary);
		if (!file.is_open()) {
			return "Error: Cannot open file";
		}

		// 设置 wifstream 使用 UTF-16 编码（适用于 UTF-16LE 文件）
		file.imbue(std::locale(file.getloc(), new std::codecvt_utf16<wchar_t, 0x10ffff, std::little_endian>));

		std::wstringstream wss;
		wss << file.rdbuf();

		std::wstring wcontent = wss.str();


		// 如果你不在乎乱码，可以直接把 wchar_t 转成 char
		//std::string result(wcontent.begin(), wcontent.end()); // 强制转成 std::string
		// 使用方案 1/2/3 中的编码转换函数
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
		std::string result = converter.to_bytes(wcontent);

		return result;
	}
	

	std::string base64_encode(const std::string& in) {
		std::string out;
		DWORD len = 0;
		if (!CryptBinaryToStringA((const BYTE*)in.c_str(), static_cast<DWORD>(in.length()),
			CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF,
			NULL, &len)) {
			return "";
		}

		out.resize(len);
		if (!CryptBinaryToStringA((const BYTE*)in.c_str(), static_cast<DWORD>(in.length()),
			CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF,
			&out[0], &len)) {
			return "";
		}

		// Remove null terminator
		out.resize(len - 1);
		return out;
	}

	std::string extractParamValue(const std::string& commandLine, const std::string& paramName) {
		// 查找参数名的位置
		size_t paramPos = commandLine.find(paramName);
		if (paramPos == std::string::npos) {
			return ""; // 参数不存在
		}

		// 跳过参数名和等号
		size_t valueStart = paramPos + paramName.length();
		if (commandLine[valueStart] == '=') {
			valueStart++; // 跳过等号
		}

		// 查找值的结束位置（空格或字符串结尾）
		size_t valueEnd = commandLine.find(' ', valueStart);
		if (valueEnd == std::string::npos) {
			valueEnd = commandLine.length();
		}

		// 提取值
		std::string value = commandLine.substr(valueStart, valueEnd - valueStart);

		// 去除可能的引号
		value.erase(std::remove(value.begin(), value.end(), '\"'), value.end());

		return value;
	}



	std::string getUserPass(const std::wstring& command);

	bool httpAuthSend(const std::string& endUrl, nlohmann::json& responseJson,  std::string param ="");


public:
	bool getParam();
	bool processTftEndGameData(nlohmann::json& data, const uint64_t& puuid);
	bool processNormalEndGameData(nlohmann::json& data, uint64_t myAccountId);
	void getAndSendInfo(const std::string& sendType, const std::string& uuid);
	bool before_main(const std::string& sendType,  std::string uuid ="");
};
