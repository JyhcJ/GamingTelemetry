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
#include <iostream>
#include "HttpClient.h"
#include "constant.h"
#include <sstream>
#include <iomanip>
#include <openssl/evp.h>



extern std::map<std::wstring, std::wstring> HEADERS;



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

//std::string UTF8ToGBK(const std::string& strUTF8) {
//	// 1. UTF-8 → UTF-16 (宽字符)
//	int len = MultiByteToWideChar(CP_UTF8, 0, strUTF8.c_str(), -1, NULL, 0);
//	if (len <= 0) return "";
//	wchar_t* wstr = new wchar_t[len];
//	MultiByteToWideChar(CP_UTF8, 0, strUTF8.c_str(), -1, wstr, len);
//
//	// 2. UTF-16 → GBK
//	len = WideCharToMultiByte(CP_ACP, 0, wstr, -1, NULL, 0, NULL, NULL);
//	if (len <= 0) {
//		delete[] wstr;
//		return "";
//	}
//	char* str = new char[len];
//	WideCharToMultiByte(CP_ACP, 0, wstr, -1, str, len, NULL, NULL);
//
//	std::string result(str);
//	delete[] wstr;
//	delete[] str;
//	return result;
//}

std::string UTF8ToGBK(const std::string& strUTF8) {
	// 使用智能指针自动释放内存
	std::unique_ptr<wchar_t[]> wstr;
	std::unique_ptr<char[]> str;
	int len = 0;

	try {
		// 1. UTF-8 → UTF-16
		len = MultiByteToWideChar(CP_UTF8, 0, strUTF8.c_str(), -1, nullptr, 0);
		if (len <= 0) {
			throw std::runtime_error("MultiByteToWideChar failed with error: " + std::to_string(GetLastError()));
		}

		wstr = std::make_unique<wchar_t[]>(len);
		if (MultiByteToWideChar(CP_UTF8, 0, strUTF8.c_str(), -1, wstr.get(), len) == 0) {
			throw std::runtime_error("MultiByteToWideChar conversion failed with error: " + std::to_string(GetLastError()));
		}

		// 2. UTF-16 → GBK
		len = WideCharToMultiByte(CP_ACP, 0, wstr.get(), -1, nullptr, 0, nullptr, nullptr);
		if (len <= 0) {
			throw std::runtime_error("WideCharToMultiByte failed with error: " + std::to_string(GetLastError()));
		}

		str = std::make_unique<char[]>(len);
		if (WideCharToMultiByte(CP_ACP, 0, wstr.get(), -1, str.get(), len, nullptr, nullptr) == 0) {
			throw std::runtime_error("WideCharToMultiByte conversion failed with error: " + std::to_string(GetLastError()));
		}

		return std::string(str.get());
	}
	catch (const std::exception& e) {
		// 记录错误日志（实际使用时替换为您的日志系统）
		LOG_EXCEPTION_WITH_STACK(e)
		//LOG_ERROR(("UTF8ToGBK error: " + std::string(e.what()) + "\n").c_str());
		return ""; // 返回空字符串表示失败
	}
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
		//LOG_IMMEDIATE("Computer Name: " + hostName);

	}
	else {
		LOG_IMMEDIATE("gethostName :: Failed to get computer name. Error: " + GetLastError());
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
	try {
		HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		if (hSnapshot == INVALID_HANDLE_VALUE) {
			LOG_IMMEDIATE("------hSnapshot == INVALID_HANDLE_VALUE");
			return false;
		}

		PROCESSENTRY32W pe32;
		pe32.dwSize = sizeof(PROCESSENTRY32W);

		if (!Process32FirstW(hSnapshot, &pe32)) {
			CloseHandle(hSnapshot);
			LOG_IMMEDIATE("------Process32FirstW 失败 ");
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
	catch (const std::exception& e) {
		LOG_EXCEPTION_WITH_STACK(e);
		//LOG_IMMEDIATE("NarakaStateMonitor::OnClientStarted():" + std::string(e.what()));
		return false;
	}
	catch (...) {
		LOG_IMMEDIATE("common.cpp::IsProcessRunning::未知错误");
		return false;
	}
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

std::string GenerateUUID() {
	GUID guid;
	CoCreateGuid(&guid);

	char uuid[37]; // 格式：xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx + '\0'
	sprintf_s(uuid,
		"%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X",
		guid.Data1, guid.Data2, guid.Data3,
		guid.Data4[0], guid.Data4[1],
		guid.Data4[2], guid.Data4[3],
		guid.Data4[4], guid.Data4[5],
		guid.Data4[6], guid.Data4[7]);

	return std::string(uuid);
}

std::string getComputerName() {
	return WStringToString(GetComputerNameWString());
}

std::map<std::wstring, std::wstring> getHeader() {
	//std::map<std::wstring, std::wstring> HEADERS = {
	//	/*	{ L"Content-Type", L"application/json" },
	//		{ L"User-Agent", L"Mozilla/5.0" },
	//		{ L"token", L"{{bToken}}" },*/

	//		{   L"organizationType",L"\"BAR\""                                                                                                          },
	//		{   L"merchantId",L"53" },
	//		{   L"barId",L"98"                                                                                                                          },
	//		{   L"token",L"eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJSZW1vdGVJcCI6IiIsIkxvY2FsTG9naW4iOjAsIkNvbnRleHQiOnsidXNlcl9pZCI6MjQ3LCJ1c2VyX25hbWUiOiJ4eHgiLCJ1dWlkIjoiIiwicmlkIjoxOCwibWFudWZhY3R1cmVfaWQiOjUzLCJiYXJfaWQiOjk4LCJyb290X2lkIjowLCJvcmdhbml6YXRpb25fdHlwZSI6IiIsInBsYXRmb3JtIjoiYmFyY2xpZW50In0sImV4cCI6MTc1MzQyMjIzMX0.80UGut_XaJKn1h2G_Mr-XJ66ikTFDT4jtY4Mw9FKhPA"                                                                                                                  },
	//		{   L"User-Agent",L"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/136.0.0.0 Safari/537.36"        },
	//		{   L"language",L"ZH_CN" },
	//		{   L"sec-ch-ua-platform",L"\"Windows\""                                                                                                    },
	//		{   L"sec-ch-ua-mobile",L"?0"                                                                                                               },
	//		{   L"Accept",L"application/json, text/plain, */*"                                                                                          },
	//		{   L"Content-Type",L"application/json"                                                                                                     },
	//		//	{   L"Referer",L"https://dev-asz.cjmofang.com/activity/activityManagement/createActivity/MODE_SIGN/0/add/0"                                 },
	//		{   L"sec-ch-ua",L"\"Chromium\";v=\"136\", \"Google Chrome\";v=\"136\", \"Not.A/Brand\";v=\"99\""                                           },
	//		{   L"Accept-Encoding",L"gzip, deflate, br"                                                                                                 },
	//		{   L"Connection",L"keep-alive"                                                                                                             },
	//		{   L"Cache-Control",L"no-cache"                                                                                                            },
	//		{   L"Host",L"127.0.0.1:8000"                                                                                                                }
	//};
	return HEADERS;

}
std::map<std::wstring, std::wstring> getHeaderMD5(std::string jsonDump) {

	std::map<std::wstring, std::wstring> HEADERS_MD5 = HEADERS;

	std::string ret;
	_sendHttp(L"/api/client/GetGameConfig", "", ret);
	nlohmann::json jsonData1 = nlohmann::json::parse(ret);
	nlohmann::json jsonData2 = nlohmann::json::parse(remove_escape_chars(trim_quotes(jsonData1["metadata"]["value"].dump())));
	//LOG_IMMEDIATE("取到的value: " + jsonData1["metadata"]["value"].dump());
	//LOG_IMMEDIATE("去除首尾value: " + remove_escape_chars(trim_quotes(jsonData1["metadata"]["value"].dump())));
	//LOG_IMMEDIATE(jsonData2.dump() + "cjmofang.com.");
	//LOG_IMMEDIATE(generate_md5(jsonData2.dump() + "cjmofang.com."));
	HEADERS_MD5.emplace(L"sign", stringTOwstring(generate_md5(jsonData2.dump() + "cjmofang.com.")));
	LOG_IMMEDIATE("md5:" + generate_md5(jsonData2.dump() + "cjmofang.com."));
	return HEADERS_MD5;

}



std::string GetSteamInstallPath() {
	HKEY hKey;
	// 1. 修改为 HKEY_CURRENT_USER
	// 2. 使用宽字符版本(W)或ANSI版本(A)保持一致性
	if (RegOpenKeyExW(HKEY_CURRENT_USER, L"SOFTWARE\\Valve\\Steam", 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
		return "";
	}

	wchar_t steamPath[MAX_PATH];
	DWORD bufferSize = sizeof(steamPath);
	// 3. 保持宽字符版本一致性
	if (RegQueryValueExW(hKey, L"SteamPath", NULL, NULL, (LPBYTE)steamPath, &bufferSize) != ERROR_SUCCESS) {
		RegCloseKey(hKey);
		return "";
	}

	RegCloseKey(hKey);

	// 4. 将宽字符转换为多字节字符串
	char narrowPath[MAX_PATH];
	WideCharToMultiByte(CP_UTF8, 0, steamPath, -1, narrowPath, MAX_PATH, NULL, NULL);
	return narrowPath;
}
//[HKEY_LOCAL_MACHINE\SOFTWARE\Classes\wegame\DefaultIcon]
//@ = "O:\\网络游戏\\WeGame顺网专版\\wegame.exe"
std::string GetWGPath_REG() {
	HKEY hKey;
	if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Classes\\wegame\\DefaultIcon", 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
		return "";
	}

	wchar_t wgPath[MAX_PATH];
	DWORD bufferSize = sizeof(wgPath);
	// 3. 保持宽字符版本一致性
	if (RegQueryValueExW(hKey, L"", NULL, NULL, (LPBYTE)wgPath, &bufferSize) != ERROR_SUCCESS) {
		RegCloseKey(hKey);
		return "";
	}

	RegCloseKey(hKey);

	// 4. 将宽字符转换为多字节字符串
	char narrowPath[MAX_PATH];
	WideCharToMultiByte(CP_UTF8, 0, wgPath, -1, narrowPath, MAX_PATH, NULL, NULL);
	LOG_IMMEDIATE("Wegame路径: " + WStringToString(wgPath));
	return narrowPath;
}

std::string GetPath_REG(HKEY first, const WCHAR* reg, const WCHAR* name) {
	HKEY hKey;
	//if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\WeGame", 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
	if (RegOpenKeyExW(first, reg, 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
		std::wstring wstr(reg);  // 直接构造函数
		LOG_IMMEDIATE("没有找到注册表:" + WStringToString(wstr));
		return "";
	}

	wchar_t wgPath[MAX_PATH];
	DWORD bufferSize = sizeof(wgPath);
	// 3. 保持宽字符版本一致性
	if (RegQueryValueExW(hKey, name, NULL, NULL, (LPBYTE)wgPath, &bufferSize) != ERROR_SUCCESS) {
		RegCloseKey(hKey);
		std::wstring wstr1(reg);  // 直接构造函数
		std::wstring wstr2(name);  // 直接构造函数
		LOG_IMMEDIATE("注册表:" + WStringToString(wstr1) + "没有" + WStringToString(wstr2));
		return "";
	}

	RegCloseKey(hKey);

	std::string utf8Path = WStringToGBK(wgPath);
	LOG_IMMEDIATE("Val路径: " + utf8Path);
	return utf8Path;

}


bool WGRefresh(const std::wstring& exePath, const std::wstring& parameters) {
	STARTUPINFOW si = { sizeof(si) };
	PROCESS_INFORMATION pi;

	// 构造完整命令行（注意：第一个参数必须是 exe 路径）

	std::wstring cmdLine = L"\"" + exePath + L"\" " + parameters;
	std::vector<wchar_t> cmdLineBuf(cmdLine.begin(), cmdLine.end());
	cmdLineBuf.push_back(L'\0');  // 确保以 NULL 结尾
	if (!CreateProcessW(
		exePath.c_str(),      // 应用程序路径
		cmdLineBuf.data(),       // 命令行参数
		nullptr,             // 进程安全属性
		nullptr,             // 线程安全属性
		FALSE,               // 不继承句柄
		0,                  // 无特殊标志
		nullptr,             // 使用父进程环境变量
		nullptr,             // 使用父进程工作目录
		&si,                 // 启动信息
		&pi                 // 进程信息
	)) {
		std::cerr << "CreateProcess failed (" << GetLastError() << ")\n";
		return false;
	}

	// 关闭句柄（避免资源泄漏）
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	return true;
}

std::vector<std::string> ParseSteamLibraryPaths(const std::string& vdfPath) {
	std::vector<std::string> paths;
	std::ifstream file(vdfPath);
	if (!file.is_open()) {
		std::cerr << "无法打开文件: " << vdfPath << std::endl;
		return paths;
	}

	std::string line;
	std::regex pathRegex("\"path\"\\s*\"([^\"]+)\"");
	std::smatch match;

	while (std::getline(file, line)) {
		if (std::regex_search(line, match, pathRegex)) {
			paths.push_back(match[1].str());
		}
	}

	return paths;
}

std::string FindGamePath(const std::string& relativePath) {
	std::string steamPath = GetSteamInstallPath();
	LOG_IMMEDIATE("steamPath:" + steamPath);
	if (steamPath.empty()) {
		return "";
	}

	//std::string vdfPath = R"(E:\steam\steamapps\libraryfolders.vdf)"; // 这里换成你的路径
	std::string vdfPath = steamPath + "\\steamapps\\libraryfolders.vdf";
	auto paths = ParseSteamLibraryPaths(vdfPath);

	if (paths.empty()) {
		std::cout << "未找到任何路径\n";
	}
	else {
	
	}


	// 检查常见安装位置
	std::vector<std::string> possiblePaths = {
		steamPath + "\\steamapps\\common" + relativePath,
		steamPath + "\\steamapps\\common" + relativePath.substr(1), // 去掉开头的反斜杠
		"C:\\Program Files (x86)\\Steam\\steamapps\\common" + relativePath,
		"D:\\SteamLibrary\\steamapps\\common" + relativePath
	};

	for (auto& p : paths) {
		//std::cout << " - " << p << "\\steamapps\\common\\" << std::endl;
		LOG_IMMEDIATE("steam游戏路径:" + p);
		possiblePaths.emplace_back(p + "\\steamapps\\common" + relativePath);
		possiblePaths.emplace_back(p + "\\steamapps\\common" + relativePath.substr(1));
	}



	for (const auto& path : possiblePaths) {
		DWORD attrib = GetFileAttributesA(path.c_str());
		if (attrib != INVALID_FILE_ATTRIBUTES && (attrib & FILE_ATTRIBUTE_DIRECTORY)) {
			return path;
		}
	}

	return "";
}

BOOL Call_提升权限(BOOL bEnable) //OpenProcess失败的情况调用
{
	BOOL fOK = FALSE;
	HANDLE hToken;
	if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken)) //打开进程访问令牌
	{
		TOKEN_PRIVILEGES tp;
		LUID luid;
		LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &luid);
		tp.PrivilegeCount = 1;
		tp.Privileges[0].Luid = luid;
		tp.Privileges[0].Attributes = bEnable ? SE_PRIVILEGE_ENABLED : 0;
		AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), NULL, NULL);
		fOK = (GetLastError() == ERROR_SUCCESS);
		CloseHandle(hToken);
	}
	return fOK;
}

