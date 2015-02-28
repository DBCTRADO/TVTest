#include "stdafx.h"
#include "TVTest.h"
#include "Favorites.h"
#include "AppMain.h"
#include "DialogUtil.h"
#include "LogoManager.h"
#include "DriverManager.h"
#include "resource.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


namespace TVTest
{

	enum
	{
		FAVORITES_ICON_EMPTY,
		FAVORITES_ICON_FOLDER,
		FAVORITES_ICON_ADD,
		FAVORITES_ICON_ORGANIZE
	};


	bool CFavoriteItem::SetName(LPCTSTR pszName)
	{
		if (pszName==nullptr)
			return false;

		m_Name=pszName;

		return true;
	}


	CFavoriteFolder::CFavoriteFolder()
		: CFavoriteItem(ITEM_FOLDER)
	{
	}

	CFavoriteFolder::CFavoriteFolder(const CFavoriteFolder &Src)
		: CFavoriteItem(ITEM_FOLDER)
	{
		*this=Src;
	}

	CFavoriteFolder::~CFavoriteFolder()
	{
		Clear();
	}

	CFavoriteFolder &CFavoriteFolder::operator=(const CFavoriteFolder &Src)
	{
		if (&Src!=this) {
			Clear();
			m_Name=Src.m_Name;
			if (!Src.m_Children.empty()) {
				m_Children.reserve(Src.m_Children.size());
				for (auto i=Src.m_Children.begin();i!=Src.m_Children.end();i++)
					m_Children.push_back((*i)->Duplicate());
			}
		}

		return *this;
	}

	CFavoriteFolder *CFavoriteFolder::Duplicate() const
	{
		return new CFavoriteFolder(*this);
	}

	void CFavoriteFolder::Clear()
	{
		m_Name.clear();
		for (auto i=m_Children.begin();i!=m_Children.end();i++)
			delete *i;
		m_Children.clear();
	}

	size_t CFavoriteFolder::GetItemCount() const
	{
		return m_Children.size();
	}

	size_t CFavoriteFolder::GetSubItemCount() const
	{
		size_t Count=m_Children.size();

		for (auto i=m_Children.begin();i!=m_Children.end();i++) {
			if ((*i)->GetType()==ITEM_FOLDER) {
				const CFavoriteFolder *pFolder=dynamic_cast<CFavoriteFolder*>(*i);
				if (pFolder!=nullptr)
					Count+=pFolder->GetSubItemCount();
			}
		}

		return Count;
	}

	CFavoriteItem *CFavoriteFolder::GetItem(size_t Index)
	{
		if (Index>=m_Children.size())
			return nullptr;

		return m_Children[Index];
	}

	const CFavoriteItem *CFavoriteFolder::GetItem(size_t Index) const
	{
		if (Index>=m_Children.size())
			return nullptr;

		return m_Children[Index];
	}

	bool CFavoriteFolder::AddItem(CFavoriteItem *pItem)
	{
		if (pItem==nullptr)
			return false;

		m_Children.push_back(pItem);

		return true;
	}

	bool CFavoriteFolder::AddItem(size_t Pos,CFavoriteItem *pItem)
	{
		if (Pos>m_Children.size())
			return false;

		auto i=m_Children.begin();
		std::advance(i,Pos);
		m_Children.insert(i,pItem);

		return true;
	}

	bool CFavoriteFolder::DeleteItem(size_t Index)
	{
		if (Index>=m_Children.size())
			return false;

		auto i=m_Children.begin();
		std::advance(i,Index);
		delete *i;
		m_Children.erase(i);

		return true;
	}

	CFavoriteItem *CFavoriteFolder::RemoveItem(size_t Index)
	{
		if (Index>=m_Children.size())
			return nullptr;

		auto i=m_Children.begin();
		std::advance(i,Index);
		CFavoriteItem *pItem=*i;
		m_Children.erase(i);

		return pItem;
	}

	bool CFavoriteFolder::MoveItem(size_t From,size_t To)
	{
		if (From>=m_Children.size() || To>=m_Children.size())
			return false;

		if (From!=To) {
			auto i=m_Children.begin();
			std::advance(i,From);
			CFavoriteItem *pItem=*i;
			m_Children.erase(i);
			i=m_Children.begin();
			std::advance(i,To);
			m_Children.insert(i,pItem);
		}

		return true;
	}
	
	CFavoriteFolder *CFavoriteFolder::FindSubFolder(LPCTSTR pszName)
	{
		if (pszName==nullptr)
			return false;

		for (auto i=m_Children.begin();i!=m_Children.end();i++) {
			if ((*i)->GetType()==ITEM_FOLDER) {
				CFavoriteFolder *pFolder=dynamic_cast<CFavoriteFolder*>(*i);
				if (pFolder!=nullptr && pFolder->m_Name.compare(pszName)==0)
					return pFolder;
			}
		}

		return nullptr;
	}


	CFavoriteChannel::CFavoriteChannel(const CChannelInfo &ChannelInfo)
		: CFavoriteItem(ITEM_CHANNEL)
		, m_ChannelInfo(ChannelInfo)
		, m_fForceBonDriverChange(false)
	{
		m_Name=ChannelInfo.GetName();
	}

	CFavoriteChannel::~CFavoriteChannel()
	{
	}

	CFavoriteChannel *CFavoriteChannel::Duplicate() const
	{
		return new CFavoriteChannel(*this);
	}

	bool CFavoriteChannel::SetBonDriverFileName(LPCTSTR pszFileName)
	{
		if (pszFileName!=nullptr)
			m_BonDriverFileName=pszFileName;
		else
			m_BonDriverFileName.clear();

		return true;
	}

	bool CFavoriteChannel::SetChannelInfo(const CChannelInfo &ChannelInfo)
	{
		m_ChannelInfo=ChannelInfo;

		return true;
	}


	bool CFavoriteItemEnumerator::EnumItems(CFavoriteFolder &Folder)
	{
		const size_t ItemCount=Folder.GetItemCount();

		for (size_t i=0;i<ItemCount;i++) {
			CFavoriteItem *pItem=Folder.GetItem(i);

			switch (pItem->GetType()) {
			case CFavoriteItem::ITEM_FOLDER:
				{
					CFavoriteFolder *pFolder=dynamic_cast<CFavoriteFolder*>(pItem);

					if (pFolder!=nullptr) {
						if (!FolderItem(*pFolder))
							return false;
						if (!EnumItems(*pFolder))
							return false;
					}
				}
				break;

			case CFavoriteItem::ITEM_CHANNEL:
				{
					CFavoriteChannel *pChannel=dynamic_cast<CFavoriteChannel*>(pItem);

					if (pChannel!=nullptr) {
						if (!ChannelItem(Folder,*pChannel))
							return false;
					}
				}
				break;
			}
		}

		return true;
	}


#define CHANNEL_FLAG_FORCEBONDRIVERCHANGE	0x0001U

	CFavoritesManager::CFavoritesManager()
		: m_fModified(false)
	{
	}

	CFavoritesManager::~CFavoritesManager()
	{
	}

	bool CFavoritesManager::AddChannel(const CChannelInfo *pChannelInfo,LPCTSTR pszBonDriverFileName)
	{
		CFavoriteChannel *pItem=new CFavoriteChannel(*pChannelInfo);
		pItem->SetBonDriverFileName(pszBonDriverFileName);

		if (!m_RootFolder.AddItem(pItem)) {
			delete pItem;
			return false;
		}

		m_fModified=true;

		return true;
	}

	bool CFavoritesManager::SetMenu(HMENU hmenu)
	{
		if (hmenu==nullptr)
			return false;

		int ItemCount=::GetMenuItemCount(hmenu);
		for (;ItemCount>3;ItemCount--)
			::DeleteMenu(hmenu,0,MF_BYPOSITION);

		if (m_RootFolder.GetItemCount()>0) {
			int Command=CM_FAVORITECHANNEL_FIRST;
			SetFolderMenu(hmenu,0,&Command,&m_RootFolder);
		} else {
			::InsertMenu(hmenu,0,MF_BYPOSITION | MF_STRING | MF_GRAYED,0,TEXT("�Ȃ�"));
		}

		return true;
	}

	void CFavoritesManager::SetFolderMenu(HMENU hmenu,int MenuPos,int *pCommand,const CFavoriteFolder *pFolder) const
	{
		for (size_t i=0;i<pFolder->GetItemCount();i++) {
			const CFavoriteItem *pItem=pFolder->GetItem(i);

			switch (pItem->GetType()) {
			case CFavoriteItem::ITEM_FOLDER:
				{
					const CFavoriteFolder *pSubFolder=
						dynamic_cast<const CFavoriteFolder*>(pItem);

					if (pSubFolder!=nullptr) {
						HMENU hmenuSub=::CreatePopupMenu();

						::InsertMenu(hmenu,MenuPos,MF_BYPOSITION | MF_POPUP | MF_ENABLED,
									 reinterpret_cast<UINT_PTR>(hmenuSub),
									 // TODO: & -> &&
									 pSubFolder->GetName());
						SetFolderMenu(hmenuSub,0,pCommand,pSubFolder);
						MenuPos++;
					}
				}
				break;

			case CFavoriteItem::ITEM_CHANNEL:
				{
					const CFavoriteChannel *pChannel=
						dynamic_cast<const CFavoriteChannel*>(pItem);

					if (pChannel!=nullptr) {
						::InsertMenu(hmenu,MenuPos,MF_BYPOSITION | MF_STRING | MF_ENABLED,
									 // TODO: & -> &&
									 *pCommand,pChannel->GetName());
						(*pCommand)++;
						MenuPos++;
					}
				}
				break;
			}
		}
	}

