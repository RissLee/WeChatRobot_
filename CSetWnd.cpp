#include "stdafx.h"

#include "CSetWnd.h"

CSetWnd::CSetWnd(CDuiString pathXml) :
m_PathXml(pathXml)
{
}
CSetWnd::~CSetWnd()
{
}

LRESULT CSetWnd::OnCreate(WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	return 0;
}
void CSetWnd::InitWindow()
{
}

void CSetWnd::Notify(TNotifyUI& msg)
{ 
	if (msg.sType==DUI_MSGTYPE_CLICK)
	{
		if (msg.pSender->GetName() == L"btnClose")
			Close(IDOK);
	}
	if (msg.sType==DUI_MSGTYPE_SELECTCHANGED)
	{
		CDuiString name = msg.pSender->GetName();
		CTabLayoutUI* pControl = static_cast<CTabLayoutUI*>(m_PaintManager.FindControl(_T("tabSet")));
		ASSERT(pControl);
		if (name == _T("optAccount"))
			pControl->SelectItem(0);
		else if (name == _T("optGeneral"))
			pControl->SelectItem(1);
		else if (name == _T("optHotKey"))
			pControl->SelectItem(2);
		else if (name == _T("optChatBackup"))
			pControl->SelectItem(3);
		else if (name == _T("optAbout"))
			pControl->SelectItem(4);;
	}
}


