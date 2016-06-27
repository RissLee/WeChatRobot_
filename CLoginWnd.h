#pragma once


class CLoginWnd :
	public WindowImplBase
{
public:
	CControlUI *m_imgErWeiMa, *m_imgUserAvatar, *m_txtTips;
public:
	CLoginWnd(CDuiString pathxml);
	~CLoginWnd(void);

protected:
	LRESULT OnCreate(WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	virtual CDuiString GetSkinFolder() { return _T ("skin"); };
	virtual CDuiString GetSkinFile()  { return m_PathXml; };
	virtual LPCTSTR GetWindowClassName(void) const { return _T ("WeChatRobot_Login"); };
	virtual void Notify(TNotifyUI& msg);
 	virtual void InitWindow();
	virtual LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
	virtual LRESULT OnNcPaint(UINT, WPARAM, LPARAM, BOOL&);
//	virtual HRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	HRESULT OnErasebkgnd(WPARAM wParam, LPARAM lParam, BOOL& bHandled);	//²Á³ý´°¿Ú±³¾°
	void ShowErWeima();

private:
	CDuiString m_PathXml;
	
};




