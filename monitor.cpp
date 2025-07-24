#include "pch.h"
#include <windows.h>
#include <tlhelp32.h>
#include <iostream>
#include <chrono>
#include <thread>
#include <string>
#include <ctime>
#include "monitor.h"
#include "common.h"
#include "lol.h"
#include "ThreadWrapper.h"
#include "ThreadSafeLogger.h"
#include "lol_before.h"
#include "LoLStateMonitor.h"

//#include "ValStateMonitor.h"
//#include "py.h"
#include "cs2.h"
#include "ProcessMemoryReader.h"
#include "pubg_name.h"
#include "pubg.h"
#include "ValStateMonitor.h"

//extern bool lol_running;
//extern std::string BEFORE_STATE;
extern std::mutex g_mtx;
// ���ò���
const std::wstring LOL_PROCESS_NAME = L"LeagueClient.exe";
const std::wstring LOL_GAME_PROCESS_NAME = L"League of Legends.exe";
const int CHECK_INTERVAL_MS = 5000; // �����5��

// ȫ�ֱ���
std::chrono::system_clock::time_point lol_start_time;
bool is_lol_running = false;
bool is_lol_game_running = false;
std::string g_hostName;
double total_lol_time = 0.0;



// ��ȡ��ǰʱ���ַ���
std::string GetCurrentTimeString() {
	auto now = std::chrono::system_clock::now();
	auto in_time_t = std::chrono::system_clock::to_time_t(now);
	std::tm tm;
	localtime_s(&tm, &in_time_t);

	char buffer[80];
	strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &tm);
	return std::string(buffer);
}

DWORD GetPidByName(const std::wstring& processName) {
	PROCESSENTRY32W pe32; // ע��ʹ�� W �汾
	pe32.dwSize = sizeof(PROCESSENTRY32W);

	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnapshot == INVALID_HANDLE_VALUE) {
		/*std::wcerr << L"Failed to create process snapshot (Error: " << GetLastError() << L")" << std::endl;*/
		return 0;
	}

	if (Process32FirstW(hSnapshot, &pe32)) { // ʹ�ÿ��ַ��汾 Process32FirstW
		do {
			if (_wcsicmp(pe32.szExeFile, processName.c_str()) == 0) { // ���ַ��Ƚ�
				CloseHandle(hSnapshot);
				return pe32.th32ProcessID;
			}
		} while (Process32NextW(hSnapshot, &pe32)); // ʹ�ÿ��ַ��汾
	}

	CloseHandle(hSnapshot);
	return 0; // δ�ҵ�����
}

double GetProcessUptime(DWORD pid) {
	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
	if (hProcess == NULL) {
		//std::cerr << "Failed to open process (Error: " << GetLastError() << ")" << std::endl;
		return -1.0;
	}

	FILETIME createTime, exitTime, kernelTime, userTime;
	if (!GetProcessTimes(hProcess, &createTime, &exitTime, &kernelTime, &userTime)) {
		//std::cerr << "Failed to get process times (Error: " << GetLastError() << ")" << std::endl;
		CloseHandle(hProcess);
		return -1.0;
	}

	CloseHandle(hProcess);

	// �� FILETIME ת��Ϊ 64 λ����
	ULARGE_INTEGER uliCreateTime;
	uliCreateTime.LowPart = createTime.dwLowDateTime;
	uliCreateTime.HighPart = createTime.dwHighDateTime;

	// ��ȡ��ǰϵͳʱ��
	SYSTEMTIME currentSysTime;
	GetSystemTime(&currentSysTime);
	FILETIME currentFileTime;
	SystemTimeToFileTime(&currentSysTime, &currentFileTime);
	ULARGE_INTEGER uliCurrentTime;
	uliCurrentTime.LowPart = currentFileTime.dwLowDateTime;
	uliCurrentTime.HighPart = currentFileTime.dwHighDateTime;

	// ��������ʱ�䣨��λ���룩
	double uptime = (uliCurrentTime.QuadPart - uliCreateTime.QuadPart) / 1e7; // 100ns �� s
	return uptime;
}

