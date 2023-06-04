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
#include "WheelCommand.h"
#include "resource.h"
#include "Common/DebugDef.h"


namespace TVTest
{


CWheelCommandManager::CWheelCommandManager()
{
	static const struct {
		int ID;
		LPCTSTR pszIDText;
	} CommandList[] = {
		{CM_WHEEL_VOLUME,      TEXT("WheelVolume")},
		{CM_WHEEL_CHANNEL,     TEXT("WheelChannel")},
		{CM_WHEEL_AUDIO,       TEXT("WheelAudio")},
		{CM_WHEEL_ZOOM,        TEXT("WheelZoom")},
		{CM_WHEEL_ASPECTRATIO, TEXT("WheelAspectRatio")},
		{CM_WHEEL_AUDIODELAY,  TEXT("WheelAudioDelay")},
	};

	m_CommandList.resize(lengthof(CommandList));
	for (int i = 0; i < lengthof(CommandList); i++) {
		m_CommandList[i].ID = CommandList[i].ID;
		m_CommandList[i].IDText = CommandList[i].pszIDText;
	}
}


int CWheelCommandManager::GetCommandCount() const
{
	return static_cast<int>(m_CommandList.size());
}


int CWheelCommandManager::GetCommandID(int Index) const
{
	if (Index < 0 || static_cast<size_t>(Index) >= m_CommandList.size())
		return 0;

	return m_CommandList[Index].ID;
}


int CWheelCommandManager::GetCommandParsableName(int ID, LPTSTR pszName, int MaxName) const
{
	if (pszName == nullptr || MaxName < 1)
		return -1;

	pszName[0] = '\0';

	if (ID <= 0)
		return 0;

	for (const auto &e : m_CommandList) {
		if (e.ID == ID) {
			StringCopy(pszName, e.IDText.c_str(), MaxName);
			return static_cast<int>(e.IDText.length());
		}
	}

	return -1;
}


int CWheelCommandManager::GetCommandText(int ID, LPTSTR pszText, int MaxText) const
{
	if (pszText == nullptr || MaxText < 1)
		return 0;

	return ::LoadString(GetAppClass().GetResourceInstance(), ID, pszText, MaxText);
}


int CWheelCommandManager::ParseCommand(LPCTSTR pszCommand) const
{
	if (IsStringEmpty(pszCommand))
		return 0;

	const StringView Command(pszCommand);

	for (const auto &e : m_CommandList) {
		if (StringUtility::IsEqualNoCase(e.IDText, Command)) {
			return e.ID;
		}
	}

	return 0;
}


}	// namespace TVTest
