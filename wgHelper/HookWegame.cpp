#include "pch.h"
#include <windows.h>
#include <detours/detours.h>
#include "HookWegame.h"


// 保存原始位置
RECT originalRect = { 100, 100, 800, 600 };
bool hasSavedRect = false;

//BOOL WINAPI hookShowWindow(HWND hWnd, int nCmdShow) {
//    OutputDebugInfo(" WEGAME HOOK 成功");
//    if (!hasSavedRect) {
//        // 获取并保存窗口位置
//        GetWindowRect(hWnd, &originalRect);
//        hasSavedRect = true;
//    }
//
//    if (nCmdShow == SW_MINIMIZE) {
//        // 模拟最小化：将窗口移出屏幕
//        MoveWindow(hWnd, -3000, -3000,
//            originalRect.right - originalRect.left,
//            originalRect.bottom - originalRect.top, TRUE);
//        return TRUE;
//    }
//    //else if (nCmdShow == SW_RESTORE) {
//    else if (nCmdShow == SW_SHOW) {
//        // 模拟还原：恢复原始位置
//        MoveWindow(hWnd,
//            originalRect.left,
//            originalRect.top,
//            originalRect.right - originalRect.left,
//            originalRect.bottom - originalRect.top, TRUE);
//        return TRUE;
//    }
//
//    // 默认行为
//    return oShowWindow(hWnd, nCmdShow);
//}
//void InstallHook() {
//    MH_Initialize();
//    MH_CreateHook(&ShowWindow, &hookShowWindow, reinterpret_cast<LPVOID*>(&oShowWindow));
//    MH_EnableHook(&ShowWindow);
//}




int main123()
{

    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());

    DetourTransactionCommit();

    MessageBoxW(NULL, L"Hello", L"Test", MB_OK);



    return 0;
}