	bool CFavoritesManager::GetChannelByCommand(int Command,ChannelInfo *pInfo) const
	{
		if (Command<CM_FAVORITECHANNEL_FIRST
				|| Command>CM_FAVORITECHANNEL_LAST
				|| pInfo==nullptr)
			return false;

		int BaseCommand=CM_FAVORITECHANNEL_FIRST;
		return GetChannelByCommandSub(&m_RootFolder,Command,&BaseCommand,pInfo);
	}

	bool CFavoritesManager::GetChannelByCommandSub(
		const CFavoriteFolder *pFolder,int Command,int *pBaseCommand,ChannelInfo *pInfo) const
	{
		for (size_t i=0;i<pFolder->GetItemCount();i++) {
			const CFavoriteItem *pItem=pFolder->GetItem(i);

			switch (pItem->GetType()) {
			case CFavoriteItem::ITEM_FOLDER:
				{
					const CFavoriteFolder *pSubFolder=dynamic_cast<const CFavoriteFolder*>(pItem);
					if (pSubFolder!=nullptr) {
						if (GetChannelByCommandSub(pSubFolder,Command,pBaseCommand,pInfo))
							return true;
					}
				}
				break;

			case CFavoriteItem::ITEM_CHANNEL:
				{
					const CFavoriteChannel *pChannel=
						dynamic_cast<const CFavoriteChannel*>(pItem);

					if (pChannel!=nullptr) {
						if (Command==*pBaseCommand) {
							pInfo->Channel=pChannel->GetChannelInfo();
							pInfo->BonDriverFileName=pChannel->GetBonDriverFileName();
							pInfo->fForceBonDriverChange=pChannel->GetForceBonDriverChange();
							return true;
						}
						(*pBaseCommand)++;
					}
				}
				break;
			}
		}

		return false;
	}

	bool CFavoritesManager::Load(LPCTSTR pszFileName)
	{
		if (IsStringEmpty(pszFileName))
			return false;

		HANDLE hFile=::CreateFile(pszFileName,GENERIC_READ,FILE_SHARE_READ,nullptr,
								  OPEN_EXISTING,0,nullptr);
		if (hFile==INVALID_HANDLE_VALUE)
			return false;

		BYTE *pBuffer=nullptr;
		LPWSTR pszBuffer;

		try {
			LARGE_INTEGER FileSize;
			if (!::GetFileSizeEx(hFile,&FileSize)
					|| FileSize.QuadPart<=4 || FileSize.QuadPart>1024*1024)
				throw __LINE__;

			pBuffer=new BYTE[FileSize.LowPart+3];
			DWORD Read;
			if (!::ReadFile(hFile,pBuffer,FileSize.LowPart,&Read,nullptr)
					|| Read!=FileSize.LowPart)
				throw __LINE__;
			pBuffer[FileSize.LowPart+0]=0;
			pBuffer[FileSize.LowPart+1]=0;
			pBuffer[FileSize.LowPart+2]=0;

			if (pBuffer[0]==0xFF && pBuffer[1]==0xFE) {
				pszBuffer=(LPWSTR)(pBuffer+2);
			} else {
				BYTE *pSrc=pBuffer;
				int Length=::MultiByteToWideChar(CP_ACP,0,(char*)pSrc,FileSize.LowPart,nullptr,0);
				pBuffer=new BYTE[(Length+1)*sizeof(WCHAR)];
				pszBuffer=(LPWSTR)pBuffer;
				::MultiByteToWideChar(CP_ACP,0,(char*)pSrc,FileSize.LowPart,pszBuffer,Length);
				pszBuffer[Length]=L'\0';
				delete [] pSrc;
			}
		} catch (...) {
			delete [] pBuffer;
			::CloseHandle(hFile);
			return false;
		}

		::CloseHandle(hFile);

		LPCTSTR p=pszBuffer;

		while (*p!=_T('\0')) {
			String::size_type Length;
			for (Length=0;p[Length]!=_T('\0') && p[Length]!=_T('\r') && p[Length]!=_T('\n');Length++);
			if (Length>0) {
				String Line;
				Line.assign(p,Length);
				std::vector<String> Items;
				StringUtility::Split(Line,TEXT(","),&Items);
				if (Items.size()>=1) {
					std::vector<String> PathItems;
					String Temp;
					StringUtility::Split(Items[0],TEXT("\\"),&PathItems);
					for (size_t i=0;i<PathItems.size();i++) {
						if (!PathItems[i].empty()) {
							StringUtility::Decode(PathItems[i].c_str(),&Temp);
							PathItems[i]=Temp;
						}
					}
					CFavoriteFolder *pFolder=&m_RootFolder;
					for (size_t i=0;i+1<PathItems.size();i++) {
						CFavoriteFolder *p=pFolder->FindSubFolder(PathItems[i].c_str());
						if (p==nullptr) {
							p=new CFavoriteFolder;
							p->SetName(PathItems[i].c_str());
							pFolder->AddItem(p);
						}
						pFolder=p;
					}
					if (Items.size()>=8) {
						CChannelInfo ChannelInfo(::wcstol(Items[2].c_str(),nullptr,0),
												 ::wcstol(Items[3].c_str(),nullptr,0),
												 ::wcstol(Items[4].c_str(),nullptr,0),
												 PathItems[PathItems.size()-1].c_str());
						ChannelInfo.SetServiceID((WORD)::wcstol(Items[5].c_str(),nullptr,0));
						ChannelInfo.SetNetworkID((WORD)::wcstol(Items[6].c_str(),nullptr,0));
						ChannelInfo.SetTransportStreamID((WORD)::wcstol(Items[7].c_str(),nullptr,0));

						CFavoriteChannel *pChannel=new CFavoriteChannel(ChannelInfo);
						pChannel->SetName(PathItems[PathItems.size()-1].c_str());
						pChannel->SetBonDriverFileName(Items[1].c_str());

						if (Items.size()>=9) {
							unsigned long Flags=::wcstoul(Items[8].c_str(),nullptr,0);
							pChannel->SetForceBonDriverChange((Flags&CHANNEL_FLAG_FORCEBONDRIVERCHANGE)!=0);
						}

						pFolder->AddItem(pChannel);
					}
				}

				p+=Length;
			}

			while (*p==_T('\r') || *p==_T('\n'))
				p++;
		}

		delete [] pBuffer;

		return true;
	}

