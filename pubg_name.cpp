
#include <vector>
#include <memory>
#include <windows.h>
#include <string>
#include <curl/curl.h>
#include <iostream>
#include <json/json.h>
#include <gdiplus.h> // 需要链接gdiplus.lib
#include "ThreadSafeLogger.h"
#include "common.h"
#include "pubg_name.h"
#include <filesystem>
#include <nlohmann/json.hpp>
#include "nloJson.h"

#pragma comment(lib, "gdiplus.lib")
// 获取name
#define IOCTL_READ_UNICODE_STRING CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)
// 写pid , 特征码
#define IOCTL_WRITE_UNICODE_STRING CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_WRITE_DATA)



std::string client_id = "8PzluC7AF1Rh1nqInVSdkdfv";
std::string client_secret = "1eZyKcVayoWTfBYjTU8tL7VHF2RfIDre";

int GetEncoderClsid(const WCHAR* format, CLSID* pClsid);
// 检查是否符合PUBG命名规则
bool isValidPubgName(const std::string& name) {
	if (name.length() < 3 || name.length() > 16) return false;

	//for (char c : name) {
	//    if (!isalnum(c) && c != '_' && c != '-' && c != ' ') {
	//        return false;
	//    }
	//}
	return true;
}

// 从OCR结果中过滤出有效的PUBG名称
std::vector<std::string> filterPubgNames(const Json::Value& wordsResult) {
	std::vector<std::string> validNames;

	for (const auto& item : wordsResult) {
		std::string word = item["words"].asString();
		if (isValidPubgName(word)) {
			validNames.push_back(word);
		}
	}

	return validNames;
}
struct WordLocation {
	std::string word;
	int left;
	int width;
	int centerX;
};

// 获取带位置信息的单词
std::vector<WordLocation> getWordsWithLocation(const Json::Value& wordsResult) {
	std::vector<WordLocation> words;

	for (const auto& item : wordsResult) {
		WordLocation wl;
		wl.word = item["words"].asString();
		wl.left = item["location"]["left"].asInt();
		wl.width = item["location"]["width"].asInt();
		wl.centerX = wl.left + wl.width / 2;
		words.push_back(wl);
	}

	return words;
}

// 选择水平居中且有效的PUBG名称
std::string selectCenterPubgName(const std::vector<WordLocation>& words, int imageWidth) {
	if (words.empty()) return "";

	int imageCenter = imageWidth / 2;
	int minDistance = INT_MAX;
	std::string selectedName;

	for (const auto& wl : words) {
		if (isValidPubgName(wl.word)) {
			int distance = abs(wl.centerX - imageCenter);
			if (distance < minDistance) {
				minDistance = distance;
				selectedName = wl.word;
			}
		}
	}

	return selectedName;
}
//Json::Value callDetailedOcrApi(const std::vector<uint8_t>& imageData) {
//    // 构建请求
//    std::string imageBase64 = Base64Encode(imageData.data(), imageData.size(), true);
//    std::string data = "image=" + imageBase64 + "&recognize_granularity=big&language_type=ENG";
//
//    // 发送请求（使用你现有的curl代码）
//    std::string response = sendOcrRequest(data);
//
//    // 解析JSON响应
//    Json::Value root;
//    Json::CharReaderBuilder readerBuilder;
//    std::istringstream jsonStream(response);
//    std::string parseErrors;
//
//    if (Json::parseFromStream(readerBuilder, jsonStream, &root, &parseErrors)) {
//        return root;
//    }
//
//    return Json::Value();  // 返回空值表示失败
//}

//std::string processOcrResult(const Json::Value& ocrResult, int imageWidth) {
//    // 第一次尝试：简单过滤
//    std::vector<std::string> validNames = filterPubgNames(ocrResult["words_result"]);
//
//    if (validNames.size() == 1) {
//        return validNames[0];  // 唯一有效名称
//    }
//    else if (validNames.size() > 1) {
//        // 多个有效名称，需要带位置信息的OCR
//        Json::Value detailedOcrResult = callDetailedOcrApi();  // 调用带位置信息的OCR API
//
//        // 处理带位置信息的结果
//        std::vector<WordLocation> locatedWords = getWordsWithLocation(detailedOcrResult["words_result"]);
//        std::string centerName = selectCenterPubgName(locatedWords, imageWidth);
//
//        if (!centerName.empty()) {
//            return centerName;
//        }
//    }
//
//    // 没有找到有效名称或选择失败
//    return "";
//}

