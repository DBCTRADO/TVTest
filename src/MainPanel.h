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


class CMainPanel
{
public:
	CPanelFrame Frame;
	CPanelForm Form;
	CInformationPanel InfoPanel;
	CProgramListPanel ProgramListPanel;
	CChannelPanel ChannelPanel;
	CCaptionPanel CaptionPanel;
	CControlPanel ControlPanel;
	bool fShowPanelWindow;

	CMainPanel();
	bool IsFloating() const;
	bool OnOwnerMovingOrSizing(const RECT *pOldRect,const RECT *pNewRect);
	bool IsAttached();
	void SetTheme(const TVTest::Theme::CThemeManager *pThemeManager);

private:
	class CFrameEventHandler : public CPanelFrame::CEventHandler
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
		} m_SnapEdge;
		int m_AttachOffset;

	public:
		CFrameEventHandler(CPanelFrame *pFrame);
		bool OnClose() override;
		bool OnMoving(RECT *pRect) override;
		bool OnEnterSizeMove() override;
		bool OnKeyDown(UINT KeyCode,UINT Flags) override;
		bool OnMouseWheel(WPARAM wParam,LPARAM lParam) override;
		void OnVisibleChange(bool fVisible) override;
		bool OnFloatingChange(bool fFloating) override;
	};

	class CFormEventHandler : public CPanelForm::CEventHandler
	{
		CPanelForm *m_pForm;

	public:
		CFormEventHandler(CPanelForm *pForm);
		void OnSelChange() override;
		void OnRButtonUp(int x,int y) override;
		void OnTabRButtonUp(int x,int y) override;
		bool OnKeyDown(UINT KeyCode,UINT Flags) override;
	};

	class CChannelPanelEventHandler : public CChannelPanel::CEventHandler
	{
		void OnChannelClick(const CChannelInfo *pChannelInfo) override;
	};

	CFrameEventHandler m_FrameEventHandler;
	CFormEventHandler m_FormEventHandler;
	CChannelPanelEventHandler m_ChannelPanelEventHandler;
};


#endif