	bool CFavoritesManager::Save(LPCTSTR pszFileName) const
	{
		if (IsStringEmpty(pszFileName))
			return false;

		String Buffer;

		SaveFolder(&m_RootFolder,String(),&Buffer);

		HANDLE hFile=::CreateFile(pszFileName,GENERIC_WRITE,0,nullptr,
								  CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
		if (hFile==INVALID_HANDLE_VALUE)
			return false;

		DWORD Write;
		static const WORD BOM=0xFEFF;
		const DWORD Size=(DWORD)(Buffer.length()*sizeof(String::value_type));
		if (!::WriteFile(hFile,&BOM,sizeof(BOM),&Write,nullptr)
				|| Write!=sizeof(BOM)
				|| !::WriteFile(hFile,Buffer.data(),Size,&Write,nullptr)
				|| Write!=Size) {
			::CloseHandle(hFile);
			return false;
		}

		::CloseHandle(hFile);

		return true;
	}

	void CFavoritesManager::SaveFolder(const CFavoriteFolder *pFolder,const String &Path,String *pBuffer) const
	{
		for (size_t i=0;i<pFolder->GetItemCount();i++) {
			const CFavoriteItem *pItem=pFolder->GetItem(i);

			switch (pItem->GetType()) {
			case CFavoriteItem::ITEM_FOLDER:
				{
					const CFavoriteFolder *pFolder=dynamic_cast<const CFavoriteFolder*>(pItem);
					if (pFolder!=nullptr) {
						String FolderPath(Path),Name;
						StringUtility::Encode(pFolder->GetName(),&Name);
						FolderPath+=Name;
						FolderPath+=TEXT("\\");
						if (pFolder->GetItemCount()>0) {
							SaveFolder(pFolder,FolderPath,pBuffer);
						} else {
							*pBuffer+=FolderPath;
							*pBuffer+=TEXT("\r\n");
						}
					}
				}
				break;

			case CFavoriteItem::ITEM_CHANNEL:
				{
					const CFavoriteChannel *pChannel=dynamic_cast<const CFavoriteChannel*>(pItem);
					if (pChannel!=nullptr) {
						const CChannelInfo &ChannelInfo=pChannel->GetChannelInfo();
						String Name,ChannelSpec;
						StringUtility::Encode(pChannel->GetName(),&Name);
						unsigned int Flags=0;
						if (pChannel->GetForceBonDriverChange())
							Flags|=CHANNEL_FLAG_FORCEBONDRIVERCHANGE;
						StringUtility::Format(ChannelSpec,TEXT(",%s,%d,%d,%d,%d,%d,%d,%u"),
											  pChannel->GetBonDriverFileName(),
											  ChannelInfo.GetSpace(),
											  ChannelInfo.GetChannelIndex(),
											  ChannelInfo.GetChannelNo(),
											  ChannelInfo.GetServiceID(),
											  ChannelInfo.GetNetworkID(),
											  ChannelInfo.GetTransportStreamID(),
											  Flags);
						*pBuffer+=Path;
						*pBuffer+=Name;
						*pBuffer+=ChannelSpec;
						*pBuffer+=TEXT("\r\n");
					}
				}
				break;
			}
		}

	}


	class ABSTRACT_CLASS(CFavoritesMenu::CMenuItem)
	{
	public:
		virtual ~CMenuItem() {}
	};

	class CFavoritesMenu::CFolderItem : public CFavoritesMenu::CMenuItem
	{
		const CFavoriteFolder *m_pFolder;
		int m_TextWidth;

	public:
		CFolderItem(const CFavoriteFolder *pFolder);
		LPCTSTR GetName() const { return m_pFolder->GetName(); }
		int GetTextWidth() const { return m_TextWidth; }
		void SetTextWidth(int Width) { m_TextWidth=Width; }
	};

	CFavoritesMenu::CFolderItem::CFolderItem(const CFavoriteFolder *pFolder)
		: m_pFolder(pFolder)
		, m_TextWidth(0)
	{
	}

	class CFavoritesMenu::CChannelItem : public CFavoritesMenu::CMenuItem
	{
		const CFavoriteChannel *m_pChannel;
		struct Event {
			bool fValid;
			CEventInfoData EventInfo;
			Event() : fValid(false) {}
		};
		Event m_EventList[2];
		int m_NameWidth;
		int m_EventWidth;

	public:
		CChannelItem(const CFavoriteChannel *pChannel);
		const CEventInfoData *GetEventInfo(CEpgProgramList *pProgramList,
										   int Index,const SYSTEMTIME *pCurTime=NULL);
		const CEventInfoData *GetEventInfo(int Index) const;
		LPCTSTR GetName() const { return m_pChannel->GetName(); }
		const CChannelInfo &GetChannelInfo() const { return m_pChannel->GetChannelInfo(); }
		int GetNameWidth() const { return m_NameWidth; }
		void SetNameWidth(int Width) { m_NameWidth=Width; }
		int GetEventWidth() const { return m_EventWidth; }
		void SetEventWidth(int Width) { m_EventWidth=Width; }
	};

	CFavoritesMenu::CChannelItem::CChannelItem(const CFavoriteChannel *pChannel)
		: m_pChannel(pChannel)
		, m_NameWidth(0)
		, m_EventWidth(0)
	{
	}

	const CEventInfoData *CFavoritesMenu::CChannelItem::GetEventInfo(
		CEpgProgramList *pProgramList,int Index,const SYSTEMTIME *pCurTime)
	{
		if (Index<0 || Index>=lengthof(m_EventList)
				|| (Index>0 && !m_EventList[Index-1].fValid)
				|| m_pChannel->GetChannelInfo().GetServiceID()==0)
			return NULL;

		if (!m_EventList[Index].fValid) {
			SYSTEMTIME st;

			if (Index==0) {
				if (pCurTime!=NULL)
					st=*pCurTime;
				else
					GetCurrentEpgTime(&st);
			} else {
				if (!m_EventList[Index-1].EventInfo.GetEndTime(&st))
					return NULL;
			}
			const CChannelInfo &ChannelInfo=GetChannelInfo();
			if (!pProgramList->GetEventInfo(ChannelInfo.GetNetworkID(),
											ChannelInfo.GetTransportStreamID(),
											ChannelInfo.GetServiceID(),
											&st,&m_EventList[Index].EventInfo))
				return NULL;
			m_EventList[Index].fValid=true;
		}

		return &m_EventList[Index].EventInfo;
	}

	const CEventInfoData *CFavoritesMenu::CChannelItem::GetEventInfo(int Index) const
	{
		if (Index<0 || Index>=lengthof(m_EventList) || !m_EventList[Index].fValid)
			return NULL;
		return &m_EventList[Index].EventInfo;
	}


	CFavoritesMenu::CFavoritesMenu()
		: m_Flags(0)
		, m_hwnd(NULL)
		, m_hmenu(NULL)
		, m_TextHeight(0)
		, m_TextWidth(0)
		, m_LogoWidth(26)
		, m_LogoHeight(16)
		, m_IconWidth(16)
		, m_IconHeight(16)
		, m_MenuLogoMargin(3)
		, m_himlIcons(NULL)
	{
	}

	CFavoritesMenu::~CFavoritesMenu()
	{
		Destroy();
	}

	bool CFavoritesMenu::Create(const CFavoriteFolder *pFolder,UINT Command,
								HMENU hmenu,HWND hwnd,unsigned int Flags)
	{
		if (pFolder==nullptr)
			return false;

		Destroy();

		m_FirstCommand=Command;
		m_Flags=Flags;
		m_hwnd=hwnd;

		m_MenuPainter.Initialize(hwnd);
		m_MenuPainter.GetItemMargins(&m_Margins);
		if (m_Margins.cxLeftWidth<2)
			m_Margins.cxLeftWidth=2;
		if (m_Margins.cxRightWidth<2)
			m_Margins.cxRightWidth=2;

		HDC hdc=::GetDC(hwnd);

		CreateFont(hdc);
		HFONT hfontOld=DrawUtil::SelectObject(hdc,m_Font);

		m_ItemHeight=m_TextHeight;
		if ((Flags&FLAG_SHOWLOGO)!=0) {
			int Height=max(m_LogoHeight,m_IconHeight);
			if (Height>m_ItemHeight)
				m_ItemHeight=Height;
		}
		m_ItemHeight+=m_Margins.cyTopHeight+m_Margins.cyBottomHeight;
		m_TextWidth=0;

		GetBaseTime(&m_BaseTime);
		if (hmenu==NULL) {
			m_hmenu=::CreatePopupMenu();
		} else {
			m_hmenu=hmenu;
			m_Flags|=FLAG_SHARED;
			ClearMenu(hmenu);
		}

		HINSTANCE hinstRes=GetAppClass().GetResourceInstance();
		TCHAR szText[64];

		if (pFolder->GetItemCount()>0) {
			SetFolderMenu(m_hmenu,0,hdc,&Command,pFolder);
			m_LastCommand=Command-1;

			int MenuPos=::GetMenuItemCount(m_hmenu);

			MENUITEMINFO mii;
			mii.cbSize=sizeof(MENUITEMINFO);
			mii.fMask=MIIM_FTYPE | MIIM_STATE | MIIM_ID;
			mii.fType=MFT_SEPARATOR;
			mii.fState=MFS_DISABLED;
			mii.wID=0;
			::InsertMenuItem(m_hmenu,MenuPos,TRUE,&mii);
			MenuPos++;

			static const struct {
				UINT Command;
				UINT TextID;
			} MenuItemList[] = {
				{CM_ADDTOFAVORITES,		IDS_MENU_ADDTOFAVORITES},
				{CM_ORGANIZEFAVORITES,	IDS_MENU_ORGANIZEFAVORITES},
			};
			mii.fMask=MIIM_FTYPE | MIIM_STATE | MIIM_ID;
			mii.fType=MFT_OWNERDRAW;
			mii.fState=MFS_ENABLED;

			for (int i=0;i<lengthof(MenuItemList);i++) {
				mii.wID=MenuItemList[i].Command;
				::InsertMenuItem(m_hmenu,MenuPos,TRUE,&mii);
				MenuPos++;

				int Length=::LoadString(hinstRes,MenuItemList[i].TextID,szText,lengthof(szText));
				RECT rc;
				m_MenuPainter.GetItemTextExtent(hdc,0,szText,&rc);
				if (rc.right>m_TextWidth)
					m_TextWidth=rc.right;
			}

			m_himlIcons=::ImageList_LoadImage(hinstRes,MAKEINTRESOURCE(IDB_FAVORITES),
											  16,1,0,IMAGE_BITMAP,LR_CREATEDIBSECTION);
		} else {
			m_LastCommand=Command;

			::AppendMenu(hmenu,MF_STRING | MF_GRAYED,0,TEXT("�Ȃ�"));
			::AppendMenu(hmenu,MF_SEPARATOR,0,nullptr);
			::LoadString(hinstRes,IDS_MENU_ADDTOFAVORITES,szText,lengthof(szText));
			::AppendMenu(hmenu,MF_STRING | MF_ENABLED,CM_ADDTOFAVORITES,szText);
			::LoadString(hinstRes,IDS_MENU_ORGANIZEFAVORITES,szText,lengthof(szText));
			::AppendMenu(hmenu,MF_STRING | MF_ENABLED,CM_ORGANIZEFAVORITES,szText);
		}

		::SelectObject(hdc,hfontOld);
		::ReleaseDC(hwnd,hdc);

		if ((Flags&FLAG_SHOWTOOLTIP)!=0) {
			m_Tooltip.Create(hwnd);
			m_Tooltip.SetMaxWidth(480);
			m_Tooltip.SetPopDelay(30*1000);
			m_Tooltip.AddTrackingTip(1,TEXT(""));
		}

		if ((Flags&FLAG_SHOWLOGO)!=0) {
			if (!m_LogoFrameImage.IsCreated())
				m_LogoFrameImage.Load(hinstRes,MAKEINTRESOURCE(IDB_LOGOFRAME),LR_CREATEDIBSECTION);
		}

		return true;
	}

	void CFavoritesMenu::SetFolderMenu(HMENU hmenu,int MenuPos,HDC hdc,UINT *pCommand,const CFavoriteFolder *pFolder)
	{
		CEpgProgramList &ProgramList=GetAppClass().EpgProgramList;

		int ChannelNameWidth=0;
		RECT rc;

		for (size_t i=0;i<pFolder->GetItemCount();i++) {
			const CFavoriteItem *pItem=pFolder->GetItem(i);

			if (pItem->GetType()==CFavoriteItem::ITEM_CHANNEL) {
				const CFavoriteChannel *pChannel=dynamic_cast<const CFavoriteChannel*>(pItem);

				if (pChannel!=nullptr) {
					LPCTSTR pszChannelName=pChannel->GetName();
					m_MenuPainter.GetItemTextExtent(hdc,0,pszChannelName,&rc,
													DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX);
					if (rc.right>ChannelNameWidth)
						ChannelNameWidth=rc.right;
				}
			}
		}

		MENUITEMINFO mii;
		mii.cbSize=sizeof(MENUITEMINFO);
		mii.fType=MFT_OWNERDRAW;
		mii.fState=MFS_ENABLED;

		for (size_t i=0;i<pFolder->GetItemCount();i++) {
			const CFavoriteItem *pItem=pFolder->GetItem(i);

			switch (pItem->GetType()) {
			case CFavoriteItem::ITEM_FOLDER:
				{
					const CFavoriteFolder *pSubFolder=
						dynamic_cast<const CFavoriteFolder*>(pItem);

					if (pSubFolder!=nullptr) {
						CFolderItem *pMenuItem=new CFolderItem(pSubFolder);
						m_ItemList.push_back(pMenuItem);

						HMENU hmenuSub=::CreatePopupMenu();
						mii.fMask=MIIM_FTYPE | MIIM_STATE | MIIM_ID | MIIM_SUBMENU | MIIM_DATA;
						mii.wID=CM_FAVORITESSUBMENU;
						mii.hSubMenu=hmenuSub;
						mii.dwItemData=reinterpret_cast<ULONG_PTR>(pMenuItem);
						::InsertMenuItem(hmenu,MenuPos,TRUE,&mii);
						SetFolderMenu(hmenuSub,0,hdc,pCommand,pSubFolder);
						MenuPos++;

						LPCTSTR pszName=pSubFolder->GetName();
						m_MenuPainter.GetItemTextExtent(hdc,0,pszName,&rc,
														DT_LEFT | DT_SINGLELINE | DT_NOPREFIX);
						pMenuItem->SetTextWidth(rc.right);
					}
				}
				break;

			case CFavoriteItem::ITEM_CHANNEL:
				{
					const CFavoriteChannel *pChannel=
						dynamic_cast<const CFavoriteChannel*>(pItem);

					if (pChannel!=nullptr) {
						CChannelItem *pMenuItem=new CChannelItem(pChannel);
						pMenuItem->SetNameWidth(ChannelNameWidth);
						m_ItemList.push_back(pMenuItem);

						mii.fMask=MIIM_FTYPE | MIIM_STATE | MIIM_ID | MIIM_DATA;
						mii.wID=*pCommand;
						mii.dwItemData=reinterpret_cast<ULONG_PTR>(pMenuItem);
						::InsertMenuItem(hmenu,MenuPos,TRUE,&mii);
						(*pCommand)++;
						MenuPos++;

						if ((m_Flags&FLAG_SHOWEVENTINFO)!=0) {
							const CEventInfoData *pEventInfo=pMenuItem->GetEventInfo(&ProgramList,0,&m_BaseTime);
							if (pEventInfo!=NULL) {
								TCHAR szText[256];
								GetEventText(pEventInfo,szText,lengthof(szText));
								m_MenuPainter.GetItemTextExtent(hdc,0,szText,&rc,
																DT_LEFT | DT_SINGLELINE | DT_NOPREFIX);
								pMenuItem->SetEventWidth(rc.right);
							}
						}
					}
				}
				break;
			}
		}
	}

	void CFavoritesMenu::Destroy()
	{
		if (m_hmenu) {
			if ((m_Flags&FLAG_SHARED)==0)
				::DestroyMenu(m_hmenu);
			else
				ClearMenu(m_hmenu);
			m_hmenu=NULL;
		}

		m_MenuPainter.Finalize();
		m_Tooltip.Destroy();
		m_hwnd=NULL;

		if (!m_ItemList.empty()) {
			for (auto i=m_ItemList.begin();i!=m_ItemList.end();i++)
				delete *i;
			m_ItemList.clear();
		}

		if (m_himlIcons!=NULL) {
			::ImageList_Destroy(m_himlIcons);
			m_himlIcons=NULL;
		}
	}

	bool CFavoritesMenu::Show(UINT Flags,int x,int y)
	{
		if (m_hmenu==NULL)
			return false;
		::TrackPopupMenu(m_hmenu,Flags,x,y,0,m_hwnd,NULL);
		return true;
	}

	bool CFavoritesMenu::OnMeasureItem(HWND hwnd,WPARAM wParam,LPARAM lParam)
	{
		LPMEASUREITEMSTRUCT pmis=reinterpret_cast<LPMEASUREITEMSTRUCT>(lParam);

		if (m_hmenu!=NULL && pmis->CtlType==ODT_MENU) {
			if (pmis->itemID>=m_FirstCommand && pmis->itemID<=m_LastCommand) {
				if (pmis->itemData==0)
					return false;
				const CChannelItem *pItem=reinterpret_cast<const CChannelItem*>(pmis->itemData);
				pmis->itemWidth=pItem->GetNameWidth()+m_Margins.cxLeftWidth+m_Margins.cxRightWidth;
				if ((m_Flags&FLAG_SHOWLOGO)!=0)
					pmis->itemWidth+=m_LogoWidth+m_MenuLogoMargin;
				if ((m_Flags&FLAG_SHOWEVENTINFO)!=0)
					pmis->itemWidth+=m_TextHeight+pItem->GetEventWidth();
				pmis->itemHeight=m_ItemHeight;
				return true;
			} else if (pmis->itemID==CM_FAVORITESSUBMENU) {
				const CFolderItem *pItem=reinterpret_cast<const CFolderItem*>(pmis->itemData);
				if (pItem==nullptr)
					return false;
				pmis->itemWidth=pItem->GetTextWidth()+m_Margins.cxLeftWidth+m_Margins.cxRightWidth;
				if ((m_Flags&FLAG_SHOWLOGO)!=0)
					pmis->itemWidth+=m_LogoWidth+m_MenuLogoMargin;
				pmis->itemHeight=m_ItemHeight;
				return true;
			} else if (pmis->itemID==CM_ADDTOFAVORITES
					|| pmis->itemID==CM_ORGANIZEFAVORITES) {
				pmis->itemWidth=m_TextWidth+m_Margins.cxLeftWidth+m_Margins.cxRightWidth;
				if ((m_Flags&FLAG_SHOWLOGO)!=0)
					pmis->itemWidth+=m_LogoWidth+m_MenuLogoMargin;
				pmis->itemHeight=m_ItemHeight;
				return true;
			}
		}
		return false;
	}

	bool CFavoritesMenu::OnDrawItem(HWND hwnd,WPARAM wParam,LPARAM lParam)
	{
		LPDRAWITEMSTRUCT pdis=reinterpret_cast<LPDRAWITEMSTRUCT>(lParam);

		if (m_hmenu==NULL || hwnd!=m_hwnd || pdis->CtlType!=ODT_MENU
				|| ((pdis->itemID<m_FirstCommand || pdis->itemID>m_LastCommand)
					&& pdis->itemID!=CM_ADDTOFAVORITES
					&& pdis->itemID!=CM_ORGANIZEFAVORITES
					&& pdis->itemID!=CM_FAVORITESSUBMENU))
			return false;

		TCHAR szText[256];

		m_MenuPainter.DrawItemBackground(pdis->hDC,pdis->rcItem,pdis->itemState);
		COLORREF crTextColor=m_MenuPainter.GetTextColor(pdis->itemState);
		COLORREF crOldTextColor=::SetTextColor(pdis->hDC,crTextColor);
		int OldBkMode=::SetBkMode(pdis->hDC,TRANSPARENT);
		HFONT hfontOld=DrawUtil::SelectObject(pdis->hDC,m_Font);

		RECT rc;
		rc.left=pdis->rcItem.left+m_Margins.cxLeftWidth;
		rc.top=pdis->rcItem.top+m_Margins.cyTopHeight;
		rc.right=pdis->rcItem.right-m_Margins.cxRightWidth;
		rc.bottom=pdis->rcItem.bottom-m_Margins.cyBottomHeight;

		if (pdis->itemID>=m_FirstCommand && pdis->itemID<=m_LastCommand) {
			if (pdis->itemData==0)
				return false;
			const CChannelItem *pItem=reinterpret_cast<const CChannelItem*>(pdis->itemData);

			if ((m_Flags&FLAG_SHOWLOGO)!=0) {
				const CChannelInfo &ChInfo=pItem->GetChannelInfo();
				HBITMAP hbmLogo=GetAppClass().LogoManager.GetAssociatedLogoBitmap(
					ChInfo.GetNetworkID(),ChInfo.GetServiceID(),CLogoManager::LOGOTYPE_SMALL);
				if (hbmLogo!=NULL) {
					DrawUtil::CMemoryDC MemoryDC(pdis->hDC);
					MemoryDC.SetBitmap(hbmLogo);
					int y=rc.top+((rc.bottom-rc.top)-m_LogoHeight)/2;
					BITMAP bm;
					::GetObject(hbmLogo,sizeof(bm),&bm);
					MemoryDC.DrawStretch(pdis->hDC,rc.left+1,y+1,m_LogoWidth-3,m_LogoHeight-3,
										 0,0,bm.bmWidth,bm.bmHeight);
					MemoryDC.SetBitmap(m_LogoFrameImage);
					MemoryDC.DrawAlpha(pdis->hDC,rc.left,y,0,0,m_LogoWidth,m_LogoHeight);
				}
				rc.left+=m_LogoWidth+m_MenuLogoMargin;
			}

			rc.right=rc.left+pItem->GetNameWidth();
			m_MenuPainter.DrawItemText(pdis->hDC,pdis->itemState,pItem->GetName(),rc,
									   DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

			if ((m_Flags&FLAG_SHOWEVENTINFO)!=0) {
				const CEventInfoData *pEventInfo=pItem->GetEventInfo(0);
				if (pEventInfo!=NULL) {
					GetEventText(pEventInfo,szText,lengthof(szText));
					rc.left=rc.right+m_TextHeight;
					rc.right=pdis->rcItem.right-m_Margins.cxRightWidth;
					m_MenuPainter.DrawItemText(pdis->hDC,pdis->itemState,szText,rc,
											   DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
				}
			}
		} else {
			if (pdis->itemID==CM_ADDTOFAVORITES || pdis->itemID==CM_ORGANIZEFAVORITES) {
				if ((m_Flags&FLAG_SHOWLOGO)!=0) {
					m_MenuPainter.DrawIcon(
						m_himlIcons,
						pdis->itemID==CM_ADDTOFAVORITES?FAVORITES_ICON_ADD:FAVORITES_ICON_ORGANIZE,
						pdis->hDC,
						rc.left+(m_LogoWidth-m_IconWidth)/2,
						rc.top+(rc.bottom-rc.top-m_IconHeight)/2,
						pdis->itemState);
					rc.left+=m_LogoWidth+m_MenuLogoMargin;
				}
				::LoadString(GetAppClass().GetResourceInstance(),
							 pdis->itemID==CM_ADDTOFAVORITES?IDS_MENU_ADDTOFAVORITES:IDS_MENU_ORGANIZEFAVORITES,
							 szText,lengthof(szText));
				m_MenuPainter.DrawItemText(pdis->hDC,pdis->itemState,szText,rc);
			} else if (pdis->itemID==CM_FAVORITESSUBMENU) {
				const CFolderItem *pItem=reinterpret_cast<const CFolderItem*>(pdis->itemData);
				if ((m_Flags&FLAG_SHOWLOGO)!=0) {
					m_MenuPainter.DrawIcon(m_himlIcons,FAVORITES_ICON_FOLDER,pdis->hDC,
										   rc.left+(m_LogoWidth-m_IconWidth)/2,
										   rc.top+(rc.bottom-rc.top-m_IconHeight)/2,
										   pdis->itemState);
					rc.left+=m_LogoWidth+m_MenuLogoMargin;
				}
				m_MenuPainter.DrawItemText(pdis->hDC,pdis->itemState,pItem->GetName(),rc,
										   DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
			}
		}

		::SelectObject(pdis->hDC,hfontOld);
		::SetBkMode(pdis->hDC,OldBkMode);
		::SetTextColor(pdis->hDC,crOldTextColor);

		return true;
	}

	bool CFavoritesMenu::OnMenuSelect(HWND hwnd,WPARAM wParam,LPARAM lParam)
	{
		HMENU hmenu=reinterpret_cast<HMENU>(lParam);
		UINT Command=LOWORD(wParam);

		if (hmenu==NULL || hmenu!=m_hmenu || hwnd!=m_hwnd || HIWORD(wParam)==0xFFFF
				|| Command<m_FirstCommand || Command>m_LastCommand) {
			if (m_Tooltip.IsVisible())
				m_Tooltip.TrackActivate(1,false);
			return false;
		}

		if ((m_Flags&FLAG_SHOWTOOLTIP)!=0) {
			MENUITEMINFO mii;

			mii.cbSize=sizeof(mii);
			mii.fMask=MIIM_DATA;
			if (::GetMenuItemInfo(hmenu,Command,FALSE,&mii)) {
				CChannelItem *pItem=reinterpret_cast<CChannelItem*>(mii.dwItemData);
				if (pItem==NULL)
					return false;

				const CEventInfoData *pEventInfo1,*pEventInfo2;
				pEventInfo1=pItem->GetEventInfo(0);
				if (pEventInfo1==NULL) {
					pEventInfo1=pItem->GetEventInfo(&GetAppClass().EpgProgramList,0);
				}
				if (pEventInfo1!=NULL) {
					TCHAR szText[256*2+1];
					int Length;
					POINT pt;

					Length=GetEventText(pEventInfo1,szText,lengthof(szText)/2);
					pEventInfo2=pItem->GetEventInfo(&GetAppClass().EpgProgramList,1);
					if (pEventInfo2!=NULL) {
						szText[Length++]=_T('\r');
						szText[Length++]=_T('\n');
						GetEventText(pEventInfo2,szText+Length,lengthof(szText)/2);
					}
					m_Tooltip.SetText(1,szText);
					::GetCursorPos(&pt);
					pt.x+=16;
					pt.y+=max(m_TextHeight,m_LogoHeight)+
								m_Margins.cyTopHeight+m_Margins.cyBottomHeight;
					m_Tooltip.TrackPosition(pt.x,pt.y);
					m_Tooltip.TrackActivate(1,true);
				} else {
					m_Tooltip.TrackActivate(1,false);
				}
			}
		}
		return true;
	}

	bool CFavoritesMenu::OnUninitMenuPopup(HWND hwnd,WPARAM wParam,LPARAM lParam)
	{
		if (hwnd==m_hwnd && reinterpret_cast<HMENU>(wParam)==m_hmenu) {
			Destroy();
			return true;
		}
		return false;
	}

	int CFavoritesMenu::GetEventText(const CEventInfoData *pEventInfo,
									 LPTSTR pszText,int MaxLength) const
	{
		TCHAR szTime[EpgUtil::MAX_EVENT_TIME_LENGTH];

		EpgUtil::FormatEventTime(
			pEventInfo,szTime,lengthof(szTime),EpgUtil::EVENT_TIME_HOUR_2DIGITS);

		return StdUtil::snprintf(pszText,MaxLength,TEXT("%s %s"),
								 szTime,pEventInfo->m_EventName.c_str());
	}

	void CFavoritesMenu::CreateFont(HDC hdc)
	{
		if (m_Font.IsCreated())
			return;

		LOGFONT lf;
		m_MenuPainter.GetFont(&lf);
		m_Font.Create(&lf);

		if (hdc!=NULL)
			m_TextHeight=m_Font.GetHeight(hdc);
		else
			m_TextHeight=abs(lf.lfHeight);
	}

	void CFavoritesMenu::GetBaseTime(SYSTEMTIME *pTime)
	{
		GetCurrentEpgTime(pTime);
		OffsetSystemTime(pTime,2*TimeConsts::SYSTEMTIME_MINUTE);
	}


	class CFavoritePropertiesDialog : public CBasicDialog
	{
	public:
		CFavoritePropertiesDialog(CFavoriteChannel *pChannel);
		~CFavoritePropertiesDialog();
		bool Show(HWND hwndOwner) override;

	private:
		INT_PTR DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam) override;

		CFavoriteChannel *m_pChannel;
	};

	CFavoritePropertiesDialog::CFavoritePropertiesDialog(CFavoriteChannel *pChannel)
		: m_pChannel(pChannel)
	{
	}

	CFavoritePropertiesDialog::~CFavoritePropertiesDialog()
	{
		Destroy();
	}

	bool CFavoritePropertiesDialog::Show(HWND hwndOwner)
	{
		return ShowDialog(hwndOwner,GetAppClass().GetResourceInstance(),
						  MAKEINTRESOURCE(IDD_FAVORITEPROPERTIES))==IDOK;
	}

	INT_PTR CFavoritePropertiesDialog::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
	{
		switch (uMsg) {
		case WM_INITDIALOG:
			{
				const CChannelInfo &ChannelInfo=m_pChannel->GetChannelInfo();
				TCHAR szText[64];

				::SetDlgItemText(hDlg,IDC_FAVORITEPROP_NAME,m_pChannel->GetName());

				const CDriverManager &DriverManager=GetAppClass().DriverManager;
				for (int i=0;i<DriverManager.NumDrivers();i++) {
					const CDriverInfo *pDriverInfo=DriverManager.GetDriverInfo(i);
					DlgComboBox_AddString(hDlg,IDC_INITIALSETTINGS_DRIVER,pDriverInfo->GetFileName());
				}
				::SetDlgItemText(hDlg,IDC_FAVORITEPROP_BONDRIVER,m_pChannel->GetBonDriverFileName());

				DlgCheckBox_Check(hDlg,IDC_FAVORITEPROP_FORCEBONDRIVERCHANGE,m_pChannel->GetForceBonDriverChange());

				StdUtil::snprintf(szText,lengthof(szText),TEXT("%d (0x%04x)"),
								  ChannelInfo.GetServiceID(),
								  ChannelInfo.GetServiceID());
				::SetDlgItemText(hDlg,IDC_FAVORITEPROP_SERVICEID,szText);
				StdUtil::snprintf(szText,lengthof(szText),TEXT("%d (0x%04x)"),
								  ChannelInfo.GetNetworkID(),
								  ChannelInfo.GetNetworkID());
				::SetDlgItemText(hDlg,IDC_FAVORITEPROP_NETWORKID,szText);
				StdUtil::snprintf(szText,lengthof(szText),TEXT("%d (0x%04x)"),
								  ChannelInfo.GetTransportStreamID(),
								  ChannelInfo.GetTransportStreamID());
				::SetDlgItemText(hDlg,IDC_FAVORITEPROP_TRANSPORTSTREAMID,szText);
			}
			return TRUE;

		case WM_COMMAND:
			switch (LOWORD(wParam)) {
			case IDOK:
				{
					TCHAR szText[256];

					::GetDlgItemText(hDlg,IDC_FAVORITEPROP_NAME,szText,lengthof(szText));
					m_pChannel->SetName(szText);
					::GetDlgItemText(hDlg,IDC_FAVORITEPROP_BONDRIVER,szText,lengthof(szText));
					m_pChannel->SetBonDriverFileName(szText);
					m_pChannel->SetForceBonDriverChange(
						DlgCheckBox_IsChecked(hDlg,IDC_FAVORITEPROP_FORCEBONDRIVERCHANGE));
				}
			case IDCANCEL:
				::EndDialog(hDlg,LOWORD(wParam));
				return TRUE;
			}
			return TRUE;
		}

		return FALSE;
	}


	COrganizeFavoritesDialog::COrganizeFavoritesDialog(CFavoritesManager *pManager)
		: m_pManager(pManager)
	{
	}

	COrganizeFavoritesDialog::~COrganizeFavoritesDialog()
	{
		Destroy();
	}

	bool COrganizeFavoritesDialog::Show(HWND hwndOwner)
	{
		m_Position=m_pManager->GetOrganizeDialogPos();

		bool fResult=ShowDialog(hwndOwner,
								GetAppClass().GetResourceInstance(),
								MAKEINTRESOURCE(IDD_ORGANIZEFAVORITES))==IDOK;

		m_pManager->SetOrganizeDialogPos(m_Position);

		return fResult;
	}

	INT_PTR COrganizeFavoritesDialog::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
	{
		switch (uMsg) {
		case WM_INITDIALOG:
			{
				HINSTANCE hinstRes=GetAppClass().GetResourceInstance();
				HWND hwndTree=::GetDlgItem(hDlg,IDC_FAVORITES_FOLDERTREE);

				HIMAGELIST himl=ImageList_LoadImage(hinstRes,
													MAKEINTRESOURCE(IDB_FAVORITES),
													16,1,0,IMAGE_BITMAP,LR_CREATEDIBSECTION);
				ImageList_SetBkColor(himl,TreeView_GetBkColor(hwndTree));
				TreeView_SetImageList(hwndTree,himl,TVSIL_NORMAL);

				TreeView_SetInsertMarkColor(hwndTree,::GetSysColor(COLOR_HIGHLIGHT));

				if (m_pManager->GetRootFolder().GetItemCount()>0)
					InsertTreeItems(hwndTree,TVI_ROOT,&m_pManager->GetRootFolder());

				AddControl(IDC_FAVORITES_FOLDERTREE,ALIGN_ALL);
				AddControl(IDC_FAVORITES_NEWFOLDER,ALIGN_BOTTOM);
				AddControl(IDOK,ALIGN_BOTTOM_RIGHT);
				AddControl(IDCANCEL,ALIGN_BOTTOM_RIGHT);

				m_fItemDragging=false;
				m_himlDrag=nullptr;

				HICON hico=(HICON)::LoadImage(hinstRes,MAKEINTRESOURCE(IDI_FAVORITES),
											  IMAGE_ICON,0,0,LR_DEFAULTSIZE | LR_SHARED);
				::SendMessage(hDlg,WM_SETICON,ICON_BIG,reinterpret_cast<LPARAM>(hico));
				hico=(HICON)::LoadImage(hinstRes,MAKEINTRESOURCE(IDI_FAVORITES),
										IMAGE_ICON,16,16,LR_DEFAULTSIZE | LR_SHARED);
				::SendMessage(hDlg,WM_SETICON,ICON_SMALL,reinterpret_cast<LPARAM>(hico));

				ApplyPosition();
			}
			return TRUE;

		case WM_MOUSEMOVE:
			if (m_fItemDragging) {
				HWND hwndTree=::GetDlgItem(hDlg,IDC_FAVORITES_FOLDERTREE);
				POINT pt;

				pt.x=GET_X_LPARAM(lParam);
				pt.y=GET_Y_LPARAM(lParam);
				ImageList_DragShowNolock(FALSE);
				TVHITTESTINFO tvhti;
				tvhti.pt=pt;
				::MapWindowPoints(hDlg,hwndTree,&tvhti.pt,1);
				HTREEITEM hItem=TreeView_HitTest(hwndTree,&tvhti);
				if (hItem!=nullptr) {
					if (hItem==m_hDraggingItem) {
						hItem=nullptr;
					} else {
						HTREEITEM hParent=TreeView_GetParent(hwndTree,hItem);
						while (hParent!=nullptr) {
							if (hParent==m_hDraggingItem) {
								hItem=nullptr;
								break;
							}
							hParent=TreeView_GetParent(hwndTree,hParent);
						}
					}
				}
				if (hItem!=nullptr) {
					TVITEM tvi;
					tvi.mask=TVIF_PARAM;
					tvi.hItem=hItem;
					if (TreeView_GetItem(hwndTree,&tvi)) {
						RECT rc;
						TreeView_GetItemRect(hwndTree,hItem,&rc,TRUE);
						const CFavoriteItem *pItem=reinterpret_cast<const CFavoriteItem*>(tvi.lParam);
						if (pItem->GetType()==CFavoriteItem::ITEM_FOLDER) {
							m_fDropInsertAfter=tvhti.pt.y>=rc.bottom-(rc.bottom-rc.top)/3;
							m_fDropToFolder=!m_fDropInsertAfter && tvhti.pt.y>=rc.top+(rc.bottom-rc.top)/3;
							if (m_fDropToFolder) {
								TreeView_SetInsertMark(hwndTree,nullptr,FALSE);
								TreeView_SelectDropTarget(hwndTree,hItem);
							} else {
								TreeView_SelectDropTarget(hwndTree,nullptr);
								TreeView_SetInsertMark(hwndTree,hItem,m_fDropInsertAfter);
							}
						} else {
							m_fDropToFolder=false;
							m_fDropInsertAfter=tvhti.pt.y>=(rc.top+rc.bottom)/2;
							TreeView_SelectDropTarget(hwndTree,nullptr);
							TreeView_SetInsertMark(hwndTree,hItem,m_fDropInsertAfter);
						}
						m_hDropTargetItem=hItem;
					}
				} else {
					if (m_hDropTargetItem!=nullptr) {
						m_hDropTargetItem=nullptr;
						TreeView_SelectDropTarget(hwndTree,nullptr);
						TreeView_SetInsertMark(hwndTree,nullptr,FALSE);
					}
				}
				ImageList_DragShowNolock(TRUE);
				::ClientToScreen(hDlg,&pt);
				ImageList_DragMove(pt.x,pt.y);
			}
			return TRUE;

		case WM_LBUTTONUP:
			if (m_fItemDragging) {
				HWND hwndTree=::GetDlgItem(hDlg,IDC_FAVORITES_FOLDERTREE);

				ImageList_DragLeave(nullptr);
				ImageList_EndDrag();
				ImageList_Destroy(m_himlDrag);
				m_himlDrag=nullptr;
				::ReleaseCapture();
				m_fItemDragging=false;
				if (m_hDropTargetItem!=nullptr) {
					TreeView_SelectDropTarget(hwndTree,nullptr);
					TreeView_SetInsertMark(hwndTree,nullptr,FALSE);
					if (m_fDropToFolder) {
						HTREEITEM hItem=CopyTreeItems(hwndTree,m_hDraggingItem,m_hDropTargetItem,TVI_LAST);
						if (hItem!=nullptr) {
							TreeView_DeleteItem(hwndTree,m_hDraggingItem);
							TVITEM tvi;
							tvi.mask=TVIF_CHILDREN;
							tvi.hItem=m_hDropTargetItem;
							tvi.cChildren=1;
							TreeView_SetItem(hwndTree,&tvi);
							TreeView_SelectItem(hwndTree,hItem);
						}
					} else {
						HTREEITEM hInsertAfter;
						if (m_fDropInsertAfter) {
							hInsertAfter=m_hDropTargetItem;
						} else {
							hInsertAfter=TreeView_GetPrevSibling(hwndTree,m_hDropTargetItem);
							if (hInsertAfter==nullptr)
								hInsertAfter=TVI_FIRST;
						}
						HTREEITEM hItem=CopyTreeItems(hwndTree,m_hDraggingItem,
													  TreeView_GetParent(hwndTree,m_hDropTargetItem),
													  hInsertAfter);
						if (hItem!=nullptr) {
							TreeView_DeleteItem(hwndTree,m_hDraggingItem);
							TreeView_SelectItem(hwndTree,hItem);
						}
					}
				}
			}
			return TRUE;

		case WM_NOTIFY:
			switch (reinterpret_cast<LPNMHDR>(lParam)->code) {
			case TVN_BEGINDRAG:
				{
					NMTREEVIEW *pnmtv=reinterpret_cast<NMTREEVIEW*>(lParam);

					m_hDraggingItem=pnmtv->itemNew.hItem;
					m_himlDrag=TreeView_CreateDragImage(pnmtv->hdr.hwndFrom,m_hDraggingItem);
					ImageList_BeginDrag(m_himlDrag,0,8,8);
					::SetCapture(hDlg);
					m_fItemDragging=true;
					m_hDropTargetItem=nullptr;
					POINT pt=pnmtv->ptDrag;
					::ClientToScreen(pnmtv->hdr.hwndFrom,&pt);
					ImageList_DragEnter(nullptr,pt.x,pt.y);
				}
				return TRUE;

			case TVN_DELETEITEM:
				{
					NMTREEVIEW *pnmtv=reinterpret_cast<NMTREEVIEW*>(lParam);

					delete reinterpret_cast<CFavoriteItem*>(pnmtv->itemOld.lParam);
				}
				return TRUE;

			case TVN_BEGINLABELEDIT:
				{
					NMTVDISPINFO *pnmtvdi=reinterpret_cast<NMTVDISPINFO*>(lParam);
					HWND hwndEdit=TreeView_GetEditControl(pnmtvdi->hdr.hwndFrom);
					::SetProp(hwndEdit,TEXT("FavoritesThis"),this);
					m_pOldEditProc=reinterpret_cast<WNDPROC>(
						::SetWindowLongPtr(hwndEdit,GWLP_WNDPROC,reinterpret_cast<LONG_PTR>(EditHookProc)));
				}
				return TRUE;

			case TVN_ENDLABELEDIT:
				{
					NMTVDISPINFO *pnmtvdi=reinterpret_cast<NMTVDISPINFO*>(lParam);

					if (pnmtvdi->item.pszText!=nullptr) {
						LRESULT Result=FALSE;

						if (pnmtvdi->item.pszText[0]!=_T('\0')) {
							CFavoriteItem *pItem=reinterpret_cast<CFavoriteItem*>(pnmtvdi->item.lParam);
							switch (pItem->GetType()) {
							case CFavoriteItem::ITEM_FOLDER:
								{
									CFavoriteFolder *pFolder=dynamic_cast<CFavoriteFolder*>(pItem);
									if (pFolder!=nullptr)
										pFolder->SetName(pnmtvdi->item.pszText);
								}
								break;
							case CFavoriteItem::ITEM_CHANNEL:
								{
									CFavoriteChannel *pChannel=dynamic_cast<CFavoriteChannel*>(pItem);
									if (pChannel!=nullptr)
										pChannel->SetName(pnmtvdi->item.pszText);
								}
								break;
							}
							Result=TRUE;
						}
						::SetWindowLongPtr(hDlg,DWLP_MSGRESULT,Result);
					}
				}
				return TRUE;

			case NM_RCLICK:
				{
					NMHDR *pnmh=reinterpret_cast<NMHDR*>(lParam);
					POINT pt;
					TVHITTESTINFO tvhti;

					::GetCursorPos(&pt);
					tvhti.pt=pt;
					::ScreenToClient(pnmh->hwndFrom,&tvhti.pt);
					HTREEITEM hItem=TreeView_HitTest(pnmh->hwndFrom,&tvhti);
					if (hItem!=nullptr && (tvhti.flags&TVHT_ONITEM)!=0) {
						CFavoriteItem *pItem=GetItem(pnmh->hwndFrom,hItem);
						if (pItem==nullptr)
							break;
						HMENU hmenu=::CreatePopupMenu();
						::AppendMenu(hmenu,MF_STRING | MF_ENABLED,IDC_FAVORITES_DELETE,TEXT("�폜(&D)"));
						::AppendMenu(hmenu,MF_STRING | MF_ENABLED,IDC_FAVORITES_RENAME,TEXT("���O�̕ύX(&R)"));
						if (pItem->GetType()==CFavoriteItem::ITEM_CHANNEL)
							::AppendMenu(hmenu,MF_STRING | MF_ENABLED,IDC_FAVORITES_PROPERTIES,TEXT("�v���p�e�B(&P)..."));
						int Result=::TrackPopupMenu(hmenu,TPM_RIGHTBUTTON | TPM_RETURNCMD,pt.x,pt.y,0,hDlg,nullptr);
						::DestroyMenu(hmenu);
						switch (Result) {
						case IDC_FAVORITES_DELETE:
							TreeView_DeleteItem(pnmh->hwndFrom,hItem);
							break;
						case IDC_FAVORITES_RENAME:
							::SetFocus(pnmh->hwndFrom);
							TreeView_SelectItem(pnmh->hwndFrom,hItem);
							TreeView_EditLabel(pnmh->hwndFrom,hItem);
							break;
						case IDC_FAVORITES_PROPERTIES:
							{
								TreeView_SelectItem(pnmh->hwndFrom,hItem);
								CFavoritePropertiesDialog Dialog(static_cast<CFavoriteChannel*>(pItem));
								Dialog.Show(hDlg);
							}
							break;
						}
					}
				}
				return TRUE;

			case NM_DBLCLK:
				{
					NMHDR *pnmh=reinterpret_cast<NMHDR*>(lParam);
					POINT pt;
					TVHITTESTINFO tvhti;

					::GetCursorPos(&pt);
					tvhti.pt=pt;
					::ScreenToClient(pnmh->hwndFrom,&tvhti.pt);
					HTREEITEM hItem=TreeView_HitTest(pnmh->hwndFrom,&tvhti);
					if (hItem!=nullptr && (tvhti.flags&TVHT_ONITEM)!=0) {
						CFavoriteItem *pItem=GetItem(pnmh->hwndFrom,hItem);
						if (pItem!=nullptr) {
							TreeView_SelectItem(pnmh->hwndFrom,hItem);
							if (pItem->GetType()==CFavoriteItem::ITEM_CHANNEL) {
								CFavoritePropertiesDialog Dialog(static_cast<CFavoriteChannel*>(pItem));
								Dialog.Show(hDlg);
							}
						}
					}
				}
				return TRUE;
			}
			break;

		case WM_COMMAND:
			switch (LOWORD(wParam)) {
			case IDC_FAVORITES_NEWFOLDER:
				{
					HWND hwndTree=::GetDlgItem(hDlg,IDC_FAVORITES_FOLDERTREE);
					HTREEITEM hSelItem=TreeView_GetSelection(hwndTree);

					CFavoriteFolder *pNewFolder=new CFavoriteFolder;
					pNewFolder->SetName(TEXT("�V�K�t�H���_"));

					TVINSERTSTRUCT tvis;
					if (hSelItem==nullptr) {
						tvis.hParent=TVI_ROOT;
					} else {
						if (TreeView_GetChild(hwndTree,hSelItem)!=nullptr) {
							tvis.hParent=hSelItem;
						} else {
							tvis.hParent=TreeView_GetParent(hwndTree,hSelItem);
						}
					}
					tvis.hInsertAfter=TVI_LAST;
					tvis.item.mask=TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_CHILDREN | TVIF_PARAM;
					tvis.item.pszText=const_cast<LPTSTR>(pNewFolder->GetName());
					tvis.item.iImage=FAVORITES_ICON_FOLDER;
					tvis.item.iSelectedImage=FAVORITES_ICON_FOLDER;
					tvis.item.cChildren=0;
					tvis.item.lParam=reinterpret_cast<LPARAM>(pNewFolder);
					HTREEITEM hItem=TreeView_InsertItem(hwndTree,&tvis);
					::SetFocus(hwndTree);
					TreeView_SelectItem(hwndTree,hItem);
					TreeView_EditLabel(hwndTree,hItem);
				}
				return TRUE;

			case IDOK:
				{
					HWND hwndTree=::GetDlgItem(hDlg,IDC_FAVORITES_FOLDERTREE);

					// TreeView��Edit�����IDOK�Ɠ���ID�Œʒm�������Ă���Ƃ����
					HWND hwndEdit=TreeView_GetEditControl(hwndTree);
					if (hwndEdit!=nullptr && reinterpret_cast<HWND>(lParam)==hwndEdit)
						break;

					CFavoriteFolder Root;

					GetTreeItems(hwndTree,TVI_ROOT,&Root);
					m_pManager->GetRootFolder()=Root;
					m_pManager->SetModified(true);
				}
			case IDCANCEL:
				::EndDialog(hDlg,LOWORD(wParam));
				return TRUE;
			}
			return TRUE;

		case WM_DESTROY:
			{
				HWND hwndTree=::GetDlgItem(hDlg,IDC_FAVORITES_FOLDERTREE);
				TreeView_DeleteAllItems(hwndTree);
				HIMAGELIST himl=TreeView_SetImageList(hwndTree,nullptr,TVSIL_NORMAL);
				if (himl!=nullptr)
					ImageList_Destroy(himl);
			}
			return TRUE;
		}

		return FALSE;
	}

	void COrganizeFavoritesDialog::InsertTreeItems(HWND hwndTree,HTREEITEM hParent,const CFavoriteFolder *pFolder)
	{
		for (size_t i=0;i<pFolder->GetItemCount();i++) {
			const CFavoriteItem *pItem=pFolder->GetItem(i);

			switch (pItem->GetType()) {
			case CFavoriteItem::ITEM_FOLDER:
				{
					const CFavoriteFolder *pSubFolder=dynamic_cast<const CFavoriteFolder*>(pItem);
					if (pSubFolder!=nullptr) {
						CFavoriteFolder *pNewFolder=new CFavoriteFolder;
						pNewFolder->SetName(pSubFolder->GetName());
						TVINSERTSTRUCT tvis;
						tvis.hParent=hParent;
						tvis.hInsertAfter=TVI_LAST;
						tvis.item.mask=TVIF_STATE | TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_CHILDREN | TVIF_PARAM;
						tvis.item.state=TVIS_EXPANDED;
						tvis.item.stateMask=~0U;
						tvis.item.pszText=const_cast<LPTSTR>(pSubFolder->GetName());
						tvis.item.iImage=FAVORITES_ICON_FOLDER;
						tvis.item.iSelectedImage=FAVORITES_ICON_FOLDER;
						tvis.item.cChildren=pSubFolder->GetItemCount()>0;
						tvis.item.lParam=reinterpret_cast<LPARAM>(pNewFolder);
						HTREEITEM hItem=TreeView_InsertItem(hwndTree,&tvis);
						InsertTreeItems(hwndTree,hItem,pSubFolder);
					}
				}
				break;

			case CFavoriteItem::ITEM_CHANNEL:
				{
					const CFavoriteChannel *pChannel=dynamic_cast<const CFavoriteChannel*>(pItem);
					if (pChannel!=nullptr) {
						TVINSERTSTRUCT tvis;
						tvis.hParent=hParent;
						tvis.hInsertAfter=TVI_LAST;
						tvis.item.mask=TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM;
						tvis.item.pszText=const_cast<LPTSTR>(pChannel->GetName());
						tvis.item.iImage=FAVORITES_ICON_EMPTY;
						tvis.item.iSelectedImage=FAVORITES_ICON_EMPTY;
						tvis.item.lParam=reinterpret_cast<LPARAM>(pChannel->Duplicate());

						const CChannelInfo &ChannelInfo=pChannel->GetChannelInfo();
						if (ChannelInfo.GetNetworkID()!=0 && ChannelInfo.GetServiceID()!=0) {
							HICON hico=GetAppClass().LogoManager.CreateLogoIcon(
								ChannelInfo.GetNetworkID(),ChannelInfo.GetServiceID(),16,16);
							if (hico!=nullptr) {
								tvis.item.iImage=ImageList_AddIcon(
									TreeView_GetImageList(hwndTree,TVSIL_NORMAL),hico);
								tvis.item.iSelectedImage=tvis.item.iImage;
								::DestroyIcon(hico);
							}
						}

						TreeView_InsertItem(hwndTree,&tvis);
					}
				}
				break;
			}
		}
	}

	HTREEITEM COrganizeFavoritesDialog::CopyTreeItems(HWND hwndTree,HTREEITEM hSrcItem,HTREEITEM hParent,HTREEITEM hInsertAfter)
	{
		TVINSERTSTRUCT tvis;

		tvis.hParent=hParent;
		tvis.hInsertAfter=hInsertAfter;
		tvis.item.mask=TVIF_STATE | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_CHILDREN | TVIF_PARAM;
		tvis.item.hItem=hSrcItem;
		tvis.item.stateMask=TVIS_EXPANDED;
		if (!TreeView_GetItem(hwndTree,&tvis.item))
			return nullptr;
		const CFavoriteItem *pItem=reinterpret_cast<const CFavoriteItem*>(tvis.item.lParam);
		CFavoriteItem *pNewItem=pItem->Duplicate();
		tvis.item.mask|=TVIF_TEXT;
		tvis.item.pszText=const_cast<LPTSTR>(pNewItem->GetName());
		tvis.item.lParam=reinterpret_cast<LPARAM>(pNewItem);
		HTREEITEM hItem=TreeView_InsertItem(hwndTree,&tvis);
		if (hItem==nullptr) {
			delete pNewItem;
			return nullptr;
		}

		HTREEITEM hChild=TreeView_GetChild(hwndTree,hSrcItem);
		while (hChild!=nullptr) {
			CopyTreeItems(hwndTree,hChild,hItem,TVI_LAST);
			hChild=TreeView_GetNextSibling(hwndTree,hChild);
		}

		return hItem;
	}

	void COrganizeFavoritesDialog::GetTreeItems(HWND hwndTree,HTREEITEM hParent,CFavoriteFolder *pFolder)
	{
		TVITEM tvi;

		tvi.mask=TVIF_PARAM;
		tvi.hItem=TreeView_GetChild(hwndTree,hParent);
		while (tvi.hItem!=nullptr) {
			TreeView_GetItem(hwndTree,&tvi);
			const CFavoriteItem *pItem=reinterpret_cast<const CFavoriteItem*>(tvi.lParam);
			CFavoriteItem *pNewItem=pItem->Duplicate();
			pFolder->AddItem(pNewItem);
			if (pNewItem->GetType()==CFavoriteItem::ITEM_FOLDER) {
				CFavoriteFolder *pSubFolder=dynamic_cast<CFavoriteFolder*>(pNewItem);
				if (pSubFolder!=nullptr)
					GetTreeItems(hwndTree,tvi.hItem,pSubFolder);
			}
			tvi.hItem=TreeView_GetNextSibling(hwndTree,tvi.hItem);
		}
	}

	CFavoriteItem *COrganizeFavoritesDialog::GetItem(HWND hwndTree,HTREEITEM hItem)
	{
		TVITEM tvi;
		tvi.mask=TVIF_PARAM;
		tvi.hItem=hItem;
		if (!TreeView_GetItem(hwndTree,&tvi))
			return nullptr;
		return reinterpret_cast<CFavoriteItem*>(tvi.lParam);
	}

	LRESULT CALLBACK COrganizeFavoritesDialog::EditHookProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
	{
		COrganizeFavoritesDialog *pThis=static_cast<COrganizeFavoritesDialog*>(::GetProp(hwnd,TEXT("FavoritesThis")));
		if (pThis==nullptr)
			return ::DefWindowProc(hwnd,uMsg,wParam,lParam);

		switch (uMsg) {
		case WM_GETDLGCODE:
			return DLGC_WANTALLKEYS;
		}

		return ::CallWindowProc(pThis->m_pOldEditProc,hwnd,uMsg,wParam,lParam);
	}

}
