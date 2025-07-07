#include "pch.h"
#include "common.h"
#include <stdio.h>
#include <windows.h>
#include <cstdarg>  // 可变参数支持
#include <cstdio>   // vsnprintf
#include <string>
#include "ThreadSafeLogger.h"
#include <locale>
#include <codecvt>
#include <TlHelp32.h>
#include <regex>
#include <Shlwapi.h>
#include <map>



void call_调试输出信息(const char* pszFormat, ...)
{
#ifdef _DEBUG
	// 定义固定前缀字符串
	const char* prefix = "[调试信息] ";
	char szbufFormat[0x1000];
	char szbufFormat_Game[0x1100] = "";
	// 将固定前缀拼接到 szbufFormat_Game
	strcat_s(szbufFormat_Game, prefix);
	va_list argList;
	va_start(argList, pszFormat);//参数列表初始化
	vsprintf_s(szbufFormat, pszFormat, argList);
	strcat_s(szbufFormat_Game, szbufFormat);
	OutputDebugStringA(szbufFormat_Game);
	va_end(argList);

#endif
}

void DebugPrintf(const char* format, ...) {
	char buffer[1024]; // 缓冲区大小可调整

	va_list args;
	va_start(args, format);
	vsnprintf(buffer, sizeof(buffer), format, args);
	va_end(args);
	printf("[DEBUG] %s", buffer);  // 如果是控制台程序
	// 输出到 DebugView
	OutputDebugStringA(buffer);
}

void DebugPrintf(const wchar_t* format, ...) {
	wchar_t buffer[1024];

	va_list args;
	va_start(args, format);
	vswprintf(buffer, sizeof(buffer) / sizeof(wchar_t), format, args);
	va_end(args);
	printf("[DEBUG] %S", buffer);  // 如果是控制台程序
	OutputDebugStringW(buffer);
}

std::string UTF8ToANSI(const std::string& utf8Str) {
	// 1. UTF-8 → UTF-16
	int wideLen = MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), -1, NULL, 0);
	wchar_t* wideStr = new wchar_t[wideLen];
	MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), -1, wideStr, wideLen);

	// 2. UTF-16 → ANSI (本地代码页)
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
	// 1. UTF-8 → UTF-16 (宽字符)
	int len = MultiByteToWideChar(CP_UTF8, 0, strUTF8.c_str(), -1, NULL, 0);
	if (len <= 0) return "";
	wchar_t* wstr = new wchar_t[len];
	MultiByteToWideChar(CP_UTF8, 0, strUTF8.c_str(), -1, wstr, len);

	// 2. UTF-16 → GBK
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


std::string WStringToGBK(const std::wstring& wstr) {
	if (wstr.empty()) return "";

	// 宽字符(UTF-16) 直接转 GBK
	int gbkSize = WideCharToMultiByte(
		936,                    // GBK代码页(简体中文)
		0,                      // 无特殊标志
		wstr.c_str(),           // 宽字符串
		(int)wstr.length(),     // 字符串长度(不包括NULL)
		NULL,                   // 不接收输出
		0,                      // 查询所需缓冲区大小
		NULL, NULL              // 使用默认字符
	);

	if (gbkSize <= 0) {
		return "";
	}

	std::string strGBK(gbkSize, 0);
	int result = WideCharToMultiByte(
		936, 0,
		wstr.c_str(), (int)wstr.length(),
		&strGBK[0], gbkSize,
		NULL, NULL
	);

	if (result <= 0) {
		return "";
	}

	return strGBK;
}

std::string TCHARToString(const TCHAR* tcharStr) {
#ifdef UNICODE
	// 如果是 Unicode（TCHAR = wchar_t），需要转换
	int size = WideCharToMultiByte(CP_UTF8, 0, tcharStr, -1, NULL, 0, NULL, NULL);
	std::string str(size, 0);
	WideCharToMultiByte(CP_UTF8, 0, tcharStr, -1, &str[0], size, NULL, NULL);
	return str;
#else
	// 如果是多字节（TCHAR = char），直接赋值
	return std::string(tcharStr);
#endif
}

