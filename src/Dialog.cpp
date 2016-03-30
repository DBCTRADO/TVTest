#include "stdafx.h"
#include "TVTest.h"
#include "Dialog.h"
#include "DialogUtil.h"
#include "Common/DebugDef.h"




CBasicDialog::CBasicDialog()
	: m_hDlg(NULL)
	, m_fModeless(false)
	, m_fSetPosition(false)
{
}


CBasicDialog::~CBasicDialog()
{
	Destroy();
}


bool CBasicDialog::IsCreated() const
{
	return m_hDlg!=NULL;
}


bool CBasicDialog::Destroy()
{
	if (m_hDlg!=NULL) {
		return ::DestroyWindow(m_hDlg)!=FALSE;
	}
	return true;
}


bool CBasicDialog::ProcessMessage(LPMSG pMsg)
{
	if (m_hDlg==NULL)
		return false;
	return ::IsDialogMessage(m_hDlg,pMsg)!=FALSE;
}


bool CBasicDialog::IsVisible() const
{
	return m_hDlg!=NULL && ::IsWindowVisible(m_hDlg);
}


bool CBasicDialog::SetVisible(bool fVisible)
{
	if (m_hDlg==NULL)
		return false;
	return ::ShowWindow(m_hDlg,fVisible?SW_SHOW:SW_HIDE)!=FALSE;
}


bool CBasicDialog::GetPosition(Position *pPosition) const
{
	if (pPosition==NULL)
		return false;
	if (m_hDlg==NULL) {
		*pPosition=m_Position;
	} else {
		RECT rc;
		::GetWindowRect(m_hDlg,&rc);
		pPosition->Set(&rc);
	}
	return true;
}


bool CBasicDialog::GetPosition(RECT *pPosition) const
{
	if (pPosition==NULL)
		return false;
	if (m_hDlg==NULL)
		m_Position.Get(pPosition);
	else
		::GetWindowRect(m_hDlg,pPosition);
	return true;
}


bool CBasicDialog::GetPosition(int *pLeft,int *pTop,int *pWidth,int *pHeight) const
{
	RECT rc;

	GetPosition(&rc);
	if (pLeft!=NULL)
		*pLeft=rc.left;
	if (pTop!=NULL)
		*pTop=rc.top;
	if (pWidth!=NULL)
		*pWidth=rc.right-rc.left;
	if (pHeight!=NULL)
		*pHeight=rc.bottom-rc.top;
	return true;
}


bool CBasicDialog::SetPosition(const Position &Pos)
{
	return SetPosition(Pos.x,Pos.y,Pos.Width,Pos.Height);
}


bool CBasicDialog::SetPosition(const RECT *pPosition)
{
	if (pPosition==NULL)
		return false;
	return SetPosition(pPosition->left,pPosition->top,
					   pPosition->right-pPosition->left,
					   pPosition->bottom-pPosition->top);
}


bool CBasicDialog::SetPosition(int Left,int Top,int Width,int Height)
{
	if (Width<0 || Height<0)
		return false;
	if (m_hDlg==NULL) {
		m_Position.x=Left;
		m_Position.y=Top;
		m_Position.Width=Width;
		m_Position.Height=Height;
		m_fSetPosition=true;
	} else {
		::MoveWindow(m_hDlg,Left,Top,Width,Height,TRUE);
	}
	return true;
}


