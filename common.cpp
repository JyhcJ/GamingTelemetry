#include "pch.h"
#include "common.h"

#include <stdio.h>

#include <windows.h>
#include <cstdarg>  // �ɱ����֧��
#include <cstdio>   // vsnprintf

// ���� printf��������� DebugView

#include <windows.h>
#include <cstdarg>
#include <cwchar>   // vswprintf
#include <string>
#include "ThreadSafeLogger.h"

void call_���������Ϣ(const char* pszFormat, ...)
{
#ifdef _DEBUG
	// ����̶�ǰ׺�ַ���
	const char* prefix = "[������Ϣ] ";
	char szbufFormat[0x1000];
	char szbufFormat_Game[0x1100] = "";
	// ���̶�ǰ׺ƴ�ӵ� szbufFormat_Game
	strcat_s(szbufFormat_Game, prefix);
	va_list argList;
	va_start(argList, pszFormat);//�����б��ʼ��
	vsprintf_s(szbufFormat, pszFormat, argList);
	strcat_s(szbufFormat_Game, szbufFormat);
	OutputDebugStringA(szbufFormat_Game);
	va_end(argList);

#endif
}

void DebugPrintf(const char* format, ...) {
	char buffer[1024]; // ��������С�ɵ���

	va_list args;
	va_start(args, format);
	vsnprintf(buffer, sizeof(buffer), format, args);
	va_end(args);
	printf("[DEBUG] %s", buffer);  // ����ǿ���̨����
	// ����� DebugView
	OutputDebugStringA(buffer);
}

void DebugPrintf(const wchar_t* format, ...) {
	wchar_t buffer[1024];

	va_list args;
	va_start(args, format);
	vswprintf(buffer, sizeof(buffer) / sizeof(wchar_t), format, args);
	va_end(args);
	printf("[DEBUG] %S", buffer);  // ����ǿ���̨����
	OutputDebugStringW(buffer);
}

std::string UTF8ToANSI(const std::string& utf8Str) {
	// 1. UTF-8 �� UTF-16
	int wideLen = MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), -1, NULL, 0);
	wchar_t* wideStr = new wchar_t[wideLen];
	MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), -1, wideStr, wideLen);

	// 2. UTF-16 �� ANSI (���ش���ҳ)
	int ansiLen = WideCharToMultiByte(CP_ACP, 0, wideStr, -1, NULL, 0, NULL, NULL);
	char* ansiStr = new char[ansiLen];
	WideCharToMultiByte(CP_ACP, 0, wideStr, -1, ansiStr, ansiLen, NULL, NULL);

	std::string result(ansiStr);
	delete[] wideStr;
	delete[] ansiStr;
	return result;
}

std::wstring Utf8ToWstring(const std::string& str) {
	int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
	std::wstring wstrTo(size_needed, 0);
	MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
	return wstrTo;
}

std::string UTF8ToGBK(const std::string& strUTF8) {
	// 1. UTF-8 �� UTF-16 (���ַ�)
	int len = MultiByteToWideChar(CP_UTF8, 0, strUTF8.c_str(), -1, NULL, 0);
	if (len <= 0) return "";
	wchar_t* wstr = new wchar_t[len];
	MultiByteToWideChar(CP_UTF8, 0, strUTF8.c_str(), -1, wstr, len);

	// 2. UTF-16 �� GBK
	len = WideCharToMultiByte(CP_ACP, 0, wstr, -1, NULL, 0, NULL, NULL);
	if (len <= 0) {
		delete[] wstr;
		return "";
	}
	char* str = new char[len];
	WideCharToMultiByte(CP_ACP, 0, wstr, -1, str, len, NULL, NULL);

	std::string result(str);
	delete[] wstr;
	delete[] str;
	return result;
}

std::string TCHARToString(const TCHAR* tcharStr) {
#ifdef UNICODE
	// ����� Unicode��TCHAR = wchar_t������Ҫת��
	int size = WideCharToMultiByte(CP_UTF8, 0, tcharStr, -1, NULL, 0, NULL, NULL);
	std::string str(size, 0);
	WideCharToMultiByte(CP_UTF8, 0, tcharStr, -1, &str[0], size, NULL, NULL);
	return str;
#else
	// ����Ƕ��ֽڣ�TCHAR = char����ֱ�Ӹ�ֵ
	return std::string(tcharStr);
#endif
}

std::string gethostName() {
	// ��ȡ������,�������
	TCHAR computerName[MAX_COMPUTERNAME_LENGTH + 1];
	DWORD size = sizeof(computerName) / sizeof(computerName[0]);
	std::string hostName = TCHARToString(computerName);
	if (GetComputerName(computerName, &size)) {
		LOG_IMMEDIATE("Computer Name: " + hostName);

	}
	else {
		LOG_IMMEDIATE("Failed to get computer name. Error: " + GetLastError());
	}
	return  hostName;
}

std::wstring GetComputerNameWString() {
	wchar_t buffer[MAX_COMPUTERNAME_LENGTH + 1];
	DWORD size = MAX_COMPUTERNAME_LENGTH + 1;

	if (GetComputerNameW(buffer, &size)) {
		return std::wstring(buffer, size);
	}
	else {
		return L"";  // ��ȡʧ��
	}
}

std::string WStringToString(const std::wstring& wstr) {
	if (wstr.empty()) return std::string();

	int size_needed = WideCharToMultiByte(
		CP_UTF8,                // ʹ�� UTF-8 ����
		0,                     // �������־
		wstr.c_str(),           // ������ַ���
		(int)wstr.size(),      // �ַ������ȣ������� NULL��
		NULL,                  // �����������NULL ��ʾ���������С��
		0,                     // �����������С
		NULL, NULL             // Ĭ���ַ����Ƿ�ʹ��Ĭ���ַ�
	);

	if (size_needed <= 0) {
		return "";  // ת��ʧ��
	}

	std::string str(size_needed, 0);
	WideCharToMultiByte(
		CP_UTF8, 0,
		wstr.c_str(), (int)wstr.size(),
		&str[0], size_needed,
		NULL, NULL
	);

	return str;
}