void EnableDebugPriv()
{

	HANDLE hToken;

	LUID sedebugnameValue;

	TOKEN_PRIVILEGES tkp;

	OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken);

	LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &sedebugnameValue);

	tkp.PrivilegeCount = 1;

	tkp.Privileges[0].Luid = sedebugnameValue;

	tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	AdjustTokenPrivileges(hToken, false, &tkp, sizeof tkp, NULL, NULL);

	CloseHandle(hToken);
}




BOOL EnableDebugPrivilege(BOOL bEnable) {
	HANDLE hToken;
	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
		DWORD err = GetLastError();
		printf("[!] OpenProcessToken failed (Error %d)\n", err);
		return FALSE;
	}

	TOKEN_PRIVILEGES tp = { 0 };
	tp.PrivilegeCount = 1;
	if (!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &tp.Privileges[0].Luid)) {
		DWORD err = GetLastError();
		printf("[!] LookupPrivilegeValue failed (Error %d)\n", err);
		CloseHandle(hToken);
		return FALSE;
	}

	tp.Privileges[0].Attributes = bEnable ? SE_PRIVILEGE_ENABLED : 0;
	if (!AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), NULL, NULL)) {
		DWORD err = GetLastError();
		printf("[!] AdjustTokenPrivileges failed (Error %d)\n", err);
		CloseHandle(hToken);
		return FALSE;
	}

	// 检查是否真正生效
	DWORD lastError = GetLastError();
	if (lastError == ERROR_NOT_ALL_ASSIGNED) {
		printf("[!] AdjustTokenPrivileges: Not all privileges assigned (Error %d)\n", lastError);
		CloseHandle(hToken);
		return FALSE;
	}

	CloseHandle(hToken);
	return (lastError == ERROR_SUCCESS);
}
// 将 GetLastError() 的错误码转换为错误信息字符串
std::string GetLastErrorAsString(std::string str, DWORD errorCode) {
	if (errorCode == 0) {
		errorCode = GetLastError(); // 如果没有传入错误码，自动获取最新的
	}

	LPSTR messageBuffer = nullptr;
	DWORD size = FormatMessageA(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |  // 自动分配缓冲区
		FORMAT_MESSAGE_FROM_SYSTEM |      // 从系统错误表获取消息
		FORMAT_MESSAGE_IGNORE_INSERTS,    // 忽略插入参数
		nullptr,                          // 无源字符串
		errorCode,                        // 错误码
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // 默认语言
		(LPSTR)&messageBuffer,            // 输出缓冲区
		0,                               // 最小缓冲区大小
		nullptr                          // 无参数
	);

	if (size == 0) {
		return "Failed to retrieve error message.";
	}

	std::string errorMessage(messageBuffer, size);
	LocalFree(messageBuffer); // 释放 FormatMessage 分配的缓冲区

	// 移除末尾的换行符（如果有）
	if (!errorMessage.empty() && errorMessage.back() == '\n') {
		errorMessage.pop_back();
	}
	if (!errorMessage.empty() && errorMessage.back() == '\r') {
		errorMessage.pop_back();
	}
	LOG_IMMEDIATE_ERROR("Error Code: " + std::to_string(errorCode) + " - " + errorMessage + "-" + str);
	return errorMessage;
}