bool CBasicDialog::SetPosition(int Left,int Top)
{
	if (m_hDlg==NULL) {
		m_Position.x=Left;
		m_Position.y=Top;
		m_fSetPosition=true;
	} else {
		::SetWindowPos(m_hDlg,NULL,Left,Top,0,0,SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
	}
	return true;
}


LRESULT CBasicDialog::SendMessage(UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	if (m_hDlg==nullptr)
		return 0;
	return ::SendMessage(m_hDlg,uMsg,wParam,lParam);
}


int CBasicDialog::ShowDialog(HWND hwndOwner,HINSTANCE hinst,LPCTSTR pszTemplate)
{
	if (m_hDlg!=NULL)
		return -1;
	return (int)::DialogBoxParam(hinst,pszTemplate,hwndOwner,DialogProc,
								 reinterpret_cast<LPARAM>(this));
}


bool CBasicDialog::CreateDialogWindow(HWND hwndOwner,HINSTANCE hinst,LPCTSTR pszTemplate)
{
	if (m_hDlg!=NULL)
		return false;
	if (::CreateDialogParam(hinst,pszTemplate,hwndOwner,DialogProc,
							reinterpret_cast<LPARAM>(this))==NULL)
		return false;
	m_fModeless=true;
	return true;
}


CBasicDialog *CBasicDialog::GetThis(HWND hDlg)
{
	return static_cast<CBasicDialog*>(::GetProp(hDlg,TEXT("This")));
}


INT_PTR CALLBACK CBasicDialog::DialogProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	CBasicDialog *pThis;

	if (uMsg==WM_INITDIALOG) {
		pThis=reinterpret_cast<CBasicDialog*>(lParam);
		pThis->m_hDlg=hDlg;
		::SetProp(hDlg,TEXT("This"),pThis);
	} else {
		pThis=GetThis(hDlg);
		if (uMsg==WM_NCDESTROY) {
			if (pThis!=NULL) {
				pThis->HandleMessage(hDlg,uMsg,wParam,lParam);
				pThis->m_hDlg=NULL;
			}
			::RemoveProp(hDlg,TEXT("This"));
			return TRUE;
		}
	}
	if (pThis!=NULL)
		return pThis->HandleMessage(hDlg,uMsg,wParam,lParam);
	return FALSE;
}


INT_PTR CBasicDialog::HandleMessage(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		if (m_fSetPosition)
			ApplyPosition();
		break;

	case WM_DESTROY:
		StorePosition();
		break;
	}

	return DlgProc(hDlg,uMsg,wParam,lParam);
}


INT_PTR CBasicDialog::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		return TRUE;
	}

	return FALSE;
}


bool CBasicDialog::ApplyPosition()
{
	if (m_hDlg==NULL || !m_fSetPosition)
		return false;

	RECT rc;
	m_Position.Get(&rc);
	HMONITOR hMonitor=::MonitorFromRect(&rc,MONITOR_DEFAULTTONEAREST);
	MONITORINFO mi;
	mi.cbSize=sizeof(mi);
	if (::GetMonitorInfo(hMonitor,&mi)) {
		if (rc.left<mi.rcMonitor.left)
			m_Position.x=mi.rcMonitor.left;
		else if (rc.right>mi.rcMonitor.right)
			m_Position.x=mi.rcMonitor.right-m_Position.Width;
		if (rc.top<mi.rcMonitor.top)
			m_Position.y=mi.rcMonitor.top;
		else if (rc.bottom>mi.rcMonitor.bottom)
			m_Position.y=mi.rcMonitor.bottom-m_Position.Height;
	}

	if (m_Position.Width<=0 || m_Position.Height<=0) {
		::GetWindowRect(m_hDlg,&rc);
		if (m_Position.Width<=0)
			m_Position.Width=rc.right-rc.left;
		if (m_Position.Height<=0)
			m_Position.Height=rc.bottom-rc.top;
	}

	return ::SetWindowPos(m_hDlg,NULL,
						  m_Position.x,m_Position.y,
						  m_Position.Width,m_Position.Height,
						  SWP_NOZORDER)!=FALSE;
}


void CBasicDialog::StorePosition()
{
	if (m_hDlg!=NULL) {
		RECT rc;

		if (::GetWindowRect(m_hDlg,&rc)) {
			m_Position.Set(&rc);
			m_fSetPosition=true;
		}
	}
}




CResizableDialog::CResizableDialog()
	: m_hwndSizeGrip(NULL)
{
}


CResizableDialog::~CResizableDialog()
{
}


INT_PTR CResizableDialog::HandleMessage(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			RECT rc;

			::GetWindowRect(hDlg,&rc);
			m_MinSize.cx=rc.right-rc.left;
			m_MinSize.cy=rc.bottom-rc.top;
			::GetClientRect(hDlg,&rc);
			m_OriginalClientSize.cx=rc.right-rc.left;
			m_OriginalClientSize.cy=rc.bottom-rc.top;
			if ((::GetWindowLong(hDlg,GWL_STYLE)&WS_CHILD)==0) {
				m_hwndSizeGrip=::CreateWindowEx(0,TEXT("SCROLLBAR"),NULL,
					WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | SBS_SIZEGRIP |
													SBS_SIZEBOXBOTTOMRIGHTALIGN,
					0,0,rc.right,rc.bottom,m_hDlg,(HMENU)0,
					reinterpret_cast<HINSTANCE>(::GetWindowLongPtr(m_hDlg,GWLP_HINSTANCE)),NULL);
				::SetWindowPos(m_hwndSizeGrip,HWND_BOTTOM,0,0,0,0,SWP_NOMOVE | SWP_NOSIZE);
			} else {
				m_hwndSizeGrip=NULL;
			}
		}
		break;

	case WM_GETMINMAXINFO:
		{
			MINMAXINFO *pmmi=reinterpret_cast<MINMAXINFO*>(lParam);

			pmmi->ptMinTrackSize.x=m_MinSize.cx;
			pmmi->ptMinTrackSize.y=m_MinSize.cy;
		}
		return TRUE;

	case WM_SIZE:
		DoLayout();
		break;

	case WM_DESTROY:
		StorePosition();
		break;
	}

	return DlgProc(hDlg,uMsg,wParam,lParam);
}


