#include "pch.h"
#include <windows.h>
#include <detours/detours.h>
#include "HookWegame.h"


// ����ԭʼλ��
RECT originalRect = { 100, 100, 800, 600 };
bool hasSavedRect = false;

//BOOL WINAPI hookShowWindow(HWND hWnd, int nCmdShow) {
//    OutputDebugInfo(" WEGAME HOOK �ɹ�");
//    if (!hasSavedRect) {
//        // ��ȡ�����洰��λ��
//        GetWindowRect(hWnd, &originalRect);
//        hasSavedRect = true;
//    }
//
//    if (nCmdShow == SW_MINIMIZE) {
//        // ģ����С�����������Ƴ���Ļ
//        MoveWindow(hWnd, -3000, -3000,
//            originalRect.right - originalRect.left,
//            originalRect.bottom - originalRect.top, TRUE);
//        return TRUE;
//    }
//    //else if (nCmdShow == SW_RESTORE) {
//    else if (nCmdShow == SW_SHOW) {
//        // ģ�⻹ԭ���ָ�ԭʼλ��
//        MoveWindow(hWnd,
//            originalRect.left,
//            originalRect.top,
//            originalRect.right - originalRect.left,
//            originalRect.bottom - originalRect.top, TRUE);
//        return TRUE;
//    }
//
//    // Ĭ����Ϊ
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