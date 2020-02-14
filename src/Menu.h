/*
  TVTest
  Copyright(c) 2008-2020 DBCTRADO

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


#ifndef TVTEST_MENU_H
#define TVTEST_MENU_H


#include <vector>
#include <memory>
#include "Accelerator.h"
#include "ChannelList.h"
#include "Theme.h"
#include "LogoManager.h"
#include "DrawUtil.h"
#include "Graphics.h"
#include "Tooltip.h"
#include "LibISDB/LibISDB/EPG/EventInfo.hpp"


namespace TVTest
{

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
			SUBMENU_ZOOM           = 0,
			SUBMENU_ASPECTRATIO    = 1,
			SUBMENU_CHANNEL        = 5,
			SUBMENU_SERVICE        = 6,
			SUBMENU_SPACE          = 7,
			SUBMENU_FAVORITES      = 8,
			SUBMENU_CHANNELHISTORY = 9,
			SUBMENU_VOLUME         = 12,
			SUBMENU_AUDIO          = 13,
			SUBMENU_VIDEO          = 14,
			SUBMENU_RESET          = 24,
			SUBMENU_BAR            = 28,
			SUBMENU_PLUGIN         = 29,
			SUBMENU_FILTERPROPERTY = 31
		};

		CMainMenu();
		~CMainMenu();

		CMainMenu(const CMainMenu &) = delete;
		CMainMenu &operator=(const CMainMenu &) = delete;

		bool Create(HINSTANCE hinst);
		void Destroy();
		bool Show(UINT Flags, int x, int y, HWND hwnd, bool fToggle = true, const std::vector<int> *pItemList = nullptr);
		bool PopupSubMenu(
			int SubMenu, UINT Flags, HWND hwnd, const POINT *pPos = nullptr,
			bool fToggle = true, const RECT *pExcludeRect = nullptr);
		void EnableItem(UINT ID, bool fEnable);
		void CheckItem(UINT ID, bool fCheck);
		void CheckRadioItem(UINT FirstID, UINT LastID, UINT CheckID);
		bool IsMainMenu(HMENU hmenu) const;
		HMENU GetSubMenu(int SubMenu) const;
		bool SetAccelerator(CAccelerator *pAccelerator);
	};

	class CMenuPainter
	{
		int ItemStateToID(UINT State) const;

		CUxTheme m_UxTheme;
		HWND m_hwnd;
		int m_DPI;
		bool m_fFlatMenu;

	public:
		CMenuPainter();
		~CMenuPainter();

		CMenuPainter(const CMenuPainter &) = delete;
		CMenuPainter &operator=(const CMenuPainter &) = delete;

		void Initialize(HWND hwnd, int DPI);
		void Finalize();
		void GetFont(LOGFONT *pFont);
		COLORREF GetTextColor(UINT State = 0);
		void DrawItemBackground(HDC hdc, const RECT &Rect, UINT State = 0);
		void GetItemMargins(MARGINS *pMargins);
		void GetMargins(MARGINS *pMargins);
		void GetBorderSize(SIZE *pSize);
		void DrawItemText(
			HDC hdc, UINT State, LPCTSTR pszText, const RECT &Rect,
			DWORD Flags = DT_LEFT | DT_SINGLELINE | DT_VCENTER);
		bool GetItemTextExtent(
			HDC hdc, UINT State, LPCTSTR pszText, RECT *pExtent,
			DWORD Flags = DT_LEFT | DT_SINGLELINE | DT_VCENTER);
		void DrawIcon(HIMAGELIST himl, int Icon, HDC hdc, int x, int y, UINT State = 0);
		void DrawBackground(HDC hdc, const RECT &Rect);
		void DrawBorder(HDC hdc, const RECT &Rect);
		void DrawSeparator(HDC hdc, const RECT &Rect);
	};

	class CChannelMenuLogo
	{
	public:
		enum class InitializeFlag : unsigned int {
			None    = 0x0000U,
			NoFrame = 0x0001U,
		};

		CChannelMenuLogo();

		bool Initialize(int IconHeight, InitializeFlag Flags = InitializeFlag::None);
		bool DrawLogo(HDC hdc, int x, int y, const CChannelInfo &Channel);
		int GetLogoWidth() const { return m_LogoWidth; }
		int GetLogoHeight() const { return m_LogoHeight; }

	private:
		int m_LogoWidth;
		int m_LogoHeight;
		bool m_fNoFrame;
		Graphics::CImage m_FrameImage;
	};

	TVTEST_ENUM_FLAGS(CChannelMenuLogo::InitializeFlag)

	class CChannelMenu
	{
	public:
		enum class CreateFlag : unsigned int {
			None            = 0x0000U,
			ShowEventInfo   = 0x0001U,
			ShowLogo        = 0x0002U,
			ShowToolTip     = 0x0004U,
			SpaceBreak      = 0x0008U,
			CurrentServices = 0x0010U,
			Shared          = 0x1000U,
		};

	private:
		CreateFlag m_Flags;
		HWND m_hwnd;
		HMENU m_hmenu;
		CChannelList m_ChannelList;
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
		CChannelMenuLogo m_Logo;
		MARGINS m_Margins;
		int m_MenuLogoMargin;
		CTooltip m_Tooltip;

		int GetEventText(const LibISDB::EventInfo *pEventInfo, LPTSTR pszText, int MaxLength) const;
		void CreateFont(HDC hdc);
		static void GetBaseTime(LibISDB::DateTime *pTime);

	public:
		CChannelMenu();
		~CChannelMenu();

		CChannelMenu(const CChannelMenu &) = delete;
		CChannelMenu &operator=(const CChannelMenu &) = delete;

		bool Create(
			const CChannelList *pChannelList, int CurChannel,
			UINT Command, UINT LastCommand,
			HMENU hmenu, HWND hwnd, CreateFlag Flags, int MaxRows = 0, int DPI = 0);
		void Destroy();
		int Show(UINT Flags, int x, int y, const RECT *pExcludeRect = nullptr);
		bool SetHighlightedItem(int Index);
		bool OnMeasureItem(HWND hwnd, WPARAM wParam, LPARAM lParam);
		bool OnDrawItem(HWND hwnd, WPARAM wParam, LPARAM lParam);
		bool OnMenuSelect(HWND hwnd, WPARAM wParam, LPARAM lParam);
		bool OnUninitMenuPopup(HWND hwnd, WPARAM wParam, LPARAM lParam);
		bool HandleMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *pResult);
	};

	TVTEST_ENUM_FLAGS(CChannelMenu::CreateFlag)

	class CPopupMenu
	{
		enum class PopupMenuType {
			Resource,
			Created,
			Attached,
		};

		HMENU m_hmenu;
		PopupMenuType m_Type;

	public:
		CPopupMenu();
		CPopupMenu(HINSTANCE hinst, LPCTSTR pszName);
		CPopupMenu(HINSTANCE hinst, UINT ID);
		CPopupMenu(HMENU hmenu);
		~CPopupMenu();

		CPopupMenu(const CPopupMenu &) = delete;
		CPopupMenu &operator=(const CPopupMenu &) = delete;

		bool Create();
		bool IsCreated() const { return m_hmenu != nullptr; }
		bool Load(HINSTANCE hinst, LPCTSTR pszName);
		bool Load(HINSTANCE hinst, UINT ID) { return Load(hinst, MAKEINTRESOURCE(ID)); }
		bool Attach(HMENU hmenu);
		void Destroy();
		int GetItemCount() const;
		void Clear();
		HMENU GetPopupHandle() const;
		bool Append(UINT ID, LPCTSTR pszText, UINT Flags = MF_ENABLED);
		bool AppendUnformatted(UINT ID, LPCTSTR pszText, UINT Flags = MF_ENABLED);
		bool Append(HMENU hmenu, LPCTSTR pszText, UINT Flags = MF_ENABLED);
		bool AppendSeparator();
		bool EnableItem(UINT ID, bool fEnable);
		bool CheckItem(UINT ID, bool fCheck);
		bool CheckRadioItem(UINT FirstID, UINT LastID, UINT CheckID, UINT Flags = MF_BYCOMMAND);
		HMENU GetSubMenu(int Pos) const;
		int Show(HWND hwnd, const POINT *pPos = nullptr, UINT Flags = TPM_RIGHTBUTTON, const RECT *pExcludeRect = nullptr);
		int Show(
			HMENU hmenu, HWND hwnd, const POINT *pPos = nullptr, UINT Flags = TPM_RIGHTBUTTON,
			bool fToggle = true, const RECT *pExcludeRect = nullptr);
		int Show(
			HINSTANCE hinst, LPCTSTR pszName, HWND hwnd, const POINT *pPos = nullptr,
			UINT Flags = TPM_RIGHTBUTTON, bool fToggle = true, const RECT *pExcludeRect = nullptr);
		int Show(
			HINSTANCE hinst, int ID, HWND hwnd, const POINT *pPos = nullptr,
			UINT Flags = TPM_RIGHTBUTTON, bool fToggle = true, const RECT *pExcludeRect = nullptr)
		{
			return Show(hinst, MAKEINTRESOURCE(ID), hwnd, pPos, Flags, fToggle, pExcludeRect);
		}
	};

	class CIconMenu
	{
	public:
		struct ItemInfo
		{
			UINT ID;
			LPCTSTR pszIcon;
		};

		CIconMenu();
		~CIconMenu();

		CIconMenu(const CIconMenu &) = delete;
		CIconMenu &operator=(const CIconMenu &) = delete;

		bool Initialize(HMENU hmenu, HINSTANCE hinst, const ItemInfo *pItemList, int ItemCount);
		void Finalize();
		bool OnInitMenuPopup(HWND hwnd, HMENU hmenu);
		bool OnMeasureItem(HWND hwnd, WPARAM wParam, LPARAM lParam);
		bool OnDrawItem(HWND hwnd, WPARAM wParam, LPARAM lParam);
		bool CheckItem(UINT ID, bool fCheck);
		bool CheckRadioItem(UINT FirstID, UINT LastID, UINT CheckID);

	private:
		enum {
			ICON_MARGIN = 1,
			TEXT_MARGIN = 3
		};
		enum {
			ITEM_DATA_IMAGEMASK = 0x0000FFFFUL,
			ITEM_DATA_CHECKED   = 0x00010000UL
		};

		struct ItemIconInfo
		{
			UINT ID;
			int Icon;
		};

		HMENU m_hmenu;
		std::vector<ItemIconInfo> m_ItemList;
		std::vector<HBITMAP> m_BitmapList;
	};

	class CDropDownMenu
	{
	public:
		class CItem
		{
		public:
			CItem(int Command, LPCTSTR pszText);
			virtual ~CItem();
			int GetCommand() const { return m_Command; }
			LPCTSTR GetText() const { return m_Text.c_str(); }
			void SetText(LPCTSTR pszText);
			bool IsSeparator() const { return m_Command < 0; }
			virtual int GetWidth(HDC hdc);
			virtual void Draw(HDC hdc, const RECT *pRect);

		protected:
			int m_Command;
			String m_Text;
			int m_Width;
		};

		static bool Initialize(HINSTANCE hinst);

		CDropDownMenu();

		CDropDownMenu(const CDropDownMenu &) = delete;
		CDropDownMenu &operator=(const CDropDownMenu &) = delete;

		void Clear();
		bool AppendItem(CItem *pItem);
		bool InsertItem(int Index, CItem *pItem);
		bool AppendSeparator();
		bool InsertSeparator(int Index);
		bool DeleteItem(int Command);
		bool SetItemText(int Command, LPCTSTR pszText);
		int CommandToIndex(int Command) const;
		bool Show(HWND hwndOwner, HWND hwndMessage, const POINT *pPos, int CurItem = -1, UINT Flags = 0, int DPI = 0);
		bool Hide();
		bool GetPosition(RECT *pRect);

	private:
		std::vector<std::unique_ptr<CItem>> m_ItemList;
		HWND m_hwnd;
		HWND m_hwndMessage;
		MARGINS m_ItemMargin;
		MARGINS m_WindowMargin;
		DrawUtil::CFont m_Font;
		CMenuPainter m_MenuPainter;
		int m_DPI;
		int m_ItemWidth;
		int m_ItemHeight;
		int m_MaxRows;
		int m_HotItem;
		bool m_fTrackMouseEvent;

		bool GetItemRect(int Index, RECT *pRect) const;
		int HitTest(int x, int y) const;
		void UpdateItem(int Index) const;
		void Draw(HDC hdc, const RECT *pPaintRect);

		static HINSTANCE m_hinst;
		static CDropDownMenu *GetThis(HWND hwnd);
		static LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	};

}	// namespace TVTest


#endif
