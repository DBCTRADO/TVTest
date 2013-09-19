#ifndef TVTEST_FAVORITES_H
#define TVTEST_FAVORITES_H


#include "ChannelList.h"
#include "Menu.h"
#include "Dialog.h"


namespace TVTest
{

	class ABSTRACT_CLASS(CFavoriteItem)
	{
	public:
		enum ItemType
		{
			ITEM_FOLDER,
			ITEM_CHANNEL
		};

		CFavoriteItem(ItemType Type) : m_Type(Type) {}
		virtual ~CFavoriteItem() {}
		ItemType GetType() const { return m_Type; }
		virtual CFavoriteItem *Duplicate() const = 0;
		LPCTSTR GetName() const { return m_Name.c_str(); }
		bool SetName(LPCTSTR pszName);

	protected:
		ItemType m_Type;
		String m_Name;
	};

	class CFavoriteFolder : public CFavoriteItem
	{
	public:
		CFavoriteFolder();
		CFavoriteFolder(const CFavoriteFolder &Src);
		~CFavoriteFolder();
		CFavoriteFolder &operator=(const CFavoriteFolder &Src);
		CFavoriteFolder *Duplicate() const override;
		void Clear();
		size_t GetItemCount() const;
		size_t GetSubItemCount() const;
		CFavoriteItem *GetItem(size_t Index);
		const CFavoriteItem *GetItem(size_t Index) const;
		bool AddItem(CFavoriteItem *pItem);
		bool AddItem(size_t Pos,CFavoriteItem *pItem);
		bool DeleteItem(size_t Index);
		CFavoriteItem *RemoveItem(size_t Index);
		bool MoveItem(size_t From,size_t To);
		CFavoriteFolder *FindSubFolder(LPCTSTR pszName);

	private:
		std::vector<CFavoriteItem*> m_Children;
	};

	class CFavoriteChannel : public CFavoriteItem
	{
	public:
		CFavoriteChannel(const CChannelInfo &ChannelInfo);
		~CFavoriteChannel();
		CFavoriteChannel *Duplicate() const override;
		LPCTSTR GetBonDriverFileName() const { return m_BonDriverFileName.c_str(); }
		bool SetBonDriverFileName(LPCTSTR pszFileName);
		const CChannelInfo &GetChannelInfo() const { return m_ChannelInfo; }
		bool SetChannelInfo(const CChannelInfo &ChannelInfo);
		bool GetForceBonDriverChange() const { return m_fForceBonDriverChange; }
		void SetForceBonDriverChange(bool fForce) { m_fForceBonDriverChange=fForce; }

	private:
		String m_BonDriverFileName;
		CChannelInfo m_ChannelInfo;
		bool m_fForceBonDriverChange;
	};

	class CFavoriteItemEnumerator
	{
	public:
		bool EnumItems(CFavoriteFolder &Folder);

	protected:
		virtual bool ChannelItem(CFavoriteFolder &Folder,CFavoriteChannel &Channel) { return true; }
		virtual bool FolderItem(CFavoriteFolder &Folder) { return true; }
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

		CFavoritesManager();
		~CFavoritesManager();
		CFavoriteFolder &GetRootFolder() { return m_RootFolder; }
		const CFavoriteFolder &GetRootFolder() const { return m_RootFolder; }
		bool AddChannel(const CChannelInfo *pChannelInfo,LPCTSTR pszBonDriverFileName);
		bool SetMenu(HMENU hmenu);
		bool GetChannelByCommand(int Command,ChannelInfo *pInfo) const;
		bool Load(LPCTSTR pszFileName);
		bool Save(LPCTSTR pszFileName) const;
		bool GetModified() const { return m_fModified; }
		void SetModified(bool fModified) { m_fModified=fModified; }

	private:
		void SetFolderMenu(HMENU hmenu,int MenuPos,int *pCommand,const CFavoriteFolder *pFolder) const;
		bool GetChannelByCommandSub(const CFavoriteFolder *pFolder,
									int Command,int *pBaseCommand,ChannelInfo *pInfo) const;
		void SaveFolder(const CFavoriteFolder *pFolder,const String &Path,String *pBuffer) const;

		CFavoriteFolder m_RootFolder;
		bool m_fModified;
	};

	class CFavoritesMenu
	{
	public:
		enum {
			FLAG_SHOWEVENTINFO	=0x0001,
			FLAG_SHOWLOGO		=0x0002,
			FLAG_SHOWTOOLTIP	=0x0004,
			FLAG_SHARED			=0x1000
		};

		CFavoritesMenu();
		~CFavoritesMenu();
		bool Create(const CFavoriteFolder *pFolder,UINT Command,
					HMENU hmenu,HWND hwnd,unsigned int Flags);
		void Destroy();
		bool Show(UINT Flags,int x,int y);
		bool OnMeasureItem(HWND hwnd,WPARAM wParam,LPARAM lParam);
		bool OnDrawItem(HWND hwnd,WPARAM wParam,LPARAM lParam);
		bool OnMenuSelect(HWND hwnd,WPARAM wParam,LPARAM lParam);
		bool OnUninitMenuPopup(HWND hwnd,WPARAM wParam,LPARAM lParam);

	private:
		class CMenuItem;
		class CFolderItem;
		class CChannelItem;

		unsigned int m_Flags;
		HWND m_hwnd;
		HMENU m_hmenu;
		UINT m_FirstCommand;
		UINT m_LastCommand;
		DrawUtil::CFont m_Font;
		int m_ItemHeight;
		int m_TextHeight;
		int m_TextWidth;
		int m_LogoWidth;
		int m_LogoHeight;
		int m_IconWidth;
		int m_IconHeight;
		CMenuPainter m_MenuPainter;
		MARGINS m_Margins;
		int m_MenuLogoMargin;
		CTooltip m_Tooltip;
		SYSTEMTIME m_BaseTime;
		std::vector<CMenuItem*> m_ItemList;
		HIMAGELIST m_himlIcons;
		DrawUtil::CBitmap m_LogoFrameImage;

		void SetFolderMenu(HMENU hmenu,int MenuPos,HDC hdc,UINT *pCommand,const CFavoriteFolder *pFolder);
		int GetEventText(const CEventInfoData *pEventInfo,
						 LPTSTR pszText,int MaxLength) const;
		void CreateFont(HDC hdc);
		static void GetBaseTime(SYSTEMTIME *pTime);
	};

	class COrganizeFavoritesDialog : public CResizableDialog
	{
	public:
		COrganizeFavoritesDialog(CFavoritesManager *pManager);
		~COrganizeFavoritesDialog();
		bool Show(HWND hwndOwner) override;

	private:
		INT_PTR DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam) override;

		void InsertTreeItems(HWND hwndTree,HTREEITEM hParent,const CFavoriteFolder *pFolder);
		HTREEITEM CopyTreeItems(HWND hwndTree,HTREEITEM hSrcItem,HTREEITEM hParent,HTREEITEM hInsertAfter);
		void GetTreeItems(HWND hwndTree,HTREEITEM hParent,CFavoriteFolder *pFolder);
		CFavoriteItem *GetItem(HWND hwndTree,HTREEITEM hItem);

		static LRESULT CALLBACK EditHookProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);

		CFavoritesManager *m_pManager;

		bool m_fItemDragging;
		HIMAGELIST m_himlDrag;
		HTREEITEM m_hDraggingItem;
		HTREEITEM m_hDropTargetItem;
		bool m_fDropToFolder;
		bool m_fDropInsertAfter;
		WNDPROC m_pOldEditProc;
	};

}


#endif
