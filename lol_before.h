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

		// ���� wifstream ʹ�� UTF-16 ���루������ UTF-16LE �ļ���
		file.imbue(std::locale(file.getloc(), new std::codecvt_utf16<wchar_t, 0x10ffff, std::little_endian>));

		std::wstringstream wss;
		wss << file.rdbuf();

		std::wstring wcontent = wss.str();


		// ����㲻�ں����룬����ֱ�Ӱ� wchar_t ת�� char
		//std::string result(wcontent.begin(), wcontent.end()); // ǿ��ת�� std::string
		// ʹ�÷��� 1/2/3 �еı���ת������
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
		std::string result = converter.to_bytes(wcontent);

		return result;
	}
	std::map<std::string, std::string> region_map = {
		{"HN1", "��ŷ����"},
		{"HN2", "�氲"},
		{"HN3", "ŵ����˹"},
		{"HN4", "��¶���"},
		{"HN5", "Ƥ�����ַ�"},
		{"HN6", "ս��ѧԺ"},
		{"HN7", "�����"},
		{"HN8", "��ɪ�ر�"},
		{"HN9", "�þ�֮��"},
		{"HN10", "��ɫõ��"},
		{"HN11", "��Ӱ��"},
		{"HN12", "��������"},
		{"HN13", "ˮ��֮��"},
		{"HN14", "�������"},
		{"HN15", "Ӱ��"},
		{"HN16", "����֮��"},
		{"HN17", "����֮��"},
		{"HN18", "��������"},
		{"HN19", "Ƥ�Ǿ���"},
		{"WT1_NEW", "�ȶ�������"},
		{"WT2_NEW", "��������"},
		{"WT3_NEW", "���׶�׿��"},
		{"WT4_NEW", "��η�ȷ�"},
		{"WT5", "ˡ����"},
		{"WT6", "Ť������"},
		{"WT7", "����֮��"},
		{"EDU1", "������ר��"},
		{"BGP1", "�о�����"}
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
		// ���Ҳ�������λ��
		size_t paramPos = commandLine.find(paramName);
		if (paramPos == std::string::npos) {
			return ""; // ����������
		}

		// �����������͵Ⱥ�
		size_t valueStart = paramPos + paramName.length();
		if (commandLine[valueStart] == '=') {
			valueStart++; // �����Ⱥ�
		}

		// ����ֵ�Ľ���λ�ã��ո���ַ�����β��
		size_t valueEnd = commandLine.find(' ', valueStart);
		if (valueEnd == std::string::npos) {
			valueEnd = commandLine.length();
		}

		// ��ȡֵ
		std::string value = commandLine.substr(valueStart, valueEnd - valueStart);

		// ȥ�����ܵ�����
		value.erase(std::remove(value.begin(), value.end(), '\"'), value.end());

		return value;
	}

	std::string ExecuteCommandAsAdmin(const std::wstring& command) {
		std::wstring tempFile = L"C:\\output.txt";
		std::wstring cmdLine = L"/c " + command + L" > \"" + tempFile + L"\"";

		SHELLEXECUTEINFOW sei = { 0 };
		sei.cbSize = sizeof(sei);
		sei.fMask = SEE_MASK_NOCLOSEPROCESS;
		sei.lpVerb = L"runas";  // ����Ա
		sei.lpFile = L"cmd.exe";
		sei.lpParameters = cmdLine.c_str();
		sei.nShow = SW_HIDE;

		if (!ShellExecuteExW(&sei)) {
			return  "Error: Failed to launch process with admin rights.";
		}

		// �ȴ�����ִ�����
		WaitForSingleObject(sei.hProcess, INFINITE);
		CloseHandle(sei.hProcess);

		std::string str = ReadTxtFileForceUtf8(L"C:\\output.txt");

		// ��ȡ����ļ�

		return str;
	}

	bool before_main();

};
