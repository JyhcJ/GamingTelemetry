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
	std::map<std::string, std::string> region_map = {
		{"HN1", "艾欧尼亚"},
		{"HN2", "祖安"},
		{"HN3", "诺克萨斯"},
		{"HN4", "班德尔城"},
		{"HN5", "皮尔特沃夫"},
		{"HN6", "战争学院"},
		{"HN7", "巨神峰"},
		{"HN8", "雷瑟守备"},
		{"HN9", "裁决之地"},
		{"HN10", "黑色玫瑰"},
		{"HN11", "暗影岛"},
		{"HN12", "钢铁烈阳"},
		{"HN13", "水晶之痕"},
		{"HN14", "均衡教派"},
		{"HN15", "影流"},
		{"HN16", "守望之海"},
		{"HN17", "征服之海"},
		{"HN18", "卡拉曼达"},
		{"HN19", "皮城警备"},
		{"WT1_NEW", "比尔吉沃特"},
		{"WT2_NEW", "德玛西亚"},
		{"WT3_NEW", "弗雷尔卓德"},
		{"WT4_NEW", "无畏先锋"},
		{"WT5", "恕瑞玛"},
		{"WT6", "扭曲丛林"},
		{"WT7", "巨龙之巢"},
		{"EDU1", "教育网专区"},
		{"BGP1", "男爵领域"}
	};

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

	bool getParam();

	void getUserInfo();

public:

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

	std::string ExecuteCommandAsAdmin(const std::wstring& command) {
		std::wstring tempFile = L"C:\\output.txt";
		std::wstring cmdLine = L"/c " + command + L" > \"" + tempFile + L"\"";

		SHELLEXECUTEINFOW sei = { 0 };
		sei.cbSize = sizeof(sei);
		sei.fMask = SEE_MASK_NOCLOSEPROCESS;
		sei.lpVerb = L"runas";  // 管理员
		sei.lpFile = L"cmd.exe";
		sei.lpParameters = cmdLine.c_str();
		sei.nShow = SW_HIDE;

		if (!ShellExecuteExW(&sei)) {
			return  "Error: Failed to launch process with admin rights.";
		}

		// 等待命令执行完成
		WaitForSingleObject(sei.hProcess, INFINITE);
		CloseHandle(sei.hProcess);

		std::string str = ReadTxtFileForceUtf8(L"C:\\output.txt");

		// 读取输出文件

		return str;
	}

	bool before_main();

};
