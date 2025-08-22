#include <thread>
#include "common.h"
#include "ThreadSafeLogger.h"
#include <iostream>
#include "HttpClient.h"
#include "nloJson.h"
#include "constant.h"
#include "delta.h"

#include <mutex>
#include <condition_variable>

std::mutex mtx;

std::mutex g_wgHeader_mutex;
std::string g_wgHeader;

std::condition_variable cv;
std::chrono::steady_clock::time_point last_event_time;
std::atomic<bool> has_new_battle_record{ false };
std::atomic<bool> token_expired{ false };
std::atomic<bool> isUpdating{ false };





// 读取
std::string get_g_wgHeader() {
	std::lock_guard<std::mutex> header_lock(g_wgHeader_mutex);
	return g_wgHeader;
};

void set_g_wgHeader(const std::string& wgHeader) {
	std::lock_guard<std::mutex> header_lock(g_wgHeader_mutex);
	g_wgHeader = wgHeader;
}


int refreshToken(std::wstring processName) {
	try {
		isUpdating = true;
		const char* exePath = "\".\\build\\pkg\\pythonEmbed\\pythonw.exe\" dumpMain.pyw";
		std::thread([processName]() {
			std::wstring temp = stringTOwstring(GetWGPath_REG());
			//std::wstring temp = L"O:\\网络游戏\\WeGame顺网专版\\wegame.exe";
			std::this_thread::sleep_for(std::chrono::seconds(4));
			WGRefresh(temp, L"/StartFor=2001715");
			std::this_thread::sleep_for(std::chrono::seconds(3));
			WGRefresh(temp, L"/StartFor=2001918");

			// 最小化 wegame
			//std::wstring wgProcess = L"wegame.exe";  // 对象在作用域内
			//MinimizeWeGameWindow(wgProcess);
			// 最大化游戏窗口
			RestoreWindowByProcessName(processName);

			}).detach();
		TerminateProcessByName(L"python.exe");
		LOG_IMMEDIATE("-------------------minpoxyR-------------------");
		int result = executeSilently(exePath);
		if (result == -1) {
			LOG_IMMEDIATE("本地目录不存在python环境,尝试自动部署的python环境:" + std::to_string(result));
			const char* exePath = "\"C:\\Program Files\\Python313\\pythonw.exe\" dumpMain.pyw";
			result = executeSilently(exePath);
			if (result == -1) {
				LOG_IMMEDIATE("自动部署的python环境失败:" + std::to_string(result));
				return -1;
			}
		}

		LOG_IMMEDIATE("-------------------minpoxy:result-------------------" + std::to_string(result));
		LOG_IMMEDIATE("-------------------minpoxyE-------------------");

		std::this_thread::sleep_for(std::chrono::milliseconds(3000));
		// 间隔发送请求查询是否有新增战绩 ->有战绩 ->更新token

			//查询wegame信息 , 查战绩.
		nlohmann::json data;

		//[2025-08-12 14:50:55] [INFO] {"result":{"error_message":"登录信息过期","error_code":8000102}}


		if (result == 0 && is_file_exists_and_not_empty("outfileDecode")) {

			std::string content = readUtf8File("outfileDecode");
			std::ifstream f("outfileDecode");
			data = nlohmann::json::parse(f);
			//analysisStr = data.dump();

			if (!data.empty())
			{
				LOG_IMMEDIATE("mitmproxy捕获的请求有信息");
			}
			else {
				LOG_INFO("mitmproxy捕获的请求为空");
			}

		}
		else {
			LOG_IMMEDIATE_ERROR("delta:mitmproxy程序运行失败或返回错误码：" + result);
		}

		std::this_thread::sleep_for(std::chrono::seconds(5));
 		nlohmann::json p_header;
		//nlohmann::json p_responseJson;

		for (auto& flow : data) {
			// 请求部分
			auto& request = flow["request"];
			std::string str = request.value("url", "N/A");
			size_t found_pos = str.find("/api/v1/wegame.base.game.CommConfig/GetCfg");
			size_t found_pos1 = str.find("/api/v1/wegame.pallas.game.ValAssist/GetNewbieInfo");
			if (found_pos == std::string::npos && found_pos1 == std::string::npos) {
				//LOG_IMMEDIATE("未捕获到常规请求");
				continue;
			}
			//std::cout << "===== Flow =====\n";
			//std::cout << "Request URL: " << request.value("url", "N/A") << "\n";
			//std::cout << "Method: " << request.value("method", "N/A") << "\n";

			// 打印 headers
			//std::cout << "Headers:\n";
			//std::string cookies = request["headers"]["cookie"];
			std::string cookies = getNestedValue<std::string>(request, { "headers" ,"cookie"},"error");
			if (cookies == "error") {
				cookies = getNestedValue<std::string>(request, { "headers" ,"Cookie" }, "error");
			}
			//nlohmann::json::iterator it = request["headers"].find("cookie");

			// 替换所有 ',' 为 ';'
			std::replace(cookies.begin(), cookies.end(), ',', ';');

			request["headers"]["cookie"] = cookies;
			// 输出结果
			std::cout << "Updated Cookie:\n" << cookies << std::endl;
		/*	for (auto it = request["headers"].begin(); it != request["headers"].end(); ++it) {
				std::cout << "  " << it.key() << ": " << it.value() << "\n";
			}*/

			p_header = request["headers"];
			//analysisStr = p_header.dump();
			set_g_wgHeader(p_header.dump());
			// 打印请求体
			//std::cout << "Request Content: " << request.value("content", "N/A") << "\n\n";


			// 响应部分
			const auto& response = flow["response"];
			std::cout << "Response Status Code: " << response.value("status_code", 0) << "\n";

			// 打印响应 headers
			//std::cout << "Response Headers:\n";
			//for (auto it = response["headers"].begin(); it != response["headers"].end(); ++it) {
			//	std::cout << "  " << it.key() << ": " << it.value() << "\n";
			//}
			// 打印响应内容
			std::cout << "Response Content: " << response.value("content", "N/A") << "\n";
			break;
			//std::cout << "对比一下呢" << request.dump() << "\n";


		}
		isUpdating = false;
		return 1;
	}
	catch (const std::exception& e) {
		LOG_IMMEDIATE_ERROR("运行mitmproxy出现了异常:" + std::string(e.what()));
		isUpdating = false;
		return 0;
	}


}


