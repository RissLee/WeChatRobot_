#include "stdafx.h"
#include <vector>
#include <map>
#include <fstream>
#include "DataFrm.h"
#include <time.h>
#include "httpRequest.h"
#include "CLoginWnd.h"
#include "CSetWnd.h"
#include "CMainWnd.h"
#include "MyAvatar.hpp"

#define  debug_

#define WM_GET_CHAT		WM_USER+100
#define WM_GET_CONTACT	WM_USER+101
#define WM_SYNC_CHECK	WM_USER+102

#define WM_HAVE_MSG		WM_USER+105
#define WM_SEND_MSG		WM_USER+106

map<string, vector<wxMSG>>map_ChatLog;
vector <CListContainerElementUI *>chatlist;

extern WeChatInfo g_wechatinfo;

vector<ChatInfo> g_ChatList;
vector<ChatInfo> g_ContactList;
vector<wxMSG> g_MsgList;

ChatInfo curChatInfo;	//当前聊天信息
ChatInfo curContactInfo; //当前选中联系人信息
ChatInfo UserInfo;		//用户信息

static string getMsgLocalID()
{
	string msgId = getTimeStamp();
	srand(time(NULL));
	int rands = rand() % 1000;
	char strRand[5] = { 0 };
	sprintf_s(strRand, "0%d", rands);
	msgId += strRand;
	return msgId;
}
static Json::Value getBaseRequestJson()
{
	Json::Value BaseRequest;
	BaseRequest["Uin"] = atoll(g_wechatinfo.wxuin.c_str());
	BaseRequest["Sid"] = g_wechatinfo.wxsid;
	BaseRequest["Skey"] = g_wechatinfo.skey;
	BaseRequest["DeviceID"] = "e777740280289637";

	return BaseRequest;
}
static Json::Value getSyncKey()
{
	Json::Value syncKey;
	int SyncCount = g_wechatinfo.SyncCount;
	syncKey["Count"] = SyncCount;
	for (int i = 0; i < SyncCount; ++i)
	{
		Json::Value value;
		value["Key"] = g_wechatinfo.s_synckey[i].Key;
		value["Val"] = g_wechatinfo.s_synckey[i].Val;

		syncKey["List"].append(value);
	}
	return syncKey;
}

static string getFormateSyncKey()
{
	string FormatSyncKey;
	for (int i = 0; i < g_wechatinfo.SyncCount; ++i)
	{
		char strSync[20];
		sprintf_s(strSync, "%d_%ld", g_wechatinfo.s_synckey[i].Key, g_wechatinfo.s_synckey[i].Val);
		FormatSyncKey += strSync;
		if ((i + 1) < g_wechatinfo.SyncCount)	FormatSyncKey += "|";
	}
	return FormatSyncKey;
}

static string getDeviceID()
{
	srand(time(0));
	char strrand[30] = { 0 };
	sprintf_s(strrand, "e%d%d%d%d", rand(), rand(), rand(), rand());
	string deviceId = strrand;
	return deviceId.substr(0, 16);
}

static void updateSyncKey(string res)
{
	Json::Reader reader;
	Json::Value root;
	if (reader.parse(res, root))
	{
		Json::Value SyncKeyList;
		SyncKeyList = root["SyncKey"]["List"];
		int Count = SyncKeyList.size();
		g_wechatinfo.SyncCount = Count;
		for (int i = 0; i < Count; ++i)
		{
			g_wechatinfo.s_synckey[i].Key = SyncKeyList[i]["Key"].asInt();
			g_wechatinfo.s_synckey[i].Val = SyncKeyList[i]["Val"].asInt();
		}
	}
}

static void pushChatlog(wxMSG wxmsg)
{
	string mapstr;
	mapstr = (wxmsg.FromUserName == UserInfo.UserName) ? wxmsg.ToUserName : wxmsg.FromUserName;

	if (map_ChatLog.count(mapstr))
	{
		map_ChatLog[mapstr].push_back(wxmsg);
	}
	else
	{
		vector<wxMSG>vecWxmsg;
		vecWxmsg.push_back(wxmsg);
		map_ChatLog.insert(pair < string, vector<wxMSG> >(mapstr, vecWxmsg));
	}
}

