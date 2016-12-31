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
		{CM_WHEEL_VOLUME,		TEXT("WheelVolume")},
		{CM_WHEEL_CHANNEL,		TEXT("WheelChannel")},
		{CM_WHEEL_AUDIO,		TEXT("WheelAudio")},
		{CM_WHEEL_ZOOM,			TEXT("WheelZoom")},
		{CM_WHEEL_ASPECTRATIO,	TEXT("WheelAspectRatio")},
		{CM_WHEEL_AUDIODELAY,	TEXT("WheelAudioDelay")},
	};

	m_CommandList.resize(lengthof(CommandList));
	for (int i=0;i<lengthof(CommandList);i++) {
		m_CommandList[i].ID=CommandList[i].ID;
		m_CommandList[i].IDText=CommandList[i].pszIDText;
	}
}


int CWheelCommandManager::GetCommandCount() const
{
	return static_cast<int>(m_CommandList.size());
}


int CWheelCommandManager::GetCommandID(int Index) const
{
	if (Index<0 || static_cast<size_t>(Index)>=m_CommandList.size())
		return 0;

	return m_CommandList[Index].ID;
}


int CWheelCommandManager::GetCommandParsableName(int ID,LPTSTR pszName,int MaxName) const
{
	if (pszName==nullptr || MaxName<1)
		return -1;

	pszName[0]='\0';

	if (ID<=0)
		return 0;

	for (auto it=m_CommandList.begin();it!=m_CommandList.end();++it) {
		if (it->ID==ID) {
			::lstrcpyn(pszName,it->IDText.c_str(),MaxName);
			return static_cast<int>(it->IDText.length());
		}
	}

	return -1;
}


int CWheelCommandManager::GetCommandText(int ID,LPTSTR pszText,int MaxText) const
{
	if (pszText==nullptr || MaxText<1)
		return 0;

	return ::LoadString(GetAppClass().GetResourceInstance(),
						ID,pszText,MaxText);
}


int CWheelCommandManager::ParseCommand(LPCTSTR pszCommand) const
{
	if (IsStringEmpty(pszCommand))
		return 0;

	for (auto it=m_CommandList.begin();it!=m_CommandList.end();++it) {
		if (StringUtility::CompareNoCase(it->IDText,pszCommand)==0) {
			return it->ID;
		}
	}

	return 0;
}


}	// namespace TVTest