bool CResizableDialog::ApplyPosition()
{
	if (m_Position.Width<m_MinSize.cx)
		m_Position.Width=m_MinSize.cx;
	if (m_Position.Height<m_MinSize.cy)
		m_Position.Height=m_MinSize.cy;

	return CBasicDialog::ApplyPosition();
}


void CResizableDialog::DoLayout()
{
	RECT rc;
	int Width,Height;

	::GetClientRect(m_hDlg,&rc);
	Width=rc.right-rc.left;
	Height=rc.bottom-rc.top;
	for (size_t i=0;i<m_ControlList.size();i++) {
		rc=m_ControlList[i].rcOriginal;
		if ((m_ControlList[i].Align&ALIGN_RIGHT)!=0) {
			rc.right+=Width-m_OriginalClientSize.cx;
			if ((m_ControlList[i].Align&ALIGN_LEFT)==0)
				rc.left+=Width-m_OriginalClientSize.cx;
			if (rc.right<rc.left)
				rc.right=rc.left;
		}
		if ((m_ControlList[i].Align&ALIGN_BOTTOM)!=0) {
			rc.bottom+=Height-m_OriginalClientSize.cy;
			if ((m_ControlList[i].Align&ALIGN_TOP)==0)
				rc.top+=Height-m_OriginalClientSize.cy;
			if (rc.bottom<rc.top)
				rc.bottom=rc.top;
		}
		::MoveWindow(::GetDlgItem(m_hDlg,m_ControlList[i].ID),
					 rc.left,rc.top,rc.right-rc.left,rc.bottom-rc.top,TRUE);
	}
	if (m_hwndSizeGrip!=NULL) {
		::GetWindowRect(m_hwndSizeGrip,&rc);
		::OffsetRect(&rc,-rc.left,-rc.top);
		::MoveWindow(m_hwndSizeGrip,Width-rc.right,Height-rc.bottom,
					 rc.right,rc.bottom,TRUE);
	}
}


bool CResizableDialog::AddControl(int ID,unsigned int Align)
{
	HWND hwnd=::GetDlgItem(m_hDlg,ID);
	if (hwnd==NULL)
		return false;

	LayoutItem Item;

	Item.ID=ID;
	::GetWindowRect(hwnd,&Item.rcOriginal);
	::MapWindowPoints(NULL,m_hDlg,reinterpret_cast<LPPOINT>(&Item.rcOriginal),2);
	Item.Align=Align;
	m_ControlList.push_back(Item);
	return true;
}


bool CResizableDialog::AddControls(int FirstID,int LastID,unsigned int Align)
{
	if (FirstID>LastID)
		return false;
	for (int i=FirstID;i<=LastID;i++) {
		if (!AddControl(i,Align))
			return false;
	}
	return true;
}


bool CResizableDialog::UpdateControlPosition(int ID)
{
	for (size_t i=0;i<m_ControlList.size();i++) {
		if (m_ControlList[i].ID==ID) {
			GetDlgItemRect(m_hDlg,ID,&m_ControlList[i].rcOriginal);
			return true;
		}
	}

	return false;
}


void CResizableDialog::UpdateLayout()
{
	RECT rc;

	::GetClientRect(m_hDlg,&rc);
	m_OriginalClientSize.cx=rc.right;
	m_OriginalClientSize.cy=rc.bottom;

	for (size_t i=0;i<m_ControlList.size();i++)
		GetDlgItemRect(m_hDlg,m_ControlList[i].ID,&m_ControlList[i].rcOriginal);
}