void _sendHttp(std::wstring url, std::string jsonDump, std::string& ret) {
	HttpClient http;
	LOG_IMMEDIATE(jsonDump);
	LOG_IMMEDIATE(WStringToString( get_g_domain() + url));

	try {
		// 3. 发送POST请求
		std::string response = http.SendRequest(
			get_g_domain() + url,
			L"POST",
			getHeader(),
			jsonDump
		);

		LOG_IMMEDIATE("common:Response: " + UTF8ToGBK(response));
		ret = response;
	}
	catch (const std::exception& e) {
		LOG_IMMEDIATE_ERROR("_sendHttp:::");
		LOG_IMMEDIATE_ERROR(e.what());

	}
	catch (...) {  // 捕获其他所有异常
		LOG_IMMEDIATE_ERROR("_sendHttp :::Unknown exception occurred");
	}
}

DWORD GetProccessPath(DWORD pid, wchar_t* processName, DWORD size) {
	HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, 0, pid);
	if (!QueryFullProcessImageNameW(hProcess, 0, processName, (DWORD*)&size)) { size = 0; };
	CloseHandle(hProcess);
	return size;
}

std::pair<BYTE*, DWORD> GetModuleInfo(DWORD pid, std::wstring name)
{
	std::pair<BYTE*, DWORD> info;
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pid);
	if (snapshot != INVALID_HANDLE_VALUE) {
		MODULEENTRY32W modEntry = { sizeof(modEntry) };
		while (Module32NextW(snapshot, &modEntry)) {
			if (name == modEntry.szModule) { info = { (BYTE*)modEntry.modBaseAddr, modEntry.modBaseSize }; break; }
		}
	}
	return info;
}

