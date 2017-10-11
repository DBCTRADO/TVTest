#include "stdafx.h"
#include <htmlhelp.h>
#include <shlwapi.h>
#include "TVTest.h"
#include "Help.h"


namespace TVTest
{


CHtmlHelp::CHtmlHelp()
	: m_hLib(nullptr)
	, m_pHtmlHelp(nullptr)
{
}


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
	m_pHtmlHelp = (HtmlHelpFunc)::GetProcAddress(
		m_hLib,
#ifndef UNICODE
		"HtmlHelpA"
#else
		"HtmlHelpW"
#endif
		);
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