Json::Value CMainWnd::createMsgJson()
{
	Json::Value Msg;
	Msg["Type"] = 1;
	Msg["Content"] = UnicodeToUTF8(m_reditChat->GetText().GetData());
	Msg["FromUserName"] = UserInfo.UserName;
	Msg["ToUserName"] = curChatInfo.UserName;
	Msg["LocalID"] = getMsgLocalID();
	Msg["ClientMsgId"] = getMsgLocalID();

	return Msg;
}

DWORD CALLBACK httpsynccheckThread(PVOID param)
{
	CMainWnd *pMainWnd = (CMainWnd *)param;
	while (true)
	{
		string CheckUrl = "webpush.wx2.qq.com/cgi-bin/mmwebwx-bin/synccheck?r=" + string(getTimeStamp()) + "&skey=" + UrlEncode(g_wechatinfo.skey) +
			"&sid=" + UrlEncode(g_wechatinfo.wxsid) + "&uin=" + g_wechatinfo.wxuin + "&deviceid=" + getDeviceID() + "&synckey=" + UrlEncode(getFormateSyncKey());

		char *c_Url = const_cast<char *>(CheckUrl.c_str());
		char *res = HttpRequest(c_Url);		//		window.synccheck={retcode:"0",selector:"0"}

		CDuiString tips = ANSIToUnicode((res+20)).c_str();
		pMainWnd->m_labelTips->SetText(tips);
		
		int retcode = 0, selector = 0;
		sscanf_s(res, "window.synccheck={retcode:\"%d\",selector:\"%d\"}", &retcode, &selector);
		if (0!=retcode) break;
		switch (selector)
		{
		case 0:	//No MSG
			break;
		case 2: //Send MSG
			SendMessageA(pMainWnd->GetHWND(), WM_HAVE_MSG, 0, 0);
			break;
		case 4:	//Friend Group?
			break;
		case 6:	//Have MSG
			SendMessageA(pMainWnd->GetHWND(), WM_HAVE_MSG, 0, 0);
			break;
		case 7:
			SendMessageA(pMainWnd->GetHWND(), WM_HAVE_MSG, 0, 0);
		default:
			SendMessageA(pMainWnd->GetHWND(), WM_HAVE_MSG, 0, 0);
			break;
		}	
	}
		
	return 0;
}

DWORD CALLBACK httpGetChatListInfoThread(PVOID param)
{
	CMainWnd *pMainWnd = (CMainWnd *)param;

	Json::Value postdata, postdataitem;
	postdataitem["Uin"] = g_wechatinfo.wxuin;
	postdataitem["Sid"] = g_wechatinfo.wxsid;
	postdataitem["Skey"] = g_wechatinfo.skey;
	postdataitem["DeviceID"] = "e314324097640625";
	postdata["BaseRequest"] = postdataitem;
	Json::FastWriter writer;
	string podata = writer.write(postdata);
	string Url = "wx2.qq.com/cgi-bin/mmwebwx-bin/webwxinit?r=514858657&pass_ticket=" + g_wechatinfo.pass_ticket;

	char *strUrl = const_cast<char *>(Url.c_str());
	char *strPostData = const_cast<char *>(podata.c_str());
	string res = HttpRequest(strUrl, "POST",strPostData);
	res = UTF8ToANSI(res);

	Json::Reader reader;
	Json::Value root;
	if (reader.parse(res,root))
	{
		Json::Value ContactList;
		ContactList = root["ContactList"];
		if (ContactList.isNull())
		{
			return -1;
		}
		for (unsigned int i = 0; i < ContactList.size();++i)
		{
			ChatInfo s_chatinfo;

			s_chatinfo.UserName =ContactList[i]["UserName"].asString();
			s_chatinfo.NickName = ContactList[i]["NickName"].asString();
			s_chatinfo.HeadImgUrl = ContactList[i]["HeadImgUrl"].asString();
			s_chatinfo.Signature = ContactList[i]["Signature"].asString();
			s_chatinfo.Province = ContactList[i]["Province"].asString();
			s_chatinfo.City = ContactList[i]["City"].asString();
//			s_chatinfo.ContactFlag = ContactList[i]["ContactFlag"].asInt();

			//入容器
			g_ChatList.push_back(s_chatinfo);
		}
		SendMessage(pMainWnd->GetHWND(), WM_GET_CHAT, 0, 0);	//发送chat重绘消息
		//填充用户信息
		Json::Value User;
		User = root["User"];
		UserInfo.UserName = User["UserName"].asString();
		UserInfo.NickName = User["NickName"].asString();
		UserInfo.HeadImgUrl = User["HeadImgUrl"].asString();
		UserInfo.Signature = User["Signature"].asString();
// 		UserInfo.Province = User["Province"].asString();
// 		UserInfo.City =User["City"].asString();

		//获取SyncKey 
		Json::Value SyncKey;
		SyncKey = root["SyncKey"]["List"];
		g_wechatinfo.SyncCount = root["SyncKey"]["Count"].asInt();

		for (unsigned int i = 0; i < SyncKey.size();++i)
		{
			g_wechatinfo.s_synckey[i].Key = SyncKey[i]["Key"].asInt();
			g_wechatinfo.s_synckey[i].Val = SyncKey[i]["Val"].asInt();
		}
	}

	return 0;
}