int executeSilently(const char* cmd) {
	STARTUPINFOA si = { sizeof(si) };
	PROCESS_INFORMATION pi;
	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_HIDE;

	if (!CreateProcessA(
		NULL,
		(LPSTR)cmd,
		NULL,
		NULL,
		FALSE,
		CREATE_NO_WINDOW,
		NULL,
		NULL,
		&si,
		&pi))
	{
		std::cerr << "CreateProcess failed: " << GetLastError() << std::endl;
		return -1;
	}

	WaitForSingleObject(pi.hProcess, INFINITE);

	DWORD exitCode;
	GetExitCodeProcess(pi.hProcess, &exitCode);

	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	return (int)exitCode;
}

std::string GetEnvSafe(const char* key) {
	char* value = nullptr;
	size_t len = 0;
	std::string result1;
	try {


		// _dupenv_s 会分配内存，需要手动释放
		errno_t err = _dupenv_s(&value, &len, key);

		if (err != 0 || value == nullptr) {
			return "";
		}

		std::string result(value);  // 复制字符串
		result1 = result;
		free(value);                // 释放 _dupenv_s 分配的内存
	}
	catch (const std::exception& e) {
		LOG_IMMEDIATE("Exception in GetEnvSafe" + std::string(e.what()));
	}
	catch (...) {
		LOG_IMMEDIATE("Exception in GetEnvSafe未知异常");
	}
	return result1;
}