std::string gethostName() {
	// 获取机器号,计算机名
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
		return L"";  // 获取失败
	}
}

std::string WStringToString(const std::wstring& wstr) {
	if (wstr.empty()) return std::string();

	// 第一步：计算所需缓冲区大小
	int size_needed = WideCharToMultiByte(
		CP_UTF8,                // UTF-8 编码
		0,                      // 无特殊标志
		wstr.c_str(),           // 输入宽字符串
		(int)wstr.length(),     // 字符串长度（不包括 NULL）
		NULL,                   // 输出缓冲区（NULL 表示计算所需大小）
		0,                      // 输出缓冲区大小
		NULL, NULL              // 默认字符和是否使用默认字符
	);

	if (size_needed <= 0) {
		return "";  // 转换失败
	}

	// 第二步：实际转换
	std::string str(size_needed, 0);
	int result = WideCharToMultiByte(
		CP_UTF8, 0,
		wstr.c_str(), (int)wstr.length(),
		&str[0], size_needed,
		NULL, NULL
	);

	if (result <= 0) {
		return "";  // 转换失败
	}

	return str;
}

std::wstring stringTOwstring(const std::string& str) {
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	return converter.from_bytes(str);
}

//// 进程检测函数
//bool IsProcessRunning(const std::wstring& processName) {
//	// Windows实现
//	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
//	if (snapshot == INVALID_HANDLE_VALUE) return false;
//
//	PROCESSENTRY32W entry;
//	entry.dwSize = sizeof(PROCESSENTRY32W);
//
//	bool found = false;
//	if (Process32FirstW(snapshot, &entry)) {
//		do {
//			if (std::wstring(entry.szExeFile) == processName) {
//				found = true;
//				break;
//			}
//		} while (Process32NextW(snapshot, &entry));
//	}
//
//	CloseHandle(snapshot);
//	return found;
//}
bool IsProcessRunning(const std::wstring& processName) {
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnapshot == INVALID_HANDLE_VALUE) {
		return false;
	}

	PROCESSENTRY32W pe32;
	pe32.dwSize = sizeof(PROCESSENTRY32W);

	if (!Process32FirstW(hSnapshot, &pe32)) {
		CloseHandle(hSnapshot);
		return false;
	}

	bool found = false;
	do {
		if (std::wstring(pe32.szExeFile) == processName) {
			found = true;
			break;
		}
	} while (Process32NextW(hSnapshot, &pe32));

	CloseHandle(hSnapshot);
	return found;
}

bool is_file_exists_and_not_empty(const std::string& filename) {
	std::ifstream file(filename, std::ios::binary);

	if (!file) {
		return false;  // 文件无法打开（可能不存在）
	}

	file.seekg(0, std::ios::end);
	std::streampos size = file.tellg();

	return size > 0;
}

std::string preprocess_mitm_text(const std::string& input) {
	std::string output = input;

	// 去除 b'xxx' 中的 b'
	std::regex b_prefix(R"(b'([^']*)')", std::regex::extended);
	output = std::regex_replace(output, b_prefix, "'$1'");

	// 把 Headers[(...)] 转换成 { key: value } 形式
	std::regex header_pattern(R"(Headers$$$(?:\s*$(?:b?'([^']*)', b?'([^']*)')\s*,?)+$$$)");
	output = std::regex_replace(output, header_pattern, "{$1}");

	// 把 (b'key', b'value') 转换为 "key": "value"
	std::regex pair_pattern(R"($b?'([^']*)', b?'([^']*)'$)", std::regex::extended);
	output = std::regex_replace(output, pair_pattern, "\"$1\": \"$2\"");

	// 处理逗号（多个键值对之间加逗号）
	std::regex comma_pattern(R"((?<=}),\s*(?={))");
	output = std::regex_replace(output, comma_pattern, ",");

	return output;
}

