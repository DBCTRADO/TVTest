#include "stdafx.h"
#include "TVTest.h"
#include "Taskbar.h"
#include "AppMain.h"
#include "resource.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif




CTaskbarManager::CTaskbarManager()
	: m_hwnd(NULL)
	, m_TaskbarButtonCreatedMessage(0)
	, m_pTaskbarList(NULL)
{
}


CTaskbarManager::~CTaskbarManager()
{
	Finalize();
}


bool CTaskbarManager::Initialize(HWND hwnd)
{
	if (m_TaskbarButtonCreatedMessage==0) {
		if (Util::OS::IsWindows7OrLater()) {
			m_TaskbarButtonCreatedMessage=::RegisterWindowMessage(TEXT("TaskbarButtonCreated"));

			auto pChangeWindowMessageFilter=
				GET_MODULE_FUNCTION(TEXT("user32.dll"),ChangeWindowMessageFilter);
			if (pChangeWindowMessageFilter!=NULL) {
				pChangeWindowMessageFilter(m_TaskbarButtonCreatedMessage,MSGFLT_ADD);
			}

			m_hwnd=hwnd;
		}
	}
	return true;
}


void CTaskbarManager::Finalize()
{
	if (m_pTaskbarList!=NULL) {
		m_pTaskbarList->Release();
		m_pTaskbarList=NULL;
	}
	m_hwnd=NULL;
}


bool CTaskbarManager::HandleMessage(UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	if (uMsg!=0 && uMsg==m_TaskbarButtonCreatedMessage) {
		if (m_pTaskbarList==NULL) {
			if (FAILED(::CoCreateInstance(CLSID_TaskbarList,NULL,
										  CLSCTX_INPROC_SERVER,
										  IID_ITaskbarList3,
										  reinterpret_cast<void**>(&m_pTaskbarList))))
				return true;
		}
		HINSTANCE hinst=GetAppClass().GetResourceInstance();
		HIMAGELIST himl=::ImageList_LoadBitmap(hinst,
											   MAKEINTRESOURCE(IDB_THUMBBAR),
											   16,1,RGB(192,192,192));
		if (himl!=NULL) {
			m_pTaskbarList->ThumbBarSetImageList(m_hwnd,himl);
			THUMBBUTTON tb[3];
			tb[0].iId=CM_FULLSCREEN;
			tb[1].iId=CM_DISABLEVIEWER;
			tb[2].iId=CM_PROGRAMGUIDE;
			for (int i=0;i<lengthof(tb);i++) {
				tb[i].dwMask=(THUMBBUTTONMASK)(THB_BITMAP | THB_TOOLTIP | THB_FLAGS);
				tb[i].iBitmap=i;
				::LoadStringW(hinst,tb[i].iId,tb[i].szTip,lengthof(tb[0].szTip));
				tb[i].dwFlags=(THUMBBUTTONFLAGS)(THBF_ENABLED | THBF_DISMISSONCLICK);
			}
			m_pTaskbarList->ThumbBarAddButtons(m_hwnd,lengthof(tb),tb);
		}
		return true;
	}
	return false;
}


bool CTaskbarManager::SetRecordingStatus(bool fRecording)
{
	if (m_pTaskbarList!=NULL) {
		if (fRecording) {
			HICON hico=static_cast<HICON>(::LoadImage(GetAppClass().GetResourceInstance(),MAKEINTRESOURCE(IDI_TASKBAR_RECORDING),IMAGE_ICON,16,16,LR_DEFAULTCOLOR));

			if (hico==NULL)
				return false;
			m_pTaskbarList->SetOverlayIcon(m_hwnd,hico,TEXT("˜^‰æ’†"));
			::DestroyIcon(hico);
		} else {
			m_pTaskbarList->SetOverlayIcon(m_hwnd,NULL,NULL);
		}
		return true;
	}
	return false;
}


bool CTaskbarManager::SetProgress(int Pos,int Max)
{
	if (m_pTaskbarList!=NULL) {
		if (Pos>=Max) {
			m_pTaskbarList->SetProgressState(m_hwnd,TBPF_NOPROGRESS);
		} else {
			if (Pos==0)
				m_pTaskbarList->SetProgressState(m_hwnd,TBPF_NORMAL);
			m_pTaskbarList->SetProgressValue(m_hwnd,Pos,Max);
		}
		return true;
	}
	return false;
}


bool CTaskbarManager::EndProgress()
{
	if (m_pTaskbarList!=NULL)
		m_pTaskbarList->SetProgressState(m_hwnd,TBPF_NOPROGRESS);
	return true;
}
