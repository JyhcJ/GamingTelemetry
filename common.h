#pragma once

#include "string"

void call_调试输出信息(const char* pszFormat, ...);

void DebugPrintf(const char* format, ...);

void DebugPrintf(const wchar_t* format, ...);

std::string UTF8ToANSI(const std::string& utf8Str);

std::wstring Utf8ToWstring(const std::string& str);

std::string UTF8ToGBK(const std::string& strUTF8);
