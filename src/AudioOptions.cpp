/*
  TVTest
  Copyright(c) 2008-2020 DBCTRADO

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


#include "stdafx.h"
#include "TVTest.h"
#include "AppMain.h"
#include "AudioOptions.h"
#include "DialogUtil.h"
#include "EpgUtil.h"
#include "resource.h"
#include <algorithm>
#include "Common/DebugDef.h"


namespace TVTest
{


static constexpr size_t MAX_LANGUAGE_TEXT_LENGTH = LibISDB::MAX_LANGUAGE_TEXT_LENGTH + 16;


static constexpr size_t MAX_FORMAT_DOUBLE_LENGTH = 16;

static void FormatDouble(double Value, LPTSTR pszString, size_t MaxString)
{
	const size_t Length = StringFormat(pszString, MaxString, TEXT("{:.4f}"), Value);
	size_t i;
	for (i = Length - 1; i > 1; i--) {
		if (pszString[i] != TEXT('0')) {
			i++;
			break;
		}
		if (pszString[i] == TEXT('.'))
			break;
	}
	pszString[i] = TEXT('\0');
}




const LibISDB::DirectShow::AudioDecoderFilter::SurroundMixingMatrix CAudioOptions::m_DefaultSurroundMixingMatrix = {
	{
		{1.0, 0.0, 0.0, 0.0, 0.0, 0.0},
		{0.0, 1.0, 0.0, 0.0, 0.0, 0.0},
		{0.0, 0.0, 1.0, 0.0, 0.0, 0.0},
		{0.0, 0.0, 0.0, 1.0, 0.0, 0.0},
		{0.0, 0.0, 0.0, 0.0, 1.0, 0.0},
		{0.0, 0.0, 0.0, 0.0, 0.0, 1.0}
	}
};

static constexpr double PSQR = 1.0 / 1.4142135623730950488016887242097;
const LibISDB::DirectShow::AudioDecoderFilter::DownMixMatrix CAudioOptions::m_DefaultDownMixMatrix = {
	{
		{1.0, 0.0, PSQR, PSQR, PSQR, 0.0},
		{0.0, 1.0, PSQR, PSQR,  0.0, PSQR}
	}
};

const DWORD CAudioOptions::m_AudioLanguageList[] = {
	LibISDB::LANGUAGE_CODE_JPN,	// 日本語
	LibISDB::LANGUAGE_CODE_ENG,	// 英語
	LibISDB::LANGUAGE_CODE_DEU,	// ドイツ語
	LibISDB::LANGUAGE_CODE_FRA,	// フランス語
	LibISDB::LANGUAGE_CODE_ITA,	// イタリア語
	LibISDB::LANGUAGE_CODE_KOR,	// 韓国語
	LibISDB::LANGUAGE_CODE_RUS,	// ロシア語
	LibISDB::LANGUAGE_CODE_SPA,	// スペイン語
	LibISDB::LANGUAGE_CODE_ZHO,	// 中国語
	LibISDB::LANGUAGE_CODE_ETC,	// その他
};


CAudioOptions::~CAudioOptions()
{
	Destroy();
}


bool CAudioOptions::ReadSettings(CSettings &Settings)
{
	Settings.Read(TEXT("AudioDevice"), &m_AudioDevice.FriendlyName);
	Settings.Read(TEXT("AudioDeviceMoniker"), &m_AudioDevice.MonikerName);
	Settings.Read(TEXT("AudioFilter"), &m_AudioFilterName);
	int SpdifMode;
	if (Settings.Read(TEXT("SpdifMode"), &SpdifMode))
		m_SPDIFOptions.Mode = static_cast<LibISDB::DirectShow::AudioDecoderFilter::SPDIFMode>(SpdifMode);
	Settings.Read(TEXT("SpdifChannels"), &m_SPDIFOptions.PassthroughChannels);
	Settings.Read(TEXT("DownMixSurround"), &m_fDownMixSurround);

	Settings.Read(TEXT("UseCustomSurroundMixingMatrix"), &m_fUseCustomSurroundMixingMatrix);
	String Buffer;
	if (Settings.Read(TEXT("SurroundMixingMatrix"), &Buffer) && !Buffer.empty()) {
		std::vector<String> List;
		if (StringUtility::Split(Buffer, TEXT(","), &List) && List.size() >= 6 * 6) {
			for (int i = 0; i < 6; i++) {
				for (int j = 0; j < 6; j++) {
					double Value = std::_tcstod(List[i * 6 + j].c_str(), nullptr);
					if (Value == HUGE_VAL || Value == -HUGE_VAL)
						Value = 0.0;
					m_SurroundMixingMatrix.Matrix[i][j] = Value;
				}
			}
		}
	}

	Settings.Read(TEXT("UseCustomDownMixMatrix"), &m_fUseCustomDownMixMatrix);
	if (Settings.Read(TEXT("DownMixMatrix"), &Buffer) && !Buffer.empty()) {
		std::vector<String> List;
		if (StringUtility::Split(Buffer, TEXT(","), &List) && List.size() >= 2 * 6) {
			for (int i = 0; i < 2; i++) {
				for (int j = 0; j < 6; j++) {
					double Value = std::_tcstod(List[i * 6 + j].c_str(), nullptr);
					if (Value == HUGE_VAL || Value == -HUGE_VAL)
						Value = 0.0;
					m_DownMixMatrix.Matrix[i][j] = Value;
				}
			}
		}
	}

	Settings.Read(TEXT("EnableLangPriority"), &m_fEnableLanguagePriority);

	m_LanguagePriority.clear();
	for (int i = 0;; i++) {
		TCHAR szKey[32];
		String Value;
		StringFormat(szKey, TEXT("LangPriority{}"), i);
		if (!Settings.Read(szKey, &Value) || Value.length() < 3)
			break;
		AudioLanguageInfo Info;
		Info.Language = (Value[0] << 16) | (Value[1] << 8) | Value[2];
		Info.fSub = Value.length() >= 4 && Value[3] == TEXT('2');
		m_LanguagePriority.push_back(Info);
	}

	Settings.Read(TEXT("ResetAudioDelayOnChannelChange"), &m_fResetAudioDelayOnChannelChange);

	return true;
}


bool CAudioOptions::WriteSettings(CSettings &Settings)
{
	Settings.Write(TEXT("AudioDevice"), m_AudioDevice.FriendlyName);
	Settings.Write(TEXT("AudioDeviceMoniker"), m_AudioDevice.MonikerName);
	Settings.Write(TEXT("AudioFilter"), m_AudioFilterName);
	Settings.Write(TEXT("SpdifMode"), static_cast<int>(m_SPDIFOptions.Mode));
	Settings.Write(TEXT("SpdifChannels"), m_SPDIFOptions.PassthroughChannels);
	Settings.Write(TEXT("DownMixSurround"), m_fDownMixSurround);

	Settings.Write(TEXT("UseCustomSurroundMixingMatrix"), m_fUseCustomSurroundMixingMatrix);
	String Buffer;
	for (int i = 0; i < 6; i++) {
		for (int j = 0; j < 6; j++) {
			if (!Buffer.empty())
				Buffer += TEXT(',');
			TCHAR szValue[MAX_FORMAT_DOUBLE_LENGTH];
			FormatDouble(m_SurroundMixingMatrix.Matrix[i][j], szValue, lengthof(szValue));
			Buffer += szValue;
		}
	}
	Settings.Write(TEXT("SurroundMixingMatrix"), Buffer);

	Settings.Write(TEXT("UseCustomDownMixMatrix"), m_fUseCustomDownMixMatrix);
	Buffer.clear();
	for (int i = 0; i < 2; i++) {
		for (int j = 0; j < 6; j++) {
			if (!Buffer.empty())
				Buffer += TEXT(',');
			TCHAR szValue[MAX_FORMAT_DOUBLE_LENGTH];
			FormatDouble(m_DownMixMatrix.Matrix[i][j], szValue, lengthof(szValue));
			Buffer += szValue;
		}
	}
	Settings.Write(TEXT("DownMixMatrix"), Buffer);

	Settings.Write(TEXT("EnableLangPriority"), m_fEnableLanguagePriority);
	for (int i = 0;; i++) {
		TCHAR szKey[32];
		StringFormat(szKey, TEXT("LangPriority{}"), i);

		if (i < static_cast<int>(m_LanguagePriority.size())) {
			const DWORD Lang = m_LanguagePriority[i].Language;
			TCHAR szValue[8];
			StringFormat(
				szValue, TEXT("{:c}{:c}{:c}{}"),
				(Lang >> 16), (Lang >> 8) & 0xFF, Lang & 0xFF,
				m_LanguagePriority[i].fSub ? TEXT("2") : TEXT(""));
			Settings.Write(szKey, szValue);
		} else {
			if (!Settings.DeleteValue(szKey))
				break;
		}
	}

	Settings.Write(TEXT("ResetAudioDelayOnChannelChange"), m_fResetAudioDelayOnChannelChange);

	return true;
}


bool CAudioOptions::Create(HWND hwndOwner)
{
	return CreateDialogWindow(
		hwndOwner,
		GetAppClass().GetResourceInstance(),
		MAKEINTRESOURCE(IDD_OPTIONS_AUDIO));
}


bool CAudioOptions::ApplyMediaViewerOptions()
{
	LibISDB::ViewerFilter *pViewer = GetAppClass().CoreEngine.GetFilter<LibISDB::ViewerFilter>();

	if (pViewer == nullptr)
		return false;

	pViewer->SetDownMixSurround(m_fDownMixSurround);

	LibISDB::COMPointer<LibISDB::DirectShow::AudioDecoderFilter> AudioFilter(pViewer->GetAudioDecoderFilter());
	if (AudioFilter) {
		AudioFilter->SetSurroundMixingMatrix(
			m_fUseCustomSurroundMixingMatrix ? &m_SurroundMixingMatrix : nullptr);
		AudioFilter->SetDownMixMatrix(
			m_fUseCustomDownMixMatrix ? &m_DownMixMatrix : nullptr);
	}

	return true;
}


bool CAudioOptions::SetSpdifOptions(const LibISDB::DirectShow::AudioDecoderFilter::SPDIFOptions &Options)
{
	m_SPDIFOptions = Options;
	return true;
}


INT_PTR CAudioOptions::DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			int Sel = 0;
			DlgComboBox_AddString(hDlg, IDC_OPTIONS_AUDIODEVICE, TEXT("デフォルト"));
			LibISDB::DirectShow::DeviceEnumerator DevEnum;
			if (DevEnum.EnumDevice(CLSID_AudioRendererCategory)
					&& DevEnum.GetFilterList(&m_AudioDeviceList)) {
				std::ranges::stable_sort(
					m_AudioDeviceList,
					[](const FilterInfo &Filter1, const FilterInfo &Filter2) {
						return ::lstrcmp(Filter1.FriendlyName.c_str(), Filter2.FriendlyName.c_str()) < 0;
					});

				for (const FilterInfo &Filter : m_AudioDeviceList) {
					DlgComboBox_AddString(hDlg, IDC_OPTIONS_AUDIODEVICE, Filter.FriendlyName.c_str());
				}

				if (!m_AudioDevice.MonikerName.empty()) {
					for (int i = 0; i < DevEnum.GetDeviceCount(); i++) {
						if (StringUtility::CompareNoCase(m_AudioDevice.MonikerName, m_AudioDeviceList[i].MonikerName) == 0) {
							Sel = i + 1;
							break;
						}
					}
				}
				if (Sel == 0 && !m_AudioDevice.FriendlyName.empty()) {
					for (int i = 0; i < DevEnum.GetDeviceCount(); i++) {
						if (StringUtility::CompareNoCase(m_AudioDevice.FriendlyName, m_AudioDeviceList[i].FriendlyName) == 0) {
							Sel = i + 1;
							break;
						}
					}
				}
			}
			DlgComboBox_SetCurSel(hDlg, IDC_OPTIONS_AUDIODEVICE, Sel);

			Sel = 0;
			DlgComboBox_AddString(hDlg, IDC_OPTIONS_AUDIOFILTER, TEXT("なし"));
			LibISDB::DirectShow::FilterFinder FilterFinder;
			static const GUID InputTypes[] = {
				MEDIATYPE_Audio, MEDIASUBTYPE_PCM
			};
			static const GUID OutputTypes[] = {
				MEDIATYPE_Audio, MEDIASUBTYPE_PCM,
				MEDIATYPE_Audio, MEDIASUBTYPE_DOLBY_AC3,
				MEDIATYPE_Audio, MEDIASUBTYPE_DOLBY_AC3_SPDIF,
				MEDIATYPE_Audio, MEDIASUBTYPE_DTS,
				MEDIATYPE_Audio, MEDIASUBTYPE_AAC,
			};
			if (FilterFinder.FindFilters(
						InputTypes, lengthof(InputTypes),
						OutputTypes, lengthof(OutputTypes),
						0/*MERIT_DO_NOT_USE*/)) {
				String AudioFilter;
				CLSID idAudioFilter;

				for (int i = 0; i < FilterFinder.GetFilterCount(); i++) {
					if (FilterFinder.GetFilterInfo(i, &idAudioFilter, &AudioFilter)) {
						DlgComboBox_AddString(hDlg, IDC_OPTIONS_AUDIOFILTER, AudioFilter.c_str());
						if (Sel == 0 && !m_AudioFilterName.empty()
								&& StringUtility::CompareNoCase(m_AudioFilterName, AudioFilter) == 0)
							Sel = i + 1;
					}
				}
			}
			DlgComboBox_SetCurSel(hDlg, IDC_OPTIONS_AUDIOFILTER, Sel);

			static const LPCTSTR SpdifModeList[] = {
				TEXT("パススルー出力を行わない"),
				TEXT("常にパススルー出力を行う"),
				TEXT("音声の形式に応じてパススルー出力を行う"),
			};
			for (LPCTSTR pszMode : SpdifModeList)
				DlgComboBox_AddString(hDlg, IDC_OPTIONS_SPDIFMODE, pszMode);
			DlgComboBox_SetCurSel(hDlg, IDC_OPTIONS_SPDIFMODE, static_cast<int>(m_SPDIFOptions.Mode));
			DlgCheckBox_Check(
				hDlg, IDC_OPTIONS_SPDIF_CHANNELS_MONO,
				(m_SPDIFOptions.PassthroughChannels & LibISDB::DirectShow::AudioDecoderFilter::SPDIFOptions::Channel_Mono) != 0);
			DlgCheckBox_Check(
				hDlg, IDC_OPTIONS_SPDIF_CHANNELS_DUALMONO,
				(m_SPDIFOptions.PassthroughChannels & LibISDB::DirectShow::AudioDecoderFilter::SPDIFOptions::Channel_DualMono) != 0);
			DlgCheckBox_Check(
				hDlg, IDC_OPTIONS_SPDIF_CHANNELS_STEREO,
				(m_SPDIFOptions.PassthroughChannels & LibISDB::DirectShow::AudioDecoderFilter::SPDIFOptions::Channel_Stereo) != 0);
			DlgCheckBox_Check(
				hDlg, IDC_OPTIONS_SPDIF_CHANNELS_SURROUND,
				(m_SPDIFOptions.PassthroughChannels & LibISDB::DirectShow::AudioDecoderFilter::SPDIFOptions::Channel_Surround) != 0);
			EnableDlgItems(
				hDlg, IDC_OPTIONS_SPDIF_CHANNELS_LABEL, IDC_OPTIONS_SPDIF_CHANNELS_SURROUND,
				m_SPDIFOptions.Mode == LibISDB::DirectShow::AudioDecoderFilter::SPDIFMode::Auto);

			DlgCheckBox_Check(hDlg, IDC_OPTIONS_DOWNMIXSURROUND, m_fDownMixSurround);

			DlgCheckBox_Check(hDlg, IDC_OPTIONS_ENABLEAUDIOLANGUAGEPRIORITY, m_fEnableLanguagePriority);

			for (size_t i = 0; i < m_LanguagePriority.size(); i++) {
				const AudioLanguageInfo &Info = m_LanguagePriority[i];
				TCHAR szText[MAX_LANGUAGE_TEXT_LENGTH];
				GetLanguageText(Info.Language, Info.fSub, szText, lengthof(szText));
				if (DlgListBox_FindStringExact(hDlg, IDC_OPTIONS_AUDIOLANGUAGEPRIORITY, 0, szText) < 0) {
					DlgListBox_AddString(hDlg, IDC_OPTIONS_AUDIOLANGUAGEPRIORITY, szText);
					DWORD Param = Info.Language;
					if (Info.fSub)
						Param |= LANGUAGE_FLAG_SUB;
					DlgListBox_SetItemData(hDlg, IDC_OPTIONS_AUDIOLANGUAGEPRIORITY, i, Param);
				}
			}

			for (const DWORD Language : m_AudioLanguageList) {
				TCHAR szText[MAX_LANGUAGE_TEXT_LENGTH];
				GetLanguageText(Language, false, szText, lengthof(szText));
				int Index = static_cast<int>(DlgComboBox_AddString(hDlg, IDC_OPTIONS_AUDIOLANGUAGELIST, szText));
				DlgComboBox_SetItemData(hDlg, IDC_OPTIONS_AUDIOLANGUAGELIST, Index, Language);
				GetLanguageText(Language, true, szText, lengthof(szText));
				Index = static_cast<int>(DlgComboBox_AddString(hDlg, IDC_OPTIONS_AUDIOLANGUAGELIST, szText));
				DlgComboBox_SetItemData(hDlg, IDC_OPTIONS_AUDIOLANGUAGELIST, Index, Language | LANGUAGE_FLAG_SUB);
			}
			DlgComboBox_SetCurSel(hDlg, IDC_OPTIONS_AUDIOLANGUAGELIST, 0);

			UpdateLanguagePriorityControls();

			AddControls({
				{IDC_OPTIONS_AUDIODEVICE, AlignFlag::Horz},
				{IDC_OPTIONS_AUDIOFILTER, AlignFlag::Horz},
			});
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_OPTIONS_SPDIFMODE:
			if (HIWORD(wParam) == CBN_SELCHANGE) {
				EnableDlgItems(
					hDlg,
					IDC_OPTIONS_SPDIF_CHANNELS_LABEL,
					IDC_OPTIONS_SPDIF_CHANNELS_SURROUND,
					static_cast<LibISDB::DirectShow::AudioDecoderFilter::SPDIFMode>(DlgComboBox_GetCurSel(hDlg, IDC_OPTIONS_SPDIFMODE)) ==
						LibISDB::DirectShow::AudioDecoderFilter::SPDIFMode::Auto);
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
			if (HIWORD(wParam) == LBN_SELCHANGE)
				UpdateLanguagePriorityControls();
			return TRUE;

		case IDC_OPTIONS_AUDIOLANGUAGEPRIORITY_ADD:
			{
				const int Sel = static_cast<int>(DlgComboBox_GetCurSel(hDlg, IDC_OPTIONS_AUDIOLANGUAGELIST));

				if (Sel >= 0) {
					const DWORD Param = static_cast<DWORD>(DlgComboBox_GetItemData(hDlg, IDC_OPTIONS_AUDIOLANGUAGELIST, Sel));
					TCHAR szText[MAX_LANGUAGE_TEXT_LENGTH];
					GetLanguageText(
						Param & 0x00FFFFFFUL, (Param & LANGUAGE_FLAG_SUB) != 0,
						szText, lengthof(szText));
					int Index = static_cast<int>(DlgListBox_FindStringExact(hDlg, IDC_OPTIONS_AUDIOLANGUAGEPRIORITY, 0, szText));
					if (Index < 0) {
						Index = static_cast<int>(DlgListBox_AddString(hDlg, IDC_OPTIONS_AUDIOLANGUAGEPRIORITY, szText));
						DlgListBox_SetItemData(hDlg, IDC_OPTIONS_AUDIOLANGUAGEPRIORITY, Index, Param);
					}
					DlgListBox_SetCurSel(hDlg, IDC_OPTIONS_AUDIOLANGUAGEPRIORITY, Index);
					UpdateLanguagePriorityControls();
				}
			}
			return TRUE;

		case IDC_OPTIONS_AUDIOLANGUAGEPRIORITY_REMOVE:
			{
				const int Sel = static_cast<int>(DlgListBox_GetCurSel(hDlg, IDC_OPTIONS_AUDIOLANGUAGEPRIORITY));

				if (Sel >= 0) {
					DlgListBox_DeleteString(hDlg, IDC_OPTIONS_AUDIOLANGUAGEPRIORITY, Sel);
					UpdateLanguagePriorityControls();
				}
			}
			return TRUE;

		case IDC_OPTIONS_AUDIOLANGUAGEPRIORITY_UP:
		case IDC_OPTIONS_AUDIOLANGUAGEPRIORITY_DOWN:
			{
				const int From = static_cast<int>(DlgListBox_GetCurSel(hDlg, IDC_OPTIONS_AUDIOLANGUAGEPRIORITY));
				int To;

				if (LOWORD(wParam) == IDC_OPTIONS_AUDIOLANGUAGEPRIORITY_UP) {
					if (From < 1)
						return TRUE;
					To = From - 1;
				} else {
					if (From < 0 || From + 1 >= static_cast<int>(DlgListBox_GetCount(hDlg, IDC_OPTIONS_AUDIOLANGUAGEPRIORITY)))
						return TRUE;
					To = From + 1;
				}

				TCHAR szText[MAX_LANGUAGE_TEXT_LENGTH];
				DlgListBox_GetString(hDlg, IDC_OPTIONS_AUDIOLANGUAGEPRIORITY, From, szText);
				const DWORD Param = static_cast<DWORD>(DlgListBox_GetItemData(hDlg, IDC_OPTIONS_AUDIOLANGUAGEPRIORITY, From));
				DlgListBox_DeleteString(hDlg, IDC_OPTIONS_AUDIOLANGUAGEPRIORITY, From);
				DlgListBox_InsertString(hDlg, IDC_OPTIONS_AUDIOLANGUAGEPRIORITY, To, szText);
				DlgListBox_SetItemData(hDlg, IDC_OPTIONS_AUDIOLANGUAGEPRIORITY, To, Param);
				DlgListBox_SetCurSel(hDlg, IDC_OPTIONS_AUDIOLANGUAGEPRIORITY, To);
				UpdateLanguagePriorityControls();
			}
			return TRUE;
		}
		return TRUE;

	case WM_NOTIFY:
		switch (reinterpret_cast<LPNMHDR>(lParam)->code) {
		case PSN_APPLY:
			{
				FilterInfo AudioDevice;
				int Sel = static_cast<int>(DlgComboBox_GetCurSel(hDlg, IDC_OPTIONS_AUDIODEVICE));
				if (Sel > 0 && static_cast<unsigned int>(Sel) <= m_AudioDeviceList.size())
					AudioDevice = m_AudioDeviceList[Sel - 1];
				if (StringUtility::CompareNoCase(m_AudioDevice.FriendlyName, AudioDevice.FriendlyName) != 0
						|| (m_AudioDevice.MonikerName.empty() && Sel >= 2
							&& StringUtility::CompareNoCase(AudioDevice.MonikerName, m_AudioDeviceList[Sel - 2].MonikerName) == 0)
						|| (!m_AudioDevice.MonikerName.empty()
							&& StringUtility::CompareNoCase(m_AudioDevice.MonikerName, AudioDevice.MonikerName) != 0)) {
					m_AudioDevice = std::move(AudioDevice);
					SetGeneralUpdateFlag(UPDATE_GENERAL_BUILDMEDIAVIEWER);
				}

				String AudioFilter;
				Sel = static_cast<int>(DlgComboBox_GetCurSel(hDlg, IDC_OPTIONS_AUDIOFILTER));
				if (Sel > 0)
					GetDlgComboBoxItemString(hDlg, IDC_OPTIONS_AUDIOFILTER, Sel, &AudioFilter);
				if (StringUtility::CompareNoCase(m_AudioFilterName, AudioFilter) != 0) {
					m_AudioFilterName = std::move(AudioFilter);
					SetGeneralUpdateFlag(UPDATE_GENERAL_BUILDMEDIAVIEWER);
				}

				LibISDB::DirectShow::AudioDecoderFilter::SPDIFOptions SPDIFOptions;
				SPDIFOptions.Mode = static_cast<LibISDB::DirectShow::AudioDecoderFilter::SPDIFMode>(
					DlgComboBox_GetCurSel(hDlg, IDC_OPTIONS_SPDIFMODE));
				SPDIFOptions.PassthroughChannels = 0;
				if (DlgCheckBox_IsChecked(hDlg, IDC_OPTIONS_SPDIF_CHANNELS_MONO))
					SPDIFOptions.PassthroughChannels |= LibISDB::DirectShow::AudioDecoderFilter::SPDIFOptions::Channel_Mono;
				if (DlgCheckBox_IsChecked(hDlg, IDC_OPTIONS_SPDIF_CHANNELS_DUALMONO))
					SPDIFOptions.PassthroughChannels |= LibISDB::DirectShow::AudioDecoderFilter::SPDIFOptions::Channel_DualMono;
				if (DlgCheckBox_IsChecked(hDlg, IDC_OPTIONS_SPDIF_CHANNELS_STEREO))
					SPDIFOptions.PassthroughChannels |= LibISDB::DirectShow::AudioDecoderFilter::SPDIFOptions::Channel_Stereo;
				if (DlgCheckBox_IsChecked(hDlg, IDC_OPTIONS_SPDIF_CHANNELS_SURROUND))
					SPDIFOptions.PassthroughChannels |= LibISDB::DirectShow::AudioDecoderFilter::SPDIFOptions::Channel_Surround;
				if (SPDIFOptions != m_SPDIFOptions) {
					m_SPDIFOptions = SPDIFOptions;
					GetAppClass().CoreEngine.SetSPDIFOptions(m_SPDIFOptions);
				}

				m_fDownMixSurround =
					DlgCheckBox_IsChecked(hDlg, IDC_OPTIONS_DOWNMIXSURROUND);

				m_fEnableLanguagePriority =
					DlgCheckBox_IsChecked(hDlg, IDC_OPTIONS_ENABLEAUDIOLANGUAGEPRIORITY);

				const int LangCount = static_cast<int>(DlgListBox_GetCount(hDlg, IDC_OPTIONS_AUDIOLANGUAGEPRIORITY));
				m_LanguagePriority.resize(LangCount);
				for (int i = 0; i < LangCount; i++) {
					const DWORD Param = static_cast<DWORD>(DlgListBox_GetItemData(hDlg, IDC_OPTIONS_AUDIOLANGUAGEPRIORITY, i));
					m_LanguagePriority[i].Language = Param & 0x00FFFFFFUL;
					m_LanguagePriority[i].fSub = (Param & LANGUAGE_FLAG_SUB) != 0;
				}

				ApplyMediaViewerOptions();

				m_fChanged = true;
			}
			break;
		}
		break;

	case WM_DESTROY:
		m_AudioDeviceList.clear();
		return TRUE;
	}

	return FALSE;
}


