#include "stdafx.h"
#include "TVTest.h"
#include "AppMain.h"
#include "Command.h"
#include "DriverManager.h"
#include "Plugin.h"
#include "ZoomOptions.h"
#include "HelperClass/StdUtil.h"
#include "resource.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


static const struct {
	LPCTSTR pszText;
	WORD Command;
} CommandList[] = {
	{TEXT("Zoom20"),					CM_ZOOM_20},
	{TEXT("Zoom25"),					CM_ZOOM_25},
	{TEXT("Zoom33"),					CM_ZOOM_33},
	{TEXT("Zoom50"),					CM_ZOOM_50},
	{TEXT("Zoom66"),					CM_ZOOM_66},
	{TEXT("Zoom75"),					CM_ZOOM_75},
	{TEXT("Zoom100"),					CM_ZOOM_100},
	{TEXT("Zoom150"),					CM_ZOOM_150},
	{TEXT("Zoom200"),					CM_ZOOM_200},
	{TEXT("Zoom250"),					CM_ZOOM_250},
	{TEXT("Zoom300"),					CM_ZOOM_300},
	{TEXT("CustomZoom1"),				CM_CUSTOMZOOM_FIRST+0},
	{TEXT("CustomZoom2"),				CM_CUSTOMZOOM_FIRST+1},
	{TEXT("CustomZoom3"),				CM_CUSTOMZOOM_FIRST+2},
	{TEXT("CustomZoom4"),				CM_CUSTOMZOOM_FIRST+3},
	{TEXT("CustomZoom5"),				CM_CUSTOMZOOM_FIRST+4},
	{TEXT("CustomZoom6"),				CM_CUSTOMZOOM_FIRST+5},
	{TEXT("CustomZoom7"),				CM_CUSTOMZOOM_FIRST+6},
	{TEXT("CustomZoom8"),				CM_CUSTOMZOOM_FIRST+7},
	{TEXT("CustomZoom9"),				CM_CUSTOMZOOM_FIRST+8},
	{TEXT("CustomZoom10"),				CM_CUSTOMZOOM_FIRST+9},
	{TEXT("ZoomOptions"),				CM_ZOOMOPTIONS},
	{TEXT("AspectRatio"),				CM_ASPECTRATIO},
	{TEXT("AspectDefault"),				CM_ASPECTRATIO_DEFAULT},
	{TEXT("Aspect16x9"),				CM_ASPECTRATIO_16x9},
	{TEXT("LetterBox"),					CM_ASPECTRATIO_LETTERBOX},
	{TEXT("SuperFrame"),				CM_ASPECTRATIO_SUPERFRAME},
	{TEXT("SideCut"),					CM_ASPECTRATIO_SIDECUT},
	{TEXT("Aspect4x3"),					CM_ASPECTRATIO_4x3},
#ifndef TVH264_FOR_1SEG
	{TEXT("Aspect32x9"),				CM_ASPECTRATIO_32x9},
	{TEXT("Aspect16x9Left"),			CM_ASPECTRATIO_16x9_LEFT},
	{TEXT("Aspect16x9Right"),			CM_ASPECTRATIO_16x9_RIGHT},
#endif
	{TEXT("PanAndScan1"),				CM_PANANDSCAN_PRESET_FIRST+0},
	{TEXT("PanAndScan2"),				CM_PANANDSCAN_PRESET_FIRST+1},
	{TEXT("PanAndScan3"),				CM_PANANDSCAN_PRESET_FIRST+2},
	{TEXT("PanAndScan4"),				CM_PANANDSCAN_PRESET_FIRST+3},
	{TEXT("PanAndScan5"),				CM_PANANDSCAN_PRESET_FIRST+4},
	{TEXT("PanAndScan6"),				CM_PANANDSCAN_PRESET_FIRST+5},
	{TEXT("PanAndScan7"),				CM_PANANDSCAN_PRESET_FIRST+6},
	{TEXT("PanAndScan8"),				CM_PANANDSCAN_PRESET_FIRST+7},
	{TEXT("PanAndScan9"),				CM_PANANDSCAN_PRESET_FIRST+8},
	{TEXT("PanAndScan10"),				CM_PANANDSCAN_PRESET_FIRST+9},
	{TEXT("PanAndScanOptions"),			CM_PANANDSCANOPTIONS},
	{TEXT("FrameCut"),					CM_FRAMECUT},
	{TEXT("Fullscreen"),				CM_FULLSCREEN},
	{TEXT("AlwaysOnTop"),				CM_ALWAYSONTOP},
	{TEXT("ChannelUp"),					CM_CHANNEL_UP},
	{TEXT("ChannelDown"),				CM_CHANNEL_DOWN},
	{TEXT("ChannelBackward"),			CM_CHANNEL_BACKWARD},
	{TEXT("ChannelForward"),			CM_CHANNEL_FORWARD},
	{TEXT("Mute"),						CM_VOLUME_MUTE},
	{TEXT("VolumeUp"),					CM_VOLUME_UP},
	{TEXT("VolumeDown"),				CM_VOLUME_DOWN},
	{TEXT("VolumeNormalizeNone"),		CM_AUDIOGAIN_NONE},
	{TEXT("VolumeNormalize125"),		CM_AUDIOGAIN_125},
	{TEXT("VolumeNormalize150"),		CM_AUDIOGAIN_150},
	{TEXT("VolumeNormalize200"),		CM_AUDIOGAIN_200},
	{TEXT("SurroundGainNone"),			CM_SURROUNDAUDIOGAIN_NONE},
	{TEXT("SurroundGain125"),			CM_SURROUNDAUDIOGAIN_125},
	{TEXT("SurroundGain150"),			CM_SURROUNDAUDIOGAIN_150},
	{TEXT("SurroundGain200"),			CM_SURROUNDAUDIOGAIN_200},
	{TEXT("SwitchAudio"),				CM_SWITCHAUDIO},
	{TEXT("Stereo"),					CM_STEREO_THROUGH},
	{TEXT("StereoLeft"),				CM_STEREO_LEFT},
	{TEXT("StereoRight"),				CM_STEREO_RIGHT},
	{TEXT("SpdifDisabled"),				CM_SPDIF_DISABLED},
	{TEXT("SpdifPassthrough"),			CM_SPDIF_PASSTHROUGH},
	{TEXT("SpdifAuto"),					CM_SPDIF_AUTO},
	{TEXT("SpdifToggle"),				CM_SPDIF_TOGGLE},
	{TEXT("Record"),					CM_RECORD},
	{TEXT("RecordStart"),				CM_RECORD_START},
	{TEXT("RecordStop"),				CM_RECORD_STOP},
	{TEXT("RecordPause"),				CM_RECORD_PAUSE},
	{TEXT("RecordEvent"),				CM_RECORDEVENT},
	{TEXT("RecordOption"),				CM_RECORDOPTION},
	{TEXT("TimeShiftRecording"),		CM_TIMESHIFTRECORDING},
	{TEXT("EnableTimeShiftRecording"),	CM_ENABLETIMESHIFTRECORDING},
	{TEXT("DisableViewer"),				CM_DISABLEVIEWER},
	{TEXT("CaptureImage"),				CM_CAPTURE},
	{TEXT("CopyImage"),					CM_COPY},
	{TEXT("SaveImage"),					CM_SAVEIMAGE},
	{TEXT("CapturePreview"),			CM_CAPTUREPREVIEW},
	{TEXT("Reset"),						CM_RESET},
	{TEXT("ResetViewer"),				CM_RESETVIEWER},
	{TEXT("RebuildViewer"),				CM_REBUILDVIEWER},
	{TEXT("Panel"),						CM_PANEL},
	{TEXT("ProgramGuide"),				CM_PROGRAMGUIDE},
	{TEXT("TitleBar"),					CM_TITLEBAR},
	{TEXT("StatusBar"),					CM_STATUSBAR},
	{TEXT("SideBar"),					CM_SIDEBAR},
	{TEXT("Options"),					CM_OPTIONS},
	{TEXT("VideoDecoderProperty"),		CM_VIDEODECODERPROPERTY},
	{TEXT("VideoRendererProperty"),		CM_VIDEORENDERERPROPERTY},
	{TEXT("AudioFilterProperty"),		CM_AUDIOFILTERPROPERTY},
	{TEXT("AudioRendererProperty"),		CM_AUDIORENDERERPROPERTY},
	{TEXT("DemuxerProperty"),			CM_DEMULTIPLEXERPROPERTY},
	{TEXT("StreamInfo"),				CM_STREAMINFO},
	{TEXT("Close"),						CM_CLOSE},
	{TEXT("Exit"),						CM_EXIT},
	{TEXT("Menu"),						CM_MENU},
	{TEXT("Activate"),					CM_ACTIVATE},
	{TEXT("Minimize"),					CM_MINIMIZE},
	{TEXT("Maximize"),					CM_MAXIMIZE},
	{TEXT("HomeDisplay"),				CM_HOMEDISPLAY},
	{TEXT("ChannelDisplayMenu"),		CM_CHANNELDISPLAY},
	{TEXT("Buffering"),					CM_ENABLEBUFFERING},
	{TEXT("ResetBuffer"),				CM_RESETBUFFER},
	{TEXT("ResetErrorCount"),			CM_RESETERRORCOUNT},
	{TEXT("ShowTOTTime"),				CM_SHOWTOTTIME},
	{TEXT("AdjustTOTTime"),				CM_ADJUSTTOTTIME},
	{TEXT("ZoomMenu"),					CM_ZOOMMENU},
	{TEXT("AspectRatioMenu"),			CM_ASPECTRATIOMENU},
	{TEXT("ChannelMenu"),				CM_CHANNELMENU},
	{TEXT("ServiceMenu"),				CM_SERVICEMENU},
	{TEXT("TuningSpaceMenu"),			CM_TUNINGSPACEMENU},
	{TEXT("FavoritesMenu"),				CM_FAVORITESMENU},
	{TEXT("AddToFavorites"),			CM_ADDTOFAVORITES},
	{TEXT("OrganizeFavorites"),			CM_ORGANIZEFAVORITES},
	{TEXT("RecentChannelMenu"),			CM_RECENTCHANNELMENU},
	{TEXT("VolumeMenu"),				CM_VOLUMEMENU},
	{TEXT("AudioMenu"),					CM_AUDIOMENU},
	{TEXT("ResetMenu"),					CM_RESETMENU},
	{TEXT("BarMenu"),					CM_BARMENU},
	{TEXT("PluginMenu"),				CM_PLUGINMENU},
	{TEXT("FilterPropertyMenu"),		CM_FILTERPROPERTYMENU},
	{TEXT("Channel1"),					CM_CHANNELNO_1},
	{TEXT("Channel2"),					CM_CHANNELNO_2},
	{TEXT("Channel3"),					CM_CHANNELNO_3},
	{TEXT("Channel4"),					CM_CHANNELNO_4},
	{TEXT("Channel5"),					CM_CHANNELNO_5},
	{TEXT("Channel6"),					CM_CHANNELNO_6},
	{TEXT("Channel7"),					CM_CHANNELNO_7},
	{TEXT("Channel8"),					CM_CHANNELNO_8},
	{TEXT("Channel9"),					CM_CHANNELNO_9},
	{TEXT("Channel10"),					CM_CHANNELNO_10},
	{TEXT("Channel11"),					CM_CHANNELNO_11},
	{TEXT("Channel12"),					CM_CHANNELNO_12},
	{TEXT("Service1"),					CM_SERVICE_FIRST},
	{TEXT("Service2"),					CM_SERVICE_FIRST+1},
	{TEXT("Service3"),					CM_SERVICE_FIRST+2},
	{TEXT("Service4"),					CM_SERVICE_FIRST+3},
	{TEXT("Service5"),					CM_SERVICE_FIRST+4},
	{TEXT("Service6"),					CM_SERVICE_FIRST+5},
	{TEXT("Service7"),					CM_SERVICE_FIRST+6},
	{TEXT("Service8"),					CM_SERVICE_FIRST+7},
	{TEXT("TuningSpace1"),				CM_SPACE_FIRST},
	{TEXT("TuningSpace2"),				CM_SPACE_FIRST+1},
	{TEXT("TuningSpace3"),				CM_SPACE_FIRST+2},
	{TEXT("TuningSpace4"),				CM_SPACE_FIRST+3},
	{TEXT("TuningSpace5"),				CM_SPACE_FIRST+4},
	{TEXT("ChannelNo2Digit"),			CM_CHANNELNO_2DIGIT},
	{TEXT("ChannelNo3Digit"),			CM_CHANNELNO_3DIGIT},
};




