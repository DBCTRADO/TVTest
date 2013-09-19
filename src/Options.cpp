#include "stdafx.h"
#include "TVTest.h"
#include "Options.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif




COptionFrame *COptions::m_pFrame=NULL;
DWORD COptions::m_GeneralUpdateFlags=0;


COptions::COptions()
	: m_UpdateFlags(0)
{
}


COptions::COptions(LPCTSTR pszSection)
	: CSettingsBase(pszSection)
	, m_UpdateFlags(0)
{
}


COptions::~COptions()
{
}


DWORD COptions::SetUpdateFlag(DWORD Flag)
{
	m_UpdateFlags|=Flag;
	return m_UpdateFlags;
}


DWORD COptions::SetGeneralUpdateFlag(DWORD Flag)
{
	m_GeneralUpdateFlags|=Flag;
	return m_GeneralUpdateFlags;
}


void COptions::SettingError()
{
	if (m_pFrame!=NULL)
		m_pFrame->OnSettingError(this);
}
