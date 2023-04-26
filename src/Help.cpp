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
#include <htmlhelp.h>
#include <shlwapi.h>
#include "TVTest.h"
#include "Help.h"


namespace TVTest
{


CHtmlHelp::~CHtmlHelp()
{
	Finalize();
}


bool CHtmlHelp::Initialize()
{
	if (m_hLib != nullptr)
		return true;
	m_hLib = Util::LoadSystemLibrary(TEXT("hhctrl.ocx"));
	if (m_hLib == nullptr)
		return false;
	m_pHtmlHelp = reinterpret_cast<HtmlHelpFunc>(::GetProcAddress(
		m_hLib,
#ifndef UNICODE
		"HtmlHelpA"
#else
		"HtmlHelpW"
#endif
		));
	if (m_pHtmlHelp == nullptr) {
		::FreeLibrary(m_hLib);
		m_hLib = nullptr;
		return false;
	}
	(*m_pHtmlHelp)(nullptr, nullptr, HH_INITIALIZE, reinterpret_cast<DWORD_PTR>(&m_Cookie));
	::GetModuleFileName(nullptr, m_szFileName, lengthof(m_szFileName));
	::PathRenameExtension(m_szFileName, TEXT(".chm"));
	return true;
}


void CHtmlHelp::Finalize()
{
	if (m_hLib != nullptr) {
		(*m_pHtmlHelp)(nullptr, nullptr, HH_CLOSE_ALL, 0);
		(*m_pHtmlHelp)(nullptr, nullptr, HH_UNINITIALIZE, m_Cookie);
		::FreeLibrary(m_hLib);
		m_hLib = nullptr;
		m_pHtmlHelp = nullptr;
	}
}


bool CHtmlHelp::ShowIndex()
{
	if (!Initialize())
		return false;
	(*m_pHtmlHelp)(nullptr, m_szFileName, HH_DISPLAY_TOC, 0);
	return true;
}


bool CHtmlHelp::ShowContent(int ID)
{
	if (!Initialize())
		return false;
	(*m_pHtmlHelp)(nullptr, m_szFileName, HH_HELP_CONTEXT, ID);
	return true;
}


bool CHtmlHelp::PreTranslateMessage(MSG *pmsg)
{
	return m_pHtmlHelp != nullptr
		&& (*m_pHtmlHelp)(nullptr, nullptr, HH_PRETRANSLATEMESSAGE, reinterpret_cast<DWORD_PTR>(pmsg));
}


}	// namespace TVTest