CCommandList::CCommandList()
{
}


CCommandList::~CCommandList()
{
}


bool CCommandList::Initialize(const CDriverManager *pDriverManager,
							  const CPluginManager *pPluginManager)
{
	m_DriverList.clear();
	if (pDriverManager!=NULL) {
		for (int i=0;i<pDriverManager->NumDrivers();i++)
			m_DriverList.push_back(CDynamicString(::PathFindFileName(pDriverManager->GetDriverInfo(i)->GetFileName())));
	}

	m_PluginList.clear();
	m_PluginCommandList.clear();
	if (pPluginManager!=NULL) {
		for (int i=0;i<pPluginManager->NumPlugins();i++) {
			const CPlugin *pPlugin=pPluginManager->GetPlugin(i);
			LPCTSTR pszFileName=::PathFindFileName(pPlugin->GetFileName());

			m_PluginList.push_back(CDynamicString(pszFileName));
			for (int j=0;j<pPlugin->NumPluginCommands();j++) {
				TVTest::CommandInfo Info;
				LPTSTR pszText;

				pPlugin->GetPluginCommandInfo(j,&Info);
				pszText=new TCHAR[::lstrlen(pszFileName)+1+::lstrlen(Info.pszText)+1];
				::wsprintf(pszText,TEXT("%s:%s"),pszFileName,Info.pszText);
				m_PluginCommandList.push_back(PluginCommandInfo(pszText,Info.pszName));
				delete [] pszText;
			}
		}
	}

	return true;
}