RECT CalculateCaptureRegion(int windowWidth, int windowHeight) {
	// 原始比例下的区域 (564,135,1480,271) @ 1920x1080
	const double x1_ratio = 564.0 / 1920.0;
	const double y1_ratio = 135.0 / 1080.0;
	const double x2_ratio = 1480.0 / 1920.0;
	const double y2_ratio = 271.0 / 1080.0;

	RECT rect;
	rect.left = static_cast<int>(windowWidth * x1_ratio);
	rect.top = static_cast<int>(windowHeight * y1_ratio);
	rect.right = static_cast<int>(windowWidth * x2_ratio);
	rect.bottom = static_cast<int>(windowHeight * y2_ratio);

	return rect;
}
inline size_t onWriteData(void* buffer, size_t size, size_t nmemb, void* userp)
{
	std::string* str = dynamic_cast<std::string*>((std::string*)userp);
	str->append((char*)buffer, size * nmemb);
	return nmemb;
}
std::string getAccessToken()
{
	std::string result;
	CURL* curl;
	CURLcode res;
	curl = curl_easy_init();
	if (curl)
	{
		curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
		curl_easy_setopt(curl, CURLOPT_URL, "https://aip.baidubce.com/oauth/2.0/token");
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
		curl_easy_setopt(curl, CURLOPT_DEFAULT_PROTOCOL, "https");
		struct curl_slist* headers = NULL;
		headers = curl_slist_append(headers, "Content-Type: application/x-www-form-urlencoded");
		headers = curl_slist_append(headers, "Accept: application/json");
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
		std::string data = "grant_type=client_credentials&client_id=" + client_id + "&client_secret=" + client_secret;
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &result);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, onWriteData);
		res = curl_easy_perform(curl);
	}
	curl_easy_cleanup(curl);
	Json::Value obj;
	std::string error;
	Json::CharReaderBuilder crbuilder;
	std::unique_ptr<Json::CharReader> reader(crbuilder.newCharReader());
	reader->parse(result.data(), result.data() + result.size(), &obj, &error);
	return obj["access_token"].asString();
}
std::string Base64Encode(const uint8_t* data, size_t length, bool urlencoded) {
	std::string ret;
	ret.reserve((length + 2) / 3 * 4);

	const std::string base64_chars =
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"abcdefghijklmnopqrstuvwxyz"
		"0123456789+/";

	int i = 0;
	int j = 0;
	unsigned char char_array_3[3];
	unsigned char char_array_4[4];

	while (length--) {
		char_array_3[i++] = *(data++);
		if (i == 3) {
			char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
			char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
			char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
			char_array_4[3] = char_array_3[2] & 0x3f;

			for (i = 0; i < 4; i++) {
				ret += base64_chars[char_array_4[i]];
			}
			i = 0;
		}
	}

	if (i) {
		for (j = i; j < 3; j++) {
			char_array_3[j] = '\0';
		}

		char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
		char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
		char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
		char_array_4[3] = char_array_3[2] & 0x3f;

		for (j = 0; j < i + 1; j++) {
			ret += base64_chars[char_array_4[j]];
		}

		while (i++ < 3) {
			ret += '=';
		}
	}

	if (urlencoded) {
		char* escaped = curl_escape(ret.c_str(), ret.length());
		if (escaped) {
			ret = escaped;
			curl_free(escaped);
		}
	}

	return ret;
}
// 初始化GDI+
struct GdiplusInitializer {
	Gdiplus::GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR gdiplusToken;

	GdiplusInitializer() {
		Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
	}

	~GdiplusInitializer() {
		Gdiplus::GdiplusShutdown(gdiplusToken);
	}
};

// 截图并返回PNG格式的内存数据
std::vector<BYTE> CaptureWindowToPNG(HWND hwnd, const RECT& region) {
	static GdiplusInitializer gdiplus; // 静态初始化，只执行一次

	int width = region.right - region.left;
	int height = region.bottom - region.top;

	HDC hdcWindow = GetDC(hwnd);
	HDC hdcMem = CreateCompatibleDC(hdcWindow);
	HBITMAP hBitmap = CreateCompatibleBitmap(hdcWindow, width, height);
	SelectObject(hdcMem, hBitmap);

	// 执行截图
	BitBlt(hdcMem, 0, 0, width, height, hdcWindow, region.left, region.top, SRCCOPY);

	// 使用GDI+将位图转换为PNG
	Gdiplus::Bitmap bitmap(hBitmap, NULL);

	// 准备内存流
	IStream* pStream = NULL;
	CreateStreamOnHGlobal(NULL, TRUE, &pStream);

	// 保存为PNG到内存流
	CLSID pngClsid;
	GetEncoderClsid(L"image/png", &pngClsid);
	bitmap.Save(pStream, &pngClsid, NULL);

	// 获取流数据
	HGLOBAL hGlobal = NULL;
	GetHGlobalFromStream(pStream, &hGlobal);
	BYTE* pData = (BYTE*)GlobalLock(hGlobal);
	DWORD size = GlobalSize(hGlobal);

	std::vector<BYTE> pngData(pData, pData + size);

	// 清理资源
	GlobalUnlock(hGlobal);
	pStream->Release();
	DeleteObject(hBitmap);
	DeleteDC(hdcMem);
	ReleaseDC(hwnd, hdcWindow);

	return pngData;
}

