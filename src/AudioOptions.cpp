#include "stdafx.h"
#include "TVTest.h"
#include "AppMain.h"
#include "AudioOptions.h"
#include "DirectShowFilter/DirectShowUtil.h"
#include "DialogUtil.h"
#include "EpgUtil.h"
#include "resource.h"
#include <algorithm>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


static const int MAX_LANGUAGE_TEXT_LENGTH=EpgUtil::MAX_LANGUAGE_TEXT_LENGTH+16;


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




const CAudioDecFilter::SurroundMixingMatrix CAudioOptions::m_DefaultSurroundMixingMatrix = {
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
const CAudioDecFilter::DownMixMatrix CAudioOptions::m_DefaultDownMixMatrix = {
	{
		{1.0, 0.0, PSQR, PSQR, PSQR, 0.0},
		{0.0, 1.0, PSQR, PSQR, 0.0,  PSQR}
	}
};

const DWORD CAudioOptions::m_AudioLanguageList[] = {
	LANGUAGE_CODE_JPN,	// ���{��
	LANGUAGE_CODE_ENG,	// �p��
	LANGUAGE_CODE_DEU,	// �h�C�c��
	LANGUAGE_CODE_FRA,	// �t�����X��
	LANGUAGE_CODE_ITA,	// �C�^���A��
	LANGUAGE_CODE_KOR,	// �؍���
	LANGUAGE_CODE_RUS,	// ���V�A��
	LANGUAGE_CODE_SPA,	// �X�y�C����
	LANGUAGE_CODE_ZHO,	// ������
	LANGUAGE_CODE_ETC,	// ���̑�
};


CAudioOptions::CAudioOptions()
	: m_SpdifOptions(CAudioDecFilter::SPDIF_MODE_DISABLED,CAudioDecFilter::SPDIF_CHANNELS_SURROUND)
	, m_fDownMixSurround(true)
	, m_fUseCustomSurroundMixingMatrix(false)
	, m_SurroundMixingMatrix(m_DefaultSurroundMixingMatrix)
	, m_fUseCustomDownMixMatrix(false)
	, m_DownMixMatrix(m_DefaultDownMixMatrix)
	, m_fEnableLanguagePriority(false)
{
}


CAudioOptions::~CAudioOptions()
{
	Destroy();
}


bool CAudioOptions::ReadSettings(CSettings &Settings)
{
	Settings.Read(TEXT("AudioDevice"),&m_AudioDeviceName);
	Settings.Read(TEXT("AudioFilter"),&m_AudioFilterName);
	int SpdifMode;
	if (Settings.Read(TEXT("SpdifMode"),&SpdifMode))
		m_SpdifOptions.Mode=(CAudioDecFilter::SpdifMode)SpdifMode;
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

	Settings.Read(TEXT("EnableLangPriority"),&m_fEnableLanguagePriority);

	m_LanguagePriority.clear();
	for (int i=0;;i++) {
		TCHAR szKey[32];
		TVTest::String Value;
		StdUtil::snprintf(szKey,lengthof(szKey),TEXT("LangPriority%d"),i);
		if (!Settings.Read(szKey,&Value) || Value.length()<3)
			break;
		AudioLanguageInfo Info;
		Info.Language=(Value[0]<<16)|(Value[1]<<8)|Value[2];
		Info.fSub=Value.length()>=4 && Value[3]==TEXT('2');
		m_LanguagePriority.push_back(Info);
	}

	return true;
}


bool CAudioOptions::WriteSettings(CSettings &Settings)
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

	Settings.Write(TEXT("EnableLangPriority"),m_fEnableLanguagePriority);
	for (int i=0;;i++) {
		TCHAR szKey[32];
		StdUtil::snprintf(szKey,lengthof(szKey),TEXT("LangPriority%d"),i);

		if (i<(int)m_LanguagePriority.size()) {
			const DWORD Lang=m_LanguagePriority[i].Language;
			TCHAR szValue[8];
			StdUtil::snprintf(szValue,lengthof(szValue),TEXT("%c%c%c%s"),
							  (Lang>>16),(Lang>>8)&0xFF,Lang&0xFF,
							  m_LanguagePriority[i].fSub?TEXT("2"):TEXT(""));
			Settings.Write(szKey,szValue);
		} else {
			if (!Settings.DeleteValue(szKey))
				break;
		}
	}

	return true;
}


