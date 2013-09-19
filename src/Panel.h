#ifndef TVTEST_PANEL_H
#define TVTEST_PANEL_H


#include "Layout.h"
#include "DrawUtil.h"
#include "Theme.h"


class CPanel : public CCustomWindow
{
public:
	class ABSTRACT_CLASS(CEventHandler) {
	public:
		virtual ~CEventHandler() {}
		virtual bool OnFloating() { return false; }
		virtual bool OnClose() { return false; }
		virtual bool OnEnterSizeMove() { return false; }
		virtual bool OnMoving(RECT *pRect) { return false; }
		virtual bool OnKeyDown(UINT KeyCode,UINT Flags) { return false; }
		virtual void OnSizeChanged(int Width,int Height) {}
		virtual bool OnMenuPopup(HMENU hmenu) { return true; }
		virtual bool OnMenuSelected(int Command) { return false; }
	};
	enum { MENU_USER=100 };
	struct ThemeInfo {
		Theme::Style TitleStyle;
	};

	static bool Initialize(HINSTANCE hinst);

	CPanel();
	~CPanel();
// CBasicWindow
	bool Create(HWND hwndParent,DWORD Style,DWORD ExStyle=0,int ID=0) override;
// CPanel
	bool SetWindow(CBasicWindow *pWindow,LPCTSTR pszTitle);
	void ShowTitle(bool fShow);
	void EnableFloating(bool fEnable);
	void SetEventHandler(CEventHandler *pHandler);
	CBasicWindow *GetWindow() { return m_pWindow; }
	bool SetTheme(const ThemeInfo *pTheme);
	bool GetTheme(ThemeInfo *pTheme) const;
	bool GetTitleRect(RECT *pRect) const;
	bool GetContentRect(RECT *pRect) const;

private:
	int m_TitleMargin;
	int m_ButtonSize;
	DrawUtil::CFont m_Font;
	int m_TitleHeight;
	CBasicWindow *m_pWindow;
	CDynamicString m_Title;
	bool m_fShowTitle;
	bool m_fEnableFloating;
	ThemeInfo m_Theme;
	CEventHandler *m_pEventHandler;
	bool m_fCloseButtonPushed;
	POINT m_ptDragStartPos;
	POINT m_ptMovingWindowPos;

	static HINSTANCE m_hinst;

	void Draw(HDC hdc,const RECT &PaintRect) const;
	void OnSize(int Width,int Height);
	void GetCloseButtonRect(RECT *pRect) const;
// CCustomWindow
	LRESULT OnMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam) override;
};

class CDropHelper : public CCustomWindow
{
	int m_Opacity;
	static HINSTANCE m_hinst;

// CBasicWindow
	bool Create(HWND hwndParent,DWORD Style,DWORD ExStyle=0,int ID=0) override;
// CCustomWindow
	LRESULT OnMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam) override;

public:
	static bool Initialize(HINSTANCE hinst);

	CDropHelper();
	~CDropHelper();
	bool Show(const RECT *pRect);
	bool Hide();
};

class CPanelFrame : public CCustomWindow, public CPanel::CEventHandler
{
public:
	class ABSTRACT_CLASS(CEventHandler) {
	public:
		virtual ~CEventHandler() {}
		virtual bool OnClose() { return true; }
		virtual bool OnMoving(RECT *pRect) { return false; }
		virtual bool OnEnterSizeMove() { return false; }
		virtual bool OnKeyDown(UINT KeyCode,UINT Flags) { return false; }
		virtual bool OnMouseWheel(WPARAM wParam,LPARAM lParam) { return false; }
		virtual void OnVisibleChange(bool fVisible) {}
		virtual bool OnFloatingChange(bool fFloating) { return true; }
		virtual bool OnActivate(bool fActive) { return false; }
	};

	static bool Initialize(HINSTANCE hinst);

	CPanelFrame();
	~CPanelFrame();
	bool Create(HWND hwndOwner,Layout::CSplitter *pSplitter,int PanelID,
				CBasicWindow *pWindow,LPCTSTR pszTitle);
	CPanel *GetPanel() { return &m_Panel; }
	CBasicWindow *GetWindow() { return m_Panel.GetWindow(); }
	bool SetFloating(bool fFloating);
	bool GetFloating() const { return m_fFloating; }
	void SetEventHandler(CEventHandler *pHandler);
	bool SetPanelVisible(bool fVisible,bool fNoActivate=false);
	int GetDockingWidth() const { return m_DockingWidth; }
	bool SetDockingWidth(int Width);
	bool SetTheme(const CPanel::ThemeInfo *pTheme);
	bool GetTheme(CPanel::ThemeInfo *pTheme) const;
	bool SetOpacity(int Opacity);
	int GetOpacity() const { return m_Opacity; }
	Layout::CSplitter *m_pSplitter;
	int m_PanelID;
	CPanel m_Panel;
	bool m_fFloating;
	int m_DockingWidth;
	bool m_fKeepWidth;
	int m_Opacity;
	CDropHelper m_DropHelper;
	enum DockingPlace {
		DOCKING_NONE,
		DOCKING_LEFT,
		DOCKING_RIGHT
	};

private:
	DockingPlace m_DragDockingTarget;
	bool m_fDragMoving;
	CEventHandler *m_pEventHandler;

	static HINSTANCE m_hinst;

// CBasicWindow
	bool Create(HWND hwndParent,DWORD Sytle,DWORD ExStyle=0,int ID=0) override;
// CCustomWindow
	LRESULT OnMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam) override;
// CPanel::CEventHandler
	bool OnFloating() override;
	bool OnClose() override;
	bool OnEnterSizeMove() override;
	bool OnMoving(RECT *pRect) override;
	bool OnKeyDown(UINT KeyCode,UINT Flags) override;
	void OnSizeChanged(int Width,int Height) override;
	bool OnMenuPopup(HMENU hmenu) override;
	bool OnMenuSelected(int Command) override;
};


#endif
