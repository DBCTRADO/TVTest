#ifndef SIDE_BAR_H
#define SIDE_BAR_H


#include <vector>
#include "BasicWindow.h"
#include "UIBase.h"
#include "Command.h"
#include "DrawUtil.h"
#include "Theme.h"
#include "Tooltip.h"
#include "WindowUtil.h"


class CSideBar
	: public CCustomWindow
	, public TVTest::CUIBase
{
public:
	enum {
		ITEM_SEPARATOR=0
	};

	enum {
		ITEM_STATE_DISABLED	=0x0001U,
		ITEM_STATE_CHECKED	=0x0002U,
		ITEM_STATE_HOT		=0x0004U
	};

	struct SideBarItem {
		int Command;
		int Icon;
		unsigned int State;

		bool IsDisabled() const { return (State & ITEM_STATE_DISABLED)!=0; }
		bool IsEnabled() const { return !IsDisabled(); }
		bool IsChecked() const { return (State & ITEM_STATE_CHECKED)!=0; }
	};

	struct SideBarTheme {
		TVTest::Theme::Style ItemStyle;
		TVTest::Theme::Style HighlightItemStyle;
		TVTest::Theme::Style CheckItemStyle;
		TVTest::Theme::BorderStyle Border;
	};

	struct DrawIconInfo {
		int Command;
		unsigned int State;
		HDC hdc;
		RECT IconRect;
		COLORREF Color;
		BYTE Opacity;
		HDC hdcBuffer;
	};

	class ABSTRACT_CLASS(CEventHandler) {
	protected:
		CSideBar *m_pSideBar;
	public:
		CEventHandler();
		virtual ~CEventHandler();
		virtual void OnCommand(int Command) {}
		virtual void OnRButtonUp(int x,int y) {}
		virtual void OnMouseLeave() {}
		virtual bool GetTooltipText(int Command,LPTSTR pszText,int MaxText) { return false; }
		virtual bool DrawIcon(const DrawIconInfo *pInfo) { return false; }
		virtual void OnBarWidthChanged(int BarWidth) {}
		friend class CSideBar;
	};

	static bool Initialize(HINSTANCE hinst);

	CSideBar(const CCommandList *pCommandList);
	~CSideBar();

// CBasicWindow
	bool Create(HWND hwndParent,DWORD Style,DWORD ExStyle=0,int ID=0) override;

// CUIBase
	void SetStyle(const TVTest::Style::CStyleManager *pStyleManager) override;
	void NormalizeStyle(const TVTest::Style::CStyleManager *pStyleManager) override;
	void SetTheme(const TVTest::Theme::CThemeManager *pThemeManager) override;

// CSideBar
	int GetBarWidth() const;
	bool SetIconImage(HBITMAP hbm,int Width,int Height);
	void DeleteAllItems();
	bool AddItem(const SideBarItem *pItem);
	bool AddItems(const SideBarItem *pItemList,int NumItems);
	bool AddSeparator();
	int GetItemCount() const;
	int GetItemCommand(int Index) const;
	int CommandToIndex(int Command) const;
	bool EnableItem(int Command,bool fEnable);
	bool EnableItemByIndex(int Index,bool fEnable);
	bool IsItemEnabled(int Command) const;
	bool CheckItem(int Command,bool fCheck);
	bool CheckItemByIndex(int Index,bool fCheck);
	bool CheckRadioItem(int First,int Last,int Check);
	bool IsItemChecked(int Command) const;
	bool RedrawItem(int Command);
	bool SetSideBarTheme(const SideBarTheme &Theme);
	bool GetSideBarTheme(SideBarTheme *pTheme) const;
	void ShowToolTips(bool fShow);
	void SetVertical(bool fVertical);
	bool GetVertical() const { return m_fVertical; }
	void SetEventHandler(CEventHandler *pHandler);
	const CCommandList *GetCommandList() const { return m_pCommandList; }
	TVTest::Style::Size GetIconDrawSize() const;

protected:
	struct SideBarStyle
	{
		TVTest::Style::Size IconSize;
		TVTest::Style::Margins ItemPadding;
		TVTest::Style::IntValue SeparatorWidth;

		SideBarStyle();
		void SetStyle(const TVTest::Style::CStyleManager *pStyleManager);
		void NormalizeStyle(const TVTest::Style::CStyleManager *pStyleManager);
	};

	SideBarStyle m_Style;
	CTooltip m_Tooltip;
	bool m_fShowTooltips;
	DrawUtil::CMonoColorIconList m_Icons;
	bool m_fVertical;
	SideBarTheme m_Theme;
	std::vector<SideBarItem> m_ItemList;
	int m_HotItem;
	int m_ClickItem;
	CMouseLeaveTrack m_MouseLeaveTrack;
	CEventHandler *m_pEventHandler;
	const CCommandList *m_pCommandList;

	static const int ICON_WIDTH;
	static const int ICON_HEIGHT;
	static const LPCTSTR CLASS_NAME;
	static HINSTANCE m_hinst;

	void GetItemRect(int Item,RECT *pRect) const;
	void UpdateItem(int Item);
	int HitTest(int x,int y) const;
	void UpdateTooltipsRect();
	void Draw(HDC hdc,const RECT &PaintRect);

// CCustomWindow
	LRESULT OnMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam) override;
};


#endif
