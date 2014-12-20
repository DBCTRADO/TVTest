#include "stdafx.h"
#include "TVTest.h"
#include "AppMain.h"
#include "PlaybackOptions.h"
#include "DirectShowFilter/DirectShowUtil.h"
#include "DialogUtil.h"
#include "resource.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif




static const int MAX_FORMAT_DOUBLE_LENGTH=16;

static void FormatDouble(double Value,LPTSTR pszString,int MaxString)
{
	int Length=StdUtil::snprintf(pszString,MaxString,TEXT("%.4f"),Value);
	int i;
	for (i=Length-1;i>1;i--) {
		if (pszString[i]!=TEXT('0')) {
			i++;
			break;
		}
		if (pszString[i]==TEXT('.'))
			break;
	}
	pszString[i]=TEXT('\0');
}




const CAudioDecFilter::SurroundMixingMatrix CPlaybackOptions::m_DefaultSurroundMixingMatrix = {
	{
		{1.0, 0.0, 0.0, 0.0, 0.0, 0.0},
		{0.0, 1.0, 0.0, 0.0, 0.0, 0.0},
		{0.0, 0.0, 1.0, 0.0, 0.0, 0.0},
		{0.0, 0.0, 0.0, 1.0, 0.0, 0.0},
		{0.0, 0.0, 0.0, 0.0, 1.0, 0.0},
		{0.0, 0.0, 0.0, 0.0, 0.0, 1.0}
	}
};

static const double PSQR = 1.0 / 1.4142135623730950488016887242097;
const CAudioDecFilter::DownMixMatrix CPlaybackOptions::m_DefaultDownMixMatrix = {
	{
		{1.0, 0.0, PSQR, PSQR, PSQR, 0.0},
		{0.0, 1.0, PSQR, PSQR, 0.0,  PSQR}
	}
};

CPlaybackOptions::CPlaybackOptions()
	: m_SpdifOptions(CAudioDecFilter::SPDIF_MODE_DISABLED,CAudioDecFilter::SPDIF_CHANNELS_SURROUND)
	, m_fDownMixSurround(true)
	, m_fUseCustomSurroundMixingMatrix(false)
	, m_SurroundMixingMatrix(m_DefaultSurroundMixingMatrix)
	, m_fUseCustomDownMixMatrix(false)
	, m_DownMixMatrix(m_DefaultDownMixMatrix)
	, m_fRestoreMute(false)
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
	int Value;

	Settings.Read(TEXT("AudioDevice"),&m_AudioDeviceName);
	Settings.Read(TEXT("AudioFilter"),&m_AudioFilterName);
	if (Settings.Read(TEXT("SpdifMode"),&Value))
		m_SpdifOptions.Mode=(CAudioDecFilter::SpdifMode)Value;
	Settings.Read(TEXT("SpdifChannels"),&m_SpdifOptions.PassthroughChannels);
	Settings.Read(TEXT("DownMixSurround"),&m_fDownMixSurround);

	Settings.Read(TEXT("UseCustomSurroundMixingMatrix"),&m_fUseCustomSurroundMixingMatrix);
	TVTest::String Buffer;
	if (Settings.Read(TEXT("SurroundMixingMatrix"),&Buffer) && !Buffer.empty()) {
		std::vector<TVTest::String> List;
		if (TVTest::StringUtility::Split(Buffer,TEXT(","),&List) && List.size()>=6*6) {
			for (int i=0;i<6;i++) {
				for (int j=0;j<6;j++) {
					double Value=std::_tcstod(List[i*6+j].c_str(),NULL);
					if (Value==HUGE_VAL || Value==-HUGE_VAL)
						Value=0.0;
					m_SurroundMixingMatrix.Matrix[i][j]=Value;
				}
			}
		}
	}

	Settings.Read(TEXT("UseCustomDownMixMatrix"),&m_fUseCustomDownMixMatrix);
	if (Settings.Read(TEXT("DownMixMatrix"),&Buffer) && !Buffer.empty()) {
		std::vector<TVTest::String> List;
		if (TVTest::StringUtility::Split(Buffer,TEXT(","),&List) && List.size()>=2*6) {
			for (int i=0;i<2;i++) {
				for (int j=0;j<6;j++) {
					double Value=std::_tcstod(List[i*6+j].c_str(),NULL);
					if (Value==HUGE_VAL || Value==-HUGE_VAL)
						Value=0.0;
					m_DownMixMatrix.Matrix[i][j]=Value;
				}
			}
		}
	}

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
	Settings.Write(TEXT("AudioDevice"),m_AudioDeviceName);
	Settings.Write(TEXT("AudioFilter"),m_AudioFilterName);
	Settings.Write(TEXT("SpdifMode"),(int)m_SpdifOptions.Mode);
	Settings.Write(TEXT("SpdifChannels"),m_SpdifOptions.PassthroughChannels);
	Settings.Write(TEXT("DownMixSurround"),m_fDownMixSurround);

	Settings.Write(TEXT("UseCustomSurroundMixingMatrix"),m_fUseCustomSurroundMixingMatrix);
	TVTest::String Buffer;
	for (int i=0;i<6;i++) {
		for (int j=0;j<6;j++) {
			if (!Buffer.empty())
				Buffer+=TEXT(',');
			TCHAR szValue[MAX_FORMAT_DOUBLE_LENGTH];
			FormatDouble(m_SurroundMixingMatrix.Matrix[i][j],szValue,lengthof(szValue));
			Buffer+=szValue;
		}
	}
	Settings.Write(TEXT("SurroundMixingMatrix"),Buffer);

	Settings.Write(TEXT("UseCustomDownMixMatrix"),m_fUseCustomDownMixMatrix);
	Buffer.clear();
	for (int i=0;i<2;i++) {
		for (int j=0;j<6;j++) {
			if (!Buffer.empty())
				Buffer+=TEXT(',');
			TCHAR szValue[MAX_FORMAT_DOUBLE_LENGTH];
			FormatDouble(m_DownMixMatrix.Matrix[i][j],szValue,lengthof(szValue));
			Buffer+=szValue;
		}
	}
	Settings.Write(TEXT("DownMixMatrix"),Buffer);

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
							  GetAppClass().GetResourceInstance(),MAKEINTRESOURCE(IDD_OPTIONS_PLAYBACK));
}


