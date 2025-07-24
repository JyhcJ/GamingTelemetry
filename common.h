#pragma once

#include "string"

#include <map>
#include <filesystem>


void call_调试输出信息(const char* pszFormat, ...);

void DebugPrintf(const char* format, ...);

void DebugPrintf(const wchar_t* format, ...);

std::string UTF8ToANSI(const std::string& utf8Str);

std::wstring Utf8ToWstring(const std::string& str);

std::string UTF8ToGBK(const std::string& strUTF8);

std::string WStringToGBK(const std::wstring& wstr);

std::string TCHARToString(const TCHAR* tcharStr);

std::string gethostName();

std::wstring GetComputerNameWString();

std::string WStringToString(const std::wstring& wstr);

std::wstring stringTOwstring(const std::string& str);

bool IsProcessRunning(const std::wstring& processName);

bool is_file_exists_and_not_empty(const std::string& filename);

std::string preprocess_mitm_text(const std::string& input);

std::string unescape_json_string(const std::string& input);

void remove_control_chars(std::string& s);

bool CheckFileExistsWinAPI(const std::wstring& filePath);

void injectDLL(std::wstring process, std::wstring dllName);

std::string GenerateUUID();

std::string getComputerName();

std::map<std::wstring, std::wstring> getHeader();

std::string GetWGPath_REG();

std::string GetPath_REG(HKEY first, const WCHAR* reg, const WCHAR* name);

bool WGRefresh(const std::wstring& exePath, const std::wstring& parameters);

std::string FindGamePath(const std::string& relativePath);

BOOL Call_提升权限(BOOL bEnable);

void EnableDebugPriv();

BOOL EnableDebugPrivilege(BOOL bEnable);

std::string GetLastErrorAsString(std::string str = "", DWORD errorCode = 0);

void _sendHttp(std::wstring url, std::string jsonDump, std::string& ret);

DWORD GetProccessPath(DWORD pid, wchar_t* processName, DWORD size);

std::pair<BYTE*, DWORD> GetModuleInfo(DWORD pid, std::wstring name);