//POST 检查信箱  返回值 更新SyncKey
string httpWebwxsync()
{
	string Url = "wx2.qq.com/cgi-bin/mmwebwx-bin/webwxsync?sid=" + g_wechatinfo.wxsid + "&skey=" + g_wechatinfo.skey + "&pass_ticket=" + g_wechatinfo.pass_ticket;
	//struct Post data
	Json::Value jPostData;
	jPostData["BaseRequest"] = getBaseRequestJson();
	jPostData["SyncKey"] = getSyncKey();
	jPostData["rr"] = ~(long)atoll(getTimeStamp());		//~new date
	Json::FastWriter writer;
	string PostData = writer.write(jPostData);

	char *c_PostData = const_cast<char *>(PostData.c_str());
	char *c_url = const_cast<char *>(Url.c_str());

	string res = HttpRequest(c_url, "POST", c_PostData);

	//更新SyncKey
	updateSyncKey(res);

	return UTF8ToANSI(res);
}


DWORD CALLBACK ContactListRedrawThread(PVOID param)
{
	CMainWnd *pMainWnd = (CMainWnd *)param;
	pMainWnd->httpGetContactList();	
	//检查信息
//	httpWebwxsync();	//Post

	for (unsigned int i = 0; i < g_ContactList.size(); ++i)
	{
		string NickName = g_ContactList[i].NickName;
		string Signature = g_ContactList[i].Signature;
		string HeadImg = "cache\\";
		HeadImg += g_ContactList[i].UserName;
		//download avatar
		pMainWnd->dowondChatListAvatar(g_ContactList[i].HeadImgUrl, HeadImg);
		//add list
		pMainWnd->ContactListAddItem(ANSIToUnicode(NickName).c_str(), ANSIToUnicode(HeadImg).c_str());
	}
	SendMessage(pMainWnd->GetHWND(),WM_SYNC_CHECK, 0, 0);		//开始消息check循环
	return 0;
}

