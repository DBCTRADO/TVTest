#include "stdafx.h"
#include "TVTest.h"
#include "AppMain.h"
#include "PlaybackOptions.h"
#include "DialogUtil.h"
#include "resource.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif




CPlaybackOptions::CPlaybackOptions()
	: m_fRestoreMute(false)
	, m_fMute(false)
	, m_fRestorePlayStatus(false)
	, m_fUseAudioRendererClock(true)
	, m_fEnablePTSSync(true)
	, m_fAdjustAudioStreamTime(true)
	, m_fMinTimerResolution(true)
	, m_fPacketBuffering(false)
	, m_PacketBufferLength(40000)
	, m_PacketBufferPoolPercentage(50)
	, m_StreamThreadPriority(THREAD_PRIORITY_NORMAL)
	, m_fAdjust1SegFrameRate(true)
{
}


CPlaybackOptions::~CPlaybackOptions()
{
	Destroy();
}


bool CPlaybackOptions::Apply(DWORD Flags)
{
	CCoreEngine &CoreEngine=GetAppClass().CoreEngine;

	if ((Flags&UPDATE_ADJUSTAUDIOSTREAMTIME)!=0) {
		CoreEngine.m_DtvEngine.m_MediaViewer.SetAdjustAudioStreamTime(m_fAdjustAudioStreamTime);
	}

	if ((Flags&UPDATE_PTSSYNC)!=0) {
		CoreEngine.m_DtvEngine.m_MediaViewer.EnablePTSSync(m_fEnablePTSSync);
	}

	if ((Flags&UPDATE_PACKETBUFFERING)!=0) {
		CoreEngine.SetPacketBufferLength(m_PacketBufferLength);
		CoreEngine.SetPacketBufferPool(m_fPacketBuffering,m_PacketBufferPoolPercentage);
	}

	if ((Flags&UPDATE_STREAMTHREADPRIORITY)!=0) {
		CoreEngine.m_DtvEngine.m_BonSrcDecoder.SetStreamThreadPriority(m_StreamThreadPriority);
	}

	if ((Flags&UPDATE_ADJUSTFRAMERATE)!=0) {
		CoreEngine.m_DtvEngine.m_MediaViewer.SetAdjust1SegVideoSampleTime(m_fAdjust1SegFrameRate);
	}

	return true;
}


bool CPlaybackOptions::ReadSettings(CSettings &Settings)
{
	Settings.Read(TEXT("RestoreMute"),&m_fRestoreMute);
	Settings.Read(TEXT("Mute"),&m_fMute);
	Settings.Read(TEXT("RestorePlayStatus"),&m_fRestorePlayStatus);
	Settings.Read(TEXT("UseAudioRendererClock"),&m_fUseAudioRendererClock);
	Settings.Read(TEXT("PTSSync"),&m_fEnablePTSSync);
	Settings.Read(TEXT("AdjustAudioStreamTime"),&m_fAdjustAudioStreamTime);
	Settings.Read(TEXT("MinTimerResolution"),&m_fMinTimerResolution);
	Settings.Read(TEXT("PacketBuffering"),&m_fPacketBuffering);
	unsigned int BufferLength;
	if (Settings.Read(TEXT("PacketBufferLength"),&BufferLength))
		m_PacketBufferLength=min(BufferLength,MAX_PACKET_BUFFER_LENGTH);
	if (Settings.Read(TEXT("PacketBufferPoolPercentage"),&m_PacketBufferPoolPercentage))
		m_PacketBufferPoolPercentage=CLAMP(m_PacketBufferPoolPercentage,0,100);
	if (Settings.Read(TEXT("StreamThreadPriority"),&m_StreamThreadPriority))
		m_StreamThreadPriority=CLAMP(m_StreamThreadPriority,THREAD_PRIORITY_NORMAL,THREAD_PRIORITY_HIGHEST);
	Settings.Read(TEXT("AdjustFrameRate"),&m_fAdjust1SegFrameRate);

	return true;
}