void CAudioOptions::GetLanguageText(DWORD Language, bool fSub, LPTSTR pszText, int MaxText) const
{
	LibISDB::GetLanguageText_ja(Language, pszText, MaxText);
	if (fSub)
		::StrCatBuff(pszText, TEXT("(副音声)"), MaxText);
}


void CAudioOptions::UpdateLanguagePriorityControls()
{
	const bool fEnable = DlgCheckBox_IsChecked(m_hDlg, IDC_OPTIONS_ENABLEAUDIOLANGUAGEPRIORITY);
	int Sel;

	if (fEnable)
		Sel = static_cast<int>(DlgListBox_GetCurSel(m_hDlg, IDC_OPTIONS_AUDIOLANGUAGEPRIORITY));
	EnableDlgItem(m_hDlg, IDC_OPTIONS_AUDIOLANGUAGEPRIORITY, fEnable);
	EnableDlgItem(m_hDlg, IDC_OPTIONS_AUDIOLANGUAGEPRIORITY_ADD, fEnable);
	EnableDlgItem(m_hDlg, IDC_OPTIONS_AUDIOLANGUAGEPRIORITY_REMOVE, fEnable && Sel >= 0);
	EnableDlgItem(m_hDlg, IDC_OPTIONS_AUDIOLANGUAGEPRIORITY_UP, fEnable && Sel > 0);
	EnableDlgItem(
		m_hDlg, IDC_OPTIONS_AUDIOLANGUAGEPRIORITY_DOWN,
		fEnable && Sel >= 0 && Sel + 1 < static_cast<int>(DlgListBox_GetCount(m_hDlg, IDC_OPTIONS_AUDIOLANGUAGEPRIORITY)));
	EnableDlgItem(m_hDlg, IDC_OPTIONS_AUDIOLANGUAGELIST, fEnable);
}




