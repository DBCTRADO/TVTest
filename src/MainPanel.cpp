/*
  TVTest
  Copyright(c) 2008-2020 DBCTRADO

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


#include "stdafx.h"
#include "TVTest.h"
#include "AppMain.h"
#include "resource.h"
#include "Common/DebugDef.h"


namespace TVTest
{


CMainPanel::CMainPanel()
	: m_FrameEventHandler(&Frame)
	, m_FormEventHandler(&Form)
{
	Frame.SetFloating(false);
	Frame.SetEventHandler(&m_FrameEventHandler);
	Form.SetEventHandler(&m_FormEventHandler);
	ChannelPanel.SetEventHandler(&m_ChannelPanelEventHandler);
}


bool CMainPanel::IsFloating() const
{
	return Frame.GetFloating();
}


bool CMainPanel::OnOwnerWindowPosChanging(const RECT *pOldRect, const RECT *pNewRect)
{
	if (fShowPanelWindow && GetAppClass().PanelOptions.GetAttachToMainWindow()
			&& IsFloating()) {
		RECT rc;
		int XOffset = 0, YOffset = 0;
		bool fAttached = false;

		Frame.GetPosition(&rc);
		if (rc.top < pOldRect->bottom && rc.bottom > pOldRect->top) {
			if (rc.right == pOldRect->left) {
				XOffset = pNewRect->left - rc.right;
				fAttached = true;
			} else if (rc.left == pOldRect->right) {
				XOffset = pNewRect->right - rc.left;
				fAttached = true;
			}
			if (fAttached)
				YOffset = pNewRect->top - pOldRect->top;
		}
		if (!fAttached && rc.left < pOldRect->right && rc.right > pOldRect->left) {
			if (rc.bottom == pOldRect->top) {
				YOffset = pNewRect->top - rc.bottom;
				fAttached = true;
			} else if (rc.top == pOldRect->bottom) {
				YOffset = pNewRect->bottom - rc.top;
				fAttached = true;
			}
			if (fAttached)
				XOffset = pNewRect->left - pOldRect->left;
		}
		if (XOffset != 0 || YOffset != 0) {
			::OffsetRect(&rc, XOffset, YOffset);
			Frame.SetPosition(&rc);
			Frame.MoveToMonitorInside();
		}
		return true;
	}
	return false;
}


bool CMainPanel::IsAttached()
{
	const CAppMain &App = GetAppClass();
	bool fAttached = false;

	if (fShowPanelWindow && App.PanelOptions.GetAttachToMainWindow()
			&& IsFloating()) {
		RECT rcPanel, rcMain;

		Frame.GetPosition(&rcPanel);
		App.MainWindow.GetPosition(&rcMain);
		if (rcPanel.top < rcMain.bottom && rcPanel.bottom > rcMain.top) {
			if (rcPanel.right == rcMain.left || rcPanel.left == rcMain.right)
				fAttached = true;
		}
		if (!fAttached && rcPanel.left < rcMain.right && rcPanel.right > rcMain.left) {
			if (rcPanel.bottom == rcMain.top || rcPanel.top == rcMain.bottom)
				fAttached = true;
		}
	}
	return fAttached;
}


void CMainPanel::SetTheme(const Theme::CThemeManager *pThemeManager)
{
	Frame.SetTheme(pThemeManager);
	Form.SetTheme(pThemeManager);
	InfoPanel.SetTheme(pThemeManager);
	ProgramListPanel.SetTheme(pThemeManager);
	ChannelPanel.SetTheme(pThemeManager);
	ControlPanel.SetTheme(pThemeManager);
	CaptionPanel.SetTheme(pThemeManager);
}


void CMainPanel::UpdateContent()
{
	switch (Form.GetCurPageID()) {
	case PANEL_ID_INFORMATION:
		UpdateInformationPanel();
		break;

	case PANEL_ID_PROGRAMLIST:
		if (m_fEnableProgramListUpdate)
			UpdateProgramListPanel();
		break;

	case PANEL_ID_CHANNEL:
		UpdateChannelPanel();
		break;
	}
}


void CMainPanel::UpdateInformationPanel()
{
	InfoPanel.UpdateAllItems();
}


void CMainPanel::UpdateProgramListPanel()
{
	const CAppMain &App = GetAppClass();
	CChannelInfo ChInfo;

	if (App.Core.GetCurrentStreamChannelInfo(&ChInfo)
			&& ChInfo.GetServiceID() != 0) {
		ProgramListPanel.UpdateProgramList(&ChInfo);
	}
}


void CMainPanel::UpdateChannelPanel()
{
	const CAppMain &App = GetAppClass();
	Util::CWaitCursor WaitCursor;

	if (ChannelPanel.IsChannelListEmpty()) {
		ChannelPanel.SetChannelList(
			App.ChannelManager.GetCurrentChannelList(),
			!App.EpgOptions.IsEpgDataLoading());
	} else {
		if (!App.EpgOptions.IsEpgDataLoading())
			ChannelPanel.UpdateAllChannels();
	}
	ChannelPanel.SetCurrentChannel(App.ChannelManager.GetCurrentChannel());
}


void CMainPanel::InitControlPanel()
{
	ControlPanel.AddItem(new CTunerControlItem);
	ControlPanel.AddItem(new CChannelControlItem);

	const CChannelList *pList = GetAppClass().ChannelManager.GetCurrentChannelList();
	for (int i = 0; i < 12; i++) {
		TCHAR szText[4];
		StringFormat(szText, TEXT("{}"), i + 1);
		CControlPanelButton *pItem = new CControlPanelButton(CM_CHANNELNO_FIRST + i, szText, i % 6 == 0, 1);
		if (pList == nullptr || pList->FindChannelNo(i + 1) < 0)
			pItem->SetEnable(false);
		ControlPanel.AddItem(pItem);
	}

	ControlPanel.AddItem(new CVideoControlItem);
	ControlPanel.AddItem(new CVolumeControlItem);
	ControlPanel.AddItem(new CAudioControlItem);
}


void CMainPanel::UpdateControlPanel()
{
	const CAppMain &App = GetAppClass();
	const CChannelList *pList = App.ChannelManager.GetCurrentChannelList();
	const CChannelInfo *pCurChannel = App.ChannelManager.GetCurrentChannelInfo();

	for (int i = 0; i < 12; i++) {
		CControlPanelItem *pItem = ControlPanel.GetItem(CONTROLPANEL_ITEM_CHANNEL_1 + i);
		if (pItem != nullptr) {
			pItem->SetEnable(pList != nullptr && pList->FindChannelNo(i + 1) >= 0);
			pItem->SetCheck(false);
		}
	}
	if (pCurChannel != nullptr) {
		if (pCurChannel->GetChannelNo() >= 1 && pCurChannel->GetChannelNo() <= 12) {
			ControlPanel.CheckRadioItem(
				CM_CHANNELNO_FIRST, CM_CHANNELNO_LAST,
				CM_CHANNELNO_FIRST + pCurChannel->GetChannelNo() - 1);
		}
	}
}


CMainPanel::CFrameEventHandler::CFrameEventHandler(CPanelFrame *pFrame)
	: m_pFrame(pFrame)
{
}


bool CMainPanel::CFrameEventHandler::OnClose()
{
	GetAppClass().UICore.DoCommand(CM_PANEL);
	return false;
}


bool CMainPanel::CFrameEventHandler::OnMoving(RECT *pRect)
{
	if (!m_pFrame->GetFloating())
		return false;

	const CAppMain &App = GetAppClass();
	POINT pt;

	::GetCursorPos(&pt);
	pt.x = m_ptStartPos.x + (pt.x - m_ptDragStartCursorPos.x);
	pt.y = m_ptStartPos.y + (pt.y - m_ptDragStartCursorPos.y);
	::OffsetRect(pRect, pt.x - pRect->left, pt.y - pRect->top);
	if (App.PanelOptions.GetSnapAtMainWindow()) {
		// メインウィンドウにスナップさせる
		const int SnapMargin = App.PanelOptions.GetSnapMargin();
		RECT rc;
		int XOffset = 0, YOffset = 0;
		bool fSnap = false;

		App.MainWindow.GetPosition(&rc);
		if (pRect->top < rc.bottom && pRect->bottom > rc.top) {
			if (pRect->right >= rc.left - SnapMargin && pRect->right <= rc.left + SnapMargin) {
				XOffset = rc.left - pRect->right;
				fSnap = true;
			} else if (pRect->left >= rc.right - SnapMargin && pRect->left <= rc.right + SnapMargin) {
				XOffset = rc.right - pRect->left;
				fSnap = true;
			}
			if (fSnap) {
				if (pRect->top >= rc.top - SnapMargin && pRect->top <= rc.top + SnapMargin) {
					YOffset = rc.top - pRect->top;
				} else if (pRect->bottom >= rc.bottom - SnapMargin && pRect->bottom <= rc.bottom + SnapMargin) {
					YOffset = rc.bottom - pRect->bottom;
				}
			}
		}
		if (!fSnap && pRect->left < rc.right && pRect->right > rc.left) {
			if (pRect->bottom >= rc.top - SnapMargin && pRect->bottom <= rc.top + SnapMargin) {
				YOffset = rc.top - pRect->bottom;
				fSnap = true;
			} else if (pRect->top >= rc.bottom - SnapMargin && pRect->top <= rc.bottom + SnapMargin) {
				YOffset = rc.bottom - pRect->top;
				fSnap = true;
			}
			if (fSnap) {
				if (pRect->left >= rc.left - SnapMargin && pRect->left <= rc.left + SnapMargin) {
					XOffset = rc.left - pRect->left;
				} else if (pRect->right >= rc.right - SnapMargin && pRect->right <= rc.right + SnapMargin) {
					XOffset = rc.right - pRect->right;
				}
			}
		}
		::OffsetRect(pRect, XOffset, YOffset);
	}

	return true;
}


bool CMainPanel::CFrameEventHandler::OnEnterSizeMove()
{
	if (!m_pFrame->GetFloating())
		return false;

	::GetCursorPos(&m_ptDragStartCursorPos);
	int x, y;
	m_pFrame->GetPosition(&x, &y, nullptr, nullptr);
	m_ptStartPos.x = x;
	m_ptStartPos.y = y;

	return true;
}


bool CMainPanel::CFrameEventHandler::OnKeyDown(UINT KeyCode, UINT Flags)
{
	GetAppClass().MainWindow.SendMessage(WM_KEYDOWN, KeyCode, Flags);
	return true;
}


bool CMainPanel::CFrameEventHandler::OnMouseWheel(WPARAM wParam, LPARAM lParam)
{
	SendMessage(GetAppClass().MainWindow.GetVideoHostWindow(), WM_MOUSEWHEEL, wParam, lParam);
	return true;
}


void CMainPanel::CFrameEventHandler::OnVisibleChange(bool fVisible)
{
	if (!m_pFrame->GetFloating())
		return;

	const CAppMain &App = GetAppClass();

	if (!fVisible) {
		m_SnapEdge = EDGE_NONE;
		if (App.PanelOptions.GetAttachToMainWindow()) {
			RECT rcPanel, rcMain;

			m_pFrame->GetPosition(&rcPanel);
			App.MainWindow.GetPosition(&rcMain);
			if (rcPanel.top < rcMain.bottom && rcPanel.bottom > rcMain.top) {
				if (rcPanel.right == rcMain.left)
					m_SnapEdge = EDGE_LEFT;
				else if (rcPanel.left == rcMain.right)
					m_SnapEdge = EDGE_RIGHT;
				if (m_SnapEdge != EDGE_NONE)
					m_AttachOffset = rcPanel.top - rcMain.top;
			}
			if (rcPanel.left < rcMain.right && rcPanel.right > rcMain.left) {
				if (rcPanel.bottom == rcMain.top)
					m_SnapEdge = EDGE_TOP;
				else if (rcPanel.top == rcMain.bottom)
					m_SnapEdge = EDGE_BOTTOM;
				if (m_SnapEdge != EDGE_NONE)
					m_AttachOffset = rcPanel.left - rcMain.left;
			}
		}
	} else {
		if (m_SnapEdge != EDGE_NONE) {
			RECT rcPanel, rcMain;
			int x, y;

			m_pFrame->GetPosition(&rcPanel);
			OffsetRect(&rcPanel, -rcPanel.left, -rcPanel.top);
			App.MainWindow.GetPosition(&rcMain);
			switch (m_SnapEdge) {
			case EDGE_LEFT:
				x = rcMain.left - rcPanel.right;
				y = rcMain.top + m_AttachOffset;
				break;
			case EDGE_RIGHT:
				x = rcMain.right;
				y = rcMain.top + m_AttachOffset;
				break;
			case EDGE_TOP:
				y = rcMain.top - rcPanel.bottom;
				x = rcMain.left + m_AttachOffset;
				break;
			case EDGE_BOTTOM:
				y = rcMain.bottom;
				x = rcMain.left + m_AttachOffset;
				break;
			}
			m_pFrame->SetPosition(x, y, rcPanel.right, rcPanel.bottom);
			m_pFrame->MoveToMonitorInside();
		}
	}
}


bool CMainPanel::CFrameEventHandler::OnFloatingChange(bool fFloating)
{
	GetAppClass().MainWindow.OnPanelFloating(fFloating);
	return true;
}


void CMainPanel::CFrameEventHandler::OnDocking(CPanelFrame::DockingPlace Place)
{
	GetAppClass().MainWindow.OnPanelDocking(Place);
}


CMainPanel::CFormEventHandler::CFormEventHandler(CPanelForm *pForm)
	: m_pForm(pForm)
{
}


void CMainPanel::CFormEventHandler::OnSelChange()
{
	GetAppClass().Panel.UpdateContent();
}


void CMainPanel::CFormEventHandler::OnRButtonUp(int x, int y)
{
	GetAppClass().UICore.PopupMenu();
}


void CMainPanel::CFormEventHandler::OnTabRButtonUp(int x, int y)
{
	CPopupMenu Menu;
	Menu.Create();

	int Cur = -1;
	int VisibleCount = 0;

	for (int i = 0; i < m_pForm->NumPages(); i++) {
		CPanelForm::TabInfo TabInfo;

		m_pForm->GetTabInfo(i, &TabInfo);
		if (TabInfo.fVisible && CM_PANEL_FIRST + TabInfo.ID <= CM_PANEL_LAST) {
			String Title;
			TCHAR szMenu[64];

			m_pForm->GetTabTitle(TabInfo.ID, &Title);
			CopyToMenuText(Title.c_str(), szMenu, lengthof(szMenu));
			Menu.Append(CM_PANEL_FIRST + TabInfo.ID, szMenu);
			if (TabInfo.ID == m_pForm->GetCurPageID())
				Cur = VisibleCount;
			VisibleCount++;
		}
	}

	if (VisibleCount > 0) {
		if (Cur >= 0)
			Menu.CheckRadioItem(0, VisibleCount - 1, Cur, MF_BYPOSITION);
		POINT pt = {x, y};
		::ClientToScreen(m_pForm->GetHandle(), &pt);
		Menu.Show(GetAppClass().UICore.GetMainWindow(), &pt, TPM_RIGHTBUTTON);
	}
}


bool CMainPanel::CFormEventHandler::OnKeyDown(UINT KeyCode, UINT Flags)
{
	GetAppClass().MainWindow.SendMessage(WM_KEYDOWN, KeyCode, Flags);
	return true;
}


void CMainPanel::CChannelPanelEventHandler::OnChannelClick(const CChannelInfo *pChannelInfo)
{
	CAppMain &App = GetAppClass();
	const CChannelList *pList = App.ChannelManager.GetCurrentChannelList();

	if (pList != nullptr) {
		int Index = pList->FindByIndex(
			pChannelInfo->GetSpace(),
			pChannelInfo->GetChannelIndex(),
			pChannelInfo->GetServiceID());
		if (Index < 0 && pChannelInfo->GetServiceID() > 0)
			Index = pList->FindByIndex(
				pChannelInfo->GetSpace(),
				pChannelInfo->GetChannelIndex());
		if (Index >= 0 && CM_CHANNEL_FIRST + Index <= CM_CHANNEL_LAST)
			App.UICore.DoCommandAsync(CM_CHANNEL_FIRST + Index);
	}
}


} // namespace TVTest
