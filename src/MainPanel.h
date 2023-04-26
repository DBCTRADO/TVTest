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


#ifndef TVTEST_MAIN_PANEL_H
#define TVTEST_MAIN_PANEL_H


#include "Panel.h"
#include "PanelForm.h"
#include "InformationPanel.h"
#include "ProgramListPanel.h"
#include "ChannelPanel.h"
#include "CaptionPanel.h"
#include "ControlPanel.h"
#include "ControlPanelItems.h"


namespace TVTest
{

	class CMainPanel
	{
	public:
		CPanelFrame Frame;
		CInformationPanel InfoPanel;
		CProgramListPanel ProgramListPanel;
		CChannelPanel ChannelPanel;
		CCaptionPanel CaptionPanel;
		CControlPanel ControlPanel;
		// CPanelFormのデストラクタからパネル項目に通知が送られるので、宣言の順序に注意
		CPanelForm Form;
		bool fShowPanelWindow = false;

		CMainPanel();

		bool IsFloating() const;
		bool OnOwnerWindowPosChanging(const RECT *pOldRect, const RECT *pNewRect);
		bool IsAttached();
		void SetTheme(const Theme::CThemeManager *pThemeManager);
		void UpdateContent();
		void UpdateInformationPanel();
		void UpdateProgramListPanel();
		void UpdateChannelPanel();
		void UpdateControlPanel();
		void InitControlPanel();
		void EnableProgramListUpdate(bool fEnable) { m_fEnableProgramListUpdate = fEnable; }

	private:
		class CFrameEventHandler
			: public CPanelFrame::CEventHandler
		{
			CPanelFrame *m_pFrame;
			POINT m_ptDragStartCursorPos;
			POINT m_ptStartPos;
			enum {
				EDGE_NONE,
				EDGE_LEFT,
				EDGE_RIGHT,
				EDGE_TOP,
				EDGE_BOTTOM
			} m_SnapEdge = EDGE_NONE;
			int m_AttachOffset;

		public:
			CFrameEventHandler(CPanelFrame *pFrame);

			bool OnClose() override;
			bool OnMoving(RECT *pRect) override;
			bool OnEnterSizeMove() override;
			bool OnKeyDown(UINT KeyCode, UINT Flags) override;
			bool OnMouseWheel(WPARAM wParam, LPARAM lParam) override;
			void OnVisibleChange(bool fVisible) override;
			bool OnFloatingChange(bool fFloating) override;
			void OnDocking(CPanelFrame::DockingPlace Place) override;
		};

		class CFormEventHandler
			: public CPanelForm::CEventHandler
		{
			CPanelForm *m_pForm;

		public:
			CFormEventHandler(CPanelForm *pForm);

			void OnSelChange() override;
			void OnRButtonUp(int x, int y) override;
			void OnTabRButtonUp(int x, int y) override;
			bool OnKeyDown(UINT KeyCode, UINT Flags) override;
		};

		class CChannelPanelEventHandler
			: public CChannelPanel::CEventHandler
		{
			void OnChannelClick(const CChannelInfo *pChannelInfo) override;
		};

		CFrameEventHandler m_FrameEventHandler;
		CFormEventHandler m_FormEventHandler;
		CChannelPanelEventHandler m_ChannelPanelEventHandler;
		bool m_fEnableProgramListUpdate = true;
	};

}	// namespace TVTest


#endif