// 获取PNG编码器的CLSID
int GetEncoderClsid(const WCHAR* format, CLSID* pClsid) {
	UINT num = 0;
	UINT size = 0;
	Gdiplus::GetImageEncodersSize(&num, &size);
	if (size == 0) return -1;

	Gdiplus::ImageCodecInfo* pImageCodecInfo = (Gdiplus::ImageCodecInfo*)malloc(size);
	if (!pImageCodecInfo) return -1;

	Gdiplus::GetImageEncoders(num, size, pImageCodecInfo);

	for (UINT i = 0; i < num; ++i) {
		if (wcscmp(pImageCodecInfo[i].MimeType, format) == 0) {
			*pClsid = pImageCodecInfo[i].Clsid;
			free(pImageCodecInfo);
			return i;
		}
	}

	free(pImageCodecInfo);
	return -1;
}

std::string sendOcrDirect(const std::vector<BYTE>& imageData, int imageWidth, bool isPng = true) {
	std::string result = "";

	// Base64编码
	std::string imageBase64 = Base64Encode(imageData.data(), imageData.size(), true);

	CURL* curl = curl_easy_init();
	if (curl) {
		curl_easy_setopt(curl, CURLOPT_URL,
			("https://aip.baidubce.com/rest/2.0/ocr/v1/general?access_token=" + getAccessToken()).c_str());
		//https://aip.baidubce.com/rest/2.0/ocr/v1/general_basic
		struct curl_slist* headers = NULL;
		headers = curl_slist_append(headers, "Content-Type: application/x-www-form-urlencoded");
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

		std::string data = "image=" + imageBase64 + "&language_type=ENG&probability=true";
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, onWriteData);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &result);

		CURLcode res = curl_easy_perform(curl);
		if (res != CURLE_OK) {
			std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
		}
		else {
			// 成功处理返回结果
			Json::Value root;
			Json::CharReaderBuilder readerBuilder;
			std::istringstream jsonStream(result);
			std::string parseErrors;

			if (Json::parseFromStream(readerBuilder, jsonStream, &root, &parseErrors)) {
				// 获取图像宽度（假设你知道图像宽度，或者可以从其他途径获取）

				int imageWidth = 800; // 替换为实际的图像宽度

				// 筛选符合PUBG命名规则的单词并计算其中点位置
				std::vector<std::pair<std::string, int>> validWords; // <word, centerX>

				for (const auto& item : root["words_result"]) {

					std::string word = item["words"].asString();

					//// 检查是否符合PUBG命名规则
					if (isValidPubgName(word)) {
						// 计算单词的水平中心位置

						int left = item["location"]["left"].asInt();
						int width = item["location"]["width"].asInt();
						int centerX = left + width / 2;
						double score = item["probability"]["average"].asDouble();
						if (score > 0.5) {
							validWords.emplace_back(word, centerX);
						}
					}
				}

				if (!validWords.empty()) {
					// 找到最接近图像中心的单词
					int imageCenter = imageWidth / 2;
					auto closest = std::min_element(validWords.begin(), validWords.end(),
						[imageCenter](const auto& a, const auto& b) {
							return abs(a.second - imageCenter) < abs(b.second - imageCenter);
						});

					std::string selectedName = closest->first;
					std::cout << "Selected player name: " << selectedName << std::endl;

					// 你可以在这里使用 selectedName 做进一步处理
					result = selectedName; // 或者修改返回值为选中的名称
				}
				else {
					std::cerr << "No valid PUBG names found in OCR results" << std::endl;
				}
			}
			else {
				std::cerr << "Failed to parse JSON response: " << parseErrors << std::endl;
			}


		}

		curl_slist_free_all(headers);
		curl_easy_cleanup(curl);
	}



	return result;
}