// 判断是否是安全字符（不需要编码）
bool IsSafeChar(unsigned char c) {
	if ((c >= 'a' && c <= 'z') ||
		(c >= 'A' && c <= 'Z') ||
		(c >= '0' && c <= '9') ||
		c == '-' || c == '_' || c == '.' || c == '~')
		return true;
	return false;
}

// URL 编码（支持中文 UTF-8）
std::string UrlEncode(const std::string& value) {
	std::ostringstream escaped;
	escaped.fill('0');
	escaped << std::hex;

	for (unsigned char c : value) {
		if (IsSafeChar(c)) {
			escaped << c;
		}
		else {
			escaped << '%' << std::setw(2) << int(c);
		}
	}

	return escaped.str();
}
int string_to_int(const std::string& str) {
	if (str.empty()) {
		throw std::invalid_argument("字符串为空");
	}

	try {
		size_t pos;
		int value = std::stoi(str, &pos);
		// 检查是否整个字符串都被转换（防止 "123abc" 被部分转换）
		if (pos != str.length()) {
			LOG_ERROR("字符串包含非数字字符");

		}
		return value;
	}
	catch (const std::exception& e) {
		LOG_ERROR("string_to_int 出错了");
	}
}
// 辅助函数：查找 map，找不到则返回 key 本身
const std::string& mapLookupOrDefault(const std::map<std::string, std::string>& m, const std::string& key) {
	auto it = m.find(key);
	return (it != m.end()) ? it->second : key;
}


