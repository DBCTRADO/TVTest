#ifndef PANEL_FORM_H
#define PANEL_FORM_H


#include "BasicWindow.h"
#include "UIBase.h"
#include "Theme.h"
#include "DrawUtil.h"


class CPanelForm
	: public CCustomWindow
	, public TVTest::CUIBase
{
public:
	class ABSTRACT_CLASS(CPage) : public CCustomWindow {
	protected:
		static bool GetDefaultFont(LOGFONT *pFont);
		static HFONT CreateDefaultFont();
		static bool CreateDefaultFont(DrawUtil::CFont *pFont);
	public:
		CPage();
		virtual ~CPage()=0;
		virtual bool SetFont(const LOGFONT *pFont) { return true; }
	};

	class ABSTRACT_CLASS(CEventHandler) {
	public:
		virtual void OnSelChange() {}
		virtual void OnRButtonDown() {}
		virtual void OnTabRButtonDown(int x,int y) {}
		virtual bool OnKeyDown(UINT KeyCode,UINT Flags) { return false; }
		virtual void OnVisibleChange(bool fVisible) {}
	};

	struct TabInfo {
		int ID;
		bool fVisible;
	};

	struct PanelFormTheme {
		TVTest::Theme::Style TabStyle;
		TVTest::Theme::Style CurTabStyle;
		TVTest::Theme::Style TabMarginStyle;
		TVTest::Theme::ThemeColor BackColor;
		TVTest::Theme::ThemeColor BorderColor;
	};

	static bool Initialize(HINSTANCE hinst);

	CPanelForm();
	~CPanelForm();

// CBasicWindow
	bool Create(HWND hwndParent,DWORD Style,DWORD ExStyle=0,int ID=0) override;
	void SetVisible(bool fVisible) override;

// CUIBase
	void SetStyle(const TVTest::Style::CStyleManager *pStyleManager) override;
	void NormalizeStyle(const TVTest::Style::CStyleManager *pStyleManager) override;
	void SetTheme(const TVTest::Theme::CThemeManager *pThemeManager) override;

// CPanelForm
	bool AddWindow(CPage *pWindow,int ID,LPCTSTR pszTitle);
	int NumPages() const { return m_NumWindows; }
	CPage *GetPageByIndex(int Index);
	CPage *GetPageByID(int ID);
	int IDToIndex(int ID) const;
	int GetCurPageID() const;
	bool SetCurPageByID(int ID);
	bool SetTabVisible(int ID,bool fVisible);
	bool GetTabVisible(int ID) const;
	bool SetTabOrder(const int *pOrder);
	bool GetTabInfo(int Index,TabInfo *pInfo) const;
	void SetEventHandler(CEventHandler *pHandler);
	bool SetPanelFormTheme(const PanelFormTheme &Theme);
	bool GetPanelFormTheme(PanelFormTheme *pTheme) const;
	bool SetTabFont(const LOGFONT *pFont);
	bool SetPageFont(const LOGFONT *pFont);

private:
	enum {MAX_WINDOWS=8};

	class CWindowInfo
	{
	public:
		CPage *m_pWindow;
		int m_ID;
		CDynamicString m_Title;
		bool m_fVisible;
		CWindowInfo(CPage *pWindow,int ID,LPCTSTR pszTitle);
		~CWindowInfo();
	};

	struct PanelFormStyle
	{
		TVTest::Style::Margins TabPadding;
		TVTest::Style::Margins ClientMargin;

		PanelFormStyle();
		void SetStyle(const TVTest::Style::CStyleManager *pStyleManager);
		void NormalizeStyle(const TVTest::Style::CStyleManager *pStyleManager);
	};

	CWindowInfo *m_pWindowList[MAX_WINDOWS];
	int m_NumWindows;
	int m_TabOrder[MAX_WINDOWS];
	PanelFormStyle m_Style;
	PanelFormTheme m_Theme;
	DrawUtil::CFont m_Font;
	int m_TabHeight;
	int m_TabWidth;
	bool m_fFitTabWidth;
	int m_CurTab;
	CEventHandler *m_pEventHandler;

	static const LPCTSTR m_pszClassName;
	static HINSTANCE m_hinst;

	bool SetCurTab(int Index);
	void CalcTabSize();
	int GetRealTabWidth() const;
	int HitTest(int x,int y) const;
	void Draw(HDC hdc,const RECT &PaintRect);
// CCustomWindow
	LRESULT OnMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam) override;
};


#endif
