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

extern bool lol_running;

// ���ò���
const std::wstring LOL_PROCESS_NAME = L"LeagueClient.exe";
const std::wstring LOL_GAME_PROCESS_NAME = L"League of Legends.exe";
const int CHECK_INTERVAL_MS = 5000; // �����5��

// ȫ�ֱ���
std::chrono::system_clock::time_point lol_start_time;
bool is_lol_running = false;
bool is_lol_game_running = false;
double total_lol_time = 0.0;

// ���ָ�������Ƿ�������
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



// ����غ���
void MonitorGameProcess() {
	while (true) {
		bool currently_running = IsProcessRunning(LOL_PROCESS_NAME);
		bool currently_game_running = IsProcessRunning(LOL_GAME_PROCESS_NAME);

		if (currently_running && !is_lol_running) {
			// Ӣ�����˸�����
			lol_start_time = std::chrono::system_clock::now();
			is_lol_running = true;
			LOG_IMMEDIATE(" Ӣ������������\n");
			pollRankNum();
			_sendHttp_LOL("RUN", "");
			// �ͻ��������󲻶ϸ��� ��λ�Ͷ�������(��������Ҫ����)
			// �Ծֽ����������ʤ������(������ʤ����,��ʤ��)
			// �Ծ��з�����Ϣ(��λ,��������,�Ծ�ID,��ɱ��ɱ,�Ծ�ģʽ)

		}
		else if (!currently_running && is_lol_running) {
			// Ӣ�����˿ͻ��˹ر�
			auto end_time = std::chrono::system_clock::now();
			std::chrono::duration<double> elapsed = end_time - lol_start_time;
			total_lol_time += elapsed.count();
			is_lol_running = false;

			int hours = static_cast<int>(elapsed.count() / 3600);
			int minutes = static_cast<int>(fmod(elapsed.count(), 3600) / 60);
			int seconds = static_cast<int>(fmod(elapsed.count(), 60));

			//std::cout << "[" << GetCurrentTimeString() << "] Ӣ�������ѹر�\n";
			LOG_IMMEDIATE(" Ӣ�������ѹر�\n");
			_sendHttp_LOL("KILL", "");
			// LOG_IMMEDIATE(std::string("������Ϸʱ��") + std::to_string(hours));
			// �����Ҫ�ϴ�ʱ��
			std::cout << "������Ϸʱ��: "
				<< hours << "Сʱ "
				<< minutes << "���� "
				<< seconds << "��\n";
			std::cout << "�ۼ���Ϸʱ��: "
				<< static_cast<int>(total_lol_time / 3600) << "Сʱ "
				<< static_cast<int>(fmod(total_lol_time, 3600) / 60) << "����\n";
		}
		else if (currently_game_running && !is_lol_game_running) {
			// Ӣ�����˶Ծֿ�ʼ
			LOG_IMMEDIATE(" Ӣ�����˶Ծ�������,��ʼ���ӶԾ���Ϣ\n");
			is_lol_game_running = true;
			pollEvents();

			//ThreadWrapper thread(pollEvents);
			//// �����߳�
			//thread.Start();
		
		}
		else if (!currently_game_running && is_lol_game_running) {
			is_lol_game_running = false;

			LOG_IMMEDIATE(" Ӣ�����˶Ծֽ���?/ ����? / �ؿ�?\n");

			//����data , ��ѯ�Ծ�ID�Ƿ����
			_sendHttp_LOL("END", "");
			// TODO ��ѯ�Ծ�ID�Ƿ����


		}

		// ���������Ϸ�Ƿ����� 
		// if (IsProcessRunning(L"��η��Լ?CSGO?��������")) {
		//     std::cout << "��⵽������Ϸ��������\n";
		// }

		std::this_thread::sleep_for(std::chrono::milliseconds(CHECK_INTERVAL_MS));
	}
}

int main() {
	LOG_IMMEDIATE("DLL���ӳ���������");

	try {
		ThreadWrapper thread(MonitorGameProcess);

		//// �����߳�
		thread.Start();
		std::this_thread::sleep_for(std::chrono::seconds(1));
		thread.Detach();

		//MonitorGameProcess();
	}
	catch (const std::exception& e) {
		//std::cerr << "��������: " << e.what() << std::endl;
		 //(e.what());
		LOG_ERROR(e.what());
		return 1;;
	}

	return 0;
}


// �������
extern "C" __declspec(dllexport) const int monitorLOL() {
	return main();
}