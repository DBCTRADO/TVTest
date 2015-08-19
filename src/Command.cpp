#include "stdafx.h"
#include "TVTest.h"
#include "AppMain.h"
#include "Command.h"
#include "HelperClass/StdUtil.h"
#include "resource.h"
#include "Common/DebugDef.h"


static const struct {
	LPCTSTR pszText;
	WORD ID;
} DefaultCommandList[] = {
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
	{TEXT("Aspect32x9"),				CM_ASPECTRATIO_32x9},
	{TEXT("Aspect16x9Left"),			CM_ASPECTRATIO_16x9_LEFT},
	{TEXT("Aspect16x9Right"),			CM_ASPECTRATIO_16x9_RIGHT},
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
	{TEXT("ChannelPrevious"),			CM_CHANNEL_PREVIOUS},
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
	{TEXT("AudioDelayMinus"),			CM_AUDIODELAY_MINUS},
	{TEXT("AudioDelayPlus"),			CM_AUDIODELAY_PLUS},
	{TEXT("AudioDelayReset"),			CM_AUDIODELAY_RESET},
	{TEXT("SwitchAudio"),				CM_SWITCHAUDIO},
	{TEXT("Stereo"),					CM_DUALMONO_BOTH},
	{TEXT("StereoLeft"),				CM_DUALMONO_MAIN},
	{TEXT("StereoRight"),				CM_DUALMONO_SUB},
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
	{TEXT("OneSegMode"),				CM_1SEGMODE},
	{TEXT("CloseTuner"),				CM_CLOSETUNER},
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
	{TEXT("VideoMenu"),					CM_VIDEOMENU},
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
	{TEXT("SwitchVideo"),				CM_SWITCHVIDEO},
	{TEXT("SwitchVideoStream"),			CM_VIDEOSTREAM_SWITCH},
	{TEXT("SwitchMultiView"),			CM_MULTIVIEW_SWITCH},
};




CCommandList::CCommandList()
	: m_pEventHandler(nullptr)
{
	RegisterDefaultCommands();
}


CCommandList::~CCommandList()
{
}


int CCommandList::NumCommands() const
{
	return static_cast<int>(m_CommandList.size());
}


int CCommandList::GetCommandID(int Index) const
{
	if (Index<0 || static_cast<size_t>(Index)>=m_CommandList.size())
		return 0;

	return m_CommandList[Index].ID;
}


LPCTSTR CCommandList::GetCommandText(int Index) const
{
	if (Index<0 || static_cast<size_t>(Index)>=m_CommandList.size())
		return nullptr;

	return m_CommandList[Index].Text.c_str();
}


LPCTSTR CCommandList::GetCommandTextByID(int ID) const
{
	int Index=IDToIndex(ID);

	if (Index<0)
		return nullptr;
	return GetCommandText(Index);
}


int CCommandList::GetCommandName(int Index,LPTSTR pszName,int MaxLength) const
{
	if (pszName==nullptr || MaxLength<1)
		return 0;
	if (Index<0 || static_cast<size_t>(Index)>=m_CommandList.size()) {
		pszName[0]=_T('\0');
		return 0;
	}

	const CommandInfo &Info=m_CommandList[Index];
	const int ID=Info.ID;
	int Length=0;

	for (size_t i=0;i<m_CustomizerList.size();i++) {
		if (m_CustomizerList[i]->IsCommandValid(ID)) {
			if (m_CustomizerList[i]->GetCommandName(ID,pszName,MaxLength))
				Length=::lstrlen(pszName);
			break;
		}
	}

	if (Length==0) {
		if (!Info.Name.empty()) {
			::lstrcpyn(pszName,Info.Name.c_str(),MaxLength);
			Length=min(static_cast<int>(Info.Name.length()),MaxLength-1);
		} else {
			Length=::LoadString(GetAppClass().GetResourceInstance(),
								ID,pszName,MaxLength);
		}
	}

	return Length;
}


int CCommandList::GetCommandNameByID(int ID,LPTSTR pszName,int MaxLength) const
{
	int Index=IDToIndex(ID);

	if (Index<0)
		return 0;
	return GetCommandName(Index,pszName,MaxLength);
}


int CCommandList::GetCommandShortName(int Index,LPTSTR pszName,int MaxLength) const
{
	if (pszName==nullptr || MaxLength<1)
		return 0;
	if (Index<0 || static_cast<size_t>(Index)>=m_CommandList.size()) {
		pszName[0]=_T('\0');
		return 0;
	}

	const CommandInfo &Info=m_CommandList[Index];
	const int ID=Info.ID;
	int Length=0;

	for (size_t i=0;i<m_CustomizerList.size();i++) {
		if (m_CustomizerList[i]->IsCommandValid(ID)) {
			if (m_CustomizerList[i]->GetCommandName(ID,pszName,MaxLength))
				Length=::lstrlen(pszName);
			break;
		}
	}

	if (Length==0) {
		if (!Info.ShortName.empty()) {
			::lstrcpyn(pszName,Info.ShortName.c_str(),MaxLength);
			Length=min(static_cast<int>(Info.ShortName.length()),MaxLength-1);
		} else {
			Length=GetCommandName(Index,pszName,MaxLength);
		}
	}

	return Length;
}


