#include "stdafx.h"
#include "DataFrm.h"

extern ChatInfo UserInfo;		//用户信息

//My Avatar Interface Class
class CMyAvatarWnd :
	public WindowImplBase
{
public:
	CControlUI* m_pOwner;
	POINT m_ptPos;
public:
	CMyAvatarWnd(CDuiString pathXml) :
		m_PathXml(pathXml),
		m_pOwner(0)
	{
	};
	~CMyAvatarWnd()
	{
	};
public:
	void Init(CControlUI* pOwner, POINT pt)
	{
		if (pOwner == NULL) return;
		m_pOwner = pOwner;
		m_ptPos = pt;
		Create(pOwner->GetManager()->GetPaintWindow(), NULL, WS_VISIBLE | WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, WS_EX_TOOLWINDOW);
		HWND hWndParent = m_hWnd;
		while (::GetParent(hWndParent) != NULL) hWndParent = ::GetParent(hWndParent);
		::ShowWindow(m_hWnd, SW_SHOW);
		::SendMessage(hWndParent, WM_NCACTIVATE, TRUE, 0L);
	}
	void InitWindow()
	{
		CControlUI *pLabelNickName = m_PaintManager.FindControl(L"labelNickName");
		CControlUI *pLabelSingature = m_PaintManager.FindControl(L"labelSignature");
 		CControlUI *plabelWxID = m_PaintManager.FindControl(L"labelWxID");
// 		CControlUI *plabelCity = m_PaintManager.FindControl(L"labelCity");
		CControlUI *playHeadImg = m_PaintManager.FindControl(L"layHeadImg");

		ASSERT(pLabelSingature && pLabelNickName /*&& plabelCity && plabelWxID*/ && playHeadImg);

		pLabelNickName->SetText(ANSIToUnicode(UserInfo.NickName).c_str());
		pLabelSingature->SetText(ANSIToUnicode(UserInfo.Signature).c_str());
 		plabelWxID->SetText(ANSIToUnicode(UserInfo.UserName).c_str());
// 		plabelCity->SetText(UserInfo.Province + UserInfo.City);

		string HeadImg = "cache\\" + UserInfo.UserName;
		playHeadImg->SetBkImage(ANSIToUnicode(HeadImg).c_str());

		WindowImplBase::InitWindow();
	}

	LRESULT OnCreate(WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		return 0;
	}
	virtual CDuiString GetSkinFolder() { return _T("skin"); };
	virtual CDuiString GetSkinFile()  { return m_PathXml; };
	virtual LPCTSTR GetWindowClassName(void) const { return _T("MyAvatar_wnd"); };

	virtual void Notify(TNotifyUI& msg)
	{
		if (msg.sType == DUI_MSGTYPE_CLICK)
		{
			if (msg.pSender->GetName() == _T("btnSendMessage"))
			{
				CDuiString str = L"SendMsg to myself";
				CControlUI* labelTips = m_pOwner->GetManager()->FindControl(L"labelTips");
				ASSERT(labelTips);
				labelTips->SetText(str);
			}
		}
	}
	//调整位置 防止显示器边界越界
	void AdjustPostion() {
		CDuiRect rcWnd;
		GetWindowRect(m_hWnd, &rcWnd);
		int nWidth = rcWnd.GetWidth();
		int nHeight = rcWnd.GetHeight();
		rcWnd.left = m_ptPos.x;
		rcWnd.top = m_ptPos.y;
		rcWnd.right = rcWnd.left + nWidth;
		rcWnd.bottom = rcWnd.top + nHeight;
		MONITORINFO oMonitor = {};
		oMonitor.cbSize = sizeof(oMonitor);
		::GetMonitorInfo(::MonitorFromWindow(*this, MONITOR_DEFAULTTOPRIMARY), &oMonitor);
		CDuiRect rcWork = oMonitor.rcWork;

		if (rcWnd.bottom > rcWork.bottom) {
			if (nHeight >= rcWork.GetHeight()) {
				rcWnd.top = 0;
				rcWnd.bottom = nHeight;
			}
			else {
				rcWnd.bottom = rcWork.bottom;
				rcWnd.top = rcWnd.bottom - nHeight;
			}
		}
		if (rcWnd.right > rcWork.right) {
			if (nWidth >= rcWork.GetWidth()) {
				rcWnd.left = 0;
				rcWnd.right = nWidth;
			}
			else {
				rcWnd.right = rcWork.right;
				rcWnd.left = rcWnd.right - nWidth;
			}
		}
		::SetWindowPos(m_hWnd, NULL, rcWnd.left, rcWnd.top, rcWnd.GetWidth(), rcWnd.GetHeight(), SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);
	}
	LRESULT OnKillFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		Close();
		return 0;

	}
private:
	CDuiString m_PathXml;
};




