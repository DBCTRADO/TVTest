#ifndef TVTEST_MENU_H
#define TVTEST_MENU_H


#include <vector>
#include "Accelerator.h"
#include "ChannelList.h"
#include "EpgProgramList.h"
#include "Theme.h"
#include "LogoManager.h"
#include "DrawUtil.h"
#include "Tooltip.h"


class CMainMenu
{
	HMENU m_hmenu;
	HMENU m_hmenuPopup;
	HMENU m_hmenuShow;
	bool m_fPopup;
	int m_PopupMenu;

public:
	// サブメニュー項目の位置
	enum {
		SUBMENU_ZOOM			= 0,
		SUBMENU_ASPECTRATIO		= 1,
		SUBMENU_CHANNEL			= 5,
		SUBMENU_SERVICE			= 6,
		SUBMENU_SPACE			= 7,
		SUBMENU_FAVORITES		= 8,
		SUBMENU_CHANNELHISTORY	= 9,
		SUBMENU_VOLUME			= 11,
		SUBMENU_AUDIO			= 12,
		SUBMENU_RESET			= 22,
		SUBMENU_BAR				= 26,
		SUBMENU_PLUGIN			= 27,
		SUBMENU_FILTERPROPERTY	= 29
	};

	CMainMenu();
	~CMainMenu();
	bool Create(HINSTANCE hinst);
	void Destroy();
	bool Show(UINT Flags,int x,int y,HWND hwnd,bool fToggle=true,const std::vector<int> *pItemList=NULL);
	bool PopupSubMenu(int SubMenu,UINT Flags,int x,int y,HWND hwnd,bool fToggle=true);
	void EnableItem(UINT ID,bool fEnable);
	void CheckItem(UINT ID,bool fCheck);
	void CheckRadioItem(UINT FirstID,UINT LastID,UINT CheckID);
	bool IsMainMenu(HMENU hmenu) const;
	HMENU GetSubMenu(int SubMenu) const;
	bool SetAccelerator(CAccelerator *pAccelerator);
};

class CMenuPainter
{
	int ItemStateToID(UINT State) const;

	CUxTheme m_UxTheme;
	bool m_fFlatMenu;

public:
	CMenuPainter();
	~CMenuPainter();
	void Initialize(HWND hwnd);
	void Finalize();
	void GetFont(LOGFONT *pFont);
	COLORREF GetTextColor(UINT State=0);
	void DrawItemBackground(HDC hdc,const RECT &Rect,UINT State=0);
	void GetItemMargins(MARGINS *pMargins);
	void GetMargins(MARGINS *pMargins);
	void GetBorderSize(SIZE *pSize);
	void DrawItemText(HDC hdc,UINT State,LPCTSTR pszText,const RECT &Rect,
					  DWORD Flags=DT_LEFT | DT_SINGLELINE | DT_VCENTER);
	bool GetItemTextExtent(HDC hdc,UINT State,LPCTSTR pszText,RECT *pExtent,
						   DWORD Flags=DT_LEFT | DT_SINGLELINE | DT_VCENTER);
	void DrawIcon(HIMAGELIST himl,int Icon,HDC hdc,int x,int y,UINT State=0);
	void DrawBackground(HDC hdc,const RECT &Rect);
	void DrawBorder(HDC hdc,const RECT &Rect);
	void DrawSeparator(HDC hdc,const RECT &Rect);
};

class CChannelMenu
{
	unsigned int m_Flags;
	HWND m_hwnd;
	HMENU m_hmenu;
	CEpgProgramList *m_pProgramList;
	CLogoManager *m_pLogoManager;
	const CChannelList *m_pChannelList;
	int m_CurChannel;
	UINT m_FirstCommand;
	UINT m_LastCommand;
	DrawUtil::CFont m_Font;
	DrawUtil::CFont m_FontCurrent;
	int m_TextHeight;
	int m_ChannelNameWidth;
	int m_EventNameWidth;
	int m_LogoWidth;
	int m_LogoHeight;
	CMenuPainter m_MenuPainter;
	DrawUtil::CBitmap m_LogoFrameImage;
	MARGINS m_Margins;
	int m_MenuLogoMargin;
	CTooltip m_Tooltip;

	int GetEventText(const CEventInfoData *pEventInfo,
					 LPTSTR pszText,int MaxLength) const;
	void CreateFont(HDC hdc);
	static void GetBaseTime(SYSTEMTIME *pTime);

public:
	enum {
		FLAG_SHOWEVENTINFO	=0x0001,
		FLAG_SHOWLOGO		=0x0002,
		FLAG_SHOWTOOLTIP	=0x0004,
		FLAG_SPACEBREAK		=0x0008,
		FLAG_SHARED			=0x1000
	};

	CChannelMenu(CEpgProgramList *pProgramList,CLogoManager *pLogoManager);
	~CChannelMenu();
	bool Create(const CChannelList *pChannelList,int CurChannel,UINT Command,
				HMENU hmenu,HWND hwnd,unsigned int Flags,int MaxRows=0);
	void Destroy();
	bool Show(UINT Flags,int x,int y);
	bool OnMeasureItem(HWND hwnd,WPARAM wParam,LPARAM lParam);
	bool OnDrawItem(HWND hwnd,WPARAM wParam,LPARAM lParam);
	bool OnMenuSelect(HWND hwnd,WPARAM wParam,LPARAM lParam);
	bool OnUninitMenuPopup(HWND hwnd,WPARAM wParam,LPARAM lParam);
};

class CPopupMenu
{
	enum PopupMenuType {
		TYPE_RESOURCE,
		TYPE_CREATED,
		TYPE_ATTACHED
	};