int CCommandList::NumCommands() const
{
	return (int)(lengthof(CommandList)+m_DriverList.size()+
				 m_PluginList.size()+m_PluginCommandList.size());
}


int CCommandList::GetCommandID(int Index) const
{
	int Base;

	if (Index<0 || Index>=NumCommands())
		return 0;
	if (Index<lengthof(CommandList))
		return CommandList[Index].Command;
	Base=lengthof(CommandList);
	if (Index<Base+(int)m_DriverList.size())
		return CM_DRIVER_FIRST+Index-Base;
	Base+=(int)m_DriverList.size();
	if (Index<Base+(int)m_PluginList.size())
		return CM_PLUGIN_FIRST+Index-Base;
	Base+=(int)m_PluginList.size();
	return CM_PLUGINCOMMAND_FIRST+Index-Base;
}


LPCTSTR CCommandList::GetCommandText(int Index) const
{
	int Base;

	if (Index<0 || Index>=NumCommands())
		return NULL;
	if (Index<lengthof(CommandList))
		return CommandList[Index].pszText;
	Base=lengthof(CommandList);
	if (Index<Base+(int)m_DriverList.size())
		return m_DriverList[Index-Base].Get();
	Base+=(int)m_DriverList.size();
	if (Index<Base+(int)m_PluginList.size())
		return m_PluginList[Index-Base].Get();
	Base+=(int)m_PluginList.size();
	return m_PluginCommandList[Index-Base].Text.Get();
}