int main_delta() {
	const std::wstring gameName = L"DeltaForceClient-Win64-Shipping.exe";
	const std::wstring url = L"/api/client/SanjiaozhouPostGameData";

	// 检查wegame路径

	// 启动脚本

	std::thread([gameName, url]() {
		// 初始化计时器
		last_event_time = std::chrono::steady_clock::now();
		int init_count[2] = { 0,0 };
		bool lastState = false;
		while (true) {
			std::this_thread::sleep_for(std::chrono::seconds(2));

			if (IsProcessRunning(gameName) && lastState == false) {
				std::string ret;
				nlohmann::json jsonData;
				jsonData["type"] = "RUN";
				_sendHttp(url, jsonData.dump(), ret);
				// 记录时间50分钟
				LOG_INFO("三角洲行动游戏启动");
				bool init = true;

				//自动刷新token
				std::thread([gameName]() {
					while (IsProcessRunning(gameName)) {
						std::unique_lock<std::mutex> lock(mtx);
						// 等待条件变量通知或超时
						if (cv.wait_for(lock, std::chrono::minutes(45), [] {
							return token_expired.load() || has_new_battle_record.load();
							})) {

							// 条件满足（token过期或有新战绩）
							if (token_expired) {
								LOG_INFO("WG --> token过期,更新token");
								refreshToken(gameName);
								token_expired = false;

							}
							if (has_new_battle_record) {
								LOG_INFO("WG --> 三角洲行动新的战绩出现,更新token");
								refreshToken(gameName);
								has_new_battle_record = false; // 重置状态
							}
						}
						else {
							// 45分钟超时，没有token过期也没有新战绩
							LOG_INFO("WG -->  45分钟超时,更新token");
							refreshToken(gameName);
						}
						// 重置计时器
						last_event_time = std::chrono::steady_clock::now();
					}

					}).detach();

				std::thread([&]() {


					while (IsProcessRunning(gameName)) {
						if (!isUpdating) {

							if (init) {
								LOG_INFO("首次进入更新WGToken");
								refreshToken(gameName);
							}
							std::this_thread::sleep_for(std::chrono::seconds(30));
							nlohmann::json p_header;
							try{
								p_header = nlohmann::json::parse(get_g_wgHeader());
							}
							catch (const std::exception& e) {
								LOG_EXCEPTION_WITH_STACK(e);
							
							}
							catch (...) {
								LOG_IMMEDIATE("delta.cpp::main_delta::未知错误");
							}
							//nlohmann::json p_header = nlohmann::json::parse(get_g_wgHeader());
							nlohmann::json p_responseJson;
							if (!p_header.empty())
							{
								//去掉多余字段   cookie中,替换为;
								try {
									std::string response_json;
									std::string headers = p_header.dump();
									HttpClient http;
									std::map<std::wstring, std::wstring> header; //暂时这样命名吧
									//TODO TEST 

									header = {
									{ L"sec-ch-ua", L"\"Chromium\";v=\"109\""},
									{ L"content-type" , L"application/json" },
									{ L"sec-ch-ua-mobile" , L"?0" },
									{ L"user-agent" , L"Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/109.0.5414.120 Safari/537.36 qblink wegame.exe WeGame/6.0.3.6120 ChannelId/0 QBCore/109.0.0-HEAD.2720+g8dd4c19+chromium-109.0.5414.120 QQBrowser/9.0.2524.400" },
									{ L"trpc-caller" , L"wegame.pallas.web.Valorant" },
									{ L"sec-ch-ua-platform" , L"\"Windows\"" },
									{ L"accept" , L"*/*" },
									{ L"cookie" ,  stringTOwstring(p_header["cookie"].dump())},
									{ L"sec-fetch-site" , L"same-origin" },
									{ L"sec-fetch-mode" , L"cors" },
									{ L"sec-fetch-dest" , L"empty" },
									{ L"referer" ,L"https://www.wegame.com.cn/helper/valorant/score/detail/eef4c3fa-5483-47c0-a3b3-3fa7bb525c06" },
									{ L"accept-language",L"zh-CN,zh;q=0.9,en-US;q=0.8,en;q=0.7,en;q=0.5;q=0.6"}
									};

									nlohmann::json p_body = {
									{"from_src", "df_web"},
									{"account_type", 1},
									{"area", 36} };
									// [2025-08-12 14:50:55] [INFO] {"result":{"error_message":"登录信息过期","error_code":8000102}}
									response_json = http.SendRequest(
										L"https://www.wegame.com.cn/api/v1/wegame.pallas.dfm.DfmBattle/GetRoleInfo",
										L"POST",
										header,
										p_body.dump()
									);
									std::string realUTF8;
									std::string gbkStr = SecondEncoding2UTF8(response_json, realUTF8);
									//LOG_IMMEDIATE(gbkStr);

									if (gbkStr.find("登录信息过期") != std::string::npos ||
										gbkStr.find("\"error_code\":8000102") != std::string::npos) {
										LOG_IMMEDIATE("delta::过期的token?" + gbkStr);
										token_expired = true;
										cv.notify_one();
									}


									p_responseJson = nlohmann::json::parse(response_json);
									std::string openid = getNestedValue <std::string>(p_responseJson, { "role_info","openid" }, "error");
								
									std::string name =  getNestedValue <std::string>(p_responseJson, { "role_info","name" }, "error");
									std::string nameUTF8 ;
								
									LOG_IMMEDIATE("delta:check-->" + SecondEncoding2UTF8(name, nameUTF8));
									//LOG_IMMEDIATE(utf8ToUnicodeEscape(name));
									p_body = {
									{"from_src", "df_web"},
									{"openid", openid},
									{"area", 36},
									{"sid", "5"},
									{"account_type", 1},
									{"queue", "sol"},
									};
									response_json = http.SendRequest(
										L"https://www.wegame.com.cn/api/v1/wegame.pallas.dfm.DfmBattle/GetBattleReport",
										L"POST",
										header,
										p_body.dump()
									);
									//LOG_IMMEDIATE(UTF8ToGBK(response_json));
						
									p_responseJson = nlohmann::json::parse(SecondEncoding2UTF8(response_json, realUTF8));
									int sol_rank = getNestedValue <int>(p_responseJson, { "season","stats","ranklevel" }, -1);
									int tdm_rank = getNestedValue <int>(p_responseJson, { "season","stats","tdmRanklevel" }, -1);
									int sol_count = getNestedValue <int>(p_responseJson, { "season","stats","solTotal" }, -1);
									int tdm_count = getNestedValue <int>(p_responseJson, { "season","stats","tdmTotalFight" }, -1);

									if (init)
									{
										init_count[0] = sol_count;
										init_count[1] = tdm_count;
										init = false;
									}

									//TODO 测试;
									//sol_count += 1;


									std::string queue = "";
									int rank = 0;
									if (sol_count > init_count[0]) {
										LOG_INFO("新的烽火地带对局产生");
										queue = "sol";
										rank = sol_rank;
										init_count[0] += 1;
									}
									if (tdm_count > init_count[1]) {
										LOG_INFO("新的全面战场对局产生");
										queue = "tdm";
										rank = tdm_count;
										init_count[1] += 1;
									}

									if (queue == "") {

										continue;
									}

									has_new_battle_record = true;
									cv.notify_one();
									std::this_thread::sleep_for(std::chrono::seconds(90));
									p_body = {
									{"from_src", "df_web"},
									{"size", 2},
									{"openid", openid},
									{"area", 36},
									{"queue", queue},
									//{"after", NULL},
									//{"filters", []},
									{"account_type", 1}
									};
									response_json = http.SendRequest(
										L"https://www.wegame.com.cn/api/v1/wegame.pallas.dfm.DfmBattle/GetBattleList",
										L"POST",
										header,
										p_body.dump()
									);
									LOG_IMMEDIATE(UTF8ToGBK(response_json));

									p_responseJson = nlohmann::json::parse(SecondEncoding2UTF8(response_json, realUTF8));

									nlohmann::json sendJson;
									uint64_t ticketNum[3] = { 0,0,0 };
									size_t elementCount = sizeof(ticketNum) / sizeof(ticketNum[0]);
									nlohmann::json battleLists;
									std::string roomId;
									std::string playerId;
									int teamId;
									nlohmann::json member;
									if (queue == "sol") {
										battleLists = getNestedValue <nlohmann::json>(p_responseJson, { "sols" }, nlohmann::json());

										for (auto& battle : battleLists) {
											sendJson["computer_no"] = WStringToString(GetComputerNameWString());
											sendJson["name"] = "";
											sendJson["game_uuid"] = "yhc1" + GenerateUUID();
											sendJson["event_id"] = "yhc2" + GenerateUUID();
											sendJson["game_mode"] = queue == "sol" ? "FENG_HUO_DI_DAI" : "QUAN_MIAN_ZHAN_CHANG";
											sendJson["team_size"] = "";
											sendJson["user_game_rank"] = mapLookupOrDefault(DELTA_rankMap, rank - 1);
											sendJson["type"] = "END";
											sendJson["data"]["ren_tou_shu"] = getNestedValue <int>(battle, { "killPlayer" }, -1);
											sendJson["data"]["gold"] = getNestedValue <int>(battle, { "gainedPrice" }, -1);
											sendJson["data"]["win"] = getNestedValue <int>(battle, { "isLeave" }, -1) == 0 ? 1 : 0;
											sendJson["data"]["time"] = getNestedValue <int>(battle, { "gameTime" }, -1);

											member["id"] = nameUTF8;
											member["role"] = "self";
											sendJson["data"]["member"].push_back(member);
											sendJson["remark"] = "openid:" + openid;
											roomId = getNestedValue <std::string>(battle, { "roomId" }, "error");
											playerId = getNestedValue <std::string>(battle, { "playerId" }, "error");
											teamId = getNestedValue <int>(battle, { "teamId" }, -1);
											// 额外的判定
											ticketNum[0] = getNestedValue <int>(battle, { "trippleKill" }, -1);
											ticketNum[1] = getNestedValue <int>(battle, { "quadKill" }, -1);
											ticketNum[2] = getNestedValue <int>(battle, { "pentaKill" }, -1);
											//只判断第一个数据即可
											break;
										}
										size_t k = 0;
										for (size_t i = 0; i < elementCount; i++)
										{
											for (size_t j = 0; j < ticketNum[i]; j++) {
												sendJson["event_id"] = std::to_string(k);
												nlohmann::json sendTicketJson = sendJson;
												switch (i) {
												case 1:
													sendTicketJson["type"] = "SAN_LIAN_SHA_SHU";
													break;
												case 2:
													sendTicketJson["type"] = "SI_LIAN_SHA_SHU";
													break;
												case 3:
													sendTicketJson["type"] = "WU_LIAN_SHA_SHU";
													break;
												}

												//_sendHttp_Val(sendJson);

												k++;

											}



											//_sendHttp_Val(sendJson);
										}
									}

									if (queue == "tdm") {
										battleLists = getNestedValue <nlohmann::json>(p_responseJson, { "tdms" }, nlohmann::json());
										for (auto& battle : battleLists) {
											sendJson["computer_no"] = WStringToString(GetComputerNameWString());
											sendJson["name"] = "";
											sendJson["game_uuid"] = "yhc1" + GenerateUUID();
											sendJson["event_id"] = "yhc2" + GenerateUUID();
											sendJson["game_mode"] = queue == "sol" ? "FENG_HUO_DI_DAI" : "QUAN_MIAN_ZHAN_CHANG";
											sendJson["team_size"] = "";
											sendJson["user_game_rank"] = mapLookupOrDefault(DELTA_rankMap, rank);
											sendJson["type"] = "END";
											sendJson["data"]["ren_tou_shu"] = getNestedValue <int>(battle, { "killNum" }, -1);
											//sendJson["Data"]["gold"] = getNestedValue <int>(battle, { "gainedPrice" }, -1);
											sendJson["data"]["win"] = getNestedValue <int>(battle, { "isWinner" }, -1);
											sendJson["data"]["time"] = getNestedValue <int>(battle, { "gameTime" }, -1);
									
											member["id"] = nameUTF8;
											member["role"] = "self";
											sendJson["data"]["member"].push_back(member);
											sendJson["remark"] = "openid:" + openid;
											roomId = getNestedValue <std::string>(battle, { "roomId" }, "error");
											playerId = getNestedValue <std::string>(battle, { "playerId" }, "error");
											teamId = getNestedValue <int>(battle, { "teamId" }, -1);
											//只判断第一个数据即可
											break;
										}

									}
									p_body = {
									{"from_src", "df_web"},
									{"roomId", roomId},
									{"openid", openid},
									{"area", 36},
									{"queue", queue},
									//{"after", NULL},
									//{"filters", []},
									{"account_type", 1}
									};
									response_json = http.SendRequest(
										L"https://www.wegame.com.cn/api/v1/wegame.pallas.dfm.DfmBattle/GetBattleDetail",
										L"POST",
										header,
										p_body.dump()
									);
									//LOG_IMMEDIATE(UTF8ToGBK(response_json));
									p_responseJson = nlohmann::json::parse(response_json);
									nlohmann::json players = getNestedValue <nlohmann::json>(p_responseJson, { "battle_detail",queue+"_players"}, nlohmann::json());
									for (auto& player : players) {
										if (playerId == getNestedValue <std::string>(player, { "playerId" }, "error")) {
											continue;
										}
										if (teamId == getNestedValue <int>(player, { "teamId" }, -1)) {
											std::string name = getNestedValue<std::string>(player, { "name" }, "error");
											std::string nameUTF8;
											LOG_IMMEDIATE(SecondEncoding2UTF8(name, nameUTF8));
											member["id"] = nameUTF8;
											member["role"] = "other";
											sendJson["data"]["member"].push_back(member);
										}
	
									}
									_sendHttp(url, sendJson.dump(), ret);
								}
								catch (const nlohmann::json::parse_error& e) {
									LOG_IMMEDIATE_DEBUG("p_responseJson : " + p_responseJson);
									LOG_IMMEDIATE_DEBUG("p_responseJson 解析失败: " + std::string(e.what()));
								}
								catch (const std::exception& e) {
									LOG_IMMEDIATE_ERROR("httpAuthSend:::exception" + std::string(e.what()));
								}
								catch (...) {  // 捕获其他所有异常
									LOG_IMMEDIATE_ERROR("httpAuthSend :::Unknown exception occurred");
								}

								//LOG_IMMEDIATE(p_responseJson.dump(4));


							}
						}

						
					}
					}).detach();

			}

			if (!IsProcessRunning(gameName) && lastState == true) {
				std::string ret;
				nlohmann::json jsonData;
				jsonData["type"] = "KILL";
				_sendHttp(url, jsonData.dump(), ret);

				LOG_INFO("三角洲行动游戏关闭");
				//发送关闭.
			}

			lastState = IsProcessRunning(gameName);


		}

		}).detach();


	return 0;
}