#include "stdafx.h"
#include "TVTest.h"
#include "AppMain.h"
#include "BonTsEngine/TsInformation.h"
#include "resource.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


namespace TVTest
{


CMainDisplay::CMainDisplay(CAppMain &App)
	: m_App(App)
	, m_fViewerEnabled(false)
	, ChannelDisplayEventHandler(App)
{
}


bool CMainDisplay::Create(HWND hwndParent,int ViewID,int ContainerID,HWND hwndMessage)
{
	m_ViewWindow.Create(hwndParent,
		WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,0,ViewID);
	m_ViewWindow.SetMessageWindow(hwndMessage);
	m_VideoContainer.Create(m_ViewWindow.GetHandle(),
		WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,0,ContainerID,
		&m_App.CoreEngine.m_DtvEngine);
	m_ViewWindow.SetVideoContainer(&m_VideoContainer);

	m_DisplayBase.SetParent(&m_VideoContainer);
	m_VideoContainer.SetDisplayBase(&m_DisplayBase);

	return true;
}


bool CMainDisplay::EnableViewer(bool fEnable)
{
	if (m_fViewerEnabled!=fEnable) {
		if (fEnable && !m_App.CoreEngine.m_DtvEngine.m_MediaViewer.IsOpen())
			return false;
		if (fEnable || (!fEnable && !m_DisplayBase.IsVisible()))
			m_VideoContainer.SetVisible(fEnable);
		m_App.CoreEngine.m_DtvEngine.m_MediaViewer.SetVisible(fEnable);
		if (!m_App.CoreEngine.EnableMediaViewer(fEnable))
			return false;
		if (m_App.PlaybackOptions.GetMinTimerResolution())
			m_App.CoreEngine.SetMinTimerResolution(fEnable);
		m_fViewerEnabled=fEnable;
		m_App.AppEventManager.OnPlaybackStateChanged(fEnable);
	}
	return true;
}


bool CMainDisplay::BuildViewer(BYTE VideoStreamType)
{
	if (VideoStreamType==0) {
		VideoStreamType=m_App.CoreEngine.m_DtvEngine.GetVideoStreamType();
		if (VideoStreamType==STREAM_TYPE_UNINITIALIZED)
			return false;
	}
	LPCWSTR pszVideoDecoder=nullptr;

	switch (VideoStreamType) {
#ifdef BONTSENGINE_MPEG2_SUPPORT
	case STREAM_TYPE_MPEG2_VIDEO:
		pszVideoDecoder=m_App.GeneralOptions.GetMpeg2DecoderName();
		break;
#endif

#ifdef BONTSENGINE_H264_SUPPORT
	case STREAM_TYPE_H264:
		pszVideoDecoder=m_App.GeneralOptions.GetH264DecoderName();
		break;
#endif

#ifdef BONTSENGINE_H265_SUPPORT
	case STREAM_TYPE_H265:
		pszVideoDecoder=m_App.GeneralOptions.GetH265DecoderName();
		break;
#endif

	default:
		if (m_App.CoreEngine.m_DtvEngine.GetAudioStreamNum()==0)
			return false;
		VideoStreamType=STREAM_TYPE_INVALID;
		break;
	}

	if (m_fViewerEnabled)
		EnableViewer(false);

	m_App.AddLog(
		TEXT("DirectShowの初期化を行います(%s)..."),
		VideoStreamType==STREAM_TYPE_INVALID?
			TEXT("映像なし"):
			TsEngine::GetStreamTypeText(VideoStreamType));

	m_App.CoreEngine.m_DtvEngine.m_MediaViewer.SetAudioFilter(m_App.AudioOptions.GetAudioFilterName());
	if (!m_App.CoreEngine.BuildMediaViewer(
			m_VideoContainer.GetHandle(),
			m_VideoContainer.GetHandle(),
			m_App.GeneralOptions.GetVideoRendererType(),
			VideoStreamType,pszVideoDecoder,
			m_App.AudioOptions.GetAudioDeviceName())) {
		m_App.Core.OnError(&m_App.CoreEngine,TEXT("DirectShowの初期化ができません。"));
		return false;
	}
	m_App.AudioOptions.ApplyMediaViewerOptions();

	m_App.AddLog(TEXT("DirectShowの初期化を行いました。"));

	return true;
}


bool CMainDisplay::CloseViewer()
{
	EnableViewer(false);
	m_App.CoreEngine.CloseMediaViewer();
	return true;
}




void CMainDisplay::CHomeDisplayEventHandler::OnClose()
{
	m_pHomeDisplay->SetVisible(false);
}


void CMainDisplay::CHomeDisplayEventHandler::OnMouseMessage(UINT Msg,int x,int y)
{
	RelayMouseMessage(m_pHomeDisplay,Msg,x,y);
}




CMainDisplay::CChannelDisplayEventHandler::CChannelDisplayEventHandler(CAppMain &App)
	: m_App(App)
{
}


void CMainDisplay::CChannelDisplayEventHandler::OnTunerSelect(
	LPCTSTR pszDriverFileName,int TuningSpace)
{
	if (m_App.CoreEngine.IsTunerOpen()
			&& IsEqualFileName(m_App.CoreEngine.GetDriverFileName(),pszDriverFileName)) {
		m_App.UICore.DoCommand(CM_CHANNELDISPLAY);
	} else {
		if (!m_App.UICore.ConfirmChannelChange())
			return;

		if (m_App.Core.OpenTuner(pszDriverFileName)) {
			if (TuningSpace!=SPACE_NOTSPECIFIED) {
				m_App.UICore.DoCommand(CM_SPACE_FIRST+TuningSpace);
				if (TuningSpace==SPACE_ALL
						|| TuningSpace==m_App.RestoreChannelInfo.Space)
					m_App.Core.RestoreChannel();
			} else {
				m_App.Core.RestoreChannel();
			}
		}
		m_App.UICore.DoCommand(CM_CHANNELDISPLAY);
	}
}


void CMainDisplay::CChannelDisplayEventHandler::OnChannelSelect(
	LPCTSTR pszDriverFileName,const CChannelInfo *pChannelInfo)
{
	if (!m_App.UICore.ConfirmChannelChange())
		return;

	if (m_App.Core.OpenTuner(pszDriverFileName)) {
		int Space;
		if (m_App.RestoreChannelInfo.fAllChannels)
			Space=CChannelManager::SPACE_ALL;
		else
			Space=pChannelInfo->GetSpace();
		const CChannelList *pList=m_App.ChannelManager.GetChannelList(Space);
		if (pList!=NULL) {
			int Index=pList->FindByIndex(pChannelInfo->GetSpace(),
										 pChannelInfo->GetChannelIndex(),
										 pChannelInfo->GetServiceID());

			if (Index<0 && Space==CChannelManager::SPACE_ALL) {
				Space=pChannelInfo->GetSpace();
				pList=m_App.ChannelManager.GetChannelList(Space);
				if (pList!=NULL)
					Index=pList->FindByIndex(-1,
											 pChannelInfo->GetChannelIndex(),
											 pChannelInfo->GetServiceID());
			}
			if (Index>=0)
				m_App.Core.SetChannel(Space,Index);
		}
		m_App.UICore.DoCommand(CM_CHANNELDISPLAY);
	}
}


void CMainDisplay::CChannelDisplayEventHandler::OnClose()
{
	m_pChannelDisplay->SetVisible(false);
}


void CMainDisplay::CChannelDisplayEventHandler::OnMouseMessage(UINT Msg,int x,int y)
{
	RelayMouseMessage(m_pChannelDisplay,Msg,x,y);
}


}	// namespace TVTest