// ����غ���(�߳�)
void MonitorGameProcess() {
	g_mtx.lock();
	//g_hostName = gethostName();
	g_mtx.unlock();
	is_lol_running = true;

	while (true) {
		bool currently_running = IsProcessRunning(LOL_PROCESS_NAME);
		bool currently_game_running = IsProcessRunning(LOL_GAME_PROCESS_NAME);
		double runTime = GetProcessUptime(GetPidByName(LOL_PROCESS_NAME));
		//if (currently_running && !is_lol_running && !is_lol_game_running) {
		if (currently_running && !is_lol_game_running) {
			//if (currently_running ) {
				// Ӣ�����˸�����
				//lol_start_time = std::chrono::system_clock::now();


			if (runTime < 20.0)
			{
				LOG_IMMEDIATE(" Ӣ������������\n");
				g_mtx.lock();
				//is_lol_running = true;
				//is_lol_game_running = false;
				g_mtx.unlock();
				_sendHttp_LOL("RUN", "");
				//// �����߳�
				ThreadWrapper thread(pollRankNum);
				thread.Start();
				// �ȴ��̳߳�ʼ����ɣ�����ʵ���������������
				//while (thread.GetState() != ThreadWrapper::ThreadState::RUNNING) {
				std::this_thread::sleep_for(std::chrono::seconds(1));
				//}

				thread.Detach();
			}



			// �ͻ��������󲻶ϸ��� ��λ�Ͷ�������(��������Ҫ����)
			// �Ծֽ����������ʤ������(������ʤ����,��ʤ��)
			// �Ծ��з�����Ϣ(��λ,��������,�Ծ�ID,��ɱ��ɱ,�Ծ�ģʽ)

		}
		//else if (!currently_running && is_lol_running && !is_lol_game_running) {
		else if (!currently_running && !is_lol_game_running) {
			//else if (!currently_running) {
				// Ӣ�����˿ͻ��˹ر�
			g_mtx.lock();
			//is_lol_running = false;
			is_lol_game_running = false;
			g_mtx.unlock();

			//std::cout << "[" << GetCurrentTimeString() << "] Ӣ�������ѹر�\n";
			LOG_IMMEDIATE(" Ӣ�������ѹر�\n");
			_sendHttp_LOL("KILL", "");
			// LOG_IMMEDIATE(std::string("������Ϸʱ��") + std::to_string(hours));
			// �����Ҫ�ϴ�ʱ��

		}
		//else if (currently_game_running && !is_lol_game_running ) {
		//else if (currently_game_running && !is_lol_game_running && is_lol_running) {
		else if (currently_game_running && !is_lol_game_running) {
			//else if (currently_game_running ) {
				// Ӣ�����˶Ծֿ�ʼ
			LOG_IMMEDIATE(" Ӣ�����˶Ծ�������,��ʼ���ӶԾ���Ϣ\n");

			g_mtx.lock();
			//is_lol_running = false;
			is_lol_game_running = true;
			g_mtx.unlock();

			//// �����߳�
			ThreadWrapper thread(pollEvents);
			thread.Start();
			std::this_thread::sleep_for(std::chrono::seconds(1));
			thread.Detach();

		}
		//else if (!currently_game_running && is_lol_game_running && !is_lol_running) {
		else if (!currently_game_running && is_lol_game_running) {
			//else if (!currently_game_running) {



			/*	auto end_time = std::chrono::system_clock::now();
				std::chrono::duration<double> elapsed = end_time - lol_start_time;
				total_lol_time += elapsed.count();


				int hours = static_cast<int>(elapsed.count() / 3600);
				int minutes = static_cast<int>(fmod(elapsed.count(), 3600) / 60);
				int seconds = static_cast<int>(fmod(elapsed.count(), 60));*/

				//LOG_IMMEDIATE(" Ӣ�����˶Ծֽ���?/ ����? / �ؿ�?");
				//LOG_IMMEDIATE(" Ӣ�����˶Ծ�״̬:" + BEFORE_STATE);

				//��ѯ������ gameid �� accountid ƥ����ͳ�� �����Ѿ�������?


			Game_Before gb;
			// ��ȡ data
			gb.before_main("END");


			// ����data ,����ͨ�� ��ѯ�Ծ�ID�Ƿ����



		/*	std::cout << "������Ϸʱ��: "
				<< hours << "Сʱ "
				<< minutes << "���� "
				<< seconds << "��\n";
			std::cout << "�ۼ���Ϸʱ��: "
				<< static_cast<int>(total_lol_time / 3600) << "Сʱ "
				<< static_cast<int>(fmod(total_lol_time, 3600) / 60) << "����\n";*/

				// TODO ��ѯ�Ծ�ID�Ƿ����
				//if (BEFORE_STATE == "EndOfGame")
				//{	// ��Ϸ���� �ȴ�ͳ�ƽ�� (����ͻ���ִ�н��������ܲ���.)
				//	// ���߲�ѯ�����Ϸ��gameID�Ƿ����???
				//	_sendHttp_LOL("END", "");
				//}

		}

		// ���������Ϸ�Ƿ����� 
		// if (IsProcessRunning(L"��η��Լ?CSGO?��������")) {
		//     std::cout << "��⵽������Ϸ��������\n";
		// }

		std::this_thread::sleep_for(std::chrono::milliseconds(CHECK_INTERVAL_MS));
	}
}