std::string unescape_json_string(const std::string& input) {
	std::string output;
	for (size_t i = 0; i < input.size(); ++i) {
		if (input[i] == '\\' && i + 1 < input.size()) {
			switch (input[i + 1]) {
			case 'n':  output += '\n'; i++; break;
			case '"':  output += '"';  i++; break;
			case '\\': output += '\\'; i++; break;
			default:   output += input[i]; break;
			}
		}
		else {
			output += input[i];
		}
	}
	return output;
}

void remove_control_chars(std::string& s) {
	s.erase(
		std::remove_if(s.begin(), s.end(),
			[](char c) {
				return c == '\\r' || c == '\\n' || c == '\\t' ||
					(static_cast<unsigned char>(c) < 0x20);
			}
		),
		s.end()
	);
}

DWORD FindProcessId(const std::wstring& processName) {
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnapshot == INVALID_HANDLE_VALUE) {
		return 0;
	}

	PROCESSENTRY32W pe32;
	pe32.dwSize = sizeof(PROCESSENTRY32W);

	if (Process32FirstW(hSnapshot, &pe32)) {
		do {
			if (std::wstring(pe32.szExeFile) == processName) {
				CloseHandle(hSnapshot);
				return pe32.th32ProcessID;
			}
		} while (Process32NextW(hSnapshot, &pe32));
	}

	CloseHandle(hSnapshot);
	return 0;
}

bool IsDllInjected(DWORD pid, const std::wstring& dllName) {
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pid);
	MODULEENTRY32W me32 = { sizeof(me32) };

	if (Module32FirstW(hSnapshot, &me32)) {
		do {
			if (_wcsicmp(me32.szModule, dllName.c_str()) == 0) {
				CloseHandle(hSnapshot);
				return true;
			}
		} while (Module32NextW(hSnapshot, &me32));
	}

	CloseHandle(hSnapshot);
	return false;
}

bool CheckFileExistsWinAPI(const std::wstring& filePath) {
	DWORD attrib = GetFileAttributesW(filePath.c_str());
	return (attrib != INVALID_FILE_ATTRIBUTES &&
		!(attrib & FILE_ATTRIBUTE_DIRECTORY));
}
std::wstring GetModuleDir() {
	wchar_t path[MAX_PATH] = { 0 };
	GetModuleFileNameW(NULL, path, MAX_PATH);

	// 移除文件名，只保留目录
	PathRemoveFileSpecW(path);

	// 确保路径以 `\` 结尾
	std::wstring dirPath(path);
	if (!dirPath.empty() && dirPath.back() != L'\\') {
		dirPath += L'\\';
	}

	return dirPath;
}