	HMENU m_hmenu;
	PopupMenuType m_Type;

public:
	CPopupMenu();
	CPopupMenu(HINSTANCE hinst,LPCTSTR pszName);
	CPopupMenu(HINSTANCE hinst,UINT ID);
	CPopupMenu(HMENU hmenu);
	~CPopupMenu();
	bool Create();
	bool IsCreated() const { return m_hmenu!=NULL; }
	bool Load(HINSTANCE hinst,LPCTSTR pszName);
	bool Load(HINSTANCE hinst,UINT ID) { return Load(hinst,MAKEINTRESOURCE(ID)); }
	bool Attach(HMENU hmenu);
	void Destroy();
	int GetItemCount() const;
	void Clear();
	HMENU GetPopupHandle() const;
	bool Append(UINT ID,LPCTSTR pszText,UINT Flags=MF_ENABLED);
	bool AppendUnformatted(UINT ID,LPCTSTR pszText,UINT Flags=MF_ENABLED);
	bool Append(HMENU hmenu,LPCTSTR pszText,UINT Flags=MF_ENABLED);
	bool AppendSeparator();
	bool EnableItem(UINT ID,bool fEnable);
	bool CheckItem(UINT ID,bool fCheck);
	bool CheckRadioItem(UINT FirstID,UINT LastID,UINT CheckID,UINT Flags=MF_BYCOMMAND);
	HMENU GetSubMenu(int Pos) const;
	bool Show(HWND hwnd,const POINT *pPos=NULL,UINT Flags=TPM_RIGHTBUTTON);
	bool Show(HMENU hmenu,HWND hwnd,const POINT *pPos=NULL,UINT Flags=TPM_RIGHTBUTTON,bool fToggle=true);
	bool Show(HINSTANCE hinst,LPCTSTR pszName,HWND hwnd,const POINT *pPos=NULL,UINT Flags=TPM_RIGHTBUTTON,bool fToggle=true);
	bool Show(HINSTANCE hinst,int ID,HWND hwnd,const POINT *pPos=NULL,UINT Flags=TPM_RIGHTBUTTON,bool fToggle=true) {
		return Show(hinst,MAKEINTRESOURCE(ID),hwnd,pPos,Flags,fToggle);
	}
};

class CIconMenu
{
public:
	struct ItemInfo
	{
		UINT ID;
		int Image;
	};

	CIconMenu();
	~CIconMenu();
	bool Initialize(HMENU hmenu,HINSTANCE hinst,LPCTSTR pszImageName,
					int IconWidth,const ItemInfo *pItemList,int ItemCount);
	void Finalize();
	bool OnInitMenuPopup(HWND hwnd,HMENU hmenu);
	bool OnMeasureItem(HWND hwnd,WPARAM wParam,LPARAM lParam);
	bool OnDrawItem(HWND hwnd,WPARAM wParam,LPARAM lParam);
	bool CheckItem(UINT ID,bool fCheck);
	bool CheckRadioItem(UINT FirstID,UINT LastID,UINT CheckID);

private:
	enum {
		ICON_MARGIN=1,
		TEXT_MARGIN=3
	};
	enum {
		ITEM_DATA_IMAGEMASK	=0x0000FFFFUL,
		ITEM_DATA_CHECKED	=0x00010000UL
	};

	HMENU m_hmenu;
	std::vector<ItemInfo> m_ItemList;
	HIMAGELIST m_hImageList;
	std::vector<HBITMAP> m_BitmapList;
	CMenuPainter m_MenuPainter;
};

class CDropDownMenu
{
public:
	class CItem {
	public:
		CItem(int Command,LPCTSTR pszText);
		virtual ~CItem();
		int GetCommand() const { return m_Command; }
		LPCTSTR GetText() const { return m_Text.Get(); }
		bool SetText(LPCTSTR pszText);
		bool IsSeparator() const { return m_Command<0; }
		virtual int GetWidth(HDC hdc);
		virtual void Draw(HDC hdc,const RECT *pRect);

	protected:
		int m_Command;
		CDynamicString m_Text;
		int m_Width;
	};

	static bool Initialize(HINSTANCE hinst);

	CDropDownMenu();
	~CDropDownMenu();
	void Clear();
	bool AppendItem(CItem *pItem);
	bool InsertItem(int Index,CItem *pItem);
	bool AppendSeparator();
	bool InsertSeparator(int Index);
	bool DeleteItem(int Command);
	bool SetItemText(int Command,LPCTSTR pszText);
	int CommandToIndex(int Command) const;
	bool Show(HWND hwndOwner,HWND hwndMessage,const POINT *pPos,int CurItem=-1,UINT Flags=0);
	bool Hide();
	bool GetPosition(RECT *pRect);

private:
	std::vector<CItem*> m_ItemList;
	HWND m_hwnd;
	HWND m_hwndMessage;
	MARGINS m_ItemMargin;
	MARGINS m_WindowMargin;
	DrawUtil::CFont m_Font;
	CMenuPainter m_MenuPainter;
	int m_ItemWidth;
	int m_ItemHeight;
	int m_MaxRows;
	int m_HotItem;
	bool m_fTrackMouseEvent;

	bool GetItemRect(int Index,RECT *pRect) const;
	int HitTest(int x,int y) const;
	void UpdateItem(int Index) const;
	void Draw(HDC hdc,const RECT *pPaintRect);

	static HINSTANCE m_hinst;
	static CDropDownMenu *GetThis(HWND hwnd);
	static LRESULT CALLBACK WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
};


#endif
