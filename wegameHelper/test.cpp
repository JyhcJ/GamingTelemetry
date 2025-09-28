#include "pch.h"
#include <MinHook.h>

typedef BOOL(WINAPI* pShowWindow)(HWND, int);
pShowWindow oShowWindow = NULL;

// ����ԭʼλ��
RECT originalRect = { 100, 100, 800, 600 };
bool hasSavedRect = false;

BOOL WINAPI hookShowWindow(HWND hWnd, int nCmdShow) {
    OutputDebugStringA(" WEGAME HOOK �ɹ�");
    if (!hasSavedRect) {
        // ��ȡ�����洰��λ��
        GetWindowRect(hWnd, &originalRect);
        hasSavedRect = true;
    }

    if (nCmdShow == SW_MINIMIZE) {
        // ģ����С�����������Ƴ���Ļ
        MoveWindow(hWnd, -3000, -3000,
            originalRect.right - originalRect.left,
            originalRect.bottom - originalRect.top, TRUE);
        return TRUE;
    }
    //else if (nCmdShow == SW_RESTORE) {
    else if (nCmdShow == SW_SHOW) {
        // ģ�⻹ԭ���ָ�ԭʼλ��
        MoveWindow(hWnd,
            originalRect.left,
            originalRect.top,
            originalRect.right - originalRect.left,
            originalRect.bottom - originalRect.top, TRUE);
        return TRUE;
    }

    // Ĭ����Ϊ
    return oShowWindow(hWnd, nCmdShow);
}

//void InstallHook() {
//    MH_Initialize();
//    MH_CreateHook(&ShowWindow, &hookShowWindow, reinterpret_cast<LPVOID*>(&oShowWindow));
//    MH_EnableHook(&ShowWindow);
//}