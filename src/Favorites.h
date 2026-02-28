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


#ifndef TVTEST_FAVORITES_H
#define TVTEST_FAVORITES_H


#include "ChannelList.h"
#include "Menu.h"
#include "Dialog.h"


namespace TVTest
{

	class CFavoritesManager;

	class ABSTRACT_CLASS(CFavoriteItem)
	{
	public:
		enum class ItemType {
			Folder,
			Channel,
		};

		CFavoriteItem(ItemType Type) : m_Type(Type) {}
		virtual ~CFavoriteItem() = default;

		ItemType GetType() const { return m_Type; }
		virtual CFavoriteItem *Duplicate() const = 0;
		LPCTSTR GetName() const { return m_Name.c_str(); }
		bool SetName(LPCTSTR pszName);

	protected:
		ItemType m_Type;
		String m_Name;
	};

	class CFavoriteFolder
		: public CFavoriteItem
	{
	public:
		CFavoriteFolder();
		CFavoriteFolder(const CFavoriteFolder &Src);

		CFavoriteFolder &operator=(const CFavoriteFolder &Src);

		CFavoriteFolder *Duplicate() const override;
		void Clear();
		size_t GetItemCount() const;
		size_t GetSubItemCount() const;
		CFavoriteItem *GetItem(size_t Index);
		const CFavoriteItem *GetItem(size_t Index) const;
		bool AddItem(CFavoriteItem *pItem);
		bool AddItem(size_t Pos, CFavoriteItem *pItem);
		bool DeleteItem(size_t Index);
		CFavoriteItem *RemoveItem(size_t Index);
		bool MoveItem(size_t From, size_t To);
		CFavoriteFolder *FindSubFolder(LPCTSTR pszName);

	private:
		std::vector<std::unique_ptr<CFavoriteItem>> m_Children;
	};

	class CFavoriteChannel
		: public CFavoriteItem
	{
	public:
		CFavoriteChannel(const CChannelInfo &ChannelInfo);

		CFavoriteChannel *Duplicate() const override;
		LPCTSTR GetBonDriverFileName() const { return m_BonDriverFileName.c_str(); }
		bool SetBonDriverFileName(LPCTSTR pszFileName);
		const CChannelInfo &GetChannelInfo() const { return m_ChannelInfo; }
		bool SetChannelInfo(const CChannelInfo &ChannelInfo);
		bool GetForceBonDriverChange() const { return m_fForceBonDriverChange; }
		void SetForceBonDriverChange(bool fForce) { m_fForceBonDriverChange = fForce; }

	private:
		String m_BonDriverFileName;
		CChannelInfo m_ChannelInfo;
		bool m_fForceBonDriverChange = false;
	};

	class CFavoriteItemEnumerator
	{
	public:
		bool EnumItems(CFavoriteFolder &Folder);

	protected:
		virtual bool ChannelItem(CFavoriteFolder &Folder, CFavoriteChannel &Channel) { return true; }
		virtual bool FolderItem(CFavoriteFolder &Folder) { return true; }
	};

	class CFavoritesMenu
	{
	public:
		enum class CreateFlag : unsigned int {
			None          = 0x0000U,
			ShowEventInfo = 0x0001U,
			ShowLogo      = 0x0002U,
			ShowToolTip   = 0x0004U,
			Shared        = 0x1000U,
			TVTEST_ENUM_FLAGS_TRAILER
		};

		CFavoritesMenu() = default;
		~CFavoritesMenu();

		CFavoritesMenu(const CFavoritesMenu &) = delete;
		CFavoritesMenu &operator=(const CFavoritesMenu &) = delete;

		bool Create(
			const CFavoriteFolder *pFolder, UINT Command,
			HMENU hmenu, HWND hwnd, CreateFlag Flags, int DPI = 0);
		void Destroy();
		bool Show(UINT Flags, int x, int y);
		bool OnMeasureItem(HWND hwnd, WPARAM wParam, LPARAM lParam);
		bool OnDrawItem(HWND hwnd, WPARAM wParam, LPARAM lParam);
		bool OnMenuSelect(HWND hwnd, WPARAM wParam, LPARAM lParam);
		bool OnUninitMenuPopup(HWND hwnd, WPARAM wParam, LPARAM lParam);

	private:
		class ABSTRACT_CLASS(CMenuItem)
		{
		public:
			virtual ~CMenuItem() = default;
		};