//1
int loadDriver() {
	wchar_t strpath[1024] = { 0 };
	GetCurrentDirectory(1024, strpath);
	wcscat_s(strpath, L"\\KMDFDriver2.sys");
	//wcscpy_s(strpath, L"C:\\Users\\Administrator\\Desktop\\injectTest\\KMDFDriver2\\x64\\Debug\\KMDFDriver2.sys");

	SC_HANDLE hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (!hSCManager) {
		GetLastErrorAsString("OpenSCManager");
		return -1;
	}

	const wchar_t* DriverName = L"KMDFDriver2";
	SC_HANDLE hService = CreateService(hSCManager,
		DriverName,
		DriverName,
		SERVICE_ALL_ACCESS,
		SERVICE_KERNEL_DRIVER,
		SERVICE_DEMAND_START,
		SERVICE_ERROR_NORMAL,
		strpath,
		NULL, NULL, NULL, NULL, NULL);

	if (!hService) {
		GetLastErrorAsString("CreateService");
		// 服务可能已经存在，尝试打开
		hService = OpenService(hSCManager, DriverName, SERVICE_ALL_ACCESS);
		if (!hService) {
			GetLastErrorAsString("OpenService");
			CloseServiceHandle(hSCManager);
			return -1;
		}
	}

	int result = StartService(hService, 0, NULL);
	if (result == 0) {
		GetLastErrorAsString("StartService");
		CloseServiceHandle(hService);
		CloseServiceHandle(hSCManager);
		return 0;//返回0 看getLastError
	}

	std::wcout << L"驱动加载成功: " << DriverName << std::endl;

	CloseServiceHandle(hService);
	CloseServiceHandle(hSCManager);
	return 1;
}

BOOL StopDriver(LPCTSTR DriverId) {
	SC_HANDLE hSCManager = NULL;
	SC_HANDLE hService = NULL;
	BOOL rCode = FALSE;
	SERVICE_STATUS serviceStatus;
	DWORD error = NO_ERROR;
	hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (!hSCManager)
	{
		return FALSE;
	}
	hService = OpenService(hSCManager, DriverId, SERVICE_ALL_ACCESS);
	if (hService != NULL) {
		rCode = ControlService(hService, SERVICE_CONTROL_STOP, &serviceStatus);
		error = GetLastError();
		CloseServiceHandle(hService);
	}
	CloseServiceHandle(hSCManager);
	return rCode;
}

