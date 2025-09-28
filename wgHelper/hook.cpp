#include "pch.h"
#include <windows.h>
#include <detours/detours.h>
#include <map>
#include "HookWegame.h"

// ԭʼShowWindow����ָ��
typedef BOOL(WINAPI* RealShowWindow)(HWND, int);
RealShowWindow originalShowWindow = ShowWindow;

// �洢����ԭʼλ�õĽṹ
struct WindowPosition {
	int x;
	int y;
	int width;
	int height;
};

// �洢����ԭʼλ�õ�ӳ���
std::map<HWND, WindowPosition> windowPositions;

// ���ǵ��Զ���ShowWindow����
BOOL WINAPI MyShowWindow(HWND hWnd, int nCmdShow)
{
	if (nCmdShow == SW_MINIMIZE || nCmdShow == SW_SHOWMINIMIZED || nCmdShow == SW_SHOWMINNOACTIVE || nCmdShow == 0)
	{
		MessageBoxA(NULL, "Injected!", "Test", MB_OK);
		// ��ȡ���ڵ�ǰλ��
		WINDOWPLACEMENT wp;
		wp.length = sizeof(WINDOWPLACEMENT);
		GetWindowPlacement(hWnd, &wp);

		// ����ԭʼλ��
		WindowPosition pos;
		pos.x = wp.rcNormalPosition.left;
		pos.y = wp.rcNormalPosition.top;
		pos.width = wp.rcNormalPosition.right - wp.rcNormalPosition.left;
		pos.height = wp.rcNormalPosition.bottom - wp.rcNormalPosition.top;
		windowPositions[hWnd] = pos;

		// ��ȡ��Ļ�ߴ�
		int screenWidth = GetSystemMetrics(SM_CXSCREEN);
		int screenHeight = GetSystemMetrics(SM_CYSCREEN);

		// �������ƶ�����Ļ�⵫���ֿɼ�
		SetWindowPos(hWnd, NULL,
			//screenWidth, screenHeight,  // �ƶ�����Ļ���½���
			0, 0,  // �ƶ�����Ļ���½���
			pos.width, pos.height,
			SWP_NOACTIVATE | SWP_NOZORDER);

		// ����"�ɹ�"��ʵ����û��������С��
		return TRUE;
	}
	else if (nCmdShow == SW_RESTORE || nCmdShow == SW_SHOWNORMAL ||nCmdShow == SW_SHOW)
	{
		MessageBoxA(NULL, "Injected!", "Test", MB_OK);
		// ����Ƿ��б����λ��
		if (windowPositions.find(hWnd) != windowPositions.end())
		{
			WindowPosition pos = windowPositions[hWnd];

			// �ָ����ڵ�ԭʼλ��
			SetWindowPos(hWnd, NULL,
				pos.x, pos.y,
				pos.width, pos.height,
				SWP_NOZORDER);
			// ��ӳ������Ƴ�
			windowPositions.erase(hWnd);
			return TRUE;
		}
	}

	// �����������ԭʼ����
	return originalShowWindow(hWnd, nCmdShow);
}


typedef BOOL(WINAPI* pShowWindow)(HWND, int);
pShowWindow oShowWindow = NULL;
static BOOL(WINAPI* OriginalMessageBoxW)(HWND, LPCWSTR, LPCWSTR, UINT) = MessageBoxW;

BOOL WINAPI HookedMessageBoxW(HWND hWnd, LPCWSTR lpText, LPCWSTR lpCaption, UINT uType)
{
	return OriginalMessageBoxW(hWnd, L"[Hooked] Original message", lpCaption, uType);
}


// Hook��װ����
void InstallHook()
{
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourAttach(&(PVOID&)originalShowWindow, MyShowWindow);
	DetourAttach(&(PVOID&)OriginalMessageBoxW, HookedMessageBoxW);
	DetourTransactionCommit();
}

// Hookж�غ���
void UninstallHook()
{
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourDetach(&(PVOID&)originalShowWindow, MyShowWindow);
	DetourTransactionCommit();
}