CMainWnd::CMainWnd() :
m_labelTips(NULL)
, m_listChatPanel(NULL)
, m_reditChat(NULL)
, m_SendMessageS(NULL)
{}
CMainWnd::~CMainWnd()
{}
LRESULT CMainWnd::OnCreate(WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
#ifndef debug
	//登陆界面
	CLoginWnd* wnd = new CLoginWnd(L"WeChatRobotLogin.xml"); // 生成对象
	ASSERT(wnd);
	wnd->Create(NULL, L"WeChatLogin", UI_WNDSTYLE_DIALOG, WS_EX_WINDOWEDGE); // 创建DLG窗口
	wnd->CenterWindow(); // 窗口居中
	wnd->ShowModal();
	delete wnd; // 删除对象
#endif
	bHandled = FALSE;
	return 0;
}
void CMainWnd::InitWindow()
{
	m_labelTips = m_PaintManager.FindControl(L"labelTips");
	m_listChatPanel = static_cast<CListUI *>(m_PaintManager.FindControl(L"listChatRoom"));
	m_reditChat = m_PaintManager.FindControl(L"reditChat");
	m_SendMessageS = static_cast<CButtonUI *>(m_PaintManager.FindControl(L"btnSendMessage"));

	ASSERT(m_labelTips && m_listChatPanel && m_reditChat && m_SendMessageS);

	//获取列表
	HANDLE hThread = CreateThread(0, 0, httpGetChatListInfoThread, this, 0, 0);
	CloseHandle(hThread);
}
void CMainWnd::Notify(TNotifyUI& msg)
{
	static CDuiString sWhichTab;
	if (msg.sType == DUI_MSGTYPE_CLICK)	//判断消息类型，如果是点击
	{
		m_labelTips->SetText(msg.pSender->GetName());

		if (msg.pSender->GetName() == _T("btnClose"))	//如果控件名字是btnClose	
			PostQuitMessage(0);
		else if (msg.pSender->GetName() == L"btnMinimize")
			::SendMessageA(m_hWnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
		else if (msg.pSender->GetName() == _T("btnMaximize"))
			::SendMessageA(m_hWnd, WM_SYSCOMMAND, SC_MAXIMIZE, 0);
		else if (msg.pSender->GetName()==_T("btnMyAvatar"))
		{
			//Avatar Interface
			CMyAvatarWnd* wnd = new CMyAvatarWnd(L"MyAvatar.xml"); // 生成对象
			ASSERT(wnd);
			POINT pt = { msg.ptMouse.x, msg.ptMouse.y };
			::ClientToScreen(*this, &pt);
			wnd->Init(msg.pSender, pt);
			wnd->AdjustPostion();
		}
		else if (msg.pSender->GetName()==_T("btnSet"))
		{
			//Set Interface
			CSetWnd* wnd = new CSetWnd(L"set//setMain.xml"); // 生成对象
			ASSERT(wnd);
			wnd->Create(m_hWnd, _T("SetInterface"), UI_WNDSTYLE_DIALOG, WS_EX_WINDOWEDGE); // 创建DLG窗口
			wnd->CenterWindow(); // 窗口居中
//			wnd->ShowModal();
		}
		else if (msg.pSender->GetName()==L"btnSendMessage")
		{	
			httpSendMsg();

// 			AddMyMsgList();
// 			AddYourMsgList();
		}
		else if (msg.pSender->GetName() == L"TabCBtnSendMsg")
		{
			COptionUI *poptChat = static_cast<COptionUI *>(m_PaintManager.FindControl(L"optChat"));
			ASSERT(poptChat);
			poptChat->Activate();
			curChatInfo = curContactInfo;
		}
	}
	if (msg.sType==DUI_MSGTYPE_SELECTCHANGED)
	{
		CDuiString name = msg.pSender->GetName();
		sWhichTab = name;
		CTabLayoutUI* pTabNavigation = static_cast<CTabLayoutUI*>(m_PaintManager.FindControl(_T("tabNavigation")));
		CTabLayoutUI* pTabRoom = static_cast<CTabLayoutUI*>(m_PaintManager.FindControl(_T("tabRoom")));
		ASSERT(pTabNavigation);
		if (name == _T("optChat"))
		{
			pTabNavigation->SelectItem(0);
			pTabRoom->SelectItem(1);
		}
		else if (name == _T("optContact"))
		{
			pTabNavigation->SelectItem(1);
			pTabRoom->SelectItem(2);
			UpdataContactInfo();
		}
		else if (name == _T("optFavorites"))
		{
			pTabNavigation->SelectItem(2);
			pTabRoom->SelectItem(3);
		}
	}
	if (msg.sType==DUI_MSGTYPE_ITEMSELECT)
	{
		CDuiString name = msg.pSender->GetName();
		if (name == _T("listChat"))
		{
			UINT index = (UINT)msg.wParam;
			CControlUI *labelTitleName = m_PaintManager.FindControl(_T("labelTitleName"));
			if (index < g_ChatList.size())
			{
				curChatInfo = g_ChatList[index];
				labelTitleName->SetText(ANSIToUnicode(curChatInfo.NickName).c_str());
				UpdataChatPanel();
			}	
		}
		else if (name == _T("listContact"))
		{
			UINT index = (UINT)msg.wParam;
			CControlUI *labelTitleName = m_PaintManager.FindControl(_T("labelTitleName"));
			if (index < g_ContactList.size())
			{
				curContactInfo = g_ContactList[index];
				labelTitleName->SetText(L"");
				UpdataContactInfo();
			}
		}
		else if (name == _T("listFavorites"))
		{		
		}
		CDuiString str;
		int index = msg.wParam;
		str.Format(L"_index:%d", index);
		m_labelTips->SetText(name + str);
	}
	if (msg.sType == DUI_MSGTYPE_RETURN)
	{
		if (msg.pSender->GetName()==_T("reditChat"))
		{
			m_SendMessageS->Activate();
		}
		m_labelTips->SetText(msg.pSender->GetName());
	}

}

void CMainWnd::AddMyMsgList(wxMSG wxmsg)
{
	CDialogBuilder builder;
	CListContainerElementUI *plist_element = static_cast<CListContainerElementUI*>(builder.Create(_T("listMyMsg.xml"), 0, 0));
	ASSERT(plist_element);

	CTextUI *ptextChatContent = static_cast<CTextUI *>(plist_element->FindSubControl(L"textChatContent"));
	CLabelUI *plabelChatHeadImg = static_cast<CLabelUI *>(plist_element->FindSubControl(L"labelChatHeadImg"));
	CHorizontalLayoutUI *phLayout = static_cast<CHorizontalLayoutUI *>(plist_element->FindSubControl(L"hlayoutCont"));
	ASSERT(ptextChatContent && plabelChatHeadImg);

	CDuiString msgContent = ANSIToUnicode(wxmsg.Content).c_str();
	ptextChatContent->SetText(msgContent);

	string headimgfoder = "cache\\"+UserInfo.UserName;
	plabelChatHeadImg->SetBkImage(ANSIToUnicode(headimgfoder).c_str());

	m_listChatPanel->AddAt(plist_element, m_listChatPanel->GetCount());

	SIZE szAvailable = { 0 };
	ptextChatContent->EstimateSize(szAvailable);

	int strLen = ptextChatContent->GetFixedWidth();
	int cTextwidth = 0, cTextheight = 0;
	cTextwidth = MAX(strLen, ptextChatContent->GetMinWidth());
	cTextwidth = MIN(cTextwidth, ptextChatContent->GetMaxWidth());
	phLayout->SetFixedWidth(cTextwidth);		//keep content list item left align

	plist_element->SetFixedHeight(54 + strLen / ptextChatContent->GetMaxWidth()*21);	//控件的高度 20+34+(n-1)x25  n为字体行数 20:layout inset 
}
void CMainWnd::AddFriendMsgList(wxMSG wxmsg)
{
	CDialogBuilder builder;
	CListContainerElementUI *plist_element = static_cast<CListContainerElementUI*>(builder.Create(_T("listFriendMsg.xml"), 0, 0));
	ASSERT(plist_element);

	CTextUI *ptextChatContent = static_cast<CTextUI *>(plist_element->FindSubControl(L"textChatContent"));
	CLabelUI *plabelChatHeadImg = static_cast<CLabelUI *>(plist_element->FindSubControl(L"labelChatHeadImg"));

	ASSERT(ptextChatContent && plabelChatHeadImg);

	ptextChatContent->SetText(ANSIToUnicode(wxmsg.Content).c_str());

	CDuiString imgForder = L"cache\\";
	CDuiString imgfile = ANSIToUnicode(wxmsg.FromUserName).c_str();
	plabelChatHeadImg->SetBkImage(imgForder + imgfile);

	m_listChatPanel->AddAt(plist_element, m_listChatPanel->GetCount());

	SIZE szAvailable = { 0 };
	ptextChatContent->EstimateSize(szAvailable);
	int strLen = ptextChatContent->GetFixedWidth();
	plist_element->SetFixedHeight(70 + strLen / ptextChatContent->GetMaxWidth() * 21);	//自适应控件的高度 
}
void CMainWnd::AddGroundMsgList(wxMSG wxmsg)
{
	CDialogBuilder builder;
	CListContainerElementUI *plist_element = static_cast<CListContainerElementUI*>(builder.Create(_T("listGroundMsg.xml"), 0, 0));
	ASSERT(plist_element);

	CTextUI *ptextChatContent = static_cast<CTextUI *>(plist_element->FindSubControl(L"textChatContent"));
	CLabelUI *plabelChatHeadImg = static_cast<CLabelUI *>(plist_element->FindSubControl(L"labelChatHeadImg"));
	ASSERT(ptextChatContent && plabelChatHeadImg);

	CDuiString msgContent, userName;
	size_t pos = wxmsg.Content.find(":<br/>");
	if (pos != string::npos)
	{
		userName = ANSIToUnicode(wxmsg.Content.substr(0, pos)).c_str();
		msgContent = ANSIToUnicode(wxmsg.Content.substr(pos, wxmsg.Content.size())).c_str();
	}

	ptextChatContent->SetText(msgContent);

	CDuiString headimgpath = L"cache\\";
	headimgpath += userName;
	plabelChatHeadImg->SetBkImage(headimgpath);

	m_listChatPanel->AddAt(plist_element, m_listChatPanel->GetCount());

	SIZE szAvailable = { 0 };
	ptextChatContent->EstimateSize(szAvailable);
	int strLen = ptextChatContent->GetFixedWidth();
	plist_element->SetFixedHeight(54 + strLen / ptextChatContent->GetMaxWidth() * 21);	//控件的高度 20+34+(n-1)x25  n为字体行数 20:layout inset 

}
DWORD CMainWnd::AddMsgListAll(wxMSG wxmsg, bool bFirAdd = 1)
{
	int ret = 0;
	if (wxmsg.FromUserName ==UserInfo.UserName)
	{
		AddMyMsgList(wxmsg);
	}
	else if (wxmsg.FromUserName.find("@@") == string::npos)
	{
		AddFriendMsgList(wxmsg);
	}
	else
	{
		AddGroundMsgList(wxmsg);
	}
	if (bFirAdd)
	{
		pushChatlog(wxmsg);
	}
	return ret;
}

LRESULT CMainWnd::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT lRes;
	BOOL bHandle = TRUE;
	switch (uMsg)
	{
	case WM_CREATE:			lRes = OnCreate(wParam, lParam, bHandle);		break;
	case WM_GET_CHAT:		lRes = ChatListRedraw();						break;
	case WM_GET_CONTACT:	lRes = ContactListRedraw();						break;
	case WM_SYNC_CHECK:		lRes = httpsynccheck();							break;

	case WM_HAVE_MSG:		lRes = httpGetMsg();							break;
	case WM_SEND_MSG:		lRes = httpGetMsg();							break;

	default:
		bHandle = FALSE;
		break;
	}
	if (bHandle) return lRes;
	return WindowImplBase::HandleMessage(uMsg, wParam, lParam);
}