const std::string mapLookupOrDefault(const std::map<int, std::string>& m, int key) {
	auto it = m.find(key);
	return (it != m.end()) ? it->second : std::to_string(key);
}

const int mapLookupOrDefault(const std::map<std::string, int>& m, std::string key) {
	auto it = m.find(key);
	if (it != m.end()) {
		return it->second;
	}
	else {
		return string_to_int(key);
	}
}

// 将单个 Unicode 码点转为 \uXXXX 格式
std::string codepointToUnicodeEscape(uint32_t codepoint) {
	std::stringstream ss;
	ss << "\\u" << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << codepoint;
	return ss.str();
}

// UTF-8 解码并转为 Unicode 转义字符串
std::string utf8ToUnicodeEscape(const std::string& utf8Str) {
	std::string result;
	result.reserve(utf8Str.size() * 6); // 预分配空间（每个中文最多6字节 \uXXXX）

	for (size_t i = 0; i < utf8Str.size(); ) {
		unsigned char c = utf8Str[i];

		uint32_t codepoint;
		int bytes;

		if (c < 0x80) {
			// 1字节 ASCII
			codepoint = c;
			bytes = 1;
		}
		else if ((c & 0xE0) == 0xC0 && i + 1 < utf8Str.size()) {
			// 2字节
			codepoint = ((c & 0x1F) << 6) | (utf8Str[i + 1] & 0x3F);
			bytes = 2;
		}
		else if ((c & 0xF0) == 0xE0 && i + 2 < utf8Str.size()) {
			// 3字节
			codepoint = ((c & 0x0F) << 12) |
				((utf8Str[i + 1] & 0x3F) << 6) |
				(utf8Str[i + 2] & 0x3F);
			bytes = 3;
		}
		else if ((c & 0xF8) == 0xF0 && i + 3 < utf8Str.size()) {
			// 4字节（需转为 UTF-16 代理对，但本题不涉及）
			// 简化：直接转为 \uXXXX 形式（但严格来说应拆为两个 \u）
			codepoint = ((c & 0x07) << 18) |
				((utf8Str[i + 1] & 0x3F) << 12) |
				((utf8Str[i + 2] & 0x3F) << 6) |
				(utf8Str[i + 3] & 0x3F);
			bytes = 4;
			// 注意：超出 U+FFFF 的码点应转为代理对，但本例不需要
		}
		else {
			// 无效 UTF-8，跳过
			i++;
			continue;
		}

		// 检查是否需要代理对（> 0xFFFF）
		if (codepoint > 0xFFFF) {
			// 转为 UTF-16 代理对
			codepoint -= 0x10000;
			uint32_t hi = 0xD800 + ((codepoint >> 10) & 0x3FF);
			uint32_t lo = 0xDC00 + (codepoint & 0x3FF);
			result += codepointToUnicodeEscape(hi);
			result += codepointToUnicodeEscape(lo);
		}
		else {
			result += codepointToUnicodeEscape(static_cast<uint16_t>(codepoint));
		}

		i += bytes;
	}

	return result;
}

std::string generate_md5(const std::string& input) {
	EVP_MD_CTX* ctx = EVP_MD_CTX_new();
	EVP_DigestInit_ex(ctx, EVP_md5(), nullptr);
	EVP_DigestUpdate(ctx, input.data(), input.size());

	unsigned char digest[EVP_MAX_MD_SIZE];
	unsigned int len;
	EVP_DigestFinal_ex(ctx, digest, &len);
	EVP_MD_CTX_free(ctx);

	std::string result;
	for (unsigned int i = 0; i < len; i++) {
		char buf[3];
		snprintf(buf, sizeof(buf), "%02x", digest[i]);
		result += buf;
	}
	return result;
}

// 获取文件最后修改时间
std::time_t get_last_write_time(const std::string& path) {
	WIN32_FILE_ATTRIBUTE_DATA fileInfo;
	if (GetFileAttributesExA(path.c_str(), GetFileExInfoStandard, &fileInfo)) {
		ULARGE_INTEGER ull;
		ull.LowPart = fileInfo.ftLastWriteTime.dwLowDateTime;
		ull.HighPart = fileInfo.ftLastWriteTime.dwHighDateTime;
		return ull.QuadPart / 10000000ULL - 11644473600ULL;
	}
	return 0;
}