LPCTSTR CCommandList::GetCommandTextByID(int ID) const
{
	int Index=IDToIndex(ID);

	if (Index<0)
		return NULL;
	return GetCommandText(Index);
}


int CCommandList::GetCommandName(int Index,LPTSTR pszName,int MaxLength) const
{
	int Base;

	if (pszName==NULL || MaxLength<1)
		return 0;
	if (Index<0 || Index>=NumCommands()) {
		pszName[0]=_T('\0');
		return 0;
	}
	if (Index<lengthof(CommandList)) {
		const int Command=CommandList[Index].Command;
		int Length=0;
		for (size_t i=0;i<m_CustomizerList.size();i++) {
			if (m_CustomizerList[i]->IsCommandValid(Command)) {
				if (m_CustomizerList[i]->GetCommandName(Command,pszName,MaxLength))
					Length=::lstrlen(pszName);
				break;
			}
		}
		if (Length==0)
			Length=::LoadString(GetAppClass().GetResourceInstance(),
								Command,pszName,MaxLength);
		return Length;
	}
	Base=lengthof(CommandList);
	if (Index<Base+(int)m_DriverList.size())
		return StdUtil::snprintf(pszName,MaxLength,TEXT("ドライバ (%s)"),
								 m_DriverList[Index-Base].Get());
	Base+=(int)m_DriverList.size();
	if (Index<Base+(int)m_PluginList.size())
		return StdUtil::snprintf(pszName,MaxLength,TEXT("プラグイン (%s)"),
								 m_PluginList[Index-Base].Get());
	Base+=(int)m_PluginList.size();
	const PluginCommandInfo &PluginCommand=m_PluginCommandList[Index-Base];
	LPCTSTR pszText=PluginCommand.Text.Get();
	int Length;
	TCHAR szFileName[MAX_PATH];
	for (Length=0;pszText[Length]!=_T(':');Length++)
		szFileName[Length]=pszText[Length];
	szFileName[Length]=_T('\0');
	return StdUtil::snprintf(pszName,MaxLength,TEXT("%s (%s)"),
							 szFileName,PluginCommand.Name.Get());
}


