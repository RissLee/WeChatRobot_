#pragma  once
#include "DataFrm.h"
class CMainWnd :
	public WindowImplBase
{
public:
	CControlUI *m_labelTips;
	CListUI *m_listChatPanel;
	CControlUI *m_reditChat;
	CButtonUI *m_SendMessageS;
public:
	CMainWnd(void);
	~CMainWnd(void);
protected:
	LRESULT OnCreate(WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	virtual CDuiString GetSkinFolder() { return _T("skin"); };
	virtual CDuiString GetSkinFile()  { return _T("WeChatRobotMain.xml"); };
	virtual LPCTSTR GetWindowClassName(void) const { return _T("WeChatRobot_Main"); };
	virtual void Notify(TNotifyUI& msg);
	virtual void InitWindow();
	virtual LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
	//	virtual HRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
public:
	DWORD ChatListRedraw();
	void AddChatListItem(CDuiString NickName, CDuiString Signature, CDuiString HeadImg);
	DWORD ContactListRedraw();
	void ContactListAddItem(CDuiString NickName,CDuiString HeadImg);

	DWORD dowondChatListAvatar(std::string HeadImgUrl, std::string filepath);
	DWORD httpGetContactList();
	void UpdataContactInfo();
	void UpdataChatPanel();
	DWORD httpsynccheck();
	DWORD httpGetMsg();
	DWORD httpSendMsg();
	void AddMyMsgList(wxMSG wxmsg);
	void AddFriendMsgList(wxMSG wxmsg);
	void AddGroundMsgList(wxMSG wxmsg);
	DWORD AddMsgListAll(wxMSG wxmsg, bool bFirAdd);
	
private:
	Json::Value createMsgJson();
};