bool CPlaybackOptions::WriteSettings(CSettings &Settings)
{
	Settings.Write(TEXT("RestoreMute"),m_fRestoreMute);
	Settings.Write(TEXT("Mute"),GetAppClass().CoreEngine.GetMute());
	Settings.Write(TEXT("RestorePlayStatus"),m_fRestorePlayStatus);
	Settings.Write(TEXT("UseAudioRendererClock"),m_fUseAudioRendererClock);
	Settings.Write(TEXT("PTSSync"),m_fEnablePTSSync);
	Settings.Write(TEXT("AdjustAudioStreamTime"),m_fAdjustAudioStreamTime);
	Settings.Write(TEXT("MinTimerResolution"),m_fMinTimerResolution);
	Settings.Write(TEXT("PacketBuffering"),m_fPacketBuffering);
	Settings.Write(TEXT("PacketBufferLength"),(unsigned int)m_PacketBufferLength);
	Settings.Write(TEXT("PacketBufferPoolPercentage"),m_PacketBufferPoolPercentage);
	Settings.Write(TEXT("StreamThreadPriority"),m_StreamThreadPriority);
	Settings.Write(TEXT("AdjustFrameRate"),m_fAdjust1SegFrameRate);

	return true;
}


bool CPlaybackOptions::Create(HWND hwndOwner)
{
	return CreateDialogWindow(hwndOwner,
							  GetAppClass().GetResourceInstance(),
							  MAKEINTRESOURCE(IDD_OPTIONS_PLAYBACK));
}


void CPlaybackOptions::SetPacketBuffering(bool fBuffering)
{
	m_fPacketBuffering=fBuffering;
}


