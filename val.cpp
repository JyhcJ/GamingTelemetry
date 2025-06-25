#include "pch.h"

// 1.写状态转换, 状态来控制登录检测.
// 2. 打开wegame - > 开启无畏契约 - > 关闭无畏契约 - > 关闭wegame
#include <windows.h>
#include <string>
#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <memory>
#include <atomic>
#include <iostream>
#include "val.h"
#include "ThreadSafeLogger.h"
#include "common.h"
#include "HttpClient.h"
#include "constant.h"


// 静态成员初始化
std::unique_ptr<MitmDumpController> MitmDumpController::instance;
std::mutex MitmDumpController::instanceMutex;
nlohmann::json httpData;

extern std::map<std::wstring, std::wstring> HEADERS; //LOL

void _sendHttp_Val(nlohmann::json jsonBody) {
	HttpClient http;
	LOG_IMMEDIATE(jsonBody.dump());
	try {
		// 3. 发送POST请求
		//g_mtx_header.lock();
		std::string response = http.SendRequest(
			L"https://dev-asz.cjmofang.com/api/client/WuweiqiyuePostGameData",
			L"POST",
			HEADERS,
			jsonBody.dump()
		);
		//g_mtx_header.unlock();
		LOG_IMMEDIATE("Response: " + UTF8ToGBK(response));
	}
	catch (const std::exception& e) {
		LOG_IMMEDIATE_ERROR("_sendHttp_Val:::");
		LOG_IMMEDIATE_ERROR(e.what());
	}
	catch (...) {  // 捕获其他所有异常
		LOG_IMMEDIATE_ERROR("_sendHttp_Val :::Unknown exception occurred");
	}
}

void _sendHttp_Val(std::string type, nlohmann::json data) {
	// data留着吧
	nlohmann::json jsonBody;
	jsonBody["type"] = type;
	jsonBody["name"] = "";
	if (!data.empty())
	{
		jsonBody.update(data);
	}
	_sendHttp_Val(jsonBody);

}

int updateHeader() {
	try {
		auto& controller = MitmDumpController::getInstance();

		//if (controller.start(8080, "--mode transparent --ssl - insecure --set upstream_cert = false  --allow - hosts \"wegame.com.cn\"")) {
		//if (controller.start(8080, "-w outfileDEX --mode transparent --ssl-insecure --set upstream_cert=false  --allow-hosts \"wegame.com.cn\" --flow-detail 3")) {
		//	std::cout << "mitmdump started with PID: " << controller.pid() << std::endl;

			/*for (size_t i = 0; i < 5; i++)
			{*/

		// 读取UTF-8文件内容
		try {


			/*	if ()
				{*/
			const char* exePath = "dumpMain.exe";
			// TODO 不阻塞的方式执行
			int result = std::system(exePath);

			//运行exe 代理
			std::this_thread::sleep_for(std::chrono::milliseconds(2000));
			// 触发https...
			std::this_thread::sleep_for(std::chrono::milliseconds(5000));

			if (result == 0 && is_file_exists_and_not_empty("outfileDecode")) {

				std::string content = MitmDumpController::readUtf8File("outfileDecode");
				//content = preprocess_mitm_text(contentDEX);

				//MitmDumpController::clearFileContent("outfileDecode");

				//size_t found_pos = content.find("outfileDEX_END");
				//if (found_pos != std::string::npos) {
					//controller.stop();
					// 处理content
					//std::string str1 = controller.clean_byte_string(content);
				std::ifstream f("outfileDecode");
				nlohmann::json data = nlohmann::json::parse(f);
				std::string str = data.dump();
				//controller.parse_python_headers(str1, tempHeaderData);


				//TODO 遍历tempHeaderData 判断是否有更新 
				if (true)
				{
					// TODO 这里要筛选数据
					if (!data.empty())
					{
						httpData = data;
						// 为啥没写到日志中去呢??? 可能是改了日志部署的形式.
						LOG_IMMEDIATE("请求有信息");
					}
				
					//LOG_IMMEDIATE_DEBUG(httpData.dump(4));
				}


				//break;
				//}
				//else {

				//}

			}
			else {
				std::cout << "程序运行失败或返回错误码：" << result << std::endl;
			}
			//}

		}
		catch (const std::exception& e) {
			// 处理错误
			std::cerr << "更新缓存时出错: " << e.what() << std::endl;
		}

		//}


		// 主循环处理输出
		//while (controller.running()) {
		//    std::string output = controller.getOutput();
		//    if (!output.empty()) {
		//        std::cout << "mitmdump: " << output << std::endl;
		//    }

		//    // 在这里添加你的业务逻辑
		//    // ...

		//    // 避免CPU占用过高
		//    std::this_thread::sleep_for(std::chrono::milliseconds(500));
		//}

		//--mode transparent --ssl - insecure --set upstream_cert = false  --allow - hosts "wegame.com.cn"

		// 停止 mitmdump (可选，析构函数会自动调用)
		controller.stop();
		//}
		//else {
		//	std::cerr << "Failed to start mitmdump" << std::endl;
		//	return 1;
		//}
	}
	catch (const std::exception& e) {
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}
	catch (...) {
		std::cerr << "Error: Mitmproxy没有关闭?" << std::endl;
		return 1;
	}
	//getValinfo2send();
	return 0;
	//mpController.start(8080,"wegame.com.cn");
}

