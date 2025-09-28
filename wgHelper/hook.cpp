#include "pch.h"
#include <windows.h>
#include <detours/detours.h>
#include <map>
#include "HookWegame.h"

// 原始ShowWindow函数指针
typedef BOOL(WINAPI* RealShowWindow)(HWND, int);
RealShowWindow originalShowWindow = ShowWindow;

// 存储窗口原始位置的结构
struct WindowPosition {
	int x;
	int y;
	int width;
	int height;
};

// 存储窗口原始位置的映射表
std::map<HWND, WindowPosition> windowPositions;

// 我们的自定义ShowWindow函数
BOOL WINAPI MyShowWindow(HWND hWnd, int nCmdShow)
{
	if (nCmdShow == SW_MINIMIZE || nCmdShow == SW_SHOWMINIMIZED || nCmdShow == SW_SHOWMINNOACTIVE || nCmdShow == 0)
	{
		MessageBoxA(NULL, "Injected!", "Test", MB_OK);
		// 获取窗口当前位置
		WINDOWPLACEMENT wp;
		wp.length = sizeof(WINDOWPLACEMENT);
		GetWindowPlacement(hWnd, &wp);

		// 保存原始位置
		WindowPosition pos;
		pos.x = wp.rcNormalPosition.left;
		pos.y = wp.rcNormalPosition.top;
		pos.width = wp.rcNormalPosition.right - wp.rcNormalPosition.left;
		pos.height = wp.rcNormalPosition.bottom - wp.rcNormalPosition.top;
		windowPositions[hWnd] = pos;

		// 获取屏幕尺寸
		int screenWidth = GetSystemMetrics(SM_CXSCREEN);
		int screenHeight = GetSystemMetrics(SM_CYSCREEN);

		// 将窗口移动到屏幕外但保持可见
		SetWindowPos(hWnd, NULL,
			//screenWidth, screenHeight,  // 移动到屏幕右下角外
			0, 0,  // 移动到屏幕右下角外
			pos.width, pos.height,
			SWP_NOACTIVATE | SWP_NOZORDER);

		// 返回"成功"但实际上没有真正最小化
		return TRUE;
	}
	else if (nCmdShow == SW_RESTORE || nCmdShow == SW_SHOWNORMAL ||nCmdShow == SW_SHOW)
	{
		MessageBoxA(NULL, "Injected!", "Test", MB_OK);
		// 检查是否有保存的位置
		if (windowPositions.find(hWnd) != windowPositions.end())
		{
			WindowPosition pos = windowPositions[hWnd];

			// 恢复窗口到原始位置
			SetWindowPos(hWnd, NULL,
				pos.x, pos.y,
				pos.width, pos.height,
				SWP_NOZORDER);
			// 从映射表中移除
			windowPositions.erase(hWnd);
			return TRUE;
		}
	}

	// 其他情况调用原始函数
	return originalShowWindow(hWnd, nCmdShow);
}


typedef BOOL(WINAPI* pShowWindow)(HWND, int);
pShowWindow oShowWindow = NULL;
static BOOL(WINAPI* OriginalMessageBoxW)(HWND, LPCWSTR, LPCWSTR, UINT) = MessageBoxW;

BOOL WINAPI HookedMessageBoxW(HWND hWnd, LPCWSTR lpText, LPCWSTR lpCaption, UINT uType)
{
	return OriginalMessageBoxW(hWnd, L"[Hooked] Original message", lpCaption, uType);
}


// Hook安装函数
void InstallHook()
{
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourAttach(&(PVOID&)originalShowWindow, MyShowWindow);
	DetourAttach(&(PVOID&)OriginalMessageBoxW, HookedMessageBoxW);
	DetourTransactionCommit();
}

// Hook卸载函数
void UninstallHook()
{
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourDetach(&(PVOID&)originalShowWindow, MyShowWindow);
	DetourTransactionCommit();
}