int CCommandList::GetCommandNameByID(int ID,LPTSTR pszName,int MaxLength) const
{
	int Index=IDToIndex(ID);

	if (Index<0)
		return 0;
	return GetCommandName(Index,pszName,MaxLength);
}


int CCommandList::IDToIndex(int ID) const
{
	int Base;

	// 項目が多くなったらmapあたりを使って探すように直す
	for (int i=0;i<lengthof(CommandList);i++) {
		if (CommandList[i].Command==ID)
			return i;
	}
	Base=lengthof(CommandList);
	if (ID>=CM_DRIVER_FIRST && ID<CM_DRIVER_FIRST+(int)m_DriverList.size())
		return Base+ID-CM_DRIVER_FIRST;
	Base+=(int)m_DriverList.size();
	if (ID>=CM_PLUGIN_FIRST && ID<CM_PLUGIN_FIRST+(int)m_PluginList.size())
		return Base+ID-CM_PLUGIN_FIRST;
	Base+=(int)m_PluginList.size();
	if (ID>=CM_PLUGINCOMMAND_FIRST && ID<CM_PLUGINCOMMAND_LAST+(int)m_PluginCommandList.size())
		return Base+ID-CM_PLUGINCOMMAND_FIRST;
	return -1;
}


int CCommandList::ParseText(LPCTSTR pszText) const
{
	int i;

	if (IsStringEmpty(pszText))
		return 0;
	// 項目が多くなったらmapあたりを使って探すように直す
	for (i=0;i<lengthof(CommandList);i++) {
		if (::lstrcmpi(CommandList[i].pszText,pszText)==0)
			return CommandList[i].Command;
	}
	for (i=0;i<(int)m_DriverList.size();i++) {
		if (m_DriverList[i].CompareIgnoreCase(pszText)==0)
			return CM_DRIVER_FIRST+i;
	}
	for (i=0;i<(int)m_PluginList.size();i++) {
		if (m_PluginList[i].CompareIgnoreCase(pszText)==0)
			return CM_PLUGIN_FIRST+i;
	}
	for (i=0;i<(int)m_PluginCommandList.size();i++) {
		if (m_PluginCommandList[i].Text.CompareIgnoreCase(pszText)==0)
			return CM_PLUGINCOMMAND_FIRST+i;
	}
	return 0;
}


bool CCommandList::AddCommandCustomizer(CCommandCustomizer *pCustomizer)
{
	if (pCustomizer==NULL)
		return false;
	m_CustomizerList.push_back(pCustomizer);
	return true;
}