std::string getValinfo2send() {
	
	nlohmann::json p_header;
	nlohmann::json p_body = {
	{"from_src", "valorant_web"},
	{"size", 11},              
	{"queueID", "255"}          
	};
	nlohmann::json p_responseJson;
	for ( auto& flow : httpData) {
	
		// 请求部分
		auto& request = flow["request"];
		std::string str = request.value("url", "N/A");
	
		size_t found_pos = str.find("/api/v1/wegame.pallas.game.ValBattle/GetBattleList");
		if (found_pos == std::string::npos) {
			continue;
		}
		std::cout << "===== Flow =====\n";
		std::cout << "Request URL: " << request.value("url", "N/A") << "\n";
		std::cout << "Method: " << request.value("method", "N/A") << "\n";
	
		// 打印 headers
		std::cout << "Headers:\n";
		std::string cookies = request["headers"]["cookie"];
		//nlohmann::json::iterator it = request["headers"].find("cookie");

		// 替换所有 ',' 为 ';'
		std::replace(cookies.begin(), cookies.end(), ',', ';');


		request["headers"]["cookie"] = cookies;
		// 输出结果
		std::cout << "Updated Cookie:\n" << request["headers"]["cookie"].dump() << std::endl;
		for (auto it = request["headers"].begin(); it != request["headers"].end(); ++it) {
			std::cout << "  " << it.key() << ": " << it.value() << "\n";
		}

		p_header = request["headers"];
		// 打印请求体
		std::cout << "Request Content: " << request.value("content", "N/A") << "\n\n";


		// 响应部分
		const auto& response = flow["response"];
		std::cout << "Response Status Code: " << response.value("status_code", 0) << "\n";

		// 打印响应 headers
		std::cout << "Response Headers:\n";
		for (auto it = response["headers"].begin(); it != response["headers"].end(); ++it) {
			std::cout << "  " << it.key() << ": " << it.value() << "\n";
		}
		// 打印响应内容
		std::cout << "Response Content: " << response.value("content", "N/A") << "\n";
		break;
		//std::cout << "对比一下呢" << request.dump() << "\n";


	}

	if (!p_header.empty())
	{
		//去掉多余字段   cookie中,替换为;
		try {
			std::string response_json;
			std::string headers = p_header.dump();
			std::string body = p_body.dump();
			HttpClient http;
			std::map<std::wstring, std::wstring> HEADERS; //暂时这样命名吧
			//TODO TEST 

			HEADERS = {
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
			//https://www.wegame.com.cn/api/v1/wegame.pallas.game.ValBattle/GetBattleDetail
			response_json = http.SendRequest(
				L"https://www.wegame.com.cn/api/v1/wegame.pallas.game.ValBattle/GetBattleList",
				L"POST",
				HEADERS,
				body
			);
			LOG_IMMEDIATE(UTF8ToGBK(response_json));
			p_responseJson = nlohmann::json::parse(UTF8ToGBK(response_json));
			nlohmann::json battlesInfos = p_responseJson["battles"];
			nlohmann::json sendJson;
			for (auto& game : battlesInfos) {
				sendJson["computer_no"] = game[""];
				sendJson["name"] = game[""];
				sendJson["game_uuid"] = game["matchId"];
				sendJson["event_id"] = game["apEventId"];
				sendJson["game_mode"] = VAL_gameModMap[game["queueId"]];
				//sendJson["team_size"] = game[""];
				sendJson["user_game_rank"] = game["TODO"];
				sendJson["type"] = "END";
				sendJson["Data"]["ren_tou_shu"] = game["statsKills"];
				sendJson["Data"]["mvp"] = game["mvpsvp"];
				sendJson["Data"]["win"] = game["wonMatch"];
				sendJson["Data"]["time"] = std::to_string(game["gameLengthMillis"].get<uint64_t>()/1000);
				sendJson["Data"]["Member"]["id"] = game["subject"];
				sendJson["Data"]["Member"]["role"] = "self";
				sendJson["Data"]["remark"] = std::to_string(game["pentaKillCount"].get<uint64_t>());
				break;//就取第一个
			}
			_sendHttp_Val(sendJson);
		/*{
				"battles": [
					{
						"apEventId": "eef4c3fa-5483-47c0-a3b3-3fa7bb525c06",
						"characterId" : "eb93336a-449b-9c1b-0a54-a891f7921d69",
						"competitiveTier" : 0,
						"dmRank" : 0,
						"dtEventTime" : "2025-05-28 00:16:44",
						"firstKillCount" : 0,
						"gameLengthMillis" : 753020,
						"gameStartMillis" : "1748361746520",
						"isCompleted" : 1,
						"isMatchMvp" : 0,
						"isTeamMvp" : 0,
						"mapId" : "/Game/Maps/Canyon/Canyon",
						"matchId" : "71f46f42-1269-4602-95af-ebd352676bab",
						"mvpsvp" : 0,
						"name" : "",
						"newExpr" : "",
						"partyId" : "cad902b4-458f-443f-96a9-bd9e600e8344",
						"pentaKillCount" : 0,
						"quadraKillCount" : 0,
						"queueId" : "swiftplay",
						"roundsPlayed" : 8,
						"roundsWon" : 3,
						"sixKillCount" : 0,
						"statsAssists" : 2,
						"statsDeaths" : 7,
						"statsKills" : 2,
						"statsPlaytimeMillis" : "753020",
						"statsRoundsPlayed" : 8,
						"statsScore" : "719",
						"subject" : "9e5486bb-4da5-5d9d-b666-17af5576e731",
						"teamId" : "Blue",
						"wonMatch" : 0
					}
				],
				"result": {
					"error_code": 0,
					"error_message" : "success"
				}
			}*/
			
			//_sendHttp_Val();

		}
		catch (const nlohmann::json::parse_error& e) {
			std::string error = e.what();
			LOG_IMMEDIATE_DEBUG("p_responseJson 解析失败: " + error);
			LOG_IMMEDIATE_DEBUG("p_responseJson : " + p_responseJson);
		}
		catch (const std::exception& e) {
			LOG_IMMEDIATE_ERROR("httpAuthSend:::exception");
			LOG_IMMEDIATE_ERROR(e.what());
		}
		catch (...) {  // 捕获其他所有异常
			LOG_IMMEDIATE_ERROR("httpAuthSend :::Unknown exception occurred");
		}

		LOG_IMMEDIATE(p_responseJson.dump(4));

		return p_responseJson.dump();
	}
	
}