BOOL RemoveDriver(LPCTSTR DriverId) {
	SC_HANDLE hSCManager = NULL;
	SC_HANDLE hService = NULL;
	BOOL rCode = FALSE;

	hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (!hSCManager) {
		return FALSE;
	}
	hService = OpenService(hSCManager, DriverId, SERVICE_ALL_ACCESS);
	if (hService == NULL)
	{
		rCode = TRUE;
	}
	else {
		rCode = DeleteService(hService);
		CloseServiceHandle(hService);
	}
	CloseServiceHandle(hSCManager);
	return rCode;
}
//3
bool driverUpdate(GENERAL_CONSTRUCTION gc_in, GENERAL_CONSTRUCTION& gc_out, DWORD fun) {

	DWORD pid = 0;
	HWND hWnd = FindWindowW(L"UnrealWindow", nullptr);
	if (!hWnd) { return false; };
	GetWindowThreadProcessId(hWnd, (DWORD*)(&pid));
	if (!pid) { return false; };
	Call_提升权限(true);
	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
	//特征码:
	std::wstring featureCode = L"PUBG-Feature-Code"; // 替换为实际的特征码

	std::string response;
	//_sendHttp(L"asz.cjmofang.com/api/client/JuediqiushengGetAddress", "", response);
	response = "123";
	if (response.empty() || response =="")
	{
		LOG_IMMEDIATE("获取地址失败，请检查网络连接或服务器状态。");
		return false;
	}
	else {
		LOG_IMMEDIATE("服务器获取的地址:"+ response);
	}

	//nlohmann::json jsonRes = nlohmann::json::parse(response);
	nlohmann::json jsonTest;
	//jsonTest["type"] = "moudle";
	jsonTest["type"] = "address";
	//jsonTest["value"] = "TslGame.exe"; //[[[[7FF6D7DE7C30]+0]+290]+0]

	jsonTest["value"] = "0x7FF6D7DE7C30"; //[[[[7FF6D7DE7C30]+0]+290]+0]
	//jsonTest["offset"].push_back(0x10D77C30);
	jsonTest["offset"].push_back(0);
	jsonTest["offset"].push_back(0x290);
	jsonTest["offset"].push_back(0);
	LOG_IMMEDIATE(jsonTest.dump(4));

	std::string type = getNestedValue<std::string>(jsonTest, { "type" }, "error");
	std::wstring value;
	std::string address;
	value = stringTOwstring(getNestedValue<std::string>(jsonTest, { "value" }, "error"));
	

	if (type == "moudle") {
		
		std::pair<BYTE*, DWORD> baseAndSize = GetModuleInfo(pid, value);
		gc_in.mr.base_address = reinterpret_cast<ULONG64>(baseAndSize.first);
	}
	else {
		try {
			MEMORY_REQUEST req = { 0 };
			req.base_address = std::stoull(value, nullptr, 0); // 0 表示自动检测进制
			req.offset_count = jsonTest["offset"].size();
			// 遍历 offset 数组
			int i = 0;
			for (const auto& offset : jsonTest["offset"]) {
				req.offsets[i] = offset;
				i++;
			}
			req.buffer_size = sizeof(WCHAR) * 32;
			gc_in.mr = req;
		}
		catch (const std::invalid_argument& e) {
			LOG_IMMEDIATE("无效参数:" + std::string(e.what()));
			return false;
		}
		catch (const std::out_of_range& e) {
			LOG_IMMEDIATE("数值溢出:" + std::string(e.what()));
			return false;
		}
	}

	// DLL 代码
	HANDLE hDevice = CreateFileW(
		L"\\\\.\\KMDFDriver2", GENERIC_READ | GENERIC_WRITE,
		0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL
	);


	if (hDevice != INVALID_HANDLE_VALUE) {

		gc_in.ProcessId = pid;
		wcsncpy_s(gc_in.FeatureCode, featureCode.c_str(), _TRUNCATE);



		DWORD bytesReturned;
		BOOL success = DeviceIoControl(
			hDevice, IOCTL_WRITE_UNICODE_STRING,
			&gc_in, sizeof(gc_in), &gc_out, sizeof(gc_out), &bytesReturned, NULL
		);
		if (success) {
			// 成功发送 PID 和 featureCode 到驱动
			return true;
		}
		else {
			GetLastErrorAsString("driverUpdate - DeviceIoControl -");
			return false;
		}
		CloseHandle(hDevice);
		// buffer 中为Unicode字符串
	}
	else {
		GetLastErrorAsString("driverUpdate - CreateFileW -");
		return false;
	}
}
//4
std::string driverGetPlayerName(GENERAL_CONSTRUCTION gc_in, GENERAL_CONSTRUCTION& gc_out, DWORD fun) {
	// DLL 代码
	HANDLE hDevice = CreateFileW(
		L"\\\\.\\KMDFDriver2", GENERIC_READ | GENERIC_WRITE,
		0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL
	);

	if (hDevice != INVALID_HANDLE_VALUE) {
		DWORD bytesReturned;
		BOOL success = DeviceIoControl(
			//hDevice, IOCTL_WRITE_UNICODE_STRING,
			hDevice, IOCTL_READ_UNICODE_STRING,
			&gc_in, sizeof(gc_in), &gc_out, sizeof(gc_out), &bytesReturned, NULL
		);
		if (success) {
			std::wstring wsPlayerName(gc_out.PlayerName); // 直接构造
			LOG_IMMEDIATE("获取到pubgName:" + WStringToString(wsPlayerName));
			return WStringToString(wsPlayerName);
		}
		else {
			return ""; // 失败时返回空字符串
		}
		CloseHandle(hDevice);
		// buffer 中为Unicode字符串
	}
}

std::string getPlayerNamePUBG() {
	while (true) {

		HWND hwnd = FindWindowW(L"UnrealWindow", nullptr);
		if (!hwnd) {
			std::this_thread::sleep_for(std::chrono::seconds(3));
			continue;
		}
		// 检查窗口是否是活动窗口
		if (GetForegroundWindow() != hwnd) {
			//std::cout << "目标窗口不是活动窗口" << std::endl;
			std::this_thread::sleep_for(std::chrono::seconds(3));
			continue;
		}
		RECT windowRect;
		GetClientRect(hwnd, &windowRect);
		RECT captureRect = CalculateCaptureRegion(windowRect.right, windowRect.bottom);

		LOG_IMMEDIATE("PUBG 截图并识别");
		// 截图并获取PNG数据
		auto pngData = CaptureWindowToPNG(hwnd, captureRect);

		// 直接发送OCR
		std::string ocrResult = sendOcrDirect(pngData, captureRect.right - captureRect.left);

		//std::cout << "OCR结果: " << ocrResult << std::endl;

		LOG_IMMEDIATE("PUBG Name : " + ocrResult);

		if (ocrResult != "") {

			return ocrResult;
		}



		std::this_thread::sleep_for(std::chrono::seconds(5));
		//return ocrResult;
	}
}

