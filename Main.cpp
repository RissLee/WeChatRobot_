// WeChatRobot.cpp : ����Ӧ�ó������ڵ㡣
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

	CPaintManagerUI::SetInstance(hInstance);// ����XML��ʱ����Ҫʹ�øþ��ȥ��λEXE��·�������ܼ���XML��·��  ����ʵ�����

	CMainWnd *pMainWnd = new CMainWnd;
	pMainWnd->Create(NULL, L"WeChatMain", UI_WNDSTYLE_FRAME, WS_EX_WINDOWEDGE);
	pMainWnd->CenterWindow();
	pMainWnd->ShowWindow();
	CPaintManagerUI::MessageLoop();

	delete pMainWnd;

	::CoInitialize(NULL);
	return 0;
}