INT_PTR CPlaybackOptions::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			DlgCheckBox_Check(hDlg,IDC_OPTIONS_RESTOREMUTE,
							  m_fRestoreMute);
			DlgCheckBox_Check(hDlg,IDC_OPTIONS_RESTOREPLAYSTATUS,
							  m_fRestorePlayStatus);

			DlgCheckBox_Check(hDlg,IDC_OPTIONS_MINTIMERRESOLUTION,
							  m_fMinTimerResolution);
			DlgCheckBox_Check(hDlg,IDC_OPTIONS_ADJUSTAUDIOSTREAMTIME,
							  m_fAdjustAudioStreamTime);
			DlgCheckBox_Check(hDlg,IDC_OPTIONS_PTSSYNC,
							  m_fEnablePTSSync);
			DlgCheckBox_Check(hDlg,IDC_OPTIONS_USEDEMUXERCLOCK,
							  !m_fUseAudioRendererClock);

			// Buffering
			SetDlgItemInt(hDlg,IDC_OPTIONS_BUFFERSIZE,m_PacketBufferLength,FALSE);
			DlgUpDown_SetRange(hDlg,IDC_OPTIONS_BUFFERSIZE_UD,1,MAX_PACKET_BUFFER_LENGTH);
			DlgCheckBox_Check(hDlg,IDC_OPTIONS_ENABLEBUFFERING,
							  m_fPacketBuffering);
			EnableDlgItems(hDlg,IDC_OPTIONS_BUFFERING_FIRST,IDC_OPTIONS_BUFFERING_LAST,
						   m_fPacketBuffering);
			SetDlgItemInt(hDlg,IDC_OPTIONS_BUFFERPOOLPERCENTAGE,
						  m_PacketBufferPoolPercentage,TRUE);
			DlgUpDown_SetRange(hDlg,IDC_OPTIONS_BUFFERPOOLPERCENTAGE_UD,0,100);

			static const LPCTSTR ThreadPriorityList[] = {
				TEXT("í èÌ (çƒê∂óDêÊ)"),
				TEXT("çÇÇﬂ"),
				TEXT("ç≈çÇ (ò^âÊóDêÊ)"),
			};
			for (int i=0;i<lengthof(ThreadPriorityList);i++)
				DlgComboBox_AddString(hDlg,IDC_OPTIONS_STREAMTHREADPRIORITY,ThreadPriorityList[i]);
			DlgComboBox_SetCurSel(hDlg,IDC_OPTIONS_STREAMTHREADPRIORITY,m_StreamThreadPriority);

			DlgCheckBox_Check(hDlg,IDC_OPTIONS_ADJUSTFRAMERATE,
							  m_fAdjust1SegFrameRate);
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_OPTIONS_ENABLEBUFFERING:
			EnableDlgItemsSyncCheckBox(hDlg,
									   IDC_OPTIONS_BUFFERING_FIRST,
									   IDC_OPTIONS_BUFFERING_LAST,
									   IDC_OPTIONS_ENABLEBUFFERING);
			return TRUE;
		}
		return TRUE;

	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code) {
		case PSN_APPLY:
			{
				m_fRestoreMute=
					DlgCheckBox_IsChecked(hDlg,IDC_OPTIONS_RESTOREMUTE);
				m_fRestorePlayStatus=
					DlgCheckBox_IsChecked(hDlg,IDC_OPTIONS_RESTOREPLAYSTATUS);

				bool f=DlgCheckBox_IsChecked(hDlg,IDC_OPTIONS_MINTIMERRESOLUTION);
				if (f!=m_fMinTimerResolution) {
					m_fMinTimerResolution=f;
					if (GetAppClass().UICore.IsViewerEnabled())
						GetAppClass().CoreEngine.SetMinTimerResolution(f);
				}

				f=DlgCheckBox_IsChecked(hDlg,IDC_OPTIONS_ADJUSTAUDIOSTREAMTIME);
				if (f!=m_fAdjustAudioStreamTime) {
					m_fAdjustAudioStreamTime=f;
					SetUpdateFlag(UPDATE_ADJUSTAUDIOSTREAMTIME);
				}

				f=DlgCheckBox_IsChecked(hDlg,IDC_OPTIONS_PTSSYNC);
				if (f!=m_fEnablePTSSync) {
					m_fEnablePTSSync=f;
					SetUpdateFlag(UPDATE_PTSSYNC);
				}

				f=!DlgCheckBox_IsChecked(hDlg,IDC_OPTIONS_USEDEMUXERCLOCK);
				if (f!=m_fUseAudioRendererClock) {
					m_fUseAudioRendererClock=f;
					GetAppClass().CoreEngine.m_DtvEngine.m_MediaViewer.SetUseAudioRendererClock(f);
					SetGeneralUpdateFlag(UPDATE_GENERAL_BUILDMEDIAVIEWER);
				}

				DWORD BufferLength=::GetDlgItemInt(hDlg,IDC_OPTIONS_BUFFERSIZE,NULL,FALSE);
				BufferLength=CLAMP(BufferLength,0,MAX_PACKET_BUFFER_LENGTH);
				bool fBuffering=DlgCheckBox_IsChecked(hDlg,IDC_OPTIONS_ENABLEBUFFERING);
				int PoolPercentage=::GetDlgItemInt(hDlg,IDC_OPTIONS_BUFFERPOOLPERCENTAGE,NULL,TRUE);
				PoolPercentage=CLAMP(PoolPercentage,0,100);
				if (BufferLength!=m_PacketBufferLength
						|| fBuffering!=m_fPacketBuffering
						|| (fBuffering && PoolPercentage!=m_PacketBufferPoolPercentage))
					SetUpdateFlag(UPDATE_PACKETBUFFERING);
				m_PacketBufferLength=BufferLength;
				m_fPacketBuffering=fBuffering;
				m_PacketBufferPoolPercentage=PoolPercentage;

				int Priority=(int)DlgComboBox_GetCurSel(hDlg,IDC_OPTIONS_STREAMTHREADPRIORITY);
				if (Priority>=0 && Priority!=m_StreamThreadPriority) {
					m_StreamThreadPriority=Priority;
					SetUpdateFlag(UPDATE_STREAMTHREADPRIORITY);
				}

				f=DlgCheckBox_IsChecked(hDlg,IDC_OPTIONS_ADJUSTFRAMERATE);
				if (f!=m_fAdjust1SegFrameRate) {
					m_fAdjust1SegFrameRate=f;
					SetUpdateFlag(UPDATE_ADJUSTFRAMERATE);
				}

				m_fChanged=true;
			}
			break;
		}
		break;
	}

	return FALSE;
}
