#ifndef TITLE_BAR_H
#define TITLE_BAR_H


#include "BasicWindow.h"
#include "Theme.h"
#include "DrawUtil.h"
#include "Tooltip.h"
#include "WindowUtil.h"


class CTitleBar : public CCustomWindow
{
public:
	struct ThemeInfo {
		Theme::Style CaptionStyle;
		Theme::Style IconStyle;
		Theme::Style HighlightIconStyle;
		Theme::BorderInfo Border;
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
		virtual void OnLabelRButtonDown(int x,int y) {}
		virtual void OnIconLButtonDown(int x,int y) {}
		virtual void OnIconLButtonDoubleClick(int x,int y) {}
		friend class CTitleBar;
	};

	static bool Initialize(HINSTANCE hinst);

	CTitleBar();
	~CTitleBar();
// CBasicWindow
	bool Create(HWND hwndParent,DWORD Style,DWORD ExStyle=0,int ID=0) override;
	void SetVisible(bool fVisible) override;
// CTitleBar
	bool SetLabel(LPCTSTR pszLabel);
	LPCTSTR GetLabel() const { return m_Label.Get(); }
	void SetMaximizeMode(bool fMaximize);
	void SetFullscreenMode(bool fFullscreen);
	bool SetEventHandler(CEventHandler *pHandler);
	bool SetTheme(const ThemeInfo *pTheme);
	bool GetTheme(ThemeInfo *pTheme) const;
	bool SetFont(const LOGFONT *pFont);
	void SetIcon(HICON hIcon);

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
	DrawUtil::CFont m_Font;
	int m_FontHeight;
	ThemeInfo m_Theme;
	HBITMAP m_hbmIcons;
	CTooltip m_Tooltip;
	CDynamicString m_Label;
	HICON m_hIcon;
	int m_HotItem;
	int m_ClickItem;
	CMouseLeaveTrack m_MouseLeaveTrack;
	bool m_fMaximized;
	bool m_fFullscreen;
	CEventHandler *m_pEventHandler;

	static HINSTANCE m_hinst;

	bool GetItemRect(int Item,RECT *pRect) const;
	bool UpdateItem(int Item);
	int HitTest(int x,int y) const;
	void UpdateTooltipsRect();
	void Draw(HDC hdc,const RECT &PaintRect);

// CCustomWindow
	LRESULT OnMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam) override;
};


#endif
