#ifndef CONTROL_PANEL_H
#define CONTROL_PANEL_H


#include <vector>
#include "PanelForm.h"
#include "UIBase.h"
#include "DrawUtil.h"
#include "Theme.h"


class CControlPanelItem;

class CControlPanel
	: public CPanelForm::CPage
	, public TVTest::CUIBase
{
public:
	struct ControlPanelTheme {
		TVTest::Theme::Style ItemStyle;
		TVTest::Theme::Style OverItemStyle;
		TVTest::Theme::Style CheckedItemStyle;
		TVTest::Theme::ThemeColor MarginColor;

		ControlPanelTheme();
	};

	static bool Initialize(HINSTANCE hinst);

	CControlPanel();
	~CControlPanel();

// CBasicWindow
	bool Create(HWND hwndParent,DWORD Style,DWORD ExStyle=0,int ID=0) override;

// CUIBase
	void SetStyle(const TVTest::Style::CStyleManager *pStyleManager) override;
	void NormalizeStyle(const TVTest::Style::CStyleManager *pStyleManager) override;
	void SetTheme(const TVTest::Theme::CThemeManager *pThemeManager) override;

// CControlPanel
	bool AddItem(CControlPanelItem *pItem);
	CControlPanelItem *GetItem(int Index) const;
	bool UpdateItem(int Index);
	bool GetItemPosition(int Index,RECT *pRect) const;
	void UpdateLayout();
	bool SetControlPanelTheme(const ControlPanelTheme &Theme);
	bool GetControlPanelTheme(ControlPanelTheme *pTheme) const;
	bool SetFont(const LOGFONT *pFont);
	int GetFontHeight() const { return m_FontHeight; }
	void SetSendMessageWindow(HWND hwnd);
	bool CheckRadioItem(int FirstID,int LastID,int CheckID);
	const TVTest::Style::Margins &GetItemPadding() const;

	friend CControlPanelItem;

private:
	struct ControlPanelStyle
	{
		TVTest::Style::Margins Padding;
		TVTest::Style::Margins ItemPadding;
		TVTest::Style::IntValue TextExtraHeight;

		ControlPanelStyle();
		void SetStyle(const TVTest::Style::CStyleManager *pStyleManager);
		void NormalizeStyle(const TVTest::Style::CStyleManager *pStyleManager);
	};

	std::vector<CControlPanelItem*> m_ItemList;
	DrawUtil::CFont m_Font;
	int m_FontHeight;
	ControlPanelStyle m_Style;
	ControlPanelTheme m_Theme;
	DrawUtil::COffscreen m_Offscreen;
	HWND m_hwndMessage;
	int m_HotItem;
	bool m_fTrackMouseEvent;
	bool m_fOnButtonDown;

	static const LPCTSTR m_pszClassName;
	static HINSTANCE m_hinst;

// CCustomWindow
	LRESULT OnMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam) override;
// CControlPanel
	void Draw(HDC hdc,const RECT &PaintRect);
	void SendCommand(int Command);
	bool CalcTextSize(LPCTSTR pszText,SIZE *pSize);
	int GetTextItemHeight() const;
};

class ABSTRACT_CLASS(CControlPanelItem)
{
protected:
	RECT m_Position;
	int m_Command;
	bool m_fVisible;
	bool m_fEnable;
	bool m_fCheck;
	bool m_fBreak;
	CControlPanel *m_pControlPanel;

	bool CalcTextSize(LPCTSTR pszText,SIZE *pSize) const;
	int GetTextItemHeight() const;
	void GetMenuPos(POINT *pPos) const;

public:
	CControlPanelItem();
	virtual ~CControlPanelItem()=0;
	void GetPosition(int *pLeft,int *pTop,int *pWidth,int *pHeight) const;
	bool SetPosition(int Left,int Top,int Width,int Height);
	void GetPosition(RECT *pRect) const;
	bool GetVisible() const { return m_fVisible; }
	void SetVisible(bool fVisible);
	bool GetEnable() const { return m_fEnable; }
	void SetEnable(bool fEnable);
	bool GetCheck() const { return m_fCheck; }
	void SetCheck(bool fCheck);
	bool GetBreak() const { return m_fBreak; }
	void SetBreak(bool fBreak);
	virtual void CalcSize(int Width,SIZE *pSize);
	virtual void Draw(HDC hdc,const RECT &Rect)=0;
	virtual void OnLButtonDown(int x,int y);
	virtual void OnRButtonDown(int x,int y) {}
	virtual void OnMouseMove(int x,int y) {}
	virtual void SetStyle(const TVTest::Style::CStyleManager *pStyleManager) {}
	virtual void NormalizeStyle(const TVTest::Style::CStyleManager *pStyleManager) {}

	friend CControlPanel;
};


#endif
