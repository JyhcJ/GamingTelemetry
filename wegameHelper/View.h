#pragma once
#include "afxdialogex.h"


// View 对话框

class View : public CDialogEx
{
	DECLARE_DYNAMIC(View)

public:
	View(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~View();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG1 };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
};