// 获取最近创建的文件夹
std::unordered_set<std::string>	 get_recent_folders(const std::string& dir_path, int seconds_ago) {

	std::unordered_set<std::string>	 recent_folders;
	// 获取当前时间
	auto now = std::time(nullptr);

	std::string search_path = dir_path + "*";
	WIN32_FIND_DATAA findData;
	HANDLE hFind = FindFirstFileA(search_path.c_str(), &findData);

	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			std::string name = findData.cFileName;
			if (name == "." || name == "..") continue;

			std::string full_path = dir_path  + name;

			// 检查是否是目录
			if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) continue;

			// 获取最后修改时间
			time_t last_write = get_last_write_time(full_path);

			// 计算时间差
			double seconds_diff = difftime(now, last_write);

			if (seconds_diff <= seconds_ago) {
				recent_folders.insert(full_path);
				
			}
		} while (FindNextFileA(hFind, &findData) != 0);
		FindClose(hFind);
	}
	else {
		LOG_IMMEDIATE_ERROR("pubg:无法打开目录:" + dir_path);
	}

	return recent_folders;
}

//去除首尾"(如果有)
std::string trim_quotes(const std::string& str) {
	if (str.size() >= 2 && str.front() == '"' && str.back() == '"') {
		return str.substr(1, str.size() - 2);
	}
	return str;
}

//去除首尾
std::string trim_ic(const std::string& str) {
	if (str.size() >= 2 ) {
		return str.substr(1, str.size() - 2);
	}
	return str;
}

// 移除 \" 转义（替换成 "）
std::string remove_escape_chars(std::string str) {

	str.erase(std::remove(str.begin(), str.end(), '\\'), str.end());
	return str;
}

std::string readUtf8File(const std::string& filename) {
	std::ifstream file(filename, std::ios::binary);
	if (!file.is_open()) {
		throw std::runtime_error("Failed to open file: " + filename);
	}

	// 读取文件内容到字符串
	std::string content((std::istreambuf_iterator<char>(file)),
		std::istreambuf_iterator<char>());
	return content;
}


// 根据窗口句柄获取关联的进程名
std::wstring GetProcessNameFromWindow(HWND hwnd) {
	DWORD processId = 0;
	GetWindowThreadProcessId(hwnd, &processId);

	if (processId == 0) {
		return L"";
	}

	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnapshot == INVALID_HANDLE_VALUE) {
		return L"";
	}

	PROCESSENTRY32W pe32;
	pe32.dwSize = sizeof(PROCESSENTRY32W);

	std::wstring processName = L"";
	if (Process32FirstW(hSnapshot, &pe32)) {
		do {
			if (pe32.th32ProcessID == processId) {
				processName = pe32.szExeFile;
				break;
			}
		} while (Process32NextW(hSnapshot, &pe32));
	}

	CloseHandle(hSnapshot);
	return processName;
}

// 枚举窗口的回调函数
BOOL CALLBACK EnumWindowsCallback(HWND hwnd, LPARAM lParam) {
	const std::wstring* targetProcessName = reinterpret_cast<std::wstring*>(lParam);

	if (targetProcessName->c_str() == L"top") {
	
		return FALSE; // 找到并处理后停止枚举
	}
	else if(targetProcessName->c_str() == L"cancel"){
		SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0,
			SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
		return FALSE; // 找到并处理后停止枚举
	}

	// 跳过无效或不可见的窗口
	if (!IsWindowVisible(hwnd)) {
		return TRUE;
	}

	std::wstring windowProcessName = GetProcessNameFromWindow(hwnd);
	if (windowProcessName.empty()) {
		return TRUE;
	}

	// 比较进程名（不区分大小写）
	if (_wcsicmp(windowProcessName.c_str(), targetProcessName->c_str()) == 0) {
		// 检查窗口是否最小化
		if (IsIconic(hwnd)) {
			// 恢复最小化窗口
			ShowWindow(hwnd, SW_RESTORE);
		}
		// 确保窗口显示在最前面
		SetForegroundWindow(hwnd);
		SetFocus(hwnd);
		/*SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0,
			SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);*/
		std::wcout << L"已恢复并激活窗口: " << windowProcessName << std::endl;
		return FALSE; // 找到并处理后停止枚举
	}

	return TRUE; // 继续枚举
}

