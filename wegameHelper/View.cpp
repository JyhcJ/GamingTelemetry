// View.cpp: 实现文件
//

#include "pch.h"
#include "wegameHelper.h"
#include "afxdialogex.h"
#include "View.h"
#include "HookWegame.h"


// View 对话框

IMPLEMENT_DYNAMIC(View, CDialogEx)

View::View(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG1, pParent)
{

}

View::~View()
{
}

void View::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(View, CDialogEx)
END_MESSAGE_MAP()




BOOL View::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  在此添加额外的初始化
	MessageBoxA(NULL, "Injected!", "Test", MB_OK);
	try {
		OutputDebugStringA("[DLL]DLL Injected Successfully!\n");
		InstallHook();
		// 安装 Hook、初始化逻辑
	}
	catch (...) {
		// 防止异常传播到宿主程序
		OutputDebugStringA("注入发生了异常");
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}