bool CPlaybackOptions::ApplyMediaViewerOptions()
{
	CMediaViewer &MediaViewer=GetAppClass().CoreEngine.m_DtvEngine.m_MediaViewer;

	MediaViewer.SetDownMixSurround(m_fDownMixSurround);

	CAudioDecFilter *pAudioDecFilter;
	if (MediaViewer.GetAudioDecFilter(&pAudioDecFilter)) {
		pAudioDecFilter->SetSurroundMixingMatrix(
			m_fUseCustomSurroundMixingMatrix?&m_SurroundMixingMatrix:NULL);
		pAudioDecFilter->SetDownMixMatrix(
			m_fUseCustomDownMixMatrix?&m_DownMixMatrix:NULL);
		pAudioDecFilter->Release();
	}

	return true;
}


bool CPlaybackOptions::SetSpdifOptions(const CAudioDecFilter::SpdifOptions &Options)
{
	m_SpdifOptions=Options;
	return true;
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
			int Sel=0;
			DlgComboBox_AddString(hDlg,IDC_OPTIONS_AUDIODEVICE,TEXT("デフォルト"));
			CDirectShowDeviceEnumerator DevEnum;
			if (DevEnum.EnumDevice(CLSID_AudioRendererCategory)) {
				for (int i=0;i<DevEnum.GetDeviceCount();i++) {
					LPCTSTR pszName=DevEnum.GetDeviceFriendlyName(i);
					DlgComboBox_AddString(hDlg,IDC_OPTIONS_AUDIODEVICE,pszName);
					if (Sel==0 && !m_AudioDeviceName.empty()
							&& TVTest::StringUtility::CompareNoCase(m_AudioDeviceName,pszName)==0)
						Sel=i+1;
				}
			}
			DlgComboBox_SetCurSel(hDlg,IDC_OPTIONS_AUDIODEVICE,Sel);

			Sel=0;
			DlgComboBox_AddString(hDlg,IDC_OPTIONS_AUDIOFILTER,TEXT("なし"));
			CDirectShowFilterFinder FilterFinder;
			static const GUID InputTypes[] = {
				MEDIATYPE_Audio,	MEDIASUBTYPE_PCM
			};
			static const GUID OutputTypes[] = {
				MEDIATYPE_Audio,	MEDIASUBTYPE_PCM,
				MEDIATYPE_Audio,	MEDIASUBTYPE_DOLBY_AC3,
				MEDIATYPE_Audio,	MEDIASUBTYPE_DOLBY_AC3_SPDIF,
				MEDIATYPE_Audio,	MEDIASUBTYPE_DTS,
				MEDIATYPE_Audio,	MEDIASUBTYPE_AAC,
			};
			if (FilterFinder.FindFilter(InputTypes,lengthof(InputTypes),
										OutputTypes,lengthof(OutputTypes),
										0/*MERIT_DO_NOT_USE*/)) {
				WCHAR szAudioFilter[MAX_AUDIO_FILTER_NAME];
				CLSID idAudioFilter;

				for (int i=0;i<FilterFinder.GetFilterCount();i++) {
					if (FilterFinder.GetFilterInfo(i,&idAudioFilter,szAudioFilter,lengthof(szAudioFilter))) {
						DlgComboBox_AddString(hDlg,IDC_OPTIONS_AUDIOFILTER,szAudioFilter);
						if (Sel==0 && !m_AudioFilterName.empty()
								&& TVTest::StringUtility::CompareNoCase(m_AudioFilterName,szAudioFilter)==0)
							Sel=i+1;
					}
				}
			}
			DlgComboBox_SetCurSel(hDlg,IDC_OPTIONS_AUDIOFILTER,Sel);

			static const LPCTSTR SpdifModeList[] = {
				TEXT("パススルー出力を行わない"),
				TEXT("常にパススルー出力を行う"),
				TEXT("音声の形式に応じてパススルー出力を行う"),
			};
			for (int i=0;i<lengthof(SpdifModeList);i++)
				DlgComboBox_AddString(hDlg,IDC_OPTIONS_SPDIFMODE,SpdifModeList[i]);
			DlgComboBox_SetCurSel(hDlg,IDC_OPTIONS_SPDIFMODE,(int)m_SpdifOptions.Mode);
			DlgCheckBox_Check(hDlg,IDC_OPTIONS_SPDIF_CHANNELS_MONO,
							  (m_SpdifOptions.PassthroughChannels&CAudioDecFilter::SPDIF_CHANNELS_MONO)!=0);
			DlgCheckBox_Check(hDlg,IDC_OPTIONS_SPDIF_CHANNELS_DUALMONO,
							  (m_SpdifOptions.PassthroughChannels&CAudioDecFilter::SPDIF_CHANNELS_DUALMONO)!=0);
			DlgCheckBox_Check(hDlg,IDC_OPTIONS_SPDIF_CHANNELS_STEREO,
							  (m_SpdifOptions.PassthroughChannels&CAudioDecFilter::SPDIF_CHANNELS_STEREO)!=0);
			DlgCheckBox_Check(hDlg,IDC_OPTIONS_SPDIF_CHANNELS_SURROUND,
							  (m_SpdifOptions.PassthroughChannels&CAudioDecFilter::SPDIF_CHANNELS_SURROUND)!=0);
			EnableDlgItems(hDlg,IDC_OPTIONS_SPDIF_CHANNELS_LABEL,IDC_OPTIONS_SPDIF_CHANNELS_SURROUND,
						   m_SpdifOptions.Mode==CAudioDecFilter::SPDIF_MODE_AUTO);

			DlgCheckBox_Check(hDlg,IDC_OPTIONS_DOWNMIXSURROUND,
							  m_fDownMixSurround);
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
				TEXT("通常 (再生優先)"),
				TEXT("高め"),
				TEXT("最高 (録画優先)"),
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
		case IDC_OPTIONS_SPDIFMODE:
			if (HIWORD(wParam)==CBN_SELCHANGE) {
				EnableDlgItems(hDlg,
							   IDC_OPTIONS_SPDIF_CHANNELS_LABEL,
							   IDC_OPTIONS_SPDIF_CHANNELS_SURROUND,
							   DlgComboBox_GetCurSel(hDlg,IDC_OPTIONS_SPDIFMODE)==CAudioDecFilter::SPDIF_MODE_AUTO);
			}
			return TRUE;

		case IDC_OPTIONS_SURROUNDOPTIONS:
			{
				CSurroundOptionsDialog Dlg(this);

				Dlg.Show(hDlg);
			}
			return TRUE;

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
				TCHAR szAudioDevice[MAX_AUDIO_DEVICE_NAME];
				LRESULT Sel=DlgComboBox_GetCurSel(hDlg,IDC_OPTIONS_AUDIODEVICE);
				if (Sel<=0)
					szAudioDevice[0]=_T('\0');
				else
					DlgComboBox_GetLBString(hDlg,IDC_OPTIONS_AUDIODEVICE,Sel,szAudioDevice);
				if (TVTest::StringUtility::CompareNoCase(m_AudioDeviceName,szAudioDevice)!=0) {
					m_AudioDeviceName=szAudioDevice;
					SetGeneralUpdateFlag(UPDATE_GENERAL_BUILDMEDIAVIEWER);
				}

				TCHAR szAudioFilter[MAX_AUDIO_FILTER_NAME];
				Sel=DlgComboBox_GetCurSel(hDlg,IDC_OPTIONS_AUDIOFILTER);
				if (Sel<=0)
					szAudioFilter[0]=_T('\0');
				else
					DlgComboBox_GetLBString(hDlg,IDC_OPTIONS_AUDIOFILTER,Sel,szAudioFilter);
				if (TVTest::StringUtility::CompareNoCase(m_AudioFilterName,szAudioFilter)!=0) {
					m_AudioFilterName=szAudioFilter;
					SetGeneralUpdateFlag(UPDATE_GENERAL_BUILDMEDIAVIEWER);
				}

				CAudioDecFilter::SpdifOptions SpdifOptions;
				SpdifOptions.Mode=(CAudioDecFilter::SpdifMode)
					DlgComboBox_GetCurSel(hDlg,IDC_OPTIONS_SPDIFMODE);
				SpdifOptions.PassthroughChannels=0;
				if (DlgCheckBox_IsChecked(hDlg,IDC_OPTIONS_SPDIF_CHANNELS_MONO))
					SpdifOptions.PassthroughChannels|=CAudioDecFilter::SPDIF_CHANNELS_MONO;
				if (DlgCheckBox_IsChecked(hDlg,IDC_OPTIONS_SPDIF_CHANNELS_DUALMONO))
					SpdifOptions.PassthroughChannels|=CAudioDecFilter::SPDIF_CHANNELS_DUALMONO;
				if (DlgCheckBox_IsChecked(hDlg,IDC_OPTIONS_SPDIF_CHANNELS_STEREO))
					SpdifOptions.PassthroughChannels|=CAudioDecFilter::SPDIF_CHANNELS_STEREO;
				if (DlgCheckBox_IsChecked(hDlg,IDC_OPTIONS_SPDIF_CHANNELS_SURROUND))
					SpdifOptions.PassthroughChannels|=CAudioDecFilter::SPDIF_CHANNELS_SURROUND;
				if (SpdifOptions!=m_SpdifOptions) {
					m_SpdifOptions=SpdifOptions;
					GetAppClass().CoreEngine.SetSpdifOptions(m_SpdifOptions);
				}

				m_fDownMixSurround=
					DlgCheckBox_IsChecked(hDlg,IDC_OPTIONS_DOWNMIXSURROUND);
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

				ApplyMediaViewerOptions();

				m_fChanged=true;
			}
			break;
		}
		break;
	}

	return FALSE;
}




