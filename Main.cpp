// WeChatRobot.cpp : 定义应用程序的入口点。
//

#include "stdafx.h"

#include "CMainWnd.h"
#include "Main.h"


int APIENTRY _tWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPTSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
	::CoInitialize(NULL);

	CPaintManagerUI::SetInstance(hInstance);// 加载XML的时候，需要使用该句柄去定位EXE的路径，才能加载XML的路径  窗口实例句柄

	CMainWnd *pMainWnd = new CMainWnd;
	pMainWnd->Create(NULL, L"WeChatMain", UI_WNDSTYLE_FRAME, WS_EX_WINDOWEDGE);
	pMainWnd->CenterWindow();
	pMainWnd->ShowWindow();
	CPaintManagerUI::MessageLoop();

	delete pMainWnd;

	::CoInitialize(NULL);
	return 0;
}