// 枚举窗口的回调函数
BOOL CALLBACK EnumWindowsCallback_Minimize(HWND hwnd, LPARAM lParam) {
	const std::wstring* targetProcessName = reinterpret_cast<std::wstring*>(lParam);

	// 跳过不可见窗口
	if (!IsWindowVisible(hwnd)) {
		return TRUE;
	}

	std::wstring windowProcessName = GetProcessNameFromWindow(hwnd);
	if (windowProcessName.empty()) {
		return TRUE;
	}

	// 匹配进程名（不区分大小写）
	if (_wcsicmp(windowProcessName.c_str(), targetProcessName->c_str()) == 0) {
		// 最小化窗口
		ShowWindow(hwnd, SW_MINIMIZE);
		std::wcout << L"WeGame 窗口已最小化: " << windowProcessName << std::endl;
		return FALSE; // 找到后停止枚举
	}

	return TRUE; // 继续枚举
}

// 主函数：最小化 WeGame 窗口
bool MinimizeWeGameWindow(std::wstring& processName) {
	// 枚举所有窗口并尝试最小化 WeGame
	return EnumWindows(EnumWindowsCallback_Minimize,
		reinterpret_cast<LPARAM>(const_cast<std::wstring*>(&processName)));
}

// 主函数：恢复指定进程名的窗口
bool RestoreWindowByProcessName(const std::wstring& processName) {
	// 枚举所有顶级窗口
	return EnumWindows(EnumWindowsCallback, reinterpret_cast<LPARAM>(const_cast<std::wstring*>(&processName)));
}

// 结束指定进程名的第一个实例
bool TerminateProcessByName(const std::wstring& processName) {
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnapshot == INVALID_HANDLE_VALUE) {
		std::wcerr << L"创建进程快照失败。\n";
		return false;
	}

	PROCESSENTRY32W pe32;
	pe32.dwSize = sizeof(PROCESSENTRY32W);

	bool found = false;
	if (Process32FirstW(hSnapshot, &pe32)) {
		do {
			// 比较进程名（不区分大小写）
			if (_wcsicmp(pe32.szExeFile, processName.c_str()) == 0) {
				// 尝试打开进程
				HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pe32.th32ProcessID);
				if (hProcess != NULL) {
					// 尝试终止进程
					if (TerminateProcess(hProcess, 0)) {
						std::wcout << L"成功终止进程: " << pe32.szExeFile
							<< L" (PID: " << pe32.th32ProcessID << L")\n";
						CloseHandle(hProcess);
						found = true;
						break; // 结束第一个匹配的进程
					}
					else {
						std::wcerr << L"无法终止进程 (PID: " << pe32.th32ProcessID
							<< L"), 错误代码: " << GetLastError() << L"\n";
					}
					CloseHandle(hProcess);
				}
				else {
					std::wcerr << L"无法打开进程 (PID: " << pe32.th32ProcessID
						<< L"), 错误代码: " << GetLastError() << L"\n";
				}
			}
		} while (Process32NextW(hSnapshot, &pe32));
	}
	else {
		std::wcerr << L"Process32First 失败，错误代码: " << GetLastError() << L"\n";
	}

	CloseHandle(hSnapshot);
	return found;
}
std::string GBKToUTF8(const std::string& strGBK) {
	// 1. GBK → UTF-16 (宽字符)
	int len = MultiByteToWideChar(936, 0, strGBK.c_str(), -1, NULL, 0);
	if (len <= 0) {
		return "";
	}
	wchar_t* wstr = new wchar_t[len];
	MultiByteToWideChar(936, 0, strGBK.c_str(), -1, wstr, len);

	// 2. UTF-16 → UTF-8
	len = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
	if (len <= 0) {
		delete[] wstr;
		return "";
	}
	char* str = new char[len];
	WideCharToMultiByte(CP_UTF8, 0, wstr, -1, str, len, NULL, NULL);

	std::string result(str);
	delete[] wstr;
	delete[] str;
	return result;
}
std::string SecondEncoding2UTF8(std::string garbled,std::string& toUTF8) {
	//std::string garbled = "娴锋兜鐨刯鎻℃妸1"; // 实际是 GBK 编码的字节序列

	// 假设 garbled 是 GBK 被误读为 UTF-8 的结果
	std::string realUTF8;
	for (char c : garbled) {
		realUTF8.push_back(static_cast<unsigned char>(c));
	}

	// 现在 realUTF8 是原始 GBK 字节流，可以正确转换为 UTF-8
	std::string gbk8Str = UTF8ToGBK(realUTF8); // 需要实现 GBKToUTF8
	toUTF8 = realUTF8;
	return gbk8Str;
}