int main() {
	// ������־������ļ�
	ThreadSafeLogger::GetInstance().SetOutputFile("monitor.log");
#ifdef _DEBUG
	// Debug ģʽ
	ThreadSafeLogger::GetInstance().SetMinLogLevel(LogLevel::DEBUG1);
#else
	// Release ģʽ
	ThreadSafeLogger::GetInstance().SetMinLogLevel(LogLevel::INFO);
#endif

	LOG_IMMEDIATE("DLL���ӳ���������");
	//pymain();

	g_mtx.lock();
	g_hostName = WStringToString(GetComputerNameWString());
	std::cout << g_hostName << std::endl;
	g_mtx.unlock();
	try {
		std::thread monitor_val([]() {
			ValStateMonitor monitor_val;
			monitor_val.MonitorLoop();

			});
		monitor_val.detach();

		// �������й����߳�
		ProcessMonitor_PUBG monitor_pubg;
		monitor_pubg.start();
		//if (!EnableDebugPrivilege(TRUE)) {
		//	printf("[-] Failed to enable SeDebugPrivilege\n");
		//	return 1;
		//}

		//printf("[+] SeDebugPrivilege enabled: %d\n", IsDebugPrivilegeEnabled());

		//try {
		//	// 1. ������ȡ��ʵ��
		//	//ProcessMemoryReader reader(L"TslGame.exe");
	
		//
		//	//// 2. ����ָ���� [[[[1F1804A0060+0]+C0]+318]+18
		//	//uintptr_t baseAddress = 0x1F1804A0060;
		//	//std::vector<uintptr_t> offsets = { 0x0, 0xC0, 0x318, 0x18 };

		//	//// 3. ��ȡUnicode�ַ���
		//	//std::wstring result = reader.readUnicodeStringFromPointerChain(baseAddress, offsets);

		//	//// 4. ������
		//	//std::wcout << L"Read value: " << result << std::endl;
		//}
		//catch (const std::exception& e) {
		//	std::cerr << "Error: " << e.what() << std::endl;
		//	return 1;
		//}

		//pubg_main();

		cs2Monitor();
		//ThreadWrapper thread([&monitor]() {
		//	monitor.MonitorLoop();
		//	});

		///*monitor.StartMonitoring();*/

		//thread.Start();

		//std::this_thread::sleep_for(std::chrono::seconds(1));
		//
		//thread.Detach();
		LoLStateMonitor monitor;
		//ValStateMonitor valMonitor;

		// �����̰߳�ȫ����
		//ThreadSafeLogger::GetInstance().TestThreadSafety(100); // ����5��
		// 
		//ThreadSafeLogger::GetInstance().Log(LogLevel::INFO, "�������!");
		// 
		//// �ڵ����߳������м��
		std::thread lolMonitorThread([&monitor]() {
			monitor.MonitorLoop();
			});


		/*std::thread valMonitorThread([&valMonitor]() {
			valMonitor.MonitorLoop();
			});*/
		//monitorThread.detach();

		// ���߳̿�������������
		while (true) {
			//loadDriver();
			//GENERAL_CONSTRUCTION gc_in = GENERAL_CONSTRUCTION();
			//GENERAL_CONSTRUCTION gc_out;
			//driverUpdate(gc_in, gc_out);
			//driverGetPlayerName(gc_in, gc_out);
			//std::wstring playerName = gc_out.PlayerName;
			//LOG_IMMEDIATE("gc_out:::" + WStringToString(gc_out.PlayerName));
			std::this_thread::sleep_for(std::chrono::seconds(10));
		}

		lolMonitorThread.join();
		//valMonitorThread.join();

		//std::thread([&monitor]() {
		//	monitor.MonitorLoop();
		//	}).detach();  // �����߳�

		//MonitorGameProcess();
	}
	catch (const std::exception& e) {
		//std::cerr << "��������: " << e.what() << std::endl;
		 //(e.what());
		LOG_ERROR(e.what());
		return 1;
	}
	catch (...) {  // �������������쳣
		LOG_IMMEDIATE_ERROR("main :::Unknown exception occurred");
	}
	//����ʱ��ע��
	return 0;
}


// �������
extern "C" __declspec(dllexport) const int monitorLOL() {
	return main();
}

