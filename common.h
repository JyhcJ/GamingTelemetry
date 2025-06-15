#pragma once

#include "string"

void call_调试输出信息(const char* pszFormat, ...);

void DebugPrintf(const char* format, ...);

void DebugPrintf(const wchar_t* format, ...);

std::string UTF8ToANSI(const std::string& utf8Str);

std::wstring Utf8ToWstring(const std::string& str);

std::string UTF8ToGBK(const std::string& strUTF8);

std::string TCHARToString(const TCHAR* tcharStr);

std::string gethostName();

std::wstring GetComputerNameWString();

std::string WStringToString(const std::wstring& wstr);

std::wstring stringTOwstring(const std::string& str);