void CMainWnd::AddChatListItem(CDuiString NickName, CDuiString Signature, CDuiString HeadImg)
{
	CListUI *plist = static_cast<CListUI*>(m_PaintManager.FindControl(L"listChat"));
	ASSERT(plist);
	if (!plist)	return;
	
	CDialogBuilder builder;
	CListContainerElementUI *plist_element = static_cast<CListContainerElementUI*>(builder.Create(_T("listChat.xml"), 0, 0));
	ASSERT(plist_element);
	plist_element->SetFixedHeight(65);

	CControlUI *labelNicName = plist_element->FindSubControl(_T("labelNickName"));
	CControlUI *labelSignature = plist_element->FindSubControl(_T("labelSignature"));
	CControlUI *labelHeadImg = plist_element->FindSubControl(_T("labelHeadImg"));
	ASSERT(labelSignature && labelNicName &&labelHeadImg);

	labelNicName->SetText(NickName);
	labelSignature->SetText(Signature);
	labelHeadImg->SetBkImage(HeadImg);

	plist_element->SetTag(plist->GetCount());
	plist->AddAt(plist_element, plist->GetCount());

}
DWORD CMainWnd::dowondChatListAvatar(string HeadImgUrl,string AvaFileName)
{
	fstream file;
	file.open(AvaFileName.c_str(), ios::out | ios::binary | ios::_Noreplace);
	if (!file.is_open())	//判断头像是否已下载
	{	
		file.close();
		return 0;
	}

	string strUrl = "wx2.qq.com" + HeadImgUrl;
	char *cUrl = const_cast<char *>(strUrl.c_str());

	char *res = HttpRequest(cUrl);
	if (!res) return 1;

	file.write(res, HttpGetResBufSize() );

	file.close();
	return 0;
}