CAudioOptions::CSurroundOptionsDialog::CSurroundOptionsDialog(CAudioOptions *pOptions)
	: m_pOptions(pOptions)
{
}


bool CAudioOptions::CSurroundOptionsDialog::Show(HWND hwndOwner)
{
	return ShowDialog(
		hwndOwner,
		GetAppClass().GetResourceInstance(),
		MAKEINTRESOURCE(IDD_SURROUNDOPTIONS)) == IDOK;
}


INT_PTR CAudioOptions::CSurroundOptionsDialog::DlgProc(
	HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			DlgCheckBox_Check(
				hDlg, IDC_SURROUND_USECUSTOMDOWNMIXMATRIX,
				m_pOptions->m_fUseCustomDownMixMatrix);
			EnableDlgItems(
				hDlg,
				IDC_SURROUND_DOWNMIXMATRIX_FL_LABEL,
				IDC_SURROUND_DOWNMIXMATRIX_DEFAULT,
				m_pOptions->m_fUseCustomDownMixMatrix);
			SetDownMixMatrix(m_pOptions->m_DownMixMatrix);

			DlgCheckBox_Check(
				hDlg, IDC_SURROUND_USECUSTOMMIXINGMATRIX,
				m_pOptions->m_fUseCustomSurroundMixingMatrix);
			EnableDlgItems(
				hDlg,
				IDC_SURROUND_MIXINGMATRIX_IN_FL_LABEL,
				IDC_SURROUND_MIXINGMATRIX_SR_SR,
				m_pOptions->m_fUseCustomSurroundMixingMatrix);
			int ID = IDC_SURROUND_MIXINGMATRIX_FL_FL;
			for (int i = 0; i < 6; i++) {
				for (int j = 0; j < 6; j++) {
					TCHAR szText[MAX_FORMAT_DOUBLE_LENGTH];
					FormatDouble(
						m_pOptions->m_SurroundMixingMatrix.Matrix[i][j],
						szText, lengthof(szText));
					::SetDlgItemText(hDlg, ID, szText);
					ID++;
				}
			}
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_SURROUND_USECUSTOMDOWNMIXMATRIX:
			EnableDlgItemsSyncCheckBox(
				hDlg,
				IDC_SURROUND_DOWNMIXMATRIX_FL_LABEL,
				IDC_SURROUND_DOWNMIXMATRIX_DEFAULT,
				IDC_SURROUND_USECUSTOMDOWNMIXMATRIX);
			return TRUE;

		case IDC_SURROUND_DOWNMIXMATRIX_DEFAULT:
			SetDownMixMatrix(m_pOptions->m_DefaultDownMixMatrix);
			return TRUE;

		case IDC_SURROUND_USECUSTOMMIXINGMATRIX:
			EnableDlgItemsSyncCheckBox(
				hDlg,
				IDC_SURROUND_MIXINGMATRIX_IN_FL_LABEL,
				IDC_SURROUND_MIXINGMATRIX_SR_SR,
				IDC_SURROUND_USECUSTOMMIXINGMATRIX);
			return TRUE;

		case IDOK:
			{
				m_pOptions->m_fUseCustomDownMixMatrix =
					DlgCheckBox_IsChecked(hDlg, IDC_SURROUND_USECUSTOMDOWNMIXMATRIX);
				int ID = IDC_SURROUND_DOWNMIXMATRIX_L_FL;
				for (int i = 0; i < 2; i++) {
					for (int j = 0; j < 6; j++) {
						TCHAR szText[16];
						::GetDlgItemText(hDlg, ID, szText, lengthof(szText));
						double Value = std::_tcstod(szText, nullptr);
						if (Value == HUGE_VAL || Value == -HUGE_VAL)
							Value = 0.0;
						m_pOptions->m_DownMixMatrix.Matrix[i][j] = Value;
						ID++;
					}
				}

				m_pOptions->m_fUseCustomSurroundMixingMatrix =
					DlgCheckBox_IsChecked(hDlg, IDC_SURROUND_USECUSTOMMIXINGMATRIX);
				ID = IDC_SURROUND_MIXINGMATRIX_FL_FL;
				for (int i = 0; i < 6; i++) {
					for (int j = 0; j < 6; j++) {
						TCHAR szText[16];
						::GetDlgItemText(hDlg, ID, szText, lengthof(szText));
						double Value = std::_tcstod(szText, nullptr);
						if (Value == HUGE_VAL || Value == -HUGE_VAL)
							Value = 0.0;
						m_pOptions->m_SurroundMixingMatrix.Matrix[i][j] = Value;
						ID++;
					}
				}

				m_pOptions->m_fChanged = true;
				m_pOptions->ApplyMediaViewerOptions();
			}
			[[fallthrough]];
		case IDCANCEL:
			::EndDialog(hDlg, LOWORD(wParam));
			return TRUE;
		}
		return TRUE;
	}

	return FALSE;
}


void CAudioOptions::CSurroundOptionsDialog::SetDownMixMatrix(
	const LibISDB::DirectShow::AudioDecoderFilter::DownMixMatrix &Matrix)
{
	int ID = IDC_SURROUND_DOWNMIXMATRIX_L_FL;
	for (int i = 0; i < 2; i++) {
		for (int j = 0; j < 6; j++) {
			TCHAR szText[MAX_FORMAT_DOUBLE_LENGTH];
			FormatDouble(Matrix.Matrix[i][j], szText, lengthof(szText));
			::SetDlgItemText(m_hDlg, ID, szText);
			ID++;
		}
	}
}


}	// namespace TVTest
