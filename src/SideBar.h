#ifndef SIDE_BAR_H
#define SIDE_BAR_H


#include <vector>
#include "BasicWindow.h"
#include "Command.h"
#include "DrawUtil.h"
#include "Theme.h"
#include "Tooltip.h"
#include "WindowUtil.h"


class CSideBar : public CCustomWindow
{
public:
	enum {
		ITEM_SEPARATOR=0
	};

	struct SideBarItem {
		int Command;
		int Icon;
		unsigned int Flags;
	};
	enum {
		ITEM_FLAG_DISABLED	=0x0001,
		ITEM_FLAG_CHECKED	=0x0002
	};

	struct ThemeInfo {
		Theme::Style ItemStyle;
		Theme::Style HighlightItemStyle;
		Theme::Style CheckItemStyle;
		Theme::BorderInfo Border;
	};

	class ABSTRACT_CLASS(CEventHandler) {
	protected:
		CSideBar *m_pSideBar;
	public:
		CEventHandler();
		virtual ~CEventHandler();
		virtual void OnCommand(int Command) {}
		virtual void OnRButtonDown(int x,int y) {}
		virtual void OnMouseLeave() {}
		virtual bool GetTooltipText(int Command,LPTSTR pszText,int MaxText) { return false; }
		virtual bool DrawIcon(int Command,HDC hdc,const RECT &ItemRect,COLORREF ForeColor,HDC hdcBuffer) { return false; }
		friend class CSideBar;
	};

	static bool Initialize(HINSTANCE hinst);

	CSideBar(const CCommandList *pCommandList);
	~CSideBar();
// CBasicWindow
	bool Create(HWND hwndParent,DWORD Style,DWORD ExStyle=0,int ID=0) override;
// CSideBar
	int GetBarWidth() const;
	bool SetIconImage(HBITMAP hbm);
	void DeleteAllItems();
	bool AddItem(const SideBarItem *pItem);
	bool AddItems(const SideBarItem *pItemList,int NumItems);
	int CommandToIndex(int Command) const;
	bool EnableItem(int Command,bool fEnable);
	bool IsItemEnabled(int Command) const;
	bool CheckItem(int Command,bool fCheck);
	bool CheckRadioItem(int First,int Last,int Check);
	bool IsItemChecked(int Command) const;
	bool SetTheme(const ThemeInfo *pTheme);
	bool GetTheme(ThemeInfo *pTheme) const;
	void ShowToolTips(bool fShow);
	void SetVertical(bool fVertical);
	void SetEventHandler(CEventHandler *pHandler);
	const CCommandList *GetCommandList() const { return m_pCommandList; }

protected:
	CTooltip m_Tooltip;
	bool m_fShowTooltips;
	DrawUtil::CMonoColorBitmap m_IconBitmap;
	bool m_fVertical;
	ThemeInfo m_Theme;
	RECT m_ItemMargin;
	int m_IconWidth;
	int m_IconHeight;
	int m_SeparatorWidth;
	std::vector<SideBarItem> m_ItemList;
	int m_HotItem;
	int m_ClickItem;
	CMouseLeaveTrack m_MouseLeaveTrack;
	CEventHandler *m_pEventHandler;
	const CCommandList *m_pCommandList;

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
