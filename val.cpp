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


// 静态成员初始化
std::unique_ptr<MitmDumpController> MitmDumpController::instance;
std::mutex MitmDumpController::instanceMutex;

int updateMatch() {
    try {

        auto& controller = MitmDumpController::getInstance();
     
        if (controller.start(8080, "--mode transparent --ssl - insecure --set upstream_cert = false  --allow - hosts \"wegame.com.cn\"")) {
            std::cout << "mitmdump started with PID: " << controller.pid() << std::endl;

            // 主循环处理输出
            while (controller.running()) {
                std::string output = controller.getOutput();
                if (!output.empty()) {
                    std::cout << "mitmdump: " << output << std::endl;
                }

                // 在这里添加你的业务逻辑
                // ...

                // 避免CPU占用过高
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }

            //--mode transparent --ssl - insecure --set upstream_cert = false  --allow - hosts "wegame.com.cn"
   
            // 停止 mitmdump (可选，析构函数会自动调用)
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
        std::cerr << "Error: Mitmproxy没有关闭?" << std::endl;
        return 1;
    }

    return 0;
    //mpController.start(8080,"wegame.com.cn");
}