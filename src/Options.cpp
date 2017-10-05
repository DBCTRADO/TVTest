#include "stdafx.h"
#include "TVTest.h"
#include "Options.h"
#include "Common/DebugDef.h"




COptionFrame *COptions::m_pFrame = nullptr;
DWORD COptions::m_GeneralUpdateFlags = 0;


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
	m_UpdateFlags |= Flag;
	return m_UpdateFlags;
}


DWORD COptions::SetGeneralUpdateFlag(DWORD Flag)
{
	m_GeneralUpdateFlags |= Flag;
	return m_GeneralUpdateFlags;
}


void COptions::ActivatePage()
{
	if (m_pFrame != nullptr)
		m_pFrame->ActivatePage(this);
}


void COptions::SettingError()
{
	if (m_pFrame != nullptr)
		m_pFrame->OnSettingError(this);
}
