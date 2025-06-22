#include "pch.h"

// 1.д״̬ת��, ״̬�����Ƶ�¼���.
// 2. ��wegame - > ������η��Լ - > �ر���η��Լ - > �ر�wegame
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


// ��̬��Ա��ʼ��
std::unique_ptr<MitmDumpController> MitmDumpController::instance;
std::mutex MitmDumpController::instanceMutex;

int updateMatch() {
    try {

        auto& controller = MitmDumpController::getInstance();
     
        if (controller.start(8080, "--mode transparent --ssl - insecure --set upstream_cert = false  --allow - hosts \"wegame.com.cn\"")) {
            std::cout << "mitmdump started with PID: " << controller.pid() << std::endl;

            // ��ѭ���������
            while (controller.running()) {
                std::string output = controller.getOutput();
                if (!output.empty()) {
                    std::cout << "mitmdump: " << output << std::endl;
                }

                // ������������ҵ���߼�
                // ...

                // ����CPUռ�ù���
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }

            //--mode transparent --ssl - insecure --set upstream_cert = false  --allow - hosts "wegame.com.cn"
   
            // ֹͣ mitmdump (��ѡ�������������Զ�����)
            controller.stop();
        }
        else {
            std::cerr << "Failed to start mitmdump" << std::endl;
            return 1;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    catch (...) {
        std::cerr << "Error: Mitmproxyû�йر�?" << std::endl;
        return 1;
    }

    return 0;
    //mpController.start(8080,"wegame.com.cn");
}