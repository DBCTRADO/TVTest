#include "stdafx.h"
#include <htmlhelp.h>
#include <shlwapi.h>
#include "TVTest.h"
#include "AppMain.h"
#include "Help.h"




CHtmlHelp::CHtmlHelp()
	: m_hLib(NULL)
	, m_pHtmlHelp(NULL)
{
}


CHtmlHelp::~CHtmlHelp()
{
	Finalize();
}


bool CHtmlHelp::Initialize()
{
	if (m_hLib!=NULL)
		return true;
	m_hLib=::LoadLibrary(TEXT("hhctrl.ocx"));
	if (m_hLib==NULL)
		return false;
	m_pHtmlHelp=(HtmlHelpFunc)::GetProcAddress(m_hLib,
#ifndef UNICODE
											"HtmlHelpA"
#else
											"HtmlHelpW"
#endif
											);
	if (m_pHtmlHelp==NULL) {
		::FreeLibrary(m_hLib);
		m_hLib=NULL;
		return false;
	}
	(*m_pHtmlHelp)(NULL,NULL,HH_INITIALIZE,reinterpret_cast<DWORD_PTR>(&m_Cookie));
	::GetModuleFileName(NULL,m_szFileName,lengthof(m_szFileName));
	::PathRenameExtension(m_szFileName,TEXT(".chm"));
	return true;
}


void CHtmlHelp::Finalize()
{
	if (m_hLib!=NULL) {
		(*m_pHtmlHelp)(NULL,NULL,HH_CLOSE_ALL,0);
		(*m_pHtmlHelp)(NULL,NULL,HH_UNINITIALIZE,m_Cookie);
		::FreeLibrary(m_hLib);
		m_hLib=NULL;
		m_pHtmlHelp=NULL;
	}
}


bool CHtmlHelp::ShowIndex()
{
	if (!Initialize())
		return false;
	(*m_pHtmlHelp)(NULL,m_szFileName,HH_DISPLAY_TOC,0);
	return true;
}


bool CHtmlHelp::ShowContent(int ID)
{
	if (!Initialize())
		return false;
	(*m_pHtmlHelp)(NULL,m_szFileName,HH_HELP_CONTEXT,ID);
	return true;
}


bool CHtmlHelp::PreTranslateMessage(MSG *pmsg)
{
	return m_pHtmlHelp!=NULL
		&& (*m_pHtmlHelp)(NULL,NULL,HH_PRETRANSLATEMESSAGE,reinterpret_cast<DWORD_PTR>(pmsg));
}