std::wstring GetDllFullPath(const std::wstring& dllName) {
	return GetModuleDir() + dllName;
}
// 将错误码转换为可读的错误信息
std::wstring GetLastErrorString(DWORD errorCode = 0) {
	if (errorCode == 0) {
		errorCode = GetLastError();
	}

	LPWSTR messageBuffer = nullptr;
	DWORD bufferLength = FormatMessageW(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		errorCode,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPWSTR)&messageBuffer,
		0,
		NULL);

	std::wstring message;
	if (bufferLength != 0) {
		message = messageBuffer;
	}
	else {
		message = L"未知错误 (错误码: " + std::to_wstring(errorCode) + L")";
	}

	if (messageBuffer) {
		LocalFree(messageBuffer);
	}

	// 移除末尾的换行符
	if (!message.empty() && message.back() == L'\n') {
		message.pop_back();
	}
	if (!message.empty() && message.back() == L'\r') {
		message.pop_back();
	}

	return message;
}
bool _InjectDll(DWORD pid, const std::wstring& dllPath) {
	// 打开目标进程
	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
	// 检查进程架构匹配
	BOOL isTarget64 = FALSE;
	BOOL isInjector64 = FALSE;
	IsWow64Process(hProcess, &isTarget64);
	IsWow64Process(GetCurrentProcess(), &isInjector64);

	if (isTarget64 != isInjector64) {
		MessageBoxW(NULL, L"32/64位架构不匹配", L"错误", MB_OK);
		return false;
	}
	if (!hProcess) {
		DWORD err = GetLastError();
		std::wstring msg = L"OpenProcess 失败 (" + std::to_wstring(err) + L"): " + GetLastErrorString(err);
		MessageBoxW(NULL, msg.c_str(), NULL, MB_OK);
		return false;
	}

	// 分配远程内存
	LPVOID remoteStr = VirtualAllocEx(hProcess, NULL, (dllPath.size() + 1) * sizeof(wchar_t),
		MEM_COMMIT, PAGE_READWRITE);
	if (!remoteStr) {
		DWORD err = GetLastError();
		std::wstring msg = L"VirtualAllocEx 失败 (" + std::to_wstring(err) + L"): " + GetLastErrorString(err);
		MessageBoxW(NULL, msg.c_str(), NULL, MB_OK);
		CloseHandle(hProcess);
		return false;
	}

	// 写入DLL路径
	if (!WriteProcessMemory(hProcess, remoteStr, dllPath.c_str(),
		(dllPath.size() + 1) * sizeof(wchar_t), NULL)) {
		DWORD err = GetLastError();
		std::wstring msg = L"WriteProcessMemory 失败 (" + std::to_wstring(err) + L"): " + GetLastErrorString(err);
		MessageBoxW(NULL, msg.c_str(), NULL, MB_OK);
		VirtualFreeEx(hProcess, remoteStr, 0, MEM_RELEASE);
		CloseHandle(hProcess);
		return false;
	}

	// 获取LoadLibraryW地址
	HMODULE hKernel32 = GetModuleHandleW(L"kernel32.dll");
	FARPROC pLoadLibraryW = GetProcAddress(hKernel32, "LoadLibraryW");
	if (!pLoadLibraryW) {
		DWORD err = GetLastError();
		std::wstring msg = L"GetProcAddress 失败 (" + std::to_wstring(err) + L"): " + GetLastErrorString(err);
		MessageBoxW(NULL, msg.c_str(), NULL, MB_OK);
		VirtualFreeEx(hProcess, remoteStr, 0, MEM_RELEASE);
		CloseHandle(hProcess);
		return false;
	}

	// 创建远程线程
	HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0,
		(LPTHREAD_START_ROUTINE)pLoadLibraryW, remoteStr, 0, NULL);
	if (!hThread) {
		DWORD err = GetLastError();
		std::wstring msg = L"CreateRemoteThread 失败 (" + std::to_wstring(err) + L"): " + GetLastErrorString(err);
		MessageBoxW(NULL, msg.c_str(), NULL, MB_OK);
		VirtualFreeEx(hProcess, remoteStr, 0, MEM_RELEASE);
		CloseHandle(hProcess);
		return false;
	}

	// 等待线程结束
	WaitForSingleObject(hThread, INFINITE);

	// 检查加载结果
	DWORD exitCode = 0;
	GetExitCodeThread(hThread, &exitCode);

	if (exitCode == 0) {
		DWORD targetErr = 0;
		std::wstring msg;
		// 检查DLL是否存在
		if (GetFileAttributesW(dllPath.c_str()) == INVALID_FILE_ATTRIBUTES) {
			msg += L"- DLL文件不存在或不可访问\n";
		}
		if (GetExitCodeProcess(hProcess, &targetErr) && targetErr != 0) {
			msg += L"DLL加载失败 (" + std::to_wstring(targetErr) + L"): " + GetLastErrorString(targetErr);
		}
		else {
			MessageBoxW(NULL, L"DLL加载失败: 可能缺少依赖项或DLL初始化失败", L"错误", MB_OK);
		}
		MessageBoxW(NULL, msg.c_str(), NULL, MB_OK);

	}

	// 清理
	VirtualFreeEx(hProcess, remoteStr, 0, MEM_RELEASE);
	CloseHandle(hThread);
	CloseHandle(hProcess);

	return exitCode != 0;
}

