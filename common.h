#pragma once

#include "string"

#include <map>
#include <filesystem>
#include <windows.h>
#include<nlohmann/json.hpp>
#include <unordered_set>

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

int executeSilently(const char* cmd);

std::string GetEnvSafe(const char* key);

std::string UrlEncode(const std::string& value);

const std::string& mapLookupOrDefault(const std::map<std::string, std::string>& m, const std::string& key);

const std::string mapLookupOrDefault(const std::map<int, std::string>& m, int key);

std::string utf8ToUnicodeEscape(const std::string& utf8Str);

std::string generate_md5(const std::string& input);

std::unordered_set<std::string> get_recent_folders(const std::string& dir_path, int seconds_ago);

std::string trim_quotes(const std::string& str);

std::string trim_ic(const std::string& str);

std::string remove_escape_chars(std::string str);

std::string readUtf8File(const std::string& filename);



template<typename MapType>
typename MapType::mapped_type mapLookupOrDefaultPlus(
	const MapType& m,
	const typename MapType::key_type& key,
	const typename MapType::mapped_type& defaultValue)
{
	auto it = m.find(key);
	return it != m.end() ? it->second : defaultValue;
}

std::wstring get_g_domain();

void set_g_domain(const std::wstring& domain);