bool CAudioOptions::Create(HWND hwndOwner)
{
	return CreateDialogWindow(hwndOwner,
							  GetAppClass().GetResourceInstance(),
							  MAKEINTRESOURCE(IDD_OPTIONS_AUDIO));
}


bool CAudioOptions::ApplyMediaViewerOptions()
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


bool CAudioOptions::SetSpdifOptions(const CAudioDecFilter::SpdifOptions &Options)
{
	m_SpdifOptions=Options;
	return true;
}


INT_PTR CAudioOptions::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			int Sel=0;
			DlgComboBox_AddString(hDlg,IDC_OPTIONS_AUDIODEVICE,TEXT("�f�t�H���g"));
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
			DlgComboBox_AddString(hDlg,IDC_OPTIONS_AUDIOFILTER,TEXT("�Ȃ�"));
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
				TVTest::String AudioFilter;
				CLSID idAudioFilter;

				for (int i=0;i<FilterFinder.GetFilterCount();i++) {
					if (FilterFinder.GetFilterInfo(i,&idAudioFilter,&AudioFilter)) {
						DlgComboBox_AddString(hDlg,IDC_OPTIONS_AUDIOFILTER,AudioFilter.c_str());
						if (Sel==0 && !m_AudioFilterName.empty()
								&& TVTest::StringUtility::CompareNoCase(m_AudioFilterName,AudioFilter)==0)
							Sel=i+1;
					}
				}
			}
			DlgComboBox_SetCurSel(hDlg,IDC_OPTIONS_AUDIOFILTER,Sel);

			static const LPCTSTR SpdifModeList[] = {
				TEXT("�p�X�X���[�o�͂��s��Ȃ�"),
				TEXT("��Ƀp�X�X���[�o�͂��s��"),
				TEXT("�����̌`���ɉ����ăp�X�X���[�o�͂��s��"),
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

			DlgCheckBox_Check(hDlg,IDC_OPTIONS_ENABLEAUDIOLANGUAGEPRIORITY,
							  m_fEnableLanguagePriority);

			for (size_t i=0;i<m_LanguagePriority.size();i++) {
				const AudioLanguageInfo &Info=m_LanguagePriority[i];
				TCHAR szText[MAX_LANGUAGE_TEXT_LENGTH];
				GetLanguageText(Info.Language,Info.fSub,szText,lengthof(szText));
				if (DlgListBox_FindStringExact(hDlg,IDC_OPTIONS_AUDIOLANGUAGEPRIORITY,0,szText)<0) {
					DlgListBox_AddString(hDlg,IDC_OPTIONS_AUDIOLANGUAGEPRIORITY,szText);
					DWORD Param=Info.Language;
					if (Info.fSub)
						Param|=LANGUAGE_FLAG_SUB;
					DlgListBox_SetItemData(hDlg,IDC_OPTIONS_AUDIOLANGUAGEPRIORITY,i,Param);
				}
			}

			for (int i=0;i<lengthof(m_AudioLanguageList);i++) {
				const DWORD Language=m_AudioLanguageList[i];
				TCHAR szText[MAX_LANGUAGE_TEXT_LENGTH];
				GetLanguageText(Language,false,szText,lengthof(szText));
				int Index=(int)DlgComboBox_AddString(hDlg,IDC_OPTIONS_AUDIOLANGUAGELIST,szText);
				DlgComboBox_SetItemData(hDlg,IDC_OPTIONS_AUDIOLANGUAGELIST,Index,Language);
				GetLanguageText(Language,true,szText,lengthof(szText));
				Index=(int)DlgComboBox_AddString(hDlg,IDC_OPTIONS_AUDIOLANGUAGELIST,szText);
				DlgComboBox_SetItemData(hDlg,IDC_OPTIONS_AUDIOLANGUAGELIST,Index,Language | LANGUAGE_FLAG_SUB);
			}
			DlgComboBox_SetCurSel(hDlg,IDC_OPTIONS_AUDIOLANGUAGELIST,0);

			UpdateLanguagePriorityControls();
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

		case IDC_OPTIONS_ENABLEAUDIOLANGUAGEPRIORITY:
			UpdateLanguagePriorityControls();
			return TRUE;

		case IDC_OPTIONS_AUDIOLANGUAGEPRIORITY:
			if (HIWORD(wParam)==LBN_SELCHANGE)
				UpdateLanguagePriorityControls();
			return TRUE;

		case IDC_OPTIONS_AUDIOLANGUAGEPRIORITY_ADD:
			{
				int Sel=(int)DlgComboBox_GetCurSel(hDlg,IDC_OPTIONS_AUDIOLANGUAGELIST);

				if (Sel>=0) {
					DWORD Param=(DWORD)DlgComboBox_GetItemData(hDlg,IDC_OPTIONS_AUDIOLANGUAGELIST,Sel);
					TCHAR szText[MAX_LANGUAGE_TEXT_LENGTH];
					GetLanguageText(Param & 0x00FFFFFFUL,(Param & LANGUAGE_FLAG_SUB)!=0,
									szText,lengthof(szText));
					int Index=(int)DlgListBox_FindStringExact(hDlg,IDC_OPTIONS_AUDIOLANGUAGEPRIORITY,0,szText);
					if (Index<0) {
						Index=(int)DlgListBox_AddString(hDlg,IDC_OPTIONS_AUDIOLANGUAGEPRIORITY,szText);
						DlgListBox_SetItemData(hDlg,IDC_OPTIONS_AUDIOLANGUAGEPRIORITY,Index,Param);
					}
					DlgListBox_SetCurSel(hDlg,IDC_OPTIONS_AUDIOLANGUAGEPRIORITY,Index);
					UpdateLanguagePriorityControls();
				}
			}
			return TRUE;

		case IDC_OPTIONS_AUDIOLANGUAGEPRIORITY_REMOVE:
			{
				int Sel=(int)DlgListBox_GetCurSel(hDlg,IDC_OPTIONS_AUDIOLANGUAGEPRIORITY);

				if (Sel>=0) {
					DlgListBox_DeleteString(hDlg,IDC_OPTIONS_AUDIOLANGUAGEPRIORITY,Sel);
					UpdateLanguagePriorityControls();
				}
			}
			return TRUE;

		case IDC_OPTIONS_AUDIOLANGUAGEPRIORITY_UP:
		case IDC_OPTIONS_AUDIOLANGUAGEPRIORITY_DOWN:
			{
				int From=(int)DlgListBox_GetCurSel(hDlg,IDC_OPTIONS_AUDIOLANGUAGEPRIORITY);
				int To;

				if (LOWORD(wParam)==IDC_OPTIONS_AUDIOLANGUAGEPRIORITY_UP) {
					if (From<1)
						return TRUE;
					To=From-1;
				} else {
					if (From<0 || From+1>=(int)DlgListBox_GetCount(hDlg,IDC_OPTIONS_AUDIOLANGUAGEPRIORITY))
						return TRUE;
					To=From+1;
				}

				TCHAR szText[MAX_LANGUAGE_TEXT_LENGTH];
				DlgListBox_GetString(hDlg,IDC_OPTIONS_AUDIOLANGUAGEPRIORITY,From,szText);
				DWORD Param=(DWORD)DlgListBox_GetItemData(hDlg,IDC_OPTIONS_AUDIOLANGUAGEPRIORITY,From);
				DlgListBox_DeleteString(hDlg,IDC_OPTIONS_AUDIOLANGUAGEPRIORITY,From);
				DlgListBox_InsertString(hDlg,IDC_OPTIONS_AUDIOLANGUAGEPRIORITY,To,szText);
				DlgListBox_SetItemData(hDlg,IDC_OPTIONS_AUDIOLANGUAGEPRIORITY,To,Param);
				DlgListBox_SetCurSel(hDlg,IDC_OPTIONS_AUDIOLANGUAGEPRIORITY,To);
				UpdateLanguagePriorityControls();
			}
			return TRUE;
		}
		return TRUE;

	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code) {
		case PSN_APPLY:
			{
				TVTest::String AudioDevice;
				int Sel=(int)DlgComboBox_GetCurSel(hDlg,IDC_OPTIONS_AUDIODEVICE);
				if (Sel>0)
					GetDlgComboBoxItemString(hDlg,IDC_OPTIONS_AUDIODEVICE,Sel,&AudioDevice);
				if (TVTest::StringUtility::CompareNoCase(m_AudioDeviceName,AudioDevice)!=0) {
					m_AudioDeviceName=std::move(AudioDevice);
					SetGeneralUpdateFlag(UPDATE_GENERAL_BUILDMEDIAVIEWER);
				}

				TVTest::String AudioFilter;
				Sel=(int)DlgComboBox_GetCurSel(hDlg,IDC_OPTIONS_AUDIOFILTER);
				if (Sel>0)
					GetDlgComboBoxItemString(hDlg,IDC_OPTIONS_AUDIOFILTER,Sel,&AudioFilter);
				if (TVTest::StringUtility::CompareNoCase(m_AudioFilterName,AudioFilter)!=0) {
					m_AudioFilterName=std::move(AudioFilter);
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

				m_fEnableLanguagePriority=
					DlgCheckBox_IsChecked(hDlg,IDC_OPTIONS_ENABLEAUDIOLANGUAGEPRIORITY);

				const int LangCount=(int)DlgListBox_GetCount(hDlg,IDC_OPTIONS_AUDIOLANGUAGEPRIORITY);
				m_LanguagePriority.resize(LangCount);
				for (int i=0;i<LangCount;i++) {
					DWORD Param=(DWORD)DlgListBox_GetItemData(hDlg,IDC_OPTIONS_AUDIOLANGUAGEPRIORITY,i);
					m_LanguagePriority[i].Language=Param & 0x00FFFFFFUL;
					m_LanguagePriority[i].fSub=(Param & LANGUAGE_FLAG_SUB)!=0;
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


void CAudioOptions::GetLanguageText(DWORD Language,bool fSub,LPTSTR pszText,int MaxText) const
{
	EpgUtil::GetLanguageText(Language,pszText,MaxText,EpgUtil::LANGUAGE_TEXT_LONG);
	if (fSub)
		::StrCatBuff(pszText,TEXT("(������)"),MaxText);
}


void CAudioOptions::UpdateLanguagePriorityControls()
{
	bool fEnable=DlgCheckBox_IsChecked(m_hDlg,IDC_OPTIONS_ENABLEAUDIOLANGUAGEPRIORITY);
	int Sel;

	if (fEnable)
		Sel=(int)DlgListBox_GetCurSel(m_hDlg,IDC_OPTIONS_AUDIOLANGUAGEPRIORITY);
	EnableDlgItem(m_hDlg,IDC_OPTIONS_AUDIOLANGUAGEPRIORITY,fEnable);
	EnableDlgItem(m_hDlg,IDC_OPTIONS_AUDIOLANGUAGEPRIORITY_ADD,fEnable);
	EnableDlgItem(m_hDlg,IDC_OPTIONS_AUDIOLANGUAGEPRIORITY_REMOVE,fEnable && Sel>=0);
	EnableDlgItem(m_hDlg,IDC_OPTIONS_AUDIOLANGUAGEPRIORITY_UP,fEnable && Sel>0);
	EnableDlgItem(m_hDlg,IDC_OPTIONS_AUDIOLANGUAGEPRIORITY_DOWN,
				  fEnable && Sel>=0 && Sel+1<(int)DlgListBox_GetCount(m_hDlg,IDC_OPTIONS_AUDIOLANGUAGEPRIORITY));
	EnableDlgItem(m_hDlg,IDC_OPTIONS_AUDIOLANGUAGELIST,fEnable);
}




CAudioOptions::CSurroundOptionsDialog::CSurroundOptionsDialog(CAudioOptions *pOptions)
	: m_pOptions(pOptions)
{
}


bool CAudioOptions::CSurroundOptionsDialog::Show(HWND hwndOwner)
{
	return ShowDialog(hwndOwner,
					  GetAppClass().GetResourceInstance(),
					  MAKEINTRESOURCE(IDD_SURROUNDOPTIONS))==IDOK;
}


INT_PTR CAudioOptions::CSurroundOptionsDialog::DlgProc(
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


void CAudioOptions::CSurroundOptionsDialog::SetDownMixMatrix(
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