void injectDLL(std::wstring process, std::wstring dllName) {

	DWORD pid = FindProcessId(process);

	if (pid == 0) {
		LOG_IMMEDIATE_ERROR("WeGame process not found.");
		return;
	}

	if (IsDllInjected(pid, dllName)) {
		LOG_IMMEDIATE_ERROR("DLL already injected.");
		return;
	}
	std::wstring fullPath = GetDllFullPath(dllName);
	//if (InjectDll(pid, fullPath)) {
	//if (_InjectDll(pid, GetDllFullPath(dllName))) {
	LOG_IMMEDIATE("注入的DLL:" + WStringToGBK(fullPath));

	//LOG_IMMEDIATE(stringTOwstring(WStringToGBK(GetDllFullPath(dllName)));

	if (!CheckFileExistsWinAPI(fullPath))
	{
		LOG_IMMEDIATE_ERROR("没有找到DLL文件:" + WStringToGBK(GetDllFullPath(dllName)));
		return;
	}
	//也许可以注入自身
	//if (_InjectDll(pid, L"C:/Users/Administrator/Desktop/cjmf/"+ dllName)) {
	if (_InjectDll(pid, fullPath)) {
		LOG_IMMEDIATE("DLL already Injected successfully...");
	}
	else {
		LOG_IMMEDIATE_ERROR(WStringToString(fullPath));
		LOG_IMMEDIATE_ERROR("DLL Injection failed.");
	}
}

std::string getComputerName() {
	return WStringToString(GetComputerNameWString());
}

std::map<std::wstring, std::wstring> getHeader() {
	std::map<std::wstring, std::wstring> HEADERS = {
		/*	{ L"Content-Type", L"application/json" },
			{ L"User-Agent", L"Mozilla/5.0" },
			{ L"token", L"{{bToken}}" },*/

			{   L"organizationType",L"\"BAR\""                                                                                                          },
			{   L"merchantId",L"53" },
			{   L"barId",L"98"                                                                                                                          },
			{   L"token",L"eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJSZW1vdGVJcCI6IiIsIkxvY2FsTG9naW4iOjAsIkNvbnRleHQiOnsidXNlcl9pZCI6MjQ3LCJ1c2VyX25hbWUiOiJ4eHgiLCJ1dWlkIjoiIiwicmlkIjoxOCwibWFudWZhY3R1cmVfaWQiOjUzLCJiYXJfaWQiOjk4LCJyb290X2lkIjowLCJvcmdhbml6YXRpb25fdHlwZSI6IiIsInBsYXRmb3JtIjoiYmFyY2xpZW50In0sImV4cCI6MTc1MjEzODc4N30.OxuSFEDQOq31KK9Vh-uwL9phsuV5zovluBptoNC3eXw"                                                                                                                  },
			{   L"User-Agent",L"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/136.0.0.0 Safari/537.36\r\n"        },
			{   L"language",L"ZH_CN" },
			{   L"sec-ch-ua-platform",L"\"Windows\""                                                                                                    },
			{   L"sec-ch-ua-mobile",L"?0"                                                                                                               },
			{   L"Accept",L"application/json, text/plain, */*"                                                                                          },
			{   L"Content-Type",L"application/json"                                                                                                     },
			//	{   L"Referer",L"https://dev-asz.cjmofang.com/activity/activityManagement/createActivity/MODE_SIGN/0/add/0"                                 },
			{   L"sec-ch-ua",L"\"Chromium\";v=\"136\", \"Google Chrome\";v=\"136\", \"Not.A/Brand\";v=\"99\""                                           },
			{   L"Accept-Encoding",L"gzip, deflate, br"                                                                                                 },
			{   L"Connection",L"keep-alive"                                                                                                             },
			{   L"Cache-Control",L"no-cache"                                                                                                            },
			{   L"Host",L"127.0.0.1:8000"                                                                                                                }
	};
	return HEADERS;

}




