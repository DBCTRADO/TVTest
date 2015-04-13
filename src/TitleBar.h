#ifndef TITLE_BAR_H
#define TITLE_BAR_H


#include "BasicWindow.h"
#include "UIBase.h"
#include "Theme.h"
#include "DrawUtil.h"
#include "Tooltip.h"
#include "WindowUtil.h"


class CTitleBar
	: public CCustomWindow
	, public TVTest::CUIBase
{
public:
	struct TitleBarTheme {
		TVTest::Theme::Style CaptionStyle;
		TVTest::Theme::Style IconStyle;
		TVTest::Theme::Style HighlightIconStyle;
		TVTest::Theme::BorderStyle Border;
	};

	class ABSTRACT_CLASS(CEventHandler) {
	protected:
		class CTitleBar *m_pTitleBar;
	public:
		CEventHandler();
		virtual ~CEventHandler();
		virtual bool OnClose() { return false; }
		virtual bool OnMinimize() { return false; }
		virtual bool OnMaximize() { return false; }
		virtual bool OnFullscreen() { return false; }
		virtual void OnMouseLeave() {}
		virtual void OnLabelLButtonDown(int x,int y) {}
		virtual void OnLabelLButtonDoubleClick(int x,int y) {}
		virtual void OnLabelRButtonUp(int x,int y) {}
		virtual void OnIconLButtonDown(int x,int y) {}
		virtual void OnIconLButtonDoubleClick(int x,int y) {}
		virtual void OnHeightChanged(int Height) {}
		friend class CTitleBar;
	};

	static bool Initialize(HINSTANCE hinst);

	CTitleBar();
	~CTitleBar();

// CBasicWindow
	bool Create(HWND hwndParent,DWORD Style,DWORD ExStyle=0,int ID=0) override;
	void SetVisible(bool fVisible) override;

// CUIBase
	void SetStyle(const TVTest::Style::CStyleManager *pStyleManager) override;
	void NormalizeStyle(const TVTest::Style::CStyleManager *pStyleManager) override;
	void SetTheme(const TVTest::Theme::CThemeManager *pThemeManager) override;

// CTitleBar
	int CalcHeight() const;
	int GetButtonWidth() const;
	int GetButtonHeight() const;
	bool SetLabel(LPCTSTR pszLabel);
	LPCTSTR GetLabel() const { return m_Label.c_str(); }
	void SetMaximizeMode(bool fMaximize);
	void SetFullscreenMode(bool fFullscreen);
	bool SetEventHandler(CEventHandler *pHandler);
	bool SetTitleBarTheme(const TitleBarTheme &Theme);
	bool GetTitleBarTheme(TitleBarTheme *pTheme) const;
	bool SetFont(const LOGFONT *pFont);
	void SetIcon(HICON hIcon);
	SIZE GetIconDrawSize() const;
	bool IsIconDrawSmall() const;

private:
	enum {
		ITEM_LABEL,
		ITEM_MINIMIZE,
		ITEM_MAXIMIZE,
		ITEM_FULLSCREEN,
		ITEM_CLOSE,
		ITEM_BUTTON_FIRST=ITEM_MINIMIZE,
		ITEM_LAST=ITEM_CLOSE
	};

	struct TitleBarStyle
	{
		TVTest::Style::Margins Padding;
		TVTest::Style::Margins LabelMargin;
		TVTest::Style::IntValue LabelExtraHeight;
		TVTest::Style::Size IconSize;
		TVTest::Style::Margins IconMargin;
		TVTest::Style::Size ButtonIconSize;
		TVTest::Style::Margins ButtonPadding;

		TitleBarStyle();
		void SetStyle(const TVTest::Style::CStyleManager *pStyleManager);
		void NormalizeStyle(const TVTest::Style::CStyleManager *pStyleManager);
	};

	TitleBarStyle m_Style;
	DrawUtil::CFont m_Font;
	int m_FontHeight;
	TitleBarTheme m_Theme;
	DrawUtil::CMonoColorIconList m_ButtonIcons;
	HBITMAP m_hbmIcons;
	CTooltip m_Tooltip;
	TVTest::String m_Label;
	HICON m_hIcon;
	int m_HotItem;
	int m_ClickItem;
	CMouseLeaveTrack m_MouseLeaveTrack;
	bool m_fMaximized;
	bool m_fFullscreen;
	CEventHandler *m_pEventHandler;

	static const LPCTSTR CLASS_NAME;
	static HINSTANCE m_hinst;

	int CalcFontHeight() const;
	bool GetItemRect(int Item,RECT *pRect) const;
	bool UpdateItem(int Item);
	int HitTest(int x,int y) const;
	bool PtInIcon(int x,int y) const;
	void UpdateTooltipsRect();
	void Draw(HDC hdc,const RECT &PaintRect);

// CCustomWindow
	LRESULT OnMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam) override;
};


#endif
