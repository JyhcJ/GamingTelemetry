#include <thread>
#include "common.h"
#include "ThreadSafeLogger.h"
#include <iostream>
#include "HttpClient.h"
#include "nloJson.h"
#include "constant.h"
#include "delta.h"




int main_delta() {
	//���wegame·��

	// �����ű�

	std::thread([]() {

		while (true) {
			std::this_thread::sleep_for(std::chrono::seconds(2));

			bool lastState = false;
			if (IsProcessRunning(L"DeltaForceClient-Win64-Shipping.exe") && lastState == false) {
				std::string ret;
				_sendHttp(L"api/client/SanjiaozhouPostGameDat","", ret)
				// ��¼ʱ��50����
				LOG_INFO("�������ж���Ϸ����");
				bool init = true;
				// ������������ѯ�Ƿ�������ս�� ->��ս�� ->����token
				while (IsProcessRunning(L"DeltaForceClient-Win64-Shipping.exe")) {
					//��ѯwegame��Ϣ , ��ս��.
					nlohmann::json data;

					int init_count[2] = { 0,0 };
					//if(token���� || �Լ���ʱ50���ӽ���)
					try {

						const char* exePath = "\".\\build\\pkg\\pythonEmbed\\pythonw.exe\" dumpMain.pyw";
						std::thread([]() {
							std::wstring temp = stringTOwstring(GetWGPath_REG());
							//std::wstring temp = L"O:\\������Ϸ\\WeGame˳��ר��\\wegame.exe";
							std::this_thread::sleep_for(std::chrono::seconds(4));
							WGRefresh(temp, L"/StartFor=2001715");
							std::this_thread::sleep_for(std::chrono::seconds(3));
							WGRefresh(temp, L"/StartFor=2001918");
							}).detach();
						LOG_IMMEDIATE("-------------------minpoxyR-------------------");
						int result = executeSilently(exePath);
						if (result == -1) {
							LOG_IMMEDIATE("����Ŀ¼������python����,�����Զ������python����:" + std::to_string(result));
							const char* exePath = "\"C:\\Program Files\\Python313\\pythonw.exe\" dumpMain.pyw";
							result = executeSilently(exePath);
							if (result == -1) {
								LOG_IMMEDIATE("�Զ������python����ʧ��:" + std::to_string(result));
								return -1;
							}
						}
						LOG_IMMEDIATE("-------------------minpoxy:result-------------------" + std::to_string(result));
						LOG_IMMEDIATE("-------------------minpoxyE-------------------");
						std::this_thread::sleep_for(std::chrono::milliseconds(3000));
						if (result == 0 && is_file_exists_and_not_empty("outfileDecode")) {

							std::string content = readUtf8File("outfileDecode");
							std::ifstream f("outfileDecode");
							data = nlohmann::json::parse(f);
							std::string str = data.dump();

							if (!data.empty())
							{
								LOG_IMMEDIATE("mitmproxy�������������Ϣ");
							}
							else {
								LOG_INFO("mitmproxy���������Ϊ��");
							}

						}
						else {
							LOG_IMMEDIATE_ERROR("delta:mitmproxy��������ʧ�ܻ򷵻ش����룺" + result);
						}


						std::this_thread::sleep_for(std::chrono::seconds(15));
					}
					catch (const std::exception& e) {
						LOG_IMMEDIATE_ERROR("����mitmproxy�������쳣:" + std::string(e.what()));
					}

					nlohmann::json p_header;
					nlohmann::json p_responseJson;

					for (auto& flow : data) {
						// ���󲿷�
						auto& request = flow["request"];
						std::string str = request.value("url", "N/A");
						size_t found_pos = str.find("/api/v1/wegame.base.game.CommConfig/GetCfg");
						size_t found_pos1 = str.find("/api/v1/wegame.pallas.game.ValAssist/GetNewbieInfo");
						if (found_pos == std::string::npos && found_pos1 == std::string::npos) {
							LOG_IMMEDIATE("δ���񵽳�������");
							continue;
						}
						std::cout << "===== Flow =====\n";
						std::cout << "Request URL: " << request.value("url", "N/A") << "\n";
						std::cout << "Method: " << request.value("method", "N/A") << "\n";

						// ��ӡ headers
						std::cout << "Headers:\n";
						std::string cookies = request["headers"]["cookie"];
						//nlohmann::json::iterator it = request["headers"].find("cookie");

						// �滻���� ',' Ϊ ';'
						std::replace(cookies.begin(), cookies.end(), ',', ';');


						request["headers"]["cookie"] = cookies;
						// ������
						std::cout << "Updated Cookie:\n" << request["headers"]["cookie"].dump() << std::endl;
						for (auto it = request["headers"].begin(); it != request["headers"].end(); ++it) {
							std::cout << "  " << it.key() << ": " << it.value() << "\n";
						}

						p_header = request["headers"];
						// ��ӡ������
						std::cout << "Request Content: " << request.value("content", "N/A") << "\n\n";


						// ��Ӧ����
						const auto& response = flow["response"];
						std::cout << "Response Status Code: " << response.value("status_code", 0) << "\n";

						// ��ӡ��Ӧ headers
						std::cout << "Response Headers:\n";
						for (auto it = response["headers"].begin(); it != response["headers"].end(); ++it) {
							std::cout << "  " << it.key() << ": " << it.value() << "\n";
						}
						// ��ӡ��Ӧ����
						std::cout << "Response Content: " << response.value("content", "N/A") << "\n";
						break;
						//std::cout << "�Ա�һ����" << request.dump() << "\n";


					}

					if (!p_header.empty())
					{
						//ȥ�������ֶ�   cookie��,�滻Ϊ;
						try {
							std::string response_json;
							std::string headers = p_header.dump();
							HttpClient http;
							std::map<std::wstring, std::wstring> header; //��ʱ����������
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

							response_json = http.SendRequest(
								L"https://www.wegame.com.cn/api/v1/wegame.pallas.dfm.DfmBattle/GetRoleInfo",
								L"POST",
								header,
								p_body.dump()
							);
							LOG_IMMEDIATE(UTF8ToGBK(response_json));
							p_responseJson = nlohmann::json::parse(response_json);
							std::string openid = getNestedValue <std::string>(p_responseJson, { "role_info","openid" }, "error");
							std::string name = getNestedValue <std::string>(p_responseJson, { "role_info","name" }, "error");

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
							LOG_IMMEDIATE(UTF8ToGBK(response_json));
							p_responseJson = nlohmann::json::parse(UTF8ToGBK(response_json));
							int sol_rank = getNestedValue <int>(p_responseJson, { "season","stats","ranklevel" }, -1);
							int tdm_rank = getNestedValue <int>(p_responseJson, { "season","stats","tdmRanklevel" }, -1);
							int sol_count = getNestedValue <int>(p_responseJson, { "season","stats","solTotal" }, -1);
							int tdm_count = getNestedValue <int>(p_responseJson, { "season","stats","tdmTotalFight" }, -1);
							if (init)
							{
								init_count[0] = sol_count;
								init_count[1] = tdm_count;
							}

							std::string queue = "";
							int rank = 0;
							if (sol_count >= init_count[0]) {
								LOG_INFO("�µķ��ش��Ծֲ���");
								queue = "sol";
								rank = sol_rank;
							}
							if (tdm_count > init_count[1]) {
								LOG_INFO("�µ�ȫ��ս���Ծֲ���");
								queue = "tdm";
								rank = tdm_count;
							}

							if (queue == "") {
								continue;
							}

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
							//LOG_IMMEDIATE(UTF8ToGBK(response_json));
							p_responseJson = nlohmann::json::parse(UTF8ToGBK(response_json));

							nlohmann::json sendJson;
							uint64_t ticketNum[3] = { 0,0,0 };
							size_t elementCount = sizeof(ticketNum) / sizeof(ticketNum[0]);
							nlohmann::json battleLists;
							if (queue == "sol") {
								battleLists = getNestedValue <nlohmann::json>(p_responseJson, { "sols" }, nlohmann::json());

								for (auto& battle : battleLists) {
									sendJson["computer_no"] = gethostName();
									sendJson["name"] = "";
									sendJson["game_uuid"] = "yhc1" + GenerateUUID();
									sendJson["event_id"] = "yhc2" + GenerateUUID();
									sendJson["game_mode"] = queue == "sol" ? "FENG_HUO_DI_DAI" : "QUAN_MIAN_ZHAN_CHANG";
									sendJson["team_size"] = "";
									sendJson["user_game_rank"] = mapLookupOrDefault(DELTA_rankMap, rank);
									sendJson["type"] = "END";
									sendJson["Data"]["ren_tou_shu"] = getNestedValue <int>(battle, { "killPlayer" }, -1);
									sendJson["Data"]["gold"] = getNestedValue <int>(battle, { "gainedPrice" }, -1);
									sendJson["Data"]["win"] = getNestedValue <int>(battle, { "isLeave" }, -1) == 0 ? 1 : 0;
									sendJson["Data"]["time"] = getNestedValue <int>(battle, { "gameTime" }, -1);
									nlohmann::json Member;
									Member["id"] = name;
									Member["role"] = "self";
									sendJson["Data"]["Member"].push_back(Member);
									sendJson["remark"] = "openid:" + openid;

									// ������ж�
									ticketNum[0] = getNestedValue <int>(battle, { "trippleKill" }, -1);
									ticketNum[1] = getNestedValue <int>(battle, { "quadKill" }, -1);
									ticketNum[2] = getNestedValue <int>(battle, { "pentaKill" }, -1);
									//ֻ�жϵ�һ�����ݼ���
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
									sendJson["computer_no"] = gethostName();
									sendJson["name"] = "";
									sendJson["game_uuid"] = "yhc1" + GenerateUUID();
									sendJson["event_id"] = "yhc2" + GenerateUUID();
									sendJson["game_mode"] = queue == "sol" ? "FENG_HUO_DI_DAI" : "QUAN_MIAN_ZHAN_CHANG";
									sendJson["team_size"] = "";
									sendJson["user_game_rank"] = mapLookupOrDefault(DELTA_rankMap, rank);
									sendJson["type"] = "END";
									sendJson["Data"]["ren_tou_shu"] = getNestedValue <int>(battle, { "killNum" }, -1);
									//sendJson["Data"]["gold"] = getNestedValue <int>(battle, { "gainedPrice" }, -1);
									sendJson["Data"]["win"] = getNestedValue <int>(battle, { "isWinner" }, -1);
									sendJson["Data"]["time"] = getNestedValue <int>(battle, { "gameTime" }, -1);
									nlohmann::json Member;
									Member["id"] = name;
									Member["role"] = "self";
									sendJson["Data"]["Member"].push_back(Member);
									sendJson["remark"] = "openid:" + openid;

									//ֻ�жϵ�һ�����ݼ���
									break;
								}

							}


						}
						catch (const nlohmann::json::parse_error& e) {
							LOG_IMMEDIATE_DEBUG("p_responseJson : " + p_responseJson);
							LOG_IMMEDIATE_DEBUG("p_responseJson ����ʧ��: " + std::string(e.what()));
						}
						catch (const std::exception& e) {
							LOG_IMMEDIATE_ERROR("httpAuthSend:::exception" + std::string(e.what()));
						}
						catch (...) {  // �������������쳣
							LOG_IMMEDIATE_ERROR("httpAuthSend :::Unknown exception occurred");
						}

						//LOG_IMMEDIATE(p_responseJson.dump(4));

					}

				}
			}

			if (!IsProcessRunning(L"df����") && lastState == true) {

				LOG_INFO("df��Ϸ�ر�");
				//���͹ر�.
			}

			lastState = IsProcessRunning(L"df����");
		}

		}).detach();

	return 0;
}