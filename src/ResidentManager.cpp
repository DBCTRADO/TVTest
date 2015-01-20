#include "stdafx.h"
#include "TVTest.h"
#include "ResidentManager.h"
#include "AppMain.h"
#include "resource.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif




CResidentManager::CResidentManager()
	: m_hwnd(NULL)
	, m_TrayIconMessage(0)
	, m_fResident(false)
	, m_fMinimizeToTray(true)
	, m_Status(0)
	, m_TaskbarCreatedMessage(0)
{
}


CResidentManager::~CResidentManager()
{
	Finalize();
}


bool CResidentManager::Initialize(HWND hwnd,UINT Message)
{
	m_hwnd=hwnd;
	m_TrayIconMessage=Message;
	m_TaskbarCreatedMessage=::RegisterWindowMessage(TEXT("TaskbarCreated"));
	if (NeedTrayIcon()) {
		if (!AddTrayIcon())
			return false;
	}
	return true;
}


void CResidentManager::Finalize()
{
	if (NeedTrayIcon())
		RemoveTrayIcon();
}


bool CResidentManager::SetResident(bool fResident)
{
	if (m_fResident!=fResident) {
		if (m_hwnd!=NULL && !NeedTrayIcon() && fResident) {
			if (!AddTrayIcon())
				return false;
		}
		m_fResident=fResident;
		if (m_hwnd!=NULL && !NeedTrayIcon())
			RemoveTrayIcon();
	}
	return true;
}


bool CResidentManager::SetMinimizeToTray(bool fMinimizeToTray)
{
	if (m_fMinimizeToTray!=fMinimizeToTray) {
		if (m_hwnd!=NULL) {
			if ((m_Status&STATUS_MINIMIZED)!=0) {
				if (fMinimizeToTray && !NeedTrayIcon()) {
					if (!AddTrayIcon())
						return false;
				}
				::ShowWindow(m_hwnd,fMinimizeToTray?SW_HIDE:SW_SHOW);
			}
		}
		m_fMinimizeToTray=fMinimizeToTray;
		if (m_hwnd!=NULL && !NeedTrayIcon())
			RemoveTrayIcon();
	}
	return true;
}


bool CResidentManager::AddTrayIcon()
{
	NOTIFYICONDATA nid;

	nid.cbSize=NOTIFYICONDATA_V2_SIZE;
	nid.hWnd=m_hwnd;
	nid.uID=1;
	nid.uFlags=NIF_MESSAGE | NIF_ICON | NIF_TIP;
	nid.uCallbackMessage=m_TrayIconMessage;
	nid.hIcon=::LoadIcon(GetAppClass().GetResourceInstance(),
		MAKEINTRESOURCE((m_Status&STATUS_RECORDING)!=0?IDI_TRAY_RECORDING:IDI_TRAY));
	::lstrcpyn(nid.szTip,!m_TipText.empty()?m_TipText.c_str():APP_NAME,lengthof(nid.szTip));
	if (!::Shell_NotifyIcon(NIM_ADD,&nid))
		return false;
	/*
	nid.uVersion=NOTIFYICON_VERSION;
	::Shell_NotifyIcon(NIM_SETVERSION,&nid);
	*/
	return true;
}


bool CResidentManager::RemoveTrayIcon()
{
	NOTIFYICONDATA nid;

	nid.cbSize=NOTIFYICONDATA_V2_SIZE;
	nid.hWnd=m_hwnd;
	nid.uID=1;
	nid.uFlags=0;
	return Shell_NotifyIcon(NIM_DELETE,&nid)!=FALSE;
}


bool CResidentManager::ChangeTrayIcon()
{
	NOTIFYICONDATA nid;

	nid.cbSize=NOTIFYICONDATA_V2_SIZE;
	nid.hWnd=m_hwnd;
	nid.uID=1;
	nid.uFlags=NIF_ICON;
	nid.hIcon=LoadIcon(GetAppClass().GetResourceInstance(),
		MAKEINTRESOURCE((m_Status&STATUS_RECORDING)!=0?IDI_TRAY_RECORDING:IDI_TRAY));
	if (!Shell_NotifyIcon(NIM_MODIFY,&nid)) {
		if (!AddTrayIcon())
			return false;
	}

	return true;
}