DWORD CMainWnd::httpGetContactList()
{
	string url = "wx2.qq.com/cgi-bin/mmwebwx-bin/webwxgetcontact?r=" + string(getTimeStamp()) + "&seq=0&skey=" + g_wechatinfo.skey;

	char *strurl = const_cast<char *>(url.c_str());

	string res = HttpRequest(strurl);
	if (res.empty()) return -1;
	res = UTF8ToANSI(res);

	Json::Reader reader;
	Json::Value root;
	if (reader.parse(res,root))
	{
		Json::Value MemberList;
		MemberList = root["MemberList"];
		for (unsigned int i = 0; i < MemberList.size();++i)
		{
			ChatInfo s_chatinfo;
			s_chatinfo.UserName = MemberList[i]["UserName"].asString();
			s_chatinfo.NickName = MemberList[i]["NickName"].asString();
			s_chatinfo.HeadImgUrl =MemberList[i]["HeadImgUrl"].asString();
			s_chatinfo.Signature = MemberList[i]["Signature"].asString();
			s_chatinfo.Province = MemberList[i]["Province"].asString();
			s_chatinfo.City = MemberList[i]["City"].asString();
			//入容器
			g_ContactList.push_back(s_chatinfo);
		}
	}
	return 0;
}

void CMainWnd::ContactListAddItem(CDuiString NickName, CDuiString HeadImg)
{
	CListUI *plist = static_cast<CListUI*>(m_PaintManager.FindControl(L"listContact"));
	ASSERT(plist);
	if (!plist)	return;

	CDialogBuilder builder;
	CListContainerElementUI *plist_element = static_cast<CListContainerElementUI*>(builder.Create(_T("listContact.xml"), 0, 0));
	ASSERT(plist_element);
	plist_element->SetFixedHeight(65);

	CControlUI *labelNicName = plist_element->FindSubControl(_T("labelContactNickName"));
	CControlUI *labelHeadImg = plist_element->FindSubControl(_T("labelContactHeadImg"));
	ASSERT(labelNicName &&labelHeadImg);

	labelNicName->SetText(NickName);
	labelHeadImg->SetBkImage(HeadImg);

	plist_element->SetTag(plist->GetCount());
	plist->AddAt(plist_element, plist->GetCount());
}