int CCommandList::GetCommandShortNameByID(int ID,LPTSTR pszName,int MaxLength) const
{
	int Index=IDToIndex(ID);

	if (Index<0)
		return 0;
	return GetCommandShortName(Index,pszName,MaxLength);
}


int CCommandList::IDToIndex(int ID) const
{
	auto itr=m_CommandIDMap.find(ID);
	if (itr!=m_CommandIDMap.end())
		return static_cast<int>(itr->second);

	return -1;
}


int CCommandList::ParseText(LPCTSTR pszText) const
{
	if (IsStringEmpty(pszText))
		return 0;

	auto itr=m_CommandTextMap.find(TVTest::String(pszText));
	if (itr==m_CommandTextMap.end())
		return 0;

	return itr->second;
}


bool CCommandList::RegisterCommand(
	int ID,LPCTSTR pszText,LPCTSTR pszName,LPCTSTR pszShortName,unsigned int State)
{
	if (IsStringEmpty(pszText))
		return false;

	CommandInfo Info;

	Info.ID=ID;
	Info.State=State;
	Info.Text=pszText;
	if (!IsStringEmpty(pszName))
		Info.Name=pszName;
	if (!IsStringEmpty(pszShortName))
		Info.ShortName=pszShortName;

	m_CommandList.push_back(Info);
#ifndef _DEBUG
	m_CommandTextMap.insert(std::pair<TVTest::String,int>(Info.Text,ID));
	m_CommandIDMap.insert(std::pair<int,size_t>(ID,m_CommandList.size()-1));
#else
	if (!m_CommandTextMap.insert(std::pair<TVTest::String,int>(Info.Text,ID)).second
			|| !m_CommandIDMap.insert(std::pair<int,size_t>(ID,m_CommandList.size()-1)).second) {
		// éØï éqèdï°
		::DebugBreak();
	}
#endif

	return true;
}


bool CCommandList::AddCommandCustomizer(CCommandCustomizer *pCustomizer)
{
	if (pCustomizer==nullptr)
		return false;
	m_CustomizerList.push_back(pCustomizer);
	return true;
}


void CCommandList::SetEventHandler(CEventHandler *pEventHandler)
{
	m_pEventHandler=pEventHandler;
}


bool CCommandList::SetCommandStateByID(int ID,unsigned int State)
{
	return SetCommandStateByID(ID,~0U,State);
}


bool CCommandList::SetCommandStateByID(int ID,unsigned int Mask,unsigned int State)
{
	int Index=IDToIndex(ID);
	if (Index<0)
		return false;

	unsigned int OldState=m_CommandList[Index].State;
	unsigned int NewState=(OldState & ~Mask) | (State & Mask);
	if (OldState!=NewState) {
		m_CommandList[Index].State=NewState;
		if (m_pEventHandler!=nullptr)
			m_pEventHandler->OnCommandStateChanged(ID,OldState,NewState);
	}

	return true;
}


unsigned int CCommandList::GetCommandStateByID(int ID) const
{
	int Index=IDToIndex(ID);
	if (Index<0)
		return 0;
	return m_CommandList[Index].State;
}


bool CCommandList::SetCommandRadioCheckedState(int FirstID,int LastID,int CheckedID)
{
	if (FirstID>LastID)
		return false;

	for (int i=FirstID;i<=LastID;i++) {
		int Index=IDToIndex(i);

		if (Index>=0) {
			if (i==CheckedID)
				m_CommandList[Index].State|=COMMAND_STATE_CHECKED;
			else
				m_CommandList[Index].State&=~COMMAND_STATE_CHECKED;
		}
	}

	if (m_pEventHandler!=nullptr)
		m_pEventHandler->OnCommandRadioCheckedStateChanged(FirstID,LastID,CheckedID);

	return true;
}


void CCommandList::RegisterDefaultCommands()
{
	static const size_t ReserveSize=lengthof(DefaultCommandList)+64;

	m_CommandList.reserve(ReserveSize);
	m_CommandTextMap.rehash(ReserveSize);
	m_CommandIDMap.rehash(ReserveSize);

	for (int i=0;i<lengthof(DefaultCommandList);i++)
		RegisterCommand(DefaultCommandList[i].ID,DefaultCommandList[i].pszText);
}
