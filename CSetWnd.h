#pragma once

class CSetWnd :
	public WindowImplBase
{
public:
	CSetWnd(CDuiString pathXml);
	~CSetWnd();
protected:
	LRESULT OnCreate(WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	virtual CDuiString GetSkinFolder() { return _T("skin"); };
	virtual CDuiString GetSkinFile()  { return m_PathXml; };
	virtual LPCTSTR GetWindowClassName(void) const { return _T("WeChatRobot_Login"); };
	virtual void Notify(TNotifyUI& msg);
	virtual void InitWindow();
//	virtual LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
	CDuiString m_PathXml;

};

