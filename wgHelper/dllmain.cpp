// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "pch.h"
#include "debugV.h"
#include "HookWegame.h"


extern "C" __declspec(dllexport) void DoSomething() {
    MessageBoxA(NULL, "Injected!", "Test", MB_OK);
}

extern "C" __declspec(dllexport) int AddNumbers(int a, int b) {
    return a + b;
}
DWORD WINAPI WaitAndExecute(LPVOID lpParam) {
    HWND hwndNotepad = NULL;

    // 等待记事本窗口出现
    while (hwndNotepad == NULL) {
        hwndNotepad = FindWindow(L"NOTEPAD", NULL);
        if (hwndNotepad != NULL) {
            MessageBoxW(NULL, L"记事本窗口已找到！", L"Success", MB_OK);
            // 在这里执行你的 Hook 或其他操作
            break;
        }
        Sleep(2000);  // 避免 CPU 占用过高
    }

    return 0;
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:

        CreateThread(NULL, 0, WaitAndExecute, NULL, 0, NULL);

        DisableThreadLibraryCalls(hModule); // 优化性能
        //SetupExceptionHandler();
        //CreateThread(nullptr, 0, [](LPVOID) -> DWORD {
        //    DoSomething();
        //    return 0;
        //    }, nullptr, 0, nullptr);
        try {
            InstallHook();

            OutputDebugStringA("[DLL] DLL Injected Successfully!\n");
  
            //InstallHook();
            // 安装 Hook、初始化逻辑
        }
        catch (...) {
            // 防止异常传播到宿主程序
            OutputDebugInfo("DLL_PROCESS_ATTACH:::注入出现了异常");
            
        }

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        UninstallHook();
        break;
    }
    return TRUE;
}

