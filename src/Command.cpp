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
#include "AppMain.h"
#include "Command.h"
#include "resource.h"
#include "Common/DebugDef.h"


namespace TVTest
{


CCommandManager::CCommandManager()
	: m_CommandStateList(CM_COMMAND_LAST - CM_COMMAND_FIRST + 1, CommandState::None)
{
	static constexpr size_t ReserveSize = 256;
	m_CommandList.reserve(ReserveSize);
	m_CommandTextMap.rehash(ReserveSize);
}


bool CCommandManager::RegisterCommand(
	int FirstID, int LastID, int LastListingID,
	LPCTSTR pszIDText,
	const CommandHandler &Handler,
	LPCTSTR pszText, LPCTSTR pszShortText,
	CommandState State)
{
	if ((FirstID > LastID)
			|| ((FirstID < CM_COMMAND_FIRST) || (FirstID > CM_COMMAND_LAST))
			|| ((LastID < CM_COMMAND_FIRST) || (LastID > CM_COMMAND_LAST))
			|| (LastListingID != 0 && ((LastListingID < FirstID) || (LastListingID > LastID))))
		return false;

	CommandInfo &Info = m_CommandList.emplace_back();

	Info.FirstID = FirstID;
	Info.LastID = LastID;
	Info.LastListingID = LastListingID;
	if (!IsStringEmpty(pszIDText))
		Info.IDText = pszIDText;
	if (!IsStringEmpty(pszText))
		Info.Text = pszText;
	if (!IsStringEmpty(pszShortText))
		Info.ShortText = pszShortText;
	Info.Handler = Handler;

#ifndef _DEBUG
	m_CommandIDMap.emplace(FirstID, m_CommandList.size() - 1);
#else
	if (!m_CommandIDMap.emplace(FirstID, m_CommandList.size() - 1).second) {
		// 識別子重複
		::DebugBreak();
	}
#endif

	if (!Info.IDText.empty()) {
		if (FirstID == LastID) {
			m_CommandTextMap.emplace(Info.IDText, FirstID);
		} else if (LastListingID != 0) {
			for (int ID = FirstID; ID <= LastListingID; ID++) {
				m_CommandTextMap.emplace(GetNumberedText(Info.IDText, ID - FirstID), ID);
			}
		}
	}

	for (int ID = FirstID; ID <= LastID; ID++)
		m_CommandStateList[ID - CM_COMMAND_FIRST] = State;

	return true;
}


bool CCommandManager::InvokeCommand(int ID, InvokeFlag Flags) const
{
	const int Index = IDToIndex(ID);

	if (Index < 0) {
		TRACE(TEXT("CCommandManager::InvokeCommand() : Invalid command ID {}\n"), ID);
		return false;
	}

	const CommandInfo &Info = m_CommandList[Index];

	if (!Info.Handler)
		return false;

	TRACE(TEXT("CCommandManager::InvokeCommand() : {} {}\n"), ID, GetCommandIDText(ID));

	InvokeParameters Params;

	Params.ID = ID;
	Params.Flags = Flags;

	return Info.Handler(Params);
}


bool CCommandManager::IsCommandValid(int ID) const
{
	return IDToIndex(ID) >= 0;
}


String CCommandManager::GetCommandIDText(int ID) const
{
	const int Index = IDToIndex(ID);

	if (Index < 0)
		return String();

	const CommandInfo &Info = m_CommandList[Index];
	if (Info.IDText.empty())
		return String();

	if (Info.FirstID == Info.LastID)
		return Info.IDText;

	return GetNumberedText(Info.IDText, ID - Info.FirstID);
}


size_t CCommandManager::GetCommandText(int ID, LPTSTR pszText, size_t MaxLength) const
{
	if (pszText == nullptr || MaxLength < 1)
		return 0;

	const int Index = IDToIndex(ID);

	if (Index < 0) {
		pszText[0] = _T('\0');
		return 0;
	}

	const CommandInfo &Info = m_CommandList[Index];
	size_t Length = 0;

	for (CCommandCustomizer *pCustomizer : m_CustomizerList) {
		if (pCustomizer->IsCommandValid(ID)) {
			if (pCustomizer->GetCommandText(ID, pszText, MaxLength))
				Length = StringLength(pszText);
			break;
		}
	}

	if (Length == 0) {
		if (!Info.Text.empty()) {
			StringCopy(pszText, Info.Text.c_str(), MaxLength);
			Length = std::min(Info.Text.length(), MaxLength - 1);
		} else {
			Length = ::LoadString(GetAppClass().GetResourceInstance(), ID, pszText, static_cast<int>(MaxLength));
		}
	}

	return Length;
}


size_t CCommandManager::GetCommandShortText(int ID, LPTSTR pszText, size_t MaxLength) const
{
	if (pszText == nullptr || MaxLength < 1)
		return 0;

	const int Index = IDToIndex(ID);

	if (Index < 0) {
		pszText[0] = _T('\0');
		return 0;
	}

	const CommandInfo &Info = m_CommandList[Index];
	size_t Length = 0;

	for (CCommandCustomizer *pCustomizer : m_CustomizerList) {
		if (pCustomizer->IsCommandValid(ID)) {
			if (pCustomizer->GetCommandText(ID, pszText, MaxLength))
				Length = StringLength(pszText);
			break;
		}
	}

	if (Length == 0) {
		if (!Info.ShortText.empty()) {
			StringCopy(pszText, Info.ShortText.c_str(), MaxLength);
			Length = std::min(Info.ShortText.length(), MaxLength - 1);
		} else {
			Length = ::LoadString(GetAppClass().GetResourceInstance(), ID, pszText, static_cast<int>(MaxLength));
		}
	}

	return Length;
}


int CCommandManager::ParseIDText(LPCTSTR pszText) const
{
	if (IsStringEmpty(pszText))
		return 0;

	auto it = m_CommandTextMap.find(String(pszText));
	if (it == m_CommandTextMap.end())
		return 0;

	return it->second;
}


int CCommandManager::ParseIDText(const String &Text) const
{
	if (Text.empty())
		return 0;

	auto it = m_CommandTextMap.find(Text);
	if (it == m_CommandTextMap.end())
		return 0;

	return it->second;
}


bool CCommandManager::SetCommandState(int ID, CommandState State)
{
	return SetCommandState(ID, ~CommandState::None, State);
}


bool CCommandManager::SetCommandState(int ID, CommandState Mask, CommandState State)
{
	if ((ID < CM_COMMAND_FIRST) || (ID > CM_COMMAND_LAST))
		return false;

	const size_t Index = ID - CM_COMMAND_FIRST;

	const CommandState OldState = m_CommandStateList[Index];
	const CommandState NewState = (OldState & ~Mask) | (State & Mask);
	if (OldState != NewState) {
		m_CommandStateList[Index] = NewState;

		for (CEventListener *pEventListener : m_EventListenerList)
			pEventListener->OnCommandStateChanged(ID, OldState, NewState);
	}

	return true;
}


CCommandManager::CommandState CCommandManager::GetCommandState(int ID) const
{
	if ((ID < CM_COMMAND_FIRST) || (ID > CM_COMMAND_LAST))
		return CommandState::None;

	return m_CommandStateList[ID - CM_COMMAND_FIRST];
}


bool CCommandManager::SetCommandRadioCheckedState(int FirstID, int LastID, int CheckedID)
{
	if ((FirstID > LastID)
			|| ((FirstID < CM_COMMAND_FIRST) || (FirstID > CM_COMMAND_LAST))
			|| ((LastID < CM_COMMAND_FIRST) || (LastID > CM_COMMAND_LAST)))
		return false;

	for (int ID = FirstID; ID <= LastID; ID++) {
		if (ID == CheckedID)
			m_CommandStateList[ID - CM_COMMAND_FIRST] |= CommandState::Checked;
		else
			m_CommandStateList[ID - CM_COMMAND_FIRST] &= ~CommandState::Checked;
	}

	for (CEventListener *pEventListener : m_EventListenerList)
		pEventListener->OnCommandRadioCheckedStateChanged(FirstID, LastID, CheckedID);

	return true;
}


bool CCommandManager::AddCommandCustomizer(CCommandCustomizer *pCustomizer)
{
	if (pCustomizer == nullptr)
		return false;
	m_CustomizerList.push_back(pCustomizer);
	return true;
}


bool CCommandManager::AddEventListener(CEventListener *pEventListener)
{
	if (pEventListener == nullptr)
		return false;

	auto it = std::ranges::find(m_EventListenerList, pEventListener);
	if (it != m_EventListenerList.end())
		return false;

	m_EventListenerList.push_back(pEventListener);

	return true;
}


bool CCommandManager::RemoveEventListener(CEventListener *pEventListener)
{
	auto it = std::ranges::find(m_EventListenerList, pEventListener);
	if (it == m_EventListenerList.end())
		return false;

	m_EventListenerList.erase(it);

	return true;
}


int CCommandManager::IDToIndex(int ID) const
{
	if (m_CommandIDMap.empty())
		return -1;

	auto it = m_CommandIDMap.upper_bound(ID);
	if (it == m_CommandIDMap.begin())
		return -1;
	--it;
	const CommandInfo &Info = m_CommandList[it->second];
	if ((ID < Info.FirstID) || (ID > Info.LastID))
		return -1;

	return static_cast<int>(it->second);
}


String CCommandManager::GetNumberedText(const String &Text, int Number) const
{
	TCHAR szNumber[5];

	StringFormat(szNumber, TEXT("{}"), Number + 1);

	return Text + szNumber;
}




CCommandManager::CCommandLister::CCommandLister(const CCommandManager &Manager)
	: m_Manager(Manager)
{
}


int CCommandManager::CCommandLister::Next()
{
	if (m_Index >= m_Manager.m_CommandList.size())
		return 0;

	while ((m_Manager.m_CommandList[m_Index].LastListingID == 0)
			|| m_Manager.m_CommandList[m_Index].IDText.empty()) {
		m_Index++;
		if (m_Index == m_Manager.m_CommandList.size())
			return 0;
	}

	const CCommandManager::CommandInfo &Info = m_Manager.m_CommandList[m_Index];
	if (Info.FirstID > m_ID) {
		m_ID = Info.FirstID;
	} else if (Info.LastListingID > m_ID) {
		m_ID++;
	} else {
		m_Index++;
		m_ID = 0;
		return Next();
	}

	return m_ID;
}


void CCommandManager::CCommandLister::Reset()
{
	m_Index = 0;
	m_ID = 0;
}


} // namespace TVTest