		class CFolderItem;
		class CChannelItem;

		CreateFlag m_Flags = CreateFlag::None;
		HWND m_hwnd = nullptr;
		HMENU m_hmenu = nullptr;
		UINT m_FirstCommand = 0;
		UINT m_LastCommand = 0;
		DrawUtil::CFont m_Font;
		int m_ItemHeight = 0;
		int m_TextHeight = 0;
		int m_TextWidth = 0;
		int m_IconWidth = 16;
		int m_IconHeight = 16;
		CMenuPainter m_MenuPainter;
		MARGINS m_Margins{};
		int m_MenuLogoMargin = 3;
		CTooltip m_Tooltip;
		LibISDB::DateTime m_BaseTime;
		std::vector<std::unique_ptr<CMenuItem>> m_ItemList;
		HIMAGELIST m_himlIcons = nullptr;
		CChannelMenuLogo m_Logo;

		void SetFolderMenu(HMENU hmenu, int MenuPos, HDC hdc, UINT *pCommand, const CFavoriteFolder *pFolder);
		int GetEventText(const LibISDB::EventInfo *pEventInfo, LPTSTR pszText, int MaxLength) const;
		void CreateFont(HDC hdc);
		static void GetBaseTime(LibISDB::DateTime *pTime);
	};

	class COrganizeFavoritesDialog
		: public CResizableDialog
	{
	public:
		COrganizeFavoritesDialog(CFavoritesManager *pManager);
		~COrganizeFavoritesDialog();

		bool Show(HWND hwndOwner) override;

	private:
		INT_PTR DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

		void InsertTreeItems(HWND hwndTree, HTREEITEM hParent, const CFavoriteFolder *pFolder);
		HTREEITEM CopyTreeItems(HWND hwndTree, HTREEITEM hSrcItem, HTREEITEM hParent, HTREEITEM hInsertAfter);
		void GetTreeItems(HWND hwndTree, HTREEITEM hParent, CFavoriteFolder *pFolder);
		CFavoriteItem *GetItem(HWND hwndTree, HTREEITEM hItem);

		static LRESULT CALLBACK EditHookProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

		CFavoritesManager *m_pManager;

		int m_IconWidth = 0;
		int m_IconHeight = 0;
		bool m_fItemDragging = false;
		HIMAGELIST m_himlDrag = nullptr;
		HTREEITEM m_hDraggingItem = nullptr;
		HTREEITEM m_hDropTargetItem = nullptr;
		bool m_fDropToFolder = false;
		bool m_fDropInsertAfter = false;
		WNDPROC m_pOldEditProc = nullptr;
	};

	class CFavoritesManager
	{
	public:
		struct ChannelInfo
		{
			CChannelInfo Channel;
			String BonDriverFileName;
			bool fForceBonDriverChange;
		};

		CFavoriteFolder &GetRootFolder() { return m_RootFolder; }
		const CFavoriteFolder &GetRootFolder() const { return m_RootFolder; }
		bool AddChannel(const CChannelInfo *pChannelInfo, LPCTSTR pszBonDriverFileName);
		bool SetMenu(HMENU hmenu);
		bool GetChannelByCommand(int Command, ChannelInfo *pInfo) const;
		bool Load(LPCTSTR pszFileName);
		bool Save(LPCTSTR pszFileName) const;
		bool GetModified() const { return m_fModified; }
		void SetModified(bool fModified) { m_fModified = fModified; }
		bool ShowOrganizeDialog(HWND hwndOwner);
		bool GetOrganizeDialogPos(CBasicDialog::Position *pPos) const;
		bool SetOrganizeDialogPos(const CBasicDialog::Position &Pos);
		bool IsOrganizeDialogPosSet() const;

	private:
		void SetFolderMenu(HMENU hmenu, int MenuPos, int *pCommand, const CFavoriteFolder *pFolder) const;
		bool GetChannelByCommandSub(
			const CFavoriteFolder *pFolder,
			int Command, int *pBaseCommand, ChannelInfo *pInfo) const;
		void SaveFolder(const CFavoriteFolder *pFolder, const String &Path, String *pBuffer) const;

		CFavoriteFolder m_RootFolder;
		bool m_fModified = false;
		COrganizeFavoritesDialog m_OrganizeDialog{this};
	};

}


#endif