DWORD CMainWnd::ContactListRedraw()
{
	HANDLE hThread	=CreateThread(0, 0, ContactListRedrawThread, this, 0, 0);
	CloseHandle(hThread);
	return 0;
}

DWORD CMainWnd::ChatListRedraw()
{

	for (unsigned int i = 0; i < g_ChatList.size(); ++i)
	{
		CDuiString NickName = ANSIToUnicode(g_ChatList[i].NickName).c_str();	
		CDuiString Signature = ANSIToUnicode(g_ChatList[i].Signature).c_str();
		string HeadImg = "cache\\" + g_ChatList[i].UserName;
		//download avatar
		dowondChatListAvatar(g_ChatList[i].HeadImgUrl,HeadImg);
		//add list
		AddChatListItem(NickName, Signature,ANSIToUnicode(HeadImg).c_str());	
	}	
	SendMessage(WM_GET_CONTACT);
	return 0;
}

void CMainWnd::UpdataContactInfo()
{
	if (curContactInfo.UserName.empty()) return;
	CControlUI *pHeadImg = m_PaintManager.FindControl(L"TabClabelHeadImg");
	CControlUI *pNickName = m_PaintManager.FindControl(L"TabClabelNickName");
	CControlUI *pSignature = m_PaintManager.FindControl(L"TabClabelSignature");
	CControlUI *pWxID = m_PaintManager.FindControl(L"TabClabelWxID");
	CControlUI *pCity = m_PaintManager.FindControl(L"TabClabelCity");
	ASSERT(pHeadImg && pNickName && pSignature && pWxID && pCity);
	pNickName->SetText(ANSIToUnicode(curContactInfo.NickName).c_str());
	pSignature->SetText(ANSIToUnicode(curContactInfo.Signature).c_str());
	pWxID->SetText(ANSIToUnicode(curContactInfo.UserName).c_str());
	pCity->SetText(ANSIToUnicode(curContactInfo.Province + curContactInfo.City).c_str());

	string HeadImg = "cache\\" + curContactInfo.UserName;
	pHeadImg->SetBkImage(ANSIToUnicode(HeadImg).c_str());
}