bool CResidentManager::UpdateTipText()
{
	NOTIFYICONDATA nid;

	nid.cbSize=NOTIFYICONDATA_V2_SIZE;
	nid.hWnd=m_hwnd;
	nid.uID=1;
	nid.uFlags=NIF_TIP;
	::lstrcpyn(nid.szTip,!m_TipText.empty()?m_TipText.c_str():APP_NAME,lengthof(nid.szTip));
	return ::Shell_NotifyIcon(NIM_MODIFY,&nid)!=FALSE;
}


bool CResidentManager::NeedTrayIcon() const
{
	return m_hwnd!=NULL
		&& (m_fResident
			|| (m_Status & STATUS_STANDBY)!=0
			|| (m_fMinimizeToTray && (m_Status & STATUS_MINIMIZED)!=0));
}


bool CResidentManager::SetStatus(UINT Status,UINT Mask)
{
	const UINT NewStatus=(m_Status&~Mask)|(Status&Mask);

	if (m_Status!=NewStatus) {
		const UINT StatusDiff=m_Status^NewStatus;
		const bool fNeedTrayIconOld=NeedTrayIcon();
		bool fChangeIcon=false;

		if ((StatusDiff & STATUS_RECORDING)!=0)
			fChangeIcon=true;
		if ((StatusDiff & STATUS_MINIMIZED)!=0) {
			if (m_hwnd!=NULL && m_fMinimizeToTray)
				::ShowWindow(m_hwnd,(NewStatus & STATUS_MINIMIZED)!=0?SW_HIDE:SW_SHOW);
		}

		m_Status=NewStatus;

		if (NeedTrayIcon()) {
			if (!fNeedTrayIconOld)
				AddTrayIcon();
			else if (fChangeIcon)
				ChangeTrayIcon();
		} else {
			if (fNeedTrayIconOld)
				RemoveTrayIcon();
		}
	}

	return true;
}


bool CResidentManager::SetTipText(LPCTSTR pszText)
{
	TVTest::StringUtility::Assign(m_TipText,pszText);
	if (NeedTrayIcon())
		UpdateTipText();
	return true;
}


bool CResidentManager::ShowMessage(LPCTSTR pszText,LPCTSTR pszTitle,int Icon,DWORD TimeOut)
{
	if (!NeedTrayIcon())
		return false;

	NOTIFYICONDATA nid;

	::ZeroMemory(&nid,sizeof(nid));
	nid.cbSize=NOTIFYICONDATA_V2_SIZE;
	nid.hWnd=m_hwnd;
	nid.uID=1;
	nid.uFlags=NIF_INFO;
	::lstrcpyn(nid.szInfo,pszText,lengthof(nid.szInfo));
	if (pszTitle)
		::lstrcpyn(nid.szInfoTitle,pszTitle,lengthof(nid.szInfoTitle));
	nid.dwInfoFlags=Icon==MESSAGE_ICON_INFO?NIIF_INFO:
					Icon==MESSAGE_ICON_WARNING?NIIF_WARNING:
					Icon==MESSAGE_ICON_ERROR?NIIF_ERROR:NIIF_NONE;
	nid.uTimeout=TimeOut;
	return Shell_NotifyIcon(NIM_MODIFY,&nid)!=FALSE;
}


bool CResidentManager::HandleMessage(UINT Message,WPARAM wParam,LPARAM lParam)
{
	// ÉVÉFÉãÇ™çƒãNìÆÇµÇΩéûÇÃëŒçÙ
	if (m_TaskbarCreatedMessage!=0
			&& Message==m_TaskbarCreatedMessage) {
		if (NeedTrayIcon())
			AddTrayIcon();
		return true;
	}
	return false;
}
