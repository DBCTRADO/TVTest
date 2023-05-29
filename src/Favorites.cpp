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


#include "stdafx.h"
#include "TVTest.h"
#include "Favorites.h"
#include "AppMain.h"
#include "DialogUtil.h"
#include "LogoManager.h"
#include "DriverManager.h"
#include "GUIUtil.h"
#include "DPIUtil.h"
#include "resource.h"
#include "Common/DebugDef.h"


namespace TVTest
{

enum {
	FAVORITES_ICON_EMPTY,
	FAVORITES_ICON_FOLDER,
	FAVORITES_ICON_ADD,
	FAVORITES_ICON_ORGANIZE
};


static HIMAGELIST CreateFavoritesIconImageList(HINSTANCE hinst, int Width, int Height)
{
	static const LPCTSTR Icons[] = {
		nullptr,
		MAKEINTRESOURCE(IDI_FAVORITES_FOLDER),
		MAKEINTRESOURCE(IDI_FAVORITES_ADD),
		MAKEINTRESOURCE(IDI_FAVORITES),
	};

	return CreateImageListFromIcons(hinst, Icons, lengthof(Icons), Width, Height);
}




bool CFavoriteItem::SetName(LPCTSTR pszName)
{
	if (pszName == nullptr)
		return false;

	m_Name = pszName;

	return true;
}


CFavoriteFolder::CFavoriteFolder()
	: CFavoriteItem(ItemType::Folder)
{
}

CFavoriteFolder::CFavoriteFolder(const CFavoriteFolder &Src)
	: CFavoriteItem(ItemType::Folder)
{
	*this = Src;
}

CFavoriteFolder &CFavoriteFolder::operator=(const CFavoriteFolder &Src)
{
	if (&Src != this) {
		Clear();
		m_Name = Src.m_Name;
		if (!Src.m_Children.empty()) {
			m_Children.reserve(Src.m_Children.size());
			for (const auto  &e : Src.m_Children)
				m_Children.emplace_back(e->Duplicate());
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
	m_Children.clear();
}

size_t CFavoriteFolder::GetItemCount() const
{
	return m_Children.size();
}

size_t CFavoriteFolder::GetSubItemCount() const
{
	size_t Count = m_Children.size();

	for (const auto &e : m_Children) {
		if (e->GetType() == ItemType::Folder) {
			const CFavoriteFolder *pFolder = dynamic_cast<const CFavoriteFolder*>(e.get());
			if (pFolder != nullptr)
				Count += pFolder->GetSubItemCount();
		}
	}

	return Count;
}

CFavoriteItem *CFavoriteFolder::GetItem(size_t Index)
{
	if (Index >= m_Children.size())
		return nullptr;

	return m_Children[Index].get();
}

const CFavoriteItem *CFavoriteFolder::GetItem(size_t Index) const
{
	if (Index >= m_Children.size())
		return nullptr;

	return m_Children[Index].get();
}

bool CFavoriteFolder::AddItem(CFavoriteItem *pItem)
{
	if (pItem == nullptr)
		return false;

	m_Children.emplace_back(pItem);

	return true;
}

bool CFavoriteFolder::AddItem(size_t Pos, CFavoriteItem *pItem)
{
	if (Pos > m_Children.size())
		return false;

	auto it = m_Children.begin();
	std::advance(it, Pos);
	m_Children.emplace(it, pItem);

	return true;
}

bool CFavoriteFolder::DeleteItem(size_t Index)
{
	if (Index >= m_Children.size())
		return false;

	auto i = m_Children.begin();
	std::advance(i, Index);
	m_Children.erase(i);

	return true;
}

CFavoriteItem *CFavoriteFolder::RemoveItem(size_t Index)
{
	if (Index >= m_Children.size())
		return nullptr;

	auto i = m_Children.begin();
	std::advance(i, Index);
	CFavoriteItem *pItem = i->release();
	m_Children.erase(i);

	return pItem;
}

bool CFavoriteFolder::MoveItem(size_t From, size_t To)
{
	if (From >= m_Children.size() || To >= m_Children.size())
		return false;

	if (From != To) {
		auto i = m_Children.begin();
		std::advance(i, From);
		CFavoriteItem *pItem = i->release();
		m_Children.erase(i);
		i = m_Children.begin();
		std::advance(i, To);
		m_Children.emplace(i, pItem);
	}

	return true;
}

CFavoriteFolder *CFavoriteFolder::FindSubFolder(LPCTSTR pszName)
{
	if (pszName == nullptr)
		return nullptr;

	for (const auto &e : m_Children) {
		if (e->GetType() == ItemType::Folder) {
			CFavoriteFolder *pFolder = dynamic_cast<CFavoriteFolder*>(e.get());
			if (pFolder != nullptr && pFolder->m_Name.compare(pszName) == 0)
				return pFolder;
		}
	}

	return nullptr;
}


CFavoriteChannel::CFavoriteChannel(const CChannelInfo &ChannelInfo)
	: CFavoriteItem(ItemType::Channel)
	, m_ChannelInfo(ChannelInfo)
{
	m_Name = ChannelInfo.GetName();
}

CFavoriteChannel *CFavoriteChannel::Duplicate() const
{
	return new CFavoriteChannel(*this);
}

bool CFavoriteChannel::SetBonDriverFileName(LPCTSTR pszFileName)
{
	if (pszFileName != nullptr)
		m_BonDriverFileName = pszFileName;
	else
		m_BonDriverFileName.clear();

	return true;
}

bool CFavoriteChannel::SetChannelInfo(const CChannelInfo &ChannelInfo)
{
	m_ChannelInfo = ChannelInfo;

	return true;
}


bool CFavoriteItemEnumerator::EnumItems(CFavoriteFolder &Folder)
{
	const size_t ItemCount = Folder.GetItemCount();

	for (size_t i = 0; i < ItemCount; i++) {
		CFavoriteItem *pItem = Folder.GetItem(i);

		switch (pItem->GetType()) {
		case CFavoriteItem::ItemType::Folder:
			{
				CFavoriteFolder *pFolder = dynamic_cast<CFavoriteFolder*>(pItem);

				if (pFolder != nullptr) {
					if (!FolderItem(*pFolder))
						return false;
					if (!EnumItems(*pFolder))
						return false;
				}
			}
			break;

		case CFavoriteItem::ItemType::Channel:
			{
				CFavoriteChannel *pChannel = dynamic_cast<CFavoriteChannel*>(pItem);

				if (pChannel != nullptr) {
					if (!ChannelItem(Folder, *pChannel))
						return false;
				}
			}
			break;
		}
	}

	return true;
}


constexpr unsigned int CHANNEL_FLAG_FORCEBONDRIVERCHANGE = 0x0001U;


bool CFavoritesManager::AddChannel(const CChannelInfo *pChannelInfo, LPCTSTR pszBonDriverFileName)
{
	CFavoriteChannel *pItem = new CFavoriteChannel(*pChannelInfo);
	pItem->SetBonDriverFileName(pszBonDriverFileName);

	if (!m_RootFolder.AddItem(pItem)) {
		delete pItem;
		return false;
	}

	m_fModified = true;

	return true;
}

bool CFavoritesManager::SetMenu(HMENU hmenu)
{
	if (hmenu == nullptr)
		return false;

	int ItemCount = ::GetMenuItemCount(hmenu);
	for (; ItemCount > 3; ItemCount--)
		::DeleteMenu(hmenu, 0, MF_BYPOSITION);

	if (m_RootFolder.GetItemCount() > 0) {
		int Command = CM_FAVORITECHANNEL_FIRST;
		SetFolderMenu(hmenu, 0, &Command, &m_RootFolder);
	} else {
		::InsertMenu(hmenu, 0, MF_BYPOSITION | MF_STRING | MF_GRAYED, 0, TEXT("なし"));
	}

	return true;
}

void CFavoritesManager::SetFolderMenu(HMENU hmenu, int MenuPos, int *pCommand, const CFavoriteFolder *pFolder) const
{
	for (size_t i = 0; i < pFolder->GetItemCount(); i++) {
		const CFavoriteItem *pItem = pFolder->GetItem(i);

		switch (pItem->GetType()) {
		case CFavoriteItem::ItemType::Folder:
			{
				const CFavoriteFolder *pSubFolder =
					dynamic_cast<const CFavoriteFolder*>(pItem);

				if (pSubFolder != nullptr) {
					const HMENU hmenuSub = ::CreatePopupMenu();

					::InsertMenu(
						hmenu, MenuPos, MF_BYPOSITION | MF_POPUP | MF_ENABLED,
						reinterpret_cast<UINT_PTR>(hmenuSub),
						FormatMenuString(pSubFolder->GetName()).c_str());
					SetFolderMenu(hmenuSub, 0, pCommand, pSubFolder);
					MenuPos++;
				}
			}
			break;

		case CFavoriteItem::ItemType::Channel:
			{
				const CFavoriteChannel *pChannel =
					dynamic_cast<const CFavoriteChannel*>(pItem);

				if (pChannel != nullptr) {
					::InsertMenu(
						hmenu, MenuPos, MF_BYPOSITION | MF_STRING | MF_ENABLED,
						*pCommand, FormatMenuString(pChannel->GetName()).c_str());
					(*pCommand)++;
					MenuPos++;
				}
			}
			break;
		}
	}
}

bool CFavoritesManager::GetChannelByCommand(int Command, ChannelInfo *pInfo) const
{
	if (Command < CM_FAVORITECHANNEL_FIRST
			|| Command > CM_FAVORITECHANNEL_LAST
			|| pInfo == nullptr)
		return false;

	int BaseCommand = CM_FAVORITECHANNEL_FIRST;
	return GetChannelByCommandSub(&m_RootFolder, Command, &BaseCommand, pInfo);
}

bool CFavoritesManager::GetChannelByCommandSub(
	const CFavoriteFolder *pFolder, int Command, int *pBaseCommand, ChannelInfo *pInfo) const
{
	for (size_t i = 0; i < pFolder->GetItemCount(); i++) {
		const CFavoriteItem *pItem = pFolder->GetItem(i);

		switch (pItem->GetType()) {
		case CFavoriteItem::ItemType::Folder:
			{
				const CFavoriteFolder *pSubFolder = dynamic_cast<const CFavoriteFolder*>(pItem);
				if (pSubFolder != nullptr) {
					if (GetChannelByCommandSub(pSubFolder, Command, pBaseCommand, pInfo))
						return true;
				}
			}
			break;

		case CFavoriteItem::ItemType::Channel:
			{
				const CFavoriteChannel *pChannel =
					dynamic_cast<const CFavoriteChannel*>(pItem);

				if (pChannel != nullptr) {
					if (Command == *pBaseCommand) {
						pInfo->Channel = pChannel->GetChannelInfo();
						pInfo->BonDriverFileName = pChannel->GetBonDriverFileName();
						pInfo->fForceBonDriverChange = pChannel->GetForceBonDriverChange();
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

	const HANDLE hFile = ::CreateFile(
		pszFileName, GENERIC_READ, FILE_SHARE_READ, nullptr,
		OPEN_EXISTING, 0, nullptr);
	if (hFile == INVALID_HANDLE_VALUE)
		return false;

	std::unique_ptr<BYTE[]> Buffer;
	LPWSTR pszBuffer;

	try {
		LARGE_INTEGER FileSize;
		if (!::GetFileSizeEx(hFile, &FileSize)
				|| FileSize.QuadPart <= 4 || FileSize.QuadPart > 1024 * 1024)
			throw __LINE__;

		Buffer.reset(new BYTE[FileSize.LowPart + 3]);
		DWORD Read;
		if (!::ReadFile(hFile, Buffer.get(), FileSize.LowPart, &Read, nullptr)
				|| Read != FileSize.LowPart)
			throw __LINE__;
		Buffer[FileSize.LowPart + 0] = 0;
		Buffer[FileSize.LowPart + 1] = 0;
		Buffer[FileSize.LowPart + 2] = 0;

		if (Buffer[0] == 0xFF && Buffer[1] == 0xFE) {
			pszBuffer = reinterpret_cast<LPWSTR>(Buffer.get() + 2);
		} else {
			const int Length = ::MultiByteToWideChar(CP_ACP, 0, reinterpret_cast<char*>(Buffer.get()), FileSize.LowPart, nullptr, 0);
			std::unique_ptr<BYTE[]> ConvertedBuffer(new BYTE[(Length + 1) * sizeof(WCHAR)]);
			LPWSTR pszDst = reinterpret_cast<LPWSTR>(ConvertedBuffer.get());
			::MultiByteToWideChar(CP_ACP, 0, reinterpret_cast<char*>(Buffer.get()), FileSize.LowPart, pszDst, Length);
			pszDst[Length] = L'\0';
			Buffer = std::move(ConvertedBuffer);
			pszBuffer = pszDst;
		}
	} catch (...) {
		::CloseHandle(hFile);
		return false;
	}

	::CloseHandle(hFile);

	LPCTSTR p = pszBuffer;

	while (*p != _T('\0')) {
		String::size_type Length;
		for (Length = 0; p[Length] != _T('\0') && p[Length] != _T('\r') && p[Length] != _T('\n'); Length++);
		if (Length > 0) {
			String Line;
			Line.assign(p, Length);
			std::vector<String> Items;
			StringUtility::Split(Line, TEXT(","), &Items);
			if (Items.size() >= 1) {
				std::vector<String> PathItems;
				String Temp;
				StringUtility::Split(Items[0], TEXT("\\"), &PathItems);
				for (String &e : PathItems) {
					if (!e.empty()) {
						StringUtility::Decode(e.c_str(), &Temp);
						e = Temp;
					}
				}
				CFavoriteFolder *pFolder = &m_RootFolder;
				for (size_t i = 0; i + 1 < PathItems.size(); i++) {
					CFavoriteFolder *p = pFolder->FindSubFolder(PathItems[i].c_str());
					if (p == nullptr) {
						p = new CFavoriteFolder;
						p->SetName(PathItems[i].c_str());
						pFolder->AddItem(p);
					}
					pFolder = p;
				}
				if (Items.size() >= 8) {
					CChannelInfo ChannelInfo(
						::wcstol(Items[2].c_str(), nullptr, 0),
						::wcstol(Items[3].c_str(), nullptr, 0),
						::wcstol(Items[4].c_str(), nullptr, 0),
						PathItems[PathItems.size() - 1].c_str());
					ChannelInfo.SetServiceID(static_cast<WORD>(::wcstol(Items[5].c_str(), nullptr, 0)));
					ChannelInfo.SetNetworkID(static_cast<WORD>(::wcstol(Items[6].c_str(), nullptr, 0)));
					ChannelInfo.SetTransportStreamID(static_cast<WORD>(::wcstol(Items[7].c_str(), nullptr, 0)));

					CFavoriteChannel *pChannel = new CFavoriteChannel(ChannelInfo);
					pChannel->SetName(PathItems[PathItems.size() - 1].c_str());
					pChannel->SetBonDriverFileName(Items[1].c_str());

					if (Items.size() >= 9) {
						const unsigned long Flags = ::wcstoul(Items[8].c_str(), nullptr, 0);
						pChannel->SetForceBonDriverChange((Flags & CHANNEL_FLAG_FORCEBONDRIVERCHANGE) != 0);
					}

					pFolder->AddItem(pChannel);
				}
			}

			p += Length;
		}

		while (*p == _T('\r') || *p == _T('\n'))
			p++;
	}

	return true;
}

bool CFavoritesManager::Save(LPCTSTR pszFileName) const
{
	if (IsStringEmpty(pszFileName))
		return false;

	String Buffer;

	SaveFolder(&m_RootFolder, String(), &Buffer);

	const HANDLE hFile = ::CreateFile(
		pszFileName, GENERIC_WRITE, 0, nullptr,
		CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (hFile == INVALID_HANDLE_VALUE)
		return false;

	DWORD Write;
	static const WORD BOM = 0xFEFF;
	const DWORD Size = static_cast<DWORD>(Buffer.length() * sizeof(String::value_type));
	if (!::WriteFile(hFile, &BOM, sizeof(BOM), &Write, nullptr)
			|| Write != sizeof(BOM)
			|| !::WriteFile(hFile, Buffer.data(), Size, &Write, nullptr)
			|| Write != Size) {
		::CloseHandle(hFile);
		return false;
	}

	::CloseHandle(hFile);

	return true;
}

void CFavoritesManager::SaveFolder(const CFavoriteFolder *pFolder, const String &Path, String *pBuffer) const
{
	for (size_t i = 0; i < pFolder->GetItemCount(); i++) {
		const CFavoriteItem *pItem = pFolder->GetItem(i);

		switch (pItem->GetType()) {
		case CFavoriteItem::ItemType::Folder:
			{
				const CFavoriteFolder *pFolder = dynamic_cast<const CFavoriteFolder*>(pItem);
				if (pFolder != nullptr) {
					String FolderPath(Path), Name;
					StringUtility::Encode(pFolder->GetName(), &Name);
					FolderPath += Name;
					FolderPath += TEXT("\\");
					if (pFolder->GetItemCount() > 0) {
						SaveFolder(pFolder, FolderPath, pBuffer);
					} else {
						*pBuffer += FolderPath;
						*pBuffer += TEXT("\r\n");
					}
				}
			}
			break;

		case CFavoriteItem::ItemType::Channel:
			{
				const CFavoriteChannel *pChannel = dynamic_cast<const CFavoriteChannel*>(pItem);
				if (pChannel != nullptr) {
					const CChannelInfo &ChannelInfo = pChannel->GetChannelInfo();
					String Name, ChannelSpec;
					StringUtility::Encode(pChannel->GetName(), &Name);
					unsigned int Flags = 0;
					if (pChannel->GetForceBonDriverChange())
						Flags |= CHANNEL_FLAG_FORCEBONDRIVERCHANGE;
					StringFormat(
						&ChannelSpec, TEXT(",{},{},{},{},{},{},{},{}"),
						pChannel->GetBonDriverFileName(),
						ChannelInfo.GetSpace(),
						ChannelInfo.GetChannelIndex(),
						ChannelInfo.GetChannelNo(),
						ChannelInfo.GetServiceID(),
						ChannelInfo.GetNetworkID(),
						ChannelInfo.GetTransportStreamID(),
						Flags);
					*pBuffer += Path;
					*pBuffer += Name;
					*pBuffer += ChannelSpec;
					*pBuffer += TEXT("\r\n");
				}
			}
			break;
		}
	}

}

bool CFavoritesManager::ShowOrganizeDialog(HWND hwndOwner)
{
	return m_OrganizeDialog.Show(hwndOwner);
}

bool CFavoritesManager::GetOrganizeDialogPos(CBasicDialog::Position *pPos) const
{
	return m_OrganizeDialog.GetPosition(pPos);
}

bool CFavoritesManager::SetOrganizeDialogPos(const CBasicDialog::Position &Pos)
{
	return m_OrganizeDialog.SetPosition(Pos);
}

bool CFavoritesManager::IsOrganizeDialogPosSet() const
{
	return m_OrganizeDialog.IsPositionSet();
}


class CFavoritesMenu::CFolderItem
	: public CFavoritesMenu::CMenuItem
{
	const CFavoriteFolder *m_pFolder;
	int m_TextWidth = 0;

public:
	CFolderItem(const CFavoriteFolder *pFolder);
	LPCTSTR GetName() const { return m_pFolder->GetName(); }
	int GetTextWidth() const { return m_TextWidth; }
	void SetTextWidth(int Width) { m_TextWidth = Width; }
};

CFavoritesMenu::CFolderItem::CFolderItem(const CFavoriteFolder *pFolder)
	: m_pFolder(pFolder)
{
}

class CFavoritesMenu::CChannelItem
	: public CFavoritesMenu::CMenuItem
{
	const CFavoriteChannel *m_pChannel;
	struct Event
	{
		bool fValid = false;
		LibISDB::EventInfo EventInfo;
	};
	Event m_EventList[2];
	int m_NameWidth = 0;
	int m_EventWidth = 0;

public:
	CChannelItem(const CFavoriteChannel *pChannel);
	const LibISDB::EventInfo *GetEventInfo(
		LibISDB::EPGDatabase *pEPGDatabase, int Index, const LibISDB::DateTime *pCurTime = nullptr);
	const LibISDB::EventInfo *GetEventInfo(int Index) const;
	LPCTSTR GetName() const { return m_pChannel->GetName(); }
	const CChannelInfo &GetChannelInfo() const { return m_pChannel->GetChannelInfo(); }
	int GetNameWidth() const { return m_NameWidth; }
	void SetNameWidth(int Width) { m_NameWidth = Width; }
	int GetEventWidth() const { return m_EventWidth; }
	void SetEventWidth(int Width) { m_EventWidth = Width; }
};

CFavoritesMenu::CChannelItem::CChannelItem(const CFavoriteChannel *pChannel)
	: m_pChannel(pChannel)
{
}

const LibISDB::EventInfo *CFavoritesMenu::CChannelItem::GetEventInfo(
	LibISDB::EPGDatabase *pEPGDatabase, int Index, const LibISDB::DateTime *pCurTime)
{
	if (Index < 0 || Index >= lengthof(m_EventList)
			|| (Index > 0 && !m_EventList[Index - 1].fValid)
			|| m_pChannel->GetChannelInfo().GetServiceID() == 0)
		return nullptr;

	if (!m_EventList[Index].fValid) {
		LibISDB::DateTime Time;

		if (Index == 0) {
			if (pCurTime != nullptr)
				Time = *pCurTime;
			else
				LibISDB::GetCurrentEPGTime(&Time);
		} else {
			if (!m_EventList[Index - 1].EventInfo.GetEndTime(&Time))
				return nullptr;
		}
		const CChannelInfo &ChannelInfo = GetChannelInfo();
		if (!pEPGDatabase->GetEventInfo(
					ChannelInfo.GetNetworkID(),
					ChannelInfo.GetTransportStreamID(),
					ChannelInfo.GetServiceID(),
					Time, &m_EventList[Index].EventInfo))
			return nullptr;
		m_EventList[Index].fValid = true;
	}

	return &m_EventList[Index].EventInfo;
}

const LibISDB::EventInfo *CFavoritesMenu::CChannelItem::GetEventInfo(int Index) const
{
	if (Index < 0 || Index >= lengthof(m_EventList) || !m_EventList[Index].fValid)
		return nullptr;
	return &m_EventList[Index].EventInfo;
}


CFavoritesMenu::~CFavoritesMenu()
{
	Destroy();
}

bool CFavoritesMenu::Create(
	const CFavoriteFolder *pFolder, UINT Command,
	HMENU hmenu, HWND hwnd, CreateFlag Flags, int DPI)
{
	if (pFolder == nullptr)
		return false;

	Destroy();

	if (DPI <= 0) {
		/*
			本来はメニューそのものの DPI が必要だが、WM_MEASUREITEM で DPI を取得できないため
			とりあえずウィンドウの DPI を使っている
		*/
		DPI = GetWindowDPI(hwnd);
	}

	m_FirstCommand = Command;
	m_Flags = Flags;
	m_hwnd = hwnd;

	m_IconWidth = GetSystemMetricsWithDPI(SM_CXSMICON, DPI);
	m_IconHeight = GetSystemMetricsWithDPI(SM_CYSMICON, DPI);

	m_MenuPainter.Initialize(hwnd, DPI);
	m_MenuPainter.GetItemMargins(&m_Margins);
	if (m_Margins.cxLeftWidth < 2)
		m_Margins.cxLeftWidth = 2;
	if (m_Margins.cxRightWidth < 2)
		m_Margins.cxRightWidth = 2;

	const HDC hdc = ::GetDC(hwnd);

	CreateFont(hdc);
	const HFONT hfontOld = DrawUtil::SelectObject(hdc, m_Font);

	m_ItemHeight = m_TextHeight;
	if (!!(Flags & CreateFlag::ShowLogo)) {
		m_Logo.Initialize(m_IconHeight);
		const int Height = std::max(m_Logo.GetLogoHeight(), m_IconHeight);
		if (Height > m_ItemHeight)
			m_ItemHeight = Height;
	}
	m_ItemHeight += m_Margins.cyTopHeight + m_Margins.cyBottomHeight;
	m_TextWidth = 0;

	GetBaseTime(&m_BaseTime);
	if (hmenu == nullptr) {
		m_hmenu = ::CreatePopupMenu();
	} else {
		m_hmenu = hmenu;
		m_Flags |= CreateFlag::Shared;
		ClearMenu(hmenu);
	}

	const HINSTANCE hinstRes = GetAppClass().GetResourceInstance();
	TCHAR szText[64];

	if (pFolder->GetItemCount() > 0) {
		SetFolderMenu(m_hmenu, 0, hdc, &Command, pFolder);
		m_LastCommand = Command - 1;

		int MenuPos = ::GetMenuItemCount(m_hmenu);

		MENUITEMINFO mii;
		mii.cbSize = sizeof(MENUITEMINFO);
		mii.fMask = MIIM_FTYPE | MIIM_STATE | MIIM_ID;
		mii.fType = MFT_SEPARATOR;
		mii.fState = MFS_DISABLED;
		mii.wID = 0;
		::InsertMenuItem(m_hmenu, MenuPos, TRUE, &mii);
		MenuPos++;

		static const struct {
			UINT Command;
			UINT TextID;
		} MenuItemList[] = {
			{CM_ADDTOFAVORITES,    IDS_MENU_ADDTOFAVORITES},
			{CM_ORGANIZEFAVORITES, IDS_MENU_ORGANIZEFAVORITES},
		};
		mii.fMask = MIIM_FTYPE | MIIM_STATE | MIIM_ID;
		mii.fType = MFT_OWNERDRAW;
		mii.fState = MFS_ENABLED;

		for (const auto &e : MenuItemList) {
			mii.wID = e.Command;
			::InsertMenuItem(m_hmenu, MenuPos, TRUE, &mii);
			MenuPos++;

			const int Length = ::LoadString(hinstRes, e.TextID, szText, lengthof(szText));
			RECT rc;
			m_MenuPainter.GetItemTextExtent(hdc, 0, szText, &rc);
			if (rc.right > m_TextWidth)
				m_TextWidth = rc.right;
		}

		m_himlIcons = CreateFavoritesIconImageList(hinstRes, m_IconWidth, m_IconHeight);
	} else {
		m_LastCommand = Command;

		::AppendMenu(hmenu, MF_STRING | MF_GRAYED, 0, TEXT("なし"));
		::AppendMenu(hmenu, MF_SEPARATOR, 0, nullptr);
		::LoadString(hinstRes, IDS_MENU_ADDTOFAVORITES, szText, lengthof(szText));
		::AppendMenu(hmenu, MF_STRING | MF_ENABLED, CM_ADDTOFAVORITES, szText);
		::LoadString(hinstRes, IDS_MENU_ORGANIZEFAVORITES, szText, lengthof(szText));
		::AppendMenu(hmenu, MF_STRING | MF_ENABLED, CM_ORGANIZEFAVORITES, szText);
	}

	::SelectObject(hdc, hfontOld);
	::ReleaseDC(hwnd, hdc);

	if (!!(Flags & CreateFlag::ShowToolTip)) {
		m_Tooltip.Create(hwnd);
		m_Tooltip.SetFont(m_Font.GetHandle());
		m_Tooltip.SetMaxWidth(m_TextHeight * 40);
		m_Tooltip.SetPopDelay(30 * 1000);
		m_Tooltip.AddTrackingTip(1, TEXT(""));
	}

	return true;
}

void CFavoritesMenu::SetFolderMenu(HMENU hmenu, int MenuPos, HDC hdc, UINT *pCommand, const CFavoriteFolder *pFolder)
{
	int ChannelNameWidth = 0;
	RECT rc;

	for (size_t i = 0; i < pFolder->GetItemCount(); i++) {
		const CFavoriteItem *pItem = pFolder->GetItem(i);

		if (pItem->GetType() == CFavoriteItem::ItemType::Channel) {
			const CFavoriteChannel *pChannel = dynamic_cast<const CFavoriteChannel*>(pItem);

			if (pChannel != nullptr) {
				const LPCTSTR pszChannelName = pChannel->GetName();
				m_MenuPainter.GetItemTextExtent(
					hdc, 0, pszChannelName, &rc,
					DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX);
				if (rc.right > ChannelNameWidth)
					ChannelNameWidth = rc.right;
			}
		}
	}

	MENUITEMINFO mii;
	mii.cbSize = sizeof(MENUITEMINFO);
	mii.fType = MFT_OWNERDRAW;
	mii.fState = MFS_ENABLED;

	for (size_t i = 0; i < pFolder->GetItemCount(); i++) {
		const CFavoriteItem *pItem = pFolder->GetItem(i);

		switch (pItem->GetType()) {
		case CFavoriteItem::ItemType::Folder:
			{
				const CFavoriteFolder *pSubFolder =
					dynamic_cast<const CFavoriteFolder*>(pItem);

				if (pSubFolder != nullptr) {
					CFolderItem *pMenuItem = new CFolderItem(pSubFolder);
					m_ItemList.emplace_back(pMenuItem);

					const HMENU hmenuSub = ::CreatePopupMenu();
					mii.fMask = MIIM_FTYPE | MIIM_STATE | MIIM_ID | MIIM_SUBMENU | MIIM_DATA;
					mii.wID = CM_FAVORITESSUBMENU;
					mii.hSubMenu = hmenuSub;
					mii.dwItemData = reinterpret_cast<ULONG_PTR>(pMenuItem);
					::InsertMenuItem(hmenu, MenuPos, TRUE, &mii);
					SetFolderMenu(hmenuSub, 0, hdc, pCommand, pSubFolder);
					MenuPos++;

					const LPCTSTR pszName = pSubFolder->GetName();
					m_MenuPainter.GetItemTextExtent(
						hdc, 0, pszName, &rc,
						DT_LEFT | DT_SINGLELINE | DT_NOPREFIX);
					pMenuItem->SetTextWidth(rc.right);
				}
			}
			break;

		case CFavoriteItem::ItemType::Channel:
			{
				const CFavoriteChannel *pChannel =
					dynamic_cast<const CFavoriteChannel*>(pItem);

				if (pChannel != nullptr) {
					CChannelItem *pMenuItem = new CChannelItem(pChannel);
					pMenuItem->SetNameWidth(ChannelNameWidth);
					m_ItemList.emplace_back(pMenuItem);

					mii.fMask = MIIM_FTYPE | MIIM_STATE | MIIM_ID | MIIM_DATA;
					mii.wID = *pCommand;
					mii.dwItemData = reinterpret_cast<ULONG_PTR>(pMenuItem);
					::InsertMenuItem(hmenu, MenuPos, TRUE, &mii);
					(*pCommand)++;
					MenuPos++;

					if (!!(m_Flags & CreateFlag::ShowEventInfo)) {
						const LibISDB::EventInfo *pEventInfo =
							pMenuItem->GetEventInfo(&GetAppClass().EPGDatabase, 0, &m_BaseTime);
						if (pEventInfo != nullptr) {
							TCHAR szText[256];
							GetEventText(pEventInfo, szText, lengthof(szText));
							m_MenuPainter.GetItemTextExtent(
								hdc, 0, szText, &rc,
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
		if (!(m_Flags & CreateFlag::Shared))
			::DestroyMenu(m_hmenu);
		else
			ClearMenu(m_hmenu);
		m_hmenu = nullptr;
	}

	m_MenuPainter.Finalize();
	m_Tooltip.Destroy();
	m_hwnd = nullptr;

	m_ItemList.clear();

	if (m_himlIcons != nullptr) {
		::ImageList_Destroy(m_himlIcons);
		m_himlIcons = nullptr;
	}
}

bool CFavoritesMenu::Show(UINT Flags, int x, int y)
{
	if (m_hmenu == nullptr)
		return false;
	::TrackPopupMenu(m_hmenu, Flags, x, y, 0, m_hwnd, nullptr);
	return true;
}

bool CFavoritesMenu::OnMeasureItem(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	LPMEASUREITEMSTRUCT pmis = reinterpret_cast<LPMEASUREITEMSTRUCT>(lParam);

	if (m_hmenu != nullptr && pmis->CtlType == ODT_MENU) {
		if (pmis->itemID >= m_FirstCommand && pmis->itemID <= m_LastCommand) {
			if (pmis->itemData == 0)
				return false;
			const CChannelItem *pItem = reinterpret_cast<const CChannelItem*>(pmis->itemData);
			pmis->itemWidth = pItem->GetNameWidth() + m_Margins.cxLeftWidth + m_Margins.cxRightWidth;
			if (!!(m_Flags & CreateFlag::ShowLogo))
				pmis->itemWidth += m_Logo.GetLogoWidth() + m_MenuLogoMargin;
			if (!!(m_Flags & CreateFlag::ShowEventInfo))
				pmis->itemWidth += m_TextHeight + pItem->GetEventWidth();
			pmis->itemHeight = m_ItemHeight;
			return true;
		} else if (pmis->itemID == CM_FAVORITESSUBMENU) {
			const CFolderItem *pItem = reinterpret_cast<const CFolderItem*>(pmis->itemData);
			if (pItem == nullptr)
				return false;
			pmis->itemWidth = pItem->GetTextWidth() + m_Margins.cxLeftWidth + m_Margins.cxRightWidth;
			if (!!(m_Flags & CreateFlag::ShowLogo))
				pmis->itemWidth += m_Logo.GetLogoWidth() + m_MenuLogoMargin;
			pmis->itemHeight = m_ItemHeight;
			return true;
		} else if (pmis->itemID == CM_ADDTOFAVORITES
				|| pmis->itemID == CM_ORGANIZEFAVORITES) {
			pmis->itemWidth = m_TextWidth + m_Margins.cxLeftWidth + m_Margins.cxRightWidth;
			if (!!(m_Flags & CreateFlag::ShowLogo))
				pmis->itemWidth += m_Logo.GetLogoWidth() + m_MenuLogoMargin;
			pmis->itemHeight = m_ItemHeight;
			return true;
		}
	}
	return false;
}

bool CFavoritesMenu::OnDrawItem(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	LPDRAWITEMSTRUCT pdis = reinterpret_cast<LPDRAWITEMSTRUCT>(lParam);

	if (m_hmenu == nullptr || hwnd != m_hwnd || pdis->CtlType != ODT_MENU
			|| ((pdis->itemID < m_FirstCommand || pdis->itemID > m_LastCommand)
				&& pdis->itemID != CM_ADDTOFAVORITES
				&& pdis->itemID != CM_ORGANIZEFAVORITES
				&& pdis->itemID != CM_FAVORITESSUBMENU))
		return false;

	TCHAR szText[256];

	m_MenuPainter.DrawItemBackground(pdis->hDC, pdis->rcItem, pdis->itemState);
	const COLORREF crTextColor = m_MenuPainter.GetTextColor(pdis->itemState);
	const COLORREF crOldTextColor = ::SetTextColor(pdis->hDC, crTextColor);
	const int OldBkMode = ::SetBkMode(pdis->hDC, TRANSPARENT);
	const HFONT hfontOld = DrawUtil::SelectObject(pdis->hDC, m_Font);

	RECT rc;
	rc.left = pdis->rcItem.left + m_Margins.cxLeftWidth;
	rc.top = pdis->rcItem.top + m_Margins.cyTopHeight;
	rc.right = pdis->rcItem.right - m_Margins.cxRightWidth;
	rc.bottom = pdis->rcItem.bottom - m_Margins.cyBottomHeight;

	if (pdis->itemID >= m_FirstCommand && pdis->itemID <= m_LastCommand) {
		if (pdis->itemData == 0)
			return false;
		const CChannelItem *pItem = reinterpret_cast<const CChannelItem*>(pdis->itemData);

		if (!!(m_Flags & CreateFlag::ShowLogo)) {
			m_Logo.DrawLogo(
				pdis->hDC,
				rc.left,
				rc.top + ((rc.bottom - rc.top) - m_Logo.GetLogoHeight()) / 2,
				pItem->GetChannelInfo());
			rc.left += m_Logo.GetLogoWidth() + m_MenuLogoMargin;
		}

		rc.right = rc.left + pItem->GetNameWidth();
		m_MenuPainter.DrawItemText(
			pdis->hDC, pdis->itemState, pItem->GetName(), rc,
			DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

		if (!!(m_Flags & CreateFlag::ShowEventInfo)) {
			const LibISDB::EventInfo *pEventInfo = pItem->GetEventInfo(0);
			if (pEventInfo != nullptr) {
				GetEventText(pEventInfo, szText, lengthof(szText));
				rc.left = rc.right + m_TextHeight;
				rc.right = pdis->rcItem.right - m_Margins.cxRightWidth;
				m_MenuPainter.DrawItemText(
					pdis->hDC, pdis->itemState, szText, rc,
					DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
			}
		}
	} else {
		if (pdis->itemID == CM_ADDTOFAVORITES || pdis->itemID == CM_ORGANIZEFAVORITES) {
			if (!!(m_Flags & CreateFlag::ShowLogo)) {
				m_MenuPainter.DrawIcon(
					m_himlIcons,
					pdis->itemID == CM_ADDTOFAVORITES ? FAVORITES_ICON_ADD : FAVORITES_ICON_ORGANIZE,
					pdis->hDC,
					rc.left + (m_Logo.GetLogoWidth() - m_IconWidth) / 2,
					rc.top + (rc.bottom - rc.top - m_IconHeight) / 2,
					pdis->itemState);
				rc.left += m_Logo.GetLogoWidth() + m_MenuLogoMargin;
			}
			::LoadString(
				GetAppClass().GetResourceInstance(),
				pdis->itemID == CM_ADDTOFAVORITES ? IDS_MENU_ADDTOFAVORITES : IDS_MENU_ORGANIZEFAVORITES,
				szText, lengthof(szText));
			m_MenuPainter.DrawItemText(pdis->hDC, pdis->itemState, szText, rc);
		} else if (pdis->itemID == CM_FAVORITESSUBMENU) {
			const CFolderItem *pItem = reinterpret_cast<const CFolderItem*>(pdis->itemData);
			if (!!(m_Flags & CreateFlag::ShowLogo)) {
				m_MenuPainter.DrawIcon(
					m_himlIcons, FAVORITES_ICON_FOLDER, pdis->hDC,
					rc.left + (m_Logo.GetLogoWidth() - m_IconWidth) / 2,
					rc.top + (rc.bottom - rc.top - m_IconHeight) / 2,
					pdis->itemState);
				rc.left += m_Logo.GetLogoWidth() + m_MenuLogoMargin;
			}
			m_MenuPainter.DrawItemText(
				pdis->hDC, pdis->itemState, pItem->GetName(), rc,
				DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
		}
	}

	::SelectObject(pdis->hDC, hfontOld);
	::SetBkMode(pdis->hDC, OldBkMode);
	::SetTextColor(pdis->hDC, crOldTextColor);

	return true;
}

bool CFavoritesMenu::OnMenuSelect(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	const HMENU hmenu = reinterpret_cast<HMENU>(lParam);
	const UINT Command = LOWORD(wParam);

	if (hmenu == nullptr || hmenu != m_hmenu || hwnd != m_hwnd || HIWORD(wParam) == 0xFFFF
			|| Command < m_FirstCommand || Command > m_LastCommand) {
		if (m_Tooltip.IsVisible())
			m_Tooltip.TrackActivate(1, false);
		return false;
	}

	if (!!(m_Flags & CreateFlag::ShowToolTip)) {
		MENUITEMINFO mii;

		mii.cbSize = sizeof(mii);
		mii.fMask = MIIM_DATA;
		if (::GetMenuItemInfo(hmenu, Command, FALSE, &mii)) {
			CChannelItem *pItem = reinterpret_cast<CChannelItem*>(mii.dwItemData);
			if (pItem == nullptr)
				return false;

			const LibISDB::EventInfo *pEventInfo1, *pEventInfo2;
			pEventInfo1 = pItem->GetEventInfo(0);
			if (pEventInfo1 == nullptr) {
				pEventInfo1 = pItem->GetEventInfo(&GetAppClass().EPGDatabase, 0);
			}
			if (pEventInfo1 != nullptr) {
				TCHAR szText[256 * 2 + 1];
				int Length;
				POINT pt;

				Length = GetEventText(pEventInfo1, szText, lengthof(szText) / 2);
				pEventInfo2 = pItem->GetEventInfo(&GetAppClass().EPGDatabase, 1);
				if (pEventInfo2 != nullptr) {
					szText[Length++] = _T('\r');
					szText[Length++] = _T('\n');
					GetEventText(pEventInfo2, szText + Length, lengthof(szText) / 2);
				}
				m_Tooltip.SetText(1, szText);
				::GetCursorPos(&pt);
				pt.x += 16;
				pt.y += std::max(m_TextHeight, m_Logo.GetLogoHeight()) +
					m_Margins.cyTopHeight + m_Margins.cyBottomHeight;
				m_Tooltip.TrackPosition(pt.x, pt.y);
				m_Tooltip.TrackActivate(1, true);
			} else {
				m_Tooltip.TrackActivate(1, false);
			}
		}
	}
	return true;
}

bool CFavoritesMenu::OnUninitMenuPopup(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	if (hwnd == m_hwnd && reinterpret_cast<HMENU>(wParam) == m_hmenu) {
		Destroy();
		return true;
	}
	return false;
}

int CFavoritesMenu::GetEventText(
	const LibISDB::EventInfo *pEventInfo, LPTSTR pszText, int MaxLength) const
{
	TCHAR szTime[EpgUtil::MAX_EVENT_TIME_LENGTH];

	EpgUtil::FormatEventTime(
		*pEventInfo, szTime, lengthof(szTime), EpgUtil::FormatEventTimeFlag::Hour2Digits);

	return static_cast<int>(StringFormat(
		pszText, MaxLength, TEXT("{} {}"),
		szTime, pEventInfo->EventName));
}

void CFavoritesMenu::CreateFont(HDC hdc)
{
	LOGFONT lf;
	m_MenuPainter.GetFont(&lf);
	m_Font.Create(&lf);

	if (hdc != nullptr)
		m_TextHeight = m_Font.GetHeight(hdc);
	else
		m_TextHeight = std::abs(lf.lfHeight);
}

void CFavoritesMenu::GetBaseTime(LibISDB::DateTime *pTime)
{
	LibISDB::GetCurrentEPGTime(pTime);
	pTime->OffsetMinutes(2);
}


class CFavoritePropertiesDialog
	: public CBasicDialog
{
public:
	CFavoritePropertiesDialog(CFavoriteChannel *pChannel);
	~CFavoritePropertiesDialog();
	bool Show(HWND hwndOwner) override;

private:
	INT_PTR DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

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
	return ShowDialog(
		hwndOwner, GetAppClass().GetResourceInstance(),
		MAKEINTRESOURCE(IDD_FAVORITEPROPERTIES)) == IDOK;
}

INT_PTR CFavoritePropertiesDialog::DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			const CChannelInfo &ChannelInfo = m_pChannel->GetChannelInfo();
			TCHAR szText[64];

			::SetDlgItemText(hDlg, IDC_FAVORITEPROP_NAME, m_pChannel->GetName());

			const CDriverManager &DriverManager = GetAppClass().DriverManager;
			for (int i = 0; i < DriverManager.NumDrivers(); i++) {
				const CDriverInfo *pDriverInfo = DriverManager.GetDriverInfo(i);
				DlgComboBox_AddString(hDlg, IDC_INITIALSETTINGS_DRIVER, pDriverInfo->GetFileName());
			}
			::SetDlgItemText(hDlg, IDC_FAVORITEPROP_BONDRIVER, m_pChannel->GetBonDriverFileName());

			DlgCheckBox_Check(hDlg, IDC_FAVORITEPROP_FORCEBONDRIVERCHANGE, m_pChannel->GetForceBonDriverChange());

			StringFormat(
				szText, TEXT("{0} ({0:#04x})"),
				ChannelInfo.GetServiceID());
			::SetDlgItemText(hDlg, IDC_FAVORITEPROP_SERVICEID, szText);
			StringFormat(
				szText, TEXT("{0} ({0:#04x})"),
				ChannelInfo.GetNetworkID());
			::SetDlgItemText(hDlg, IDC_FAVORITEPROP_NETWORKID, szText);
			StringFormat(
				szText, TEXT("{0} ({0:#04x})"),
				ChannelInfo.GetTransportStreamID());
			::SetDlgItemText(hDlg, IDC_FAVORITEPROP_TRANSPORTSTREAMID, szText);
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			{
				TCHAR szText[256];

				::GetDlgItemText(hDlg, IDC_FAVORITEPROP_NAME, szText, lengthof(szText));
				m_pChannel->SetName(szText);
				::GetDlgItemText(hDlg, IDC_FAVORITEPROP_BONDRIVER, szText, lengthof(szText));
				m_pChannel->SetBonDriverFileName(szText);
				m_pChannel->SetForceBonDriverChange(
					DlgCheckBox_IsChecked(hDlg, IDC_FAVORITEPROP_FORCEBONDRIVERCHANGE));
			}
			[[fallthrough]];
		case IDCANCEL:
			::EndDialog(hDlg, LOWORD(wParam));
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
	return ShowDialog(
		hwndOwner,
		GetAppClass().GetResourceInstance(),
		MAKEINTRESOURCE(IDD_ORGANIZEFAVORITES)) == IDOK;
}

INT_PTR COrganizeFavoritesDialog::DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			const HINSTANCE hinstRes = GetAppClass().GetResourceInstance();
			const HWND hwndTree = ::GetDlgItem(hDlg, IDC_FAVORITES_FOLDERTREE);

			m_IconWidth = GetSystemMetricsWithDPI(SM_CXSMICON, m_CurrentDPI);
			m_IconHeight = GetSystemMetricsWithDPI(SM_CYSMICON, m_CurrentDPI);
			const HIMAGELIST himl = CreateFavoritesIconImageList(hinstRes, m_IconWidth, m_IconHeight);
			ImageList_SetBkColor(himl, TreeView_GetBkColor(hwndTree));
			TreeView_SetImageList(hwndTree, himl, TVSIL_NORMAL);

			TreeView_SetInsertMarkColor(hwndTree, GetThemeColor(COLOR_HIGHLIGHT));

			if (m_pManager->GetRootFolder().GetItemCount() > 0)
				InsertTreeItems(hwndTree, TVI_ROOT, &m_pManager->GetRootFolder());

			AddControl(IDC_FAVORITES_FOLDERTREE, AlignFlag::All);
			AddControl(IDC_FAVORITES_NEWFOLDER, AlignFlag::Bottom);
			AddControl(IDOK, AlignFlag::BottomRight);
			AddControl(IDCANCEL, AlignFlag::BottomRight);

			m_fItemDragging = false;
			m_himlDrag = nullptr;

			SetWindowIcon(hDlg, hinstRes, MAKEINTRESOURCE(IDI_FAVORITES));

			ApplyPosition();
		}
		return TRUE;

	case WM_MOUSEMOVE:
		if (m_fItemDragging) {
			const HWND hwndTree = ::GetDlgItem(hDlg, IDC_FAVORITES_FOLDERTREE);
			POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};

			ImageList_DragShowNolock(FALSE);
			TVHITTESTINFO tvhti;
			tvhti.pt = pt;
			::MapWindowPoints(hDlg, hwndTree, &tvhti.pt, 1);
			HTREEITEM hItem = TreeView_HitTest(hwndTree, &tvhti);
			if (hItem != nullptr) {
				if (hItem == m_hDraggingItem) {
					hItem = nullptr;
				} else {
					HTREEITEM hParent = TreeView_GetParent(hwndTree, hItem);
					while (hParent != nullptr) {
						if (hParent == m_hDraggingItem) {
							hItem = nullptr;
							break;
						}
						hParent = TreeView_GetParent(hwndTree, hParent);
					}
				}
			}
			if (hItem != nullptr) {
				TVITEM tvi;
				tvi.mask = TVIF_PARAM;
				tvi.hItem = hItem;
				if (TreeView_GetItem(hwndTree, &tvi)) {
					RECT rc;
					TreeView_GetItemRect(hwndTree, hItem, &rc, TRUE);
					const CFavoriteItem *pItem = reinterpret_cast<const CFavoriteItem*>(tvi.lParam);
					if (pItem->GetType() == CFavoriteItem::ItemType::Folder) {
						m_fDropInsertAfter = tvhti.pt.y >= rc.bottom - (rc.bottom - rc.top) / 3;
						m_fDropToFolder = !m_fDropInsertAfter && tvhti.pt.y >= rc.top + (rc.bottom - rc.top) / 3;
						if (m_fDropToFolder) {
							TreeView_SetInsertMark(hwndTree, nullptr, FALSE);
							TreeView_SelectDropTarget(hwndTree, hItem);
						} else {
							TreeView_SelectDropTarget(hwndTree, nullptr);
							TreeView_SetInsertMark(hwndTree, hItem, m_fDropInsertAfter);
						}
					} else {
						m_fDropToFolder = false;
						m_fDropInsertAfter = tvhti.pt.y >= (rc.top + rc.bottom) / 2;
						TreeView_SelectDropTarget(hwndTree, nullptr);
						TreeView_SetInsertMark(hwndTree, hItem, m_fDropInsertAfter);
					}
					m_hDropTargetItem = hItem;
				}
			} else {
				if (m_hDropTargetItem != nullptr) {
					m_hDropTargetItem = nullptr;
					TreeView_SelectDropTarget(hwndTree, nullptr);
					TreeView_SetInsertMark(hwndTree, nullptr, FALSE);
				}
			}
			ImageList_DragShowNolock(TRUE);
			::ClientToScreen(hDlg, &pt);
			ImageList_DragMove(pt.x, pt.y);
		}
		return TRUE;

	case WM_LBUTTONUP:
		if (m_fItemDragging) {
			const HWND hwndTree = ::GetDlgItem(hDlg, IDC_FAVORITES_FOLDERTREE);

			ImageList_DragLeave(nullptr);
			ImageList_EndDrag();
			ImageList_Destroy(m_himlDrag);
			m_himlDrag = nullptr;
			::ReleaseCapture();
			m_fItemDragging = false;
			if (m_hDropTargetItem != nullptr) {
				TreeView_SelectDropTarget(hwndTree, nullptr);
				TreeView_SetInsertMark(hwndTree, nullptr, FALSE);
				if (m_fDropToFolder) {
					const HTREEITEM hItem = CopyTreeItems(hwndTree, m_hDraggingItem, m_hDropTargetItem, TVI_LAST);
					if (hItem != nullptr) {
						TreeView_DeleteItem(hwndTree, m_hDraggingItem);
						TVITEM tvi;
						tvi.mask = TVIF_CHILDREN;
						tvi.hItem = m_hDropTargetItem;
						tvi.cChildren = 1;
						TreeView_SetItem(hwndTree, &tvi);
						TreeView_SelectItem(hwndTree, hItem);
					}
				} else {
					HTREEITEM hInsertAfter;
					if (m_fDropInsertAfter) {
						hInsertAfter = m_hDropTargetItem;
					} else {
						hInsertAfter = TreeView_GetPrevSibling(hwndTree, m_hDropTargetItem);
						if (hInsertAfter == nullptr)
							hInsertAfter = TVI_FIRST;
					}
					const HTREEITEM hItem = CopyTreeItems(
						hwndTree, m_hDraggingItem,
						TreeView_GetParent(hwndTree, m_hDropTargetItem),
						hInsertAfter);
					if (hItem != nullptr) {
						TreeView_DeleteItem(hwndTree, m_hDraggingItem);
						TreeView_SelectItem(hwndTree, hItem);
					}
				}
			}
		}
		return TRUE;

	case WM_NOTIFY:
		switch (reinterpret_cast<LPNMHDR>(lParam)->code) {
		case TVN_BEGINDRAG:
			{
				const NMTREEVIEW *pnmtv = reinterpret_cast<const NMTREEVIEW*>(lParam);

				m_hDraggingItem = pnmtv->itemNew.hItem;
				m_himlDrag = TreeView_CreateDragImage(pnmtv->hdr.hwndFrom, m_hDraggingItem);
				ImageList_BeginDrag(m_himlDrag, 0, 8, 8);
				::SetCapture(hDlg);
				m_fItemDragging = true;
				m_hDropTargetItem = nullptr;
				POINT pt = pnmtv->ptDrag;
				::ClientToScreen(pnmtv->hdr.hwndFrom, &pt);
				ImageList_DragEnter(nullptr, pt.x, pt.y);
			}
			return TRUE;

		case TVN_DELETEITEM:
			{
				const NMTREEVIEW *pnmtv = reinterpret_cast<const NMTREEVIEW*>(lParam);

				delete reinterpret_cast<CFavoriteItem*>(pnmtv->itemOld.lParam);
			}
			return TRUE;

		case TVN_BEGINLABELEDIT:
			{
				const NMTVDISPINFO *pnmtvdi = reinterpret_cast<const NMTVDISPINFO*>(lParam);
				const HWND hwndEdit = TreeView_GetEditControl(pnmtvdi->hdr.hwndFrom);
				::SetProp(hwndEdit, TEXT("FavoritesThis"), this);
				m_pOldEditProc = reinterpret_cast<WNDPROC>(
					::SetWindowLongPtr(hwndEdit, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(EditHookProc)));
			}
			return TRUE;

		case TVN_ENDLABELEDIT:
			{
				const NMTVDISPINFO *pnmtvdi = reinterpret_cast<const NMTVDISPINFO*>(lParam);

				if (pnmtvdi->item.pszText != nullptr) {
					LRESULT Result = FALSE;

					if (pnmtvdi->item.pszText[0] != _T('\0')) {
						CFavoriteItem *pItem = reinterpret_cast<CFavoriteItem*>(pnmtvdi->item.lParam);
						switch (pItem->GetType()) {
						case CFavoriteItem::ItemType::Folder:
							{
								CFavoriteFolder *pFolder = dynamic_cast<CFavoriteFolder*>(pItem);
								if (pFolder != nullptr)
									pFolder->SetName(pnmtvdi->item.pszText);
							}
							break;
						case CFavoriteItem::ItemType::Channel:
							{
								CFavoriteChannel *pChannel = dynamic_cast<CFavoriteChannel*>(pItem);
								if (pChannel != nullptr)
									pChannel->SetName(pnmtvdi->item.pszText);
							}
							break;
						}
						Result = TRUE;
					}
					::SetWindowLongPtr(hDlg, DWLP_MSGRESULT, Result);
				}
			}
			return TRUE;

		case NM_RCLICK:
			{
				const NMHDR *pnmh = reinterpret_cast<const NMHDR*>(lParam);
				POINT pt;
				TVHITTESTINFO tvhti;

				::GetCursorPos(&pt);
				tvhti.pt = pt;
				::ScreenToClient(pnmh->hwndFrom, &tvhti.pt);
				const HTREEITEM hItem = TreeView_HitTest(pnmh->hwndFrom, &tvhti);
				if (hItem != nullptr && (tvhti.flags & TVHT_ONITEM) != 0) {
					CFavoriteItem *pItem = GetItem(pnmh->hwndFrom, hItem);
					if (pItem == nullptr)
						break;

					CPopupMenu Menu;
					Menu.Create();
					Menu.Append(IDC_FAVORITES_DELETE, TEXT("削除(&D)"));
					Menu.Append(IDC_FAVORITES_RENAME, TEXT("名前の変更(&R)"));
					if (pItem->GetType() == CFavoriteItem::ItemType::Channel)
						Menu.Append(IDC_FAVORITES_PROPERTIES, TEXT("プロパティ(&P)..."));

					const int Result = Menu.Show(hDlg, &pt, TPM_RIGHTBUTTON | TPM_RETURNCMD);
					switch (Result) {
					case IDC_FAVORITES_DELETE:
						TreeView_DeleteItem(pnmh->hwndFrom, hItem);
						break;
					case IDC_FAVORITES_RENAME:
						::SetFocus(pnmh->hwndFrom);
						TreeView_SelectItem(pnmh->hwndFrom, hItem);
						TreeView_EditLabel(pnmh->hwndFrom, hItem);
						break;
					case IDC_FAVORITES_PROPERTIES:
						{
							TreeView_SelectItem(pnmh->hwndFrom, hItem);
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
				const NMHDR *pnmh = reinterpret_cast<const NMHDR*>(lParam);
				POINT pt;
				TVHITTESTINFO tvhti;

				::GetCursorPos(&pt);
				tvhti.pt = pt;
				::ScreenToClient(pnmh->hwndFrom, &tvhti.pt);
				const HTREEITEM hItem = TreeView_HitTest(pnmh->hwndFrom, &tvhti);
				if (hItem != nullptr && (tvhti.flags & TVHT_ONITEM) != 0) {
					CFavoriteItem *pItem = GetItem(pnmh->hwndFrom, hItem);
					if (pItem != nullptr) {
						TreeView_SelectItem(pnmh->hwndFrom, hItem);
						if (pItem->GetType() == CFavoriteItem::ItemType::Channel) {
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
				const HWND hwndTree = ::GetDlgItem(hDlg, IDC_FAVORITES_FOLDERTREE);
				const HTREEITEM hSelItem = TreeView_GetSelection(hwndTree);

				CFavoriteFolder *pNewFolder = new CFavoriteFolder;
				pNewFolder->SetName(TEXT("新規フォルダ"));

				TVINSERTSTRUCT tvis;
				if (hSelItem == nullptr) {
					tvis.hParent = TVI_ROOT;
				} else {
					if (TreeView_GetChild(hwndTree, hSelItem) != nullptr) {
						tvis.hParent = hSelItem;
					} else {
						tvis.hParent = TreeView_GetParent(hwndTree, hSelItem);
					}
				}
				tvis.hInsertAfter = TVI_LAST;
				tvis.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_CHILDREN | TVIF_PARAM;
				tvis.item.pszText = const_cast<LPTSTR>(pNewFolder->GetName());
				tvis.item.iImage = FAVORITES_ICON_FOLDER;
				tvis.item.iSelectedImage = FAVORITES_ICON_FOLDER;
				tvis.item.cChildren = 0;
				tvis.item.lParam = reinterpret_cast<LPARAM>(pNewFolder);
				const HTREEITEM hItem = TreeView_InsertItem(hwndTree, &tvis);
				::SetFocus(hwndTree);
				TreeView_SelectItem(hwndTree, hItem);
				TreeView_EditLabel(hwndTree, hItem);
			}
			return TRUE;

		case IDOK:
			{
				const HWND hwndTree = ::GetDlgItem(hDlg, IDC_FAVORITES_FOLDERTREE);

				// TreeViewのEditからもIDOKと同じIDで通知が送られてくるという罠
				const HWND hwndEdit = TreeView_GetEditControl(hwndTree);
				if (hwndEdit != nullptr && reinterpret_cast<HWND>(lParam) == hwndEdit)
					break;

				CFavoriteFolder Root;

				GetTreeItems(hwndTree, TVI_ROOT, &Root);
				m_pManager->GetRootFolder() = Root;
				m_pManager->SetModified(true);
			}
			[[fallthrough]];
		case IDCANCEL:
			::EndDialog(hDlg, LOWORD(wParam));
			return TRUE;
		}
		return TRUE;

	case WM_DESTROY:
		{
			const HWND hwndTree = ::GetDlgItem(hDlg, IDC_FAVORITES_FOLDERTREE);
			TreeView_DeleteAllItems(hwndTree);
			const HIMAGELIST himl = TreeView_SetImageList(hwndTree, nullptr, TVSIL_NORMAL);
			if (himl != nullptr)
				ImageList_Destroy(himl);
		}
		return TRUE;
	}

	return FALSE;
}

void COrganizeFavoritesDialog::InsertTreeItems(HWND hwndTree, HTREEITEM hParent, const CFavoriteFolder *pFolder)
{
	for (size_t i = 0; i < pFolder->GetItemCount(); i++) {
		const CFavoriteItem *pItem = pFolder->GetItem(i);

		switch (pItem->GetType()) {
		case CFavoriteItem::ItemType::Folder:
			{
				const CFavoriteFolder *pSubFolder = dynamic_cast<const CFavoriteFolder*>(pItem);
				if (pSubFolder != nullptr) {
					CFavoriteFolder *pNewFolder = new CFavoriteFolder;
					pNewFolder->SetName(pSubFolder->GetName());
					TVINSERTSTRUCT tvis;
					tvis.hParent = hParent;
					tvis.hInsertAfter = TVI_LAST;
					tvis.item.mask = TVIF_STATE | TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_CHILDREN | TVIF_PARAM;
					tvis.item.state = TVIS_EXPANDED;
					tvis.item.stateMask = ~0U;
					tvis.item.pszText = const_cast<LPTSTR>(pSubFolder->GetName());
					tvis.item.iImage = FAVORITES_ICON_FOLDER;
					tvis.item.iSelectedImage = FAVORITES_ICON_FOLDER;
					tvis.item.cChildren = pSubFolder->GetItemCount() > 0;
					tvis.item.lParam = reinterpret_cast<LPARAM>(pNewFolder);
					const HTREEITEM hItem = TreeView_InsertItem(hwndTree, &tvis);
					InsertTreeItems(hwndTree, hItem, pSubFolder);
				}
			}
			break;

		case CFavoriteItem::ItemType::Channel:
			{
				const CFavoriteChannel *pChannel = dynamic_cast<const CFavoriteChannel*>(pItem);
				if (pChannel != nullptr) {
					TVINSERTSTRUCT tvis;
					tvis.hParent = hParent;
					tvis.hInsertAfter = TVI_LAST;
					tvis.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM;
					tvis.item.pszText = const_cast<LPTSTR>(pChannel->GetName());
					tvis.item.iImage = FAVORITES_ICON_EMPTY;
					tvis.item.iSelectedImage = FAVORITES_ICON_EMPTY;
					tvis.item.lParam = reinterpret_cast<LPARAM>(pChannel->Duplicate());

					const CChannelInfo &ChannelInfo = pChannel->GetChannelInfo();
					if (ChannelInfo.GetNetworkID() != 0 && ChannelInfo.GetServiceID() != 0) {
						const HICON hico = GetAppClass().LogoManager.CreateLogoIcon(
							ChannelInfo.GetNetworkID(), ChannelInfo.GetServiceID(),
							m_IconWidth, m_IconHeight);
						if (hico != nullptr) {
							tvis.item.iImage = ImageList_AddIcon(
								TreeView_GetImageList(hwndTree, TVSIL_NORMAL), hico);
							tvis.item.iSelectedImage = tvis.item.iImage;
							::DestroyIcon(hico);
						}
					}

					TreeView_InsertItem(hwndTree, &tvis);
				}
			}
			break;
		}
	}
}

HTREEITEM COrganizeFavoritesDialog::CopyTreeItems(HWND hwndTree, HTREEITEM hSrcItem, HTREEITEM hParent, HTREEITEM hInsertAfter)
{
	TVINSERTSTRUCT tvis;

	tvis.hParent = hParent;
	tvis.hInsertAfter = hInsertAfter;
	tvis.item.mask = TVIF_STATE | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_CHILDREN | TVIF_PARAM;
	tvis.item.hItem = hSrcItem;
	tvis.item.stateMask = TVIS_EXPANDED;
	if (!TreeView_GetItem(hwndTree, &tvis.item))
		return nullptr;
	const CFavoriteItem *pItem = reinterpret_cast<const CFavoriteItem*>(tvis.item.lParam);
	CFavoriteItem *pNewItem = pItem->Duplicate();
	tvis.item.mask |= TVIF_TEXT;
	tvis.item.pszText = const_cast<LPTSTR>(pNewItem->GetName());
	tvis.item.lParam = reinterpret_cast<LPARAM>(pNewItem);
	const HTREEITEM hItem = TreeView_InsertItem(hwndTree, &tvis);
	if (hItem == nullptr) {
		delete pNewItem;
		return nullptr;
	}

	HTREEITEM hChild = TreeView_GetChild(hwndTree, hSrcItem);
	while (hChild != nullptr) {
		CopyTreeItems(hwndTree, hChild, hItem, TVI_LAST);
		hChild = TreeView_GetNextSibling(hwndTree, hChild);
	}

	return hItem;
}

void COrganizeFavoritesDialog::GetTreeItems(HWND hwndTree, HTREEITEM hParent, CFavoriteFolder *pFolder)
{
	TVITEM tvi;

	tvi.mask = TVIF_PARAM;
	tvi.hItem = TreeView_GetChild(hwndTree, hParent);
	while (tvi.hItem != nullptr) {
		TreeView_GetItem(hwndTree, &tvi);
		const CFavoriteItem *pItem = reinterpret_cast<const CFavoriteItem*>(tvi.lParam);
		CFavoriteItem *pNewItem = pItem->Duplicate();
		pFolder->AddItem(pNewItem);
		if (pNewItem->GetType() == CFavoriteItem::ItemType::Folder) {
			CFavoriteFolder *pSubFolder = dynamic_cast<CFavoriteFolder*>(pNewItem);
			if (pSubFolder != nullptr)
				GetTreeItems(hwndTree, tvi.hItem, pSubFolder);
		}
		tvi.hItem = TreeView_GetNextSibling(hwndTree, tvi.hItem);
	}
}

CFavoriteItem *COrganizeFavoritesDialog::GetItem(HWND hwndTree, HTREEITEM hItem)
{
	TVITEM tvi;
	tvi.mask = TVIF_PARAM;
	tvi.hItem = hItem;
	if (!TreeView_GetItem(hwndTree, &tvi))
		return nullptr;
	return reinterpret_cast<CFavoriteItem*>(tvi.lParam);
}

LRESULT CALLBACK COrganizeFavoritesDialog::EditHookProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	const COrganizeFavoritesDialog *pThis = static_cast<COrganizeFavoritesDialog*>(::GetProp(hwnd, TEXT("FavoritesThis")));
	if (pThis == nullptr)
		return ::DefWindowProc(hwnd, uMsg, wParam, lParam);

	switch (uMsg) {
	case WM_GETDLGCODE:
		return DLGC_WANTALLKEYS;
	}

	return ::CallWindowProc(pThis->m_pOldEditProc, hwnd, uMsg, wParam, lParam);
}

}