void CMainWnd::UpdataChatPanel()
{
	m_listChatPanel->RemoveAll();	//clean chat panel list
	string mapFir = curChatInfo.UserName;
	if (map_ChatLog.count(mapFir))
	{
		vector <wxMSG> tempVec;
		tempVec = map_ChatLog[mapFir];
		for (unsigned int i = 0; i < tempVec.size();++i)
		{
			AddMsgListAll(tempVec[i], 0);
		}
	}
}
DWORD CMainWnd::httpsynccheck()
{
	HANDLE hThread = CreateThread(0, 0, httpsynccheckThread, this, 0, 0);
	CloseHandle(hThread);
	return 0;
}

DWORD CMainWnd::httpGetMsg()
{
	string res = httpWebwxsync();
	Json::Reader reader;
	Json::Value root;
	if (reader.parse(res,root))
	{
		Json::Value AddMsgList;
		AddMsgList = root["AddMsgList"];
		for (unsigned int i = 0; i < AddMsgList.size();++i)
		{
			wxMSG wxmsg;
			wxmsg.FromUserName = AddMsgList[i]["FromUserName"].asString();
			wxmsg.ToUserName = AddMsgList[i]["ToUserName"].asString();
			wxmsg.Msgtype = AddMsgList[i]["MsgType"].asInt();
			wxmsg.Content=AddMsgList[i]["Content"].asString() ;	
			wxmsg.FileName = AddMsgList[i]["FileName"].asString();
			wxmsg.FileSize = AddMsgList[i]["FileSize"].asString();
			wxmsg.Url = AddMsgList[i]["Url"].asString();
//			wxmsg.Url.replace("&amp;", "&");

			AddMsgListAll(wxmsg);
		}
	}
	return 0;
}

DWORD CMainWnd::httpSendMsg()
{
	if (m_reditChat->GetText()==L"")	return 0;

	wxMSG wxmsg;
	wxmsg.FromUserName = UserInfo.UserName;
	wxmsg.ToUserName = curChatInfo.UserName;
	wxmsg.Content = UnicodeToANSI(m_reditChat->GetText().GetData());
	wxmsg.Msgtype = 1;

	AddMsgListAll(wxmsg);
	m_reditChat->SetText(L"");


	string Url = "wx2.qq.com/cgi-bin/mmwebwx-bin/webwxsendmsg?pass_ticket=" + g_wechatinfo.pass_ticket;
	Json::Value jPostData;
	jPostData["BaseRequest"] = getBaseRequestJson();
	jPostData["Msg"] = createMsgJson();
	jPostData["Scene"] = 0;

	Json::FastWriter writer;
	string PostData = writer.write(jPostData);

	char *c_Url = const_cast<char *>(Url.c_str());
	char *c_postdata = const_cast<char *>(PostData.c_str());

	string res = HttpRequest(c_Url, "POST", c_postdata);
	if (res.empty()) return -1;

	Json::Reader reader;
	Json::Value root;
	if (reader.parse(res,root))
	{
		if (0 == root["BaseResponse"]["ret"].asInt())
		{
			return 0;
		}
		else
		{
			string ErrMsg = root["BaseResponse"]["ErrMsg"].asString();
			return -2;
		}
	}
	return -3;
}