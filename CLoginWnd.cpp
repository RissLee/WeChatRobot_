#include "StdAfx.h"
#include <fstream>
#include "httpRequest.h"
#include "DataFrm.h"
#include "CLoginWnd.h"
//全局变量
BOOL needNewUUID = TRUE;
string g_uuid;
string redirect_uri;
WeChatInfo g_wechatinfo;

string getuuid()
{
	string res = HttpRequest("login.weixin.qq.com/jslogin?appid=wx782c26e4c19acffb&redirect_uri=https%3A%2F%2Fwx.qq.com%2Fcgi-bin%2Fmmwebwx-bin%2Fwebwxnewloginpage&fun = new&lang = zh_CN");

	size_t pos = res.find("uuid");
	if (pos == string::npos)
		return NULL;
	else
	{
		pos += 8;
		res = res.substr(pos, res.size() - pos - 2);
	}
	return res;
}
string xmlMatch(string res,char *strbegin,char *strend)
{
	if (res.empty()) return NULL;
	size_t index = res.find(strbegin);
	size_t indexend = res.find(strend);
	if (index == string::npos || index == string::npos)
		return NULL;
	size_t indexbegin = index + strlen(strbegin);
	return res.substr(indexbegin, indexend - indexbegin);
}
DWORD CALLBACK httpGetCookie(PVOID param)
{
	CLoginWnd *pFramWnd = (CLoginWnd *)param;
	if (redirect_uri.empty()) return -1;
	redirect_uri += "&fun=new&version=v2";
	char *strUrl = const_cast<char *>(redirect_uri.c_str());
	char *res = HttpRequest(strUrl);
	g_wechatinfo.skey = xmlMatch(res, "<skey>", "</skey>");
	g_wechatinfo.wxsid = xmlMatch(res, "<wxsid>", "</wxsid>");
	g_wechatinfo.wxuin = xmlMatch(res, "<wxuin>", "</wxuin>");
	g_wechatinfo.pass_ticket = xmlMatch(res, "<pass_ticket>", "</pass_ticket>");
	//结束登陆程序
	pFramWnd->Close();
	return 0;
}

DWORD CALLBACK httpGetLoginStatic(PVOID param)
{
	CLoginWnd *pFramWnd = (CLoginWnd *)param;
	string Url = "login.weixin.qq.com/cgi-bin/mmwebwx-bin/login?loginicon=true&uuid=" + g_uuid + "&tip=0&r=1203389591&_=" + string(getTimeStamp());
	char *strUrl = const_cast<char *>(Url.c_str());
	for (int i = 0; i < 5; i++)
	{
		char *res = HttpRequest(strUrl);
		char *pos = strstr(res, "redirect_uri");
		if (pos)
		{
			pos += 14;
			string strPos = pos;
			redirect_uri=strPos.substr(0, strPos.size() - 2);
			HANDLE hThread = CreateThread(0, 0, httpGetCookie, param, 0, 0);
			CloseHandle(hThread);
			return 1;
		}
	}
	return 0;
}
DWORD CALLBACK httpGetScanStatic(PVOID param)
{
	CLoginWnd *pFramWnd = (CLoginWnd *)param;
	string Url = "login.weixin.qq.com/cgi-bin/mmwebwx-bin/login?loginicon=true&uuid=" + g_uuid + "&tip=0&r=1203389591&_=" + string(getTimeStamp());
	char *strUrl = const_cast<char *>(Url.c_str());
	for (int i = 0; i < 5; i++)
	{
		char *res = HttpRequest(strUrl);
		char *pos = strstr(res, "base64,");	//获取到头像数据
		if (pos)
		{
			pos += 7;
			char *imagdata = base64_decode(pos, strlen(pos) - 1);
			fstream file;
			file.open("userAvatar.jpg", ios::out | ios::binary);
			file.write(imagdata, 3 * (strlen(pos) - 1) / 4);
			file.close();

			pFramWnd->m_imgUserAvatar->SetBkImage(L"userAvatar.jpg");
			pFramWnd->m_imgErWeiMa->SetVisible(false);

			RECT rc = pFramWnd->m_txtTips->GetPos();
			rc.top -= 80;
			rc.bottom -= 80;
			pFramWnd->m_txtTips->SetPos(rc);
			pFramWnd->m_txtTips->SetText(L"请在手机上确认登陆");
			//继续查询登陆状态
			HANDLE hThread = CreateThread(0, 0, httpGetLoginStatic, param, 0, 0);
			CloseHandle(hThread);
			return 1;
		}
	}
	needNewUUID = TRUE;
	return 0;
}
void CLoginWnd::ShowErWeima()
{
	if (needNewUUID)
	{
		string uuid = getuuid();
		g_uuid = uuid;
		string Url = "login.weixin.qq.com/qrcode/" + uuid;
		char *strUrl = const_cast<char *>(Url.c_str());
		char *res = HttpRequest(strUrl);
		if (!HttpGetResBufSize())
			return; //未获取到二维码

		fstream file;
		file.open("erweima.jpg", ios::out | ios::binary);
		file.write(res, HttpGetResBufSize());
		file.close();

		needNewUUID = FALSE;			//已更新新的uuid
		HANDLE hThread = CreateThread(0, 0, httpGetScanStatic, this, 0, 0);
		CloseHandle(hThread);
	}
	m_imgErWeiMa->SetBkImage(L"file='erweima.jpg' source = '27,27,402,402'");	//source 截掉二维码旁白
}

//////////////////
// CFramWindowWnd 类成员函数定义
///////////////////
CLoginWnd::CLoginWnd(CDuiString pathxml) :
m_PathXml(pathxml),
m_imgErWeiMa(NULL),
m_imgUserAvatar(NULL),
m_txtTips(NULL)
{
}

CLoginWnd::~CLoginWnd()
{
}
LRESULT CLoginWnd::OnCreate(WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	return 0;
}
void CLoginWnd::Notify(TNotifyUI& msg)
{
	if (msg.sType == DUI_MSGTYPE_CLICK)	//判断消息类型，如果是点击
	{
		if (msg.pSender->GetName() == _T("btnClose"))	//如果控件名字是btnClose	
			Close();
		if (msg.pSender->GetName() == L"btnMinSize")
			::SendMessageA(m_hWnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
		if (msg.pSender->GetName()==_T("btnLogin"))
		{
//			MessageBoxA(m_hWnd, "Login click!", "tips", MB_OK);
		}
	}


}

void CLoginWnd::InitWindow()
{
//	m_btnClose = static_cast <CButtonUI*> (m_PaintManager.FindControl(_T("btnClose")));
	m_imgErWeiMa = m_PaintManager.FindControl(_T("imgErWeiMa"));
	m_imgUserAvatar = m_PaintManager.FindControl(_T("imgUserAvatar"));
	m_txtTips = m_PaintManager.FindControl(_T("txtTips"));
	 
	ASSERT(m_imgErWeiMa || m_imgUserAvatar || m_txtTips);
}

LRESULT CLoginWnd::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_CREATE:
		break;
	case WM_PAINT:
		ShowErWeima();
		break;
	default:
		break;
	}
	return WindowImplBase::HandleMessage(uMsg, wParam, lParam);
	
}

LRESULT CLoginWnd::OnNcPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	return WindowImplBase::OnNcPaint(uMsg, wParam, lParam, bHandled);
}

// HRESULT CFramWindowWnd::OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
// {
// 
// 	return 0;
// }

HRESULT CLoginWnd::OnErasebkgnd(WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	return 0;
}