CPlaybackOptions::CSurroundOptionsDialog::CSurroundOptionsDialog(CPlaybackOptions *pOptions)
	: m_pOptions(pOptions)
{
}


bool CPlaybackOptions::CSurroundOptionsDialog::Show(HWND hwndOwner)
{
	return ShowDialog(hwndOwner,
					  GetAppClass().GetResourceInstance(),
					  MAKEINTRESOURCE(IDD_SURROUNDOPTIONS))==IDOK;
}


INT_PTR CPlaybackOptions::CSurroundOptionsDialog::DlgProc(
	HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			DlgCheckBox_Check(hDlg,IDC_SURROUND_USECUSTOMDOWNMIXMATRIX,
							  m_pOptions->m_fUseCustomDownMixMatrix);
			EnableDlgItems(hDlg,
						   IDC_SURROUND_DOWNMIXMATRIX_FL_LABEL,
						   IDC_SURROUND_DOWNMIXMATRIX_DEFAULT,
						   m_pOptions->m_fUseCustomDownMixMatrix);
			SetDownMixMatrix(m_pOptions->m_DownMixMatrix);

			DlgCheckBox_Check(hDlg,IDC_SURROUND_USECUSTOMMIXINGMATRIX,
							  m_pOptions->m_fUseCustomSurroundMixingMatrix);
			EnableDlgItems(hDlg,
						   IDC_SURROUND_MIXINGMATRIX_IN_FL_LABEL,
						   IDC_SURROUND_MIXINGMATRIX_SR_SR,
						   m_pOptions->m_fUseCustomSurroundMixingMatrix);
			int ID=IDC_SURROUND_MIXINGMATRIX_FL_FL;
			for (int i=0;i<6;i++) {
				for (int j=0;j<6;j++) {
					TCHAR szText[MAX_FORMAT_DOUBLE_LENGTH];
					FormatDouble(m_pOptions->m_SurroundMixingMatrix.Matrix[i][j],
								 szText,lengthof(szText));
					::SetDlgItemText(hDlg,ID,szText);
					ID++;
				}
			}
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_SURROUND_USECUSTOMDOWNMIXMATRIX:
			EnableDlgItemsSyncCheckBox(hDlg,
				IDC_SURROUND_DOWNMIXMATRIX_FL_LABEL,
				IDC_SURROUND_DOWNMIXMATRIX_DEFAULT,
				IDC_SURROUND_USECUSTOMDOWNMIXMATRIX);
			return TRUE;

		case IDC_SURROUND_DOWNMIXMATRIX_DEFAULT:
			SetDownMixMatrix(m_pOptions->m_DefaultDownMixMatrix);
			return TRUE;

		case IDC_SURROUND_USECUSTOMMIXINGMATRIX:
			EnableDlgItemsSyncCheckBox(hDlg,
				IDC_SURROUND_MIXINGMATRIX_IN_FL_LABEL,
				IDC_SURROUND_MIXINGMATRIX_SR_SR,
				IDC_SURROUND_USECUSTOMMIXINGMATRIX);
			return TRUE;

		case IDOK:
			{
				m_pOptions->m_fUseCustomDownMixMatrix=
					DlgCheckBox_IsChecked(hDlg,IDC_SURROUND_USECUSTOMDOWNMIXMATRIX);
				int ID=IDC_SURROUND_DOWNMIXMATRIX_L_FL;
				for (int i=0;i<2;i++) {
					for (int j=0;j<6;j++) {
						TCHAR szText[16];
						::GetDlgItemText(hDlg,ID,szText,lengthof(szText));
						double Value=std::_tcstod(szText,NULL);
						if (Value==HUGE_VAL || Value==-HUGE_VAL)
							Value=0.0;
						m_pOptions->m_DownMixMatrix.Matrix[i][j]=Value;
						ID++;
					}
				}

				m_pOptions->m_fUseCustomSurroundMixingMatrix=
					DlgCheckBox_IsChecked(hDlg,IDC_SURROUND_USECUSTOMMIXINGMATRIX);
				ID=IDC_SURROUND_MIXINGMATRIX_FL_FL;
				for (int i=0;i<6;i++) {
					for (int j=0;j<6;j++) {
						TCHAR szText[16];
						::GetDlgItemText(hDlg,ID,szText,lengthof(szText));
						double Value=std::_tcstod(szText,NULL);
						if (Value==HUGE_VAL || Value==-HUGE_VAL)
							Value=0.0;
						m_pOptions->m_SurroundMixingMatrix.Matrix[i][j]=Value;
						ID++;
					}
				}

				m_pOptions->m_fChanged=true;
				m_pOptions->ApplyMediaViewerOptions();
			}
		case IDCANCEL:
			::EndDialog(hDlg,LOWORD(wParam));
			return TRUE;
		}
		return TRUE;
	}

	return FALSE;
}


void CPlaybackOptions::CSurroundOptionsDialog::SetDownMixMatrix(
	const CAudioDecFilter::DownMixMatrix &Matrix)
{
	int ID=IDC_SURROUND_DOWNMIXMATRIX_L_FL;
	for (int i=0;i<2;i++) {
		for (int j=0;j<6;j++) {
			TCHAR szText[MAX_FORMAT_DOUBLE_LENGTH];
			FormatDouble(Matrix.Matrix[i][j],szText,lengthof(szText));
			::SetDlgItemText(m_hDlg,ID,szText);
			ID++;
		}
	}
}
