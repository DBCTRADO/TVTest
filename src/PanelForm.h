#ifndef PANEL_FORM_H
#define PANEL_FORM_H


#include "Panel.h"
#include "UIBase.h"
#include "Theme.h"
#include "DrawUtil.h"
#include "Tooltip.h"
#include <vector>


class CPanelForm
	: public CPanelContent
{
public:
	class ABSTRACT_CLASS(CPage)
		: public CCustomWindow
		, public TVTest::CUIBase
	{
	public:
		virtual ~CPage() = 0;
		virtual bool SetFont(const TVTest::Style::Font & Font) { return true; }
		virtual void OnActivate() {}
		virtual void OnDeactivate() {}
		virtual void OnVisibilityChanged(bool fVisible) {}
		virtual void OnFormDelete() {}
		virtual bool DrawIcon(
			HDC hdc, int x, int y, int Width, int Height,
			const TVTest::Theme::ThemeColor &Color) { return false; }
		virtual bool NeedKeyboardFocus() const { return false; }

	protected:
		bool CreateDefaultFont(DrawUtil::CFont *pFont);
	};

	class ABSTRACT_CLASS(CEventHandler)
	{
	public:
		virtual void OnSelChange() {}
		virtual void OnRButtonUp(int x, int y) {}
		virtual void OnTabRButtonUp(int x, int y) {}
		virtual bool OnKeyDown(UINT KeyCode, UINT Flags) { return false; }
		virtual void OnVisibleChange(bool fVisible) {}
	};

	struct PageInfo
	{
		CPage *pPage;
		LPCTSTR pszTitle;
		int ID;
		int Icon;
		bool fVisible;
	};

	struct TabInfo
	{
		int ID;
		bool fVisible;
	};

	struct PanelFormTheme
	{
		TVTest::Theme::Style TabStyle;
		TVTest::Theme::Style CurTabStyle;
		TVTest::Theme::Style TabMarginStyle;
		TVTest::Theme::ThemeColor BackColor;
		TVTest::Theme::ThemeColor BorderColor;
	};

	enum TabStyle {
		TABSTYLE_TEXT_ONLY,
		TABSTYLE_ICON_ONLY,
		TABSTYLE_ICON_AND_TEXT
	};
	static const TabStyle TABSTYLE_FIRST = TABSTYLE_TEXT_ONLY;
	static const TabStyle TABSTYLE_LAST  = TABSTYLE_ICON_AND_TEXT;

	static bool Initialize(HINSTANCE hinst);

	CPanelForm();
	~CPanelForm();

// CBasicWindow
	bool Create(HWND hwndParent, DWORD Style, DWORD ExStyle = 0, int ID = 0) override;
	void SetVisible(bool fVisible) override;

// CUIBase
	void SetStyle(const TVTest::Style::CStyleManager *pStyleManager) override;
	void NormalizeStyle(
		const TVTest::Style::CStyleManager *pStyleManager,
		const TVTest::Style::CStyleScaling *pStyleScaling) override;
	void SetTheme(const TVTest::Theme::CThemeManager *pThemeManager) override;

// CPanelForm
	bool AddPage(const PageInfo &Info);
	int NumPages() const { return (int)m_WindowList.size(); }
	CPage *GetPageByIndex(int Index);
	CPage *GetPageByID(int ID);
	int IDToIndex(int ID) const;
	int GetCurPageID() const;
	bool SetCurPageByID(int ID);
	bool SetTabVisible(int ID, bool fVisible);
	bool GetTabVisible(int ID) const;
	bool SetTabOrder(const int *pOrder, int Count);
	bool GetTabInfo(int Index, TabInfo *pInfo) const;
	int GetTabID(int Index) const;
	bool GetTabTitle(int ID, TVTest::String *pTitle) const;
	void SetEventHandler(CEventHandler *pHandler);
	bool SetPanelFormTheme(const PanelFormTheme &Theme);
	bool GetPanelFormTheme(PanelFormTheme *pTheme) const;
	bool SetTabFont(const TVTest::Style::Font &Font);
	bool SetPageFont(const TVTest::Style::Font &Font);
	bool GetPageClientRect(RECT *pRect) const;
	bool SetTabStyle(TabStyle Style);
	bool SetIconImage(HBITMAP hbm, int Width, int Height);
	SIZE GetIconDrawSize() const;
	bool EnableTooltip(bool fEnable);

private:
	enum {MAX_WINDOWS = 8};

	class CWindowInfo
	{
	public:
		CPage *m_pWindow;
		TVTest::String m_Title;
		int m_ID;
		int m_Icon;
		bool m_fVisible;
		CWindowInfo(const PageInfo &Info);
		~CWindowInfo();
	};

	struct PanelFormStyle
	{
		TVTest::Style::Margins TabPadding;
		TVTest::Style::Size TabIconSize;
		TVTest::Style::Margins TabIconMargin;
		TVTest::Style::Margins TabLabelMargin;
		TVTest::Style::IntValue TabIconLabelMargin;
		TVTest::Style::Margins ClientMargin;

		PanelFormStyle();
		void SetStyle(const TVTest::Style::CStyleManager *pStyleManager);
		void NormalizeStyle(
			const TVTest::Style::CStyleManager *pStyleManager,
			const TVTest::Style::CStyleScaling *pStyleScaling);
	};

	std::vector<CWindowInfo*> m_WindowList;
	std::vector<int> m_TabOrder;
	PanelFormStyle m_Style;
	PanelFormTheme m_Theme;
	TVTest::Style::Font m_StyleFont;
	DrawUtil::CFont m_Font;
	DrawUtil::CMonoColorIconList m_Icons;
	TabStyle m_TabStyle;
	int m_TabHeight;
	int m_TabWidth;
	int m_TabLineWidth;
	bool m_fFitTabWidth;
	int m_CurTab;
	int m_PrevActivePageID;
	CEventHandler *m_pEventHandler;
	CTooltip m_Tooltip;
	bool m_fEnableTooltip;

	static const LPCTSTR m_pszClassName;
	static HINSTANCE m_hinst;

	bool SetCurTab(int Index);
	void CalcTabSize();
	int GetRealTabWidth() const;
	int HitTest(int x, int y) const;
	void Draw(HDC hdc, const RECT &PaintRect);
	void UpdateTooltip();

// CCustomWindow
	LRESULT OnMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

// CUIBase
	void ApplyStyle() override;
	void RealizeStyle() override;
};


#endif
