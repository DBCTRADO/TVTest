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
#include "VideoOptions.h"
#include "DialogUtil.h"
#include "MessageDialog.h"
#include "resource.h"
#include <algorithm>
#include "Common/DebugDef.h"


namespace TVTest
{


const CVideoOptions::RendererInfo CVideoOptions::m_RendererList[] = {
	{LibISDB::DirectShow::VideoRenderer::RendererType::Default,            TEXT("システムデフォルト")},
	//{LibISDB::DirectShow::VideoRenderer::RendererType::VMR7,               TEXT("VMR7")},
	{LibISDB::DirectShow::VideoRenderer::RendererType::VMR9,               TEXT("VMR9")},
	//{LibISDB::DirectShow::VideoRenderer::RendererType::VMR7Renderless,     TEXT("VMR7 Renderless")},
	{LibISDB::DirectShow::VideoRenderer::RendererType::VMR9Renderless,     TEXT("VMR9 Renderless")},
	{LibISDB::DirectShow::VideoRenderer::RendererType::EVR,                TEXT("EVR")},
	{LibISDB::DirectShow::VideoRenderer::RendererType::EVRCustomPresenter, TEXT("EVR (Custom Presenter)")},
	//{LibISDB::DirectShow::VideoRenderer::RendererType::OverlayMixer,       TEXT("Overlay Mixer")},
	{LibISDB::DirectShow::VideoRenderer::RendererType::madVR,              TEXT("madVR")},
	{LibISDB::DirectShow::VideoRenderer::RendererType::MPCVideoRenderer,   TEXT("MPC Video Renderer")},
};


bool CVideoOptions::GetRendererInfo(int Index, RendererInfo *pInfo)
{
	if (Index < 0 || Index >= lengthof(m_RendererList))
		return false;
	*pInfo = m_RendererList[Index];
	return true;
}


CVideoOptions::~CVideoOptions()
{
	Destroy();
}


bool CVideoOptions::Apply(DWORD Flags)
{
	LibISDB::ViewerFilter *pViewer = GetAppClass().CoreEngine.GetFilter<LibISDB::ViewerFilter>();

	if (pViewer == nullptr)
		return false;

	if ((Flags & UPDATE_MASKCUTAREA) != 0) {
		pViewer->SetNoMaskSideCut(m_fNoMaskSideCut);
	}

	if ((Flags & UPDATE_IGNOREDISPLAYEXTENSION) != 0) {
		pViewer->SetIgnoreDisplayExtension(m_fIgnoreDisplayExtension);
	}

	if ((Flags & UPDATE_CLIPTODEVICE) != 0) {
		pViewer->SetClipToDevice(m_fClipToDevice);
	}

	return true;
}


bool CVideoOptions::ReadSettings(CSettings &Settings)
{
	int Value;

	Settings.Read(TEXT("Mpeg2Decoder"), &m_Mpeg2DecoderName);
	Settings.Read(TEXT("H264Decoder"), &m_H264DecoderName);
	Settings.Read(TEXT("H265Decoder"), &m_H265DecoderName);

	TCHAR szRenderer[32];
	if (Settings.Read(TEXT("Renderer"), szRenderer, lengthof(szRenderer))) {
		if (szRenderer[0] == '\0') {
			m_VideoRendererType = LibISDB::DirectShow::VideoRenderer::RendererType::Default;
		} else {
			const LibISDB::DirectShow::VideoRenderer::RendererType Renderer =
				LibISDB::DirectShow::VideoRenderer::ParseName(szRenderer);
			if (Renderer != LibISDB::DirectShow::VideoRenderer::RendererType::Invalid)
				m_VideoRendererType = Renderer;
		}
	}

	Settings.Read(TEXT("ResetPanScanEventChange"), &m_fResetPanScanEventChange);
	Settings.Read(TEXT("NoMaskSideCut"), &m_fNoMaskSideCut);

	if (bool fFrameCut; Settings.Read(TEXT("FrameCut"), &fFrameCut)) {
		m_StretchMode =
			fFrameCut ?
				LibISDB::ViewerFilter::ViewStretchMode::Crop :
				LibISDB::ViewerFilter::ViewStretchMode::KeepAspectRatio;
	}

	if (Settings.Read(TEXT("FullscreenStretchMode"), &Value)) {
		m_FullscreenStretchMode =
			Value == 1 ?
				LibISDB::ViewerFilter::ViewStretchMode::Crop :
				LibISDB::ViewerFilter::ViewStretchMode::KeepAspectRatio;
	}

	if (Settings.Read(TEXT("MaximizeStretchMode"), &Value)) {
		m_MaximizeStretchMode =
			Value == 1 ?
				LibISDB::ViewerFilter::ViewStretchMode::Crop :
				LibISDB::ViewerFilter::ViewStretchMode::KeepAspectRatio;
	}

	Settings.Read(TEXT("IgnoreDisplayExtension"), &m_fIgnoreDisplayExtension);
	Settings.Read(TEXT("ClipToDevice"), &m_fClipToDevice);

	return true;
}


bool CVideoOptions::WriteSettings(CSettings &Settings)
{
	Settings.Write(TEXT("Mpeg2Decoder"), m_Mpeg2DecoderName);
	Settings.Write(TEXT("H264Decoder"), m_H264DecoderName);
	Settings.Write(TEXT("H265Decoder"), m_H265DecoderName);
	Settings.Write(TEXT("Renderer"), LibISDB::DirectShow::VideoRenderer::EnumRendererName(m_VideoRendererType));
	Settings.Write(TEXT("ResetPanScanEventChange"), m_fResetPanScanEventChange);
	Settings.Write(TEXT("NoMaskSideCut"), m_fNoMaskSideCut);
	Settings.Write(TEXT("FrameCut"), m_StretchMode == LibISDB::ViewerFilter::ViewStretchMode::Crop);
	Settings.Write(TEXT("FullscreenStretchMode"), static_cast<int>(m_FullscreenStretchMode));
	Settings.Write(TEXT("MaximizeStretchMode"), static_cast<int>(m_MaximizeStretchMode));
	Settings.Write(TEXT("IgnoreDisplayExtension"), m_fIgnoreDisplayExtension);
	Settings.Write(TEXT("ClipToDevice"), m_fClipToDevice);

	return true;
}


bool CVideoOptions::Create(HWND hwndOwner)
{
	return CreateDialogWindow(
		hwndOwner,
		GetAppClass().GetResourceInstance(),
		MAKEINTRESOURCE(IDD_OPTIONS_VIDEO));
}


LPCTSTR CVideoOptions::GetMpeg2DecoderName() const
{
	return m_Mpeg2DecoderName.c_str();
}


void CVideoOptions::SetMpeg2DecoderName(LPCTSTR pszDecoderName)
{
	StringUtility::Assign(m_Mpeg2DecoderName, pszDecoderName);
}


LPCTSTR CVideoOptions::GetH264DecoderName() const
{
	return m_H264DecoderName.c_str();
}


void CVideoOptions::SetH264DecoderName(LPCTSTR pszDecoderName)
{
	StringUtility::Assign(m_H264DecoderName, pszDecoderName);
}


LPCTSTR CVideoOptions::GetH265DecoderName() const
{
	return m_H265DecoderName.c_str();
}


void CVideoOptions::SetH265DecoderName(LPCTSTR pszDecoderName)
{
	StringUtility::Assign(m_H265DecoderName, pszDecoderName);
}


LibISDB::DirectShow::VideoRenderer::RendererType CVideoOptions::GetVideoRendererType() const
{
	return m_VideoRendererType;
}


bool CVideoOptions::SetVideoRendererType(LibISDB::DirectShow::VideoRenderer::RendererType Renderer)
{
	if (LibISDB::DirectShow::VideoRenderer::EnumRendererName(Renderer) == nullptr)
		return false;
	m_VideoRendererType = Renderer;
	return true;
}


bool CVideoOptions::SetStretchMode(LibISDB::ViewerFilter::ViewStretchMode Mode)
{
	m_StretchMode = Mode;
	return true;
}


INT_PTR CVideoOptions::DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			const CAppMain &App = GetAppClass();

			// MPEG-2 デコーダ
			SetVideoDecoderList(
				IDC_OPTIONS_MPEG2DECODER,
				MEDIASUBTYPE_MPEG2_VIDEO,
				LibISDB::STREAM_TYPE_MPEG2_VIDEO,
				m_Mpeg2DecoderName);
			// H.264 デコーダ
			SetVideoDecoderList(
				IDC_OPTIONS_H264DECODER,
				MEDIASUBTYPE_H264,
				LibISDB::STREAM_TYPE_H264,
				m_H264DecoderName);
			// H.265 デコーダ
			SetVideoDecoderList(
				IDC_OPTIONS_H265DECODER,
				MEDIASUBTYPE_HEVC,
				LibISDB::STREAM_TYPE_H265,
				m_H265DecoderName);

			// 映像レンダラ
			int Sel = -1;
			RendererInfo Info;
			for (int i = 0; GetRendererInfo(i, &Info); i++) {
				DlgComboBox_AddString(hDlg, IDC_OPTIONS_RENDERER, Info.pszName);
				DlgComboBox_SetItemData(hDlg, IDC_OPTIONS_RENDERER, i, static_cast<LPARAM>(Info.Renderer));
				if (Info.Renderer == m_VideoRendererType)
					Sel = i;
			}
			DlgComboBox_SetCurSel(hDlg, IDC_OPTIONS_RENDERER, Sel);

			DlgCheckBox_Check(
				hDlg, IDC_OPTIONS_RESETPANSCANEVENTCHANGE,
				m_fResetPanScanEventChange);
			DlgCheckBox_Check(
				hDlg, IDC_OPTIONS_NOMASKSIDECUT,
				m_fNoMaskSideCut);
			DlgCheckBox_Check(
				hDlg, IDC_OPTIONS_FULLSCREENCUTFRAME,
				m_FullscreenStretchMode == LibISDB::ViewerFilter::ViewStretchMode::Crop);
			DlgCheckBox_Check(
				hDlg, IDC_OPTIONS_MAXIMIZECUTFRAME,
				m_MaximizeStretchMode == LibISDB::ViewerFilter::ViewStretchMode::Crop);
			DlgCheckBox_Check(
				hDlg, IDC_OPTIONS_IGNOREDISPLAYSIZE,
				m_fIgnoreDisplayExtension);
			DlgCheckBox_Check(
				hDlg, IDC_OPTIONS_CLIPTODEVICE,
				m_fClipToDevice);
		}
		return TRUE;

	case WM_NOTIFY:
		switch (reinterpret_cast<LPNMHDR>(lParam)->code) {
		case PSN_APPLY:
			{
				const CAppMain &App = GetAppClass();
				bool f;

				const LibISDB::DirectShow::VideoRenderer::RendererType Renderer =
					static_cast<LibISDB::DirectShow::VideoRenderer::RendererType>(
					DlgComboBox_GetItemData(hDlg, IDC_OPTIONS_RENDERER,
											DlgComboBox_GetCurSel(hDlg, IDC_OPTIONS_RENDERER)));
				if (Renderer != m_VideoRendererType) {
					if (!LibISDB::DirectShow::VideoRenderer::IsAvailable(Renderer)) {
						SettingError();
						::MessageBox(
							hDlg, TEXT("選択されたレンダラはこの環境で利用可能になっていません。"),
							nullptr, MB_OK | MB_ICONEXCLAMATION);
						return TRUE;
					}
					m_VideoRendererType = Renderer;
					SetUpdateFlag(UPDATE_RENDERER);
					SetGeneralUpdateFlag(UPDATE_GENERAL_BUILDMEDIAVIEWER);
				}

				GetVideoDecoderSetting(
					IDC_OPTIONS_MPEG2DECODER,
					LibISDB::STREAM_TYPE_MPEG2_VIDEO,
					&m_Mpeg2DecoderName);
				GetVideoDecoderSetting(
					IDC_OPTIONS_H264DECODER,
					LibISDB::STREAM_TYPE_H264,
					&m_H264DecoderName);
				GetVideoDecoderSetting(
					IDC_OPTIONS_H265DECODER,
					LibISDB::STREAM_TYPE_H265,
					&m_H265DecoderName);

				LibISDB::ViewerFilter *pViewer = App.CoreEngine.GetFilter<LibISDB::ViewerFilter>();

				m_fResetPanScanEventChange =
					DlgCheckBox_IsChecked(hDlg, IDC_OPTIONS_RESETPANSCANEVENTCHANGE);
				f = DlgCheckBox_IsChecked(hDlg, IDC_OPTIONS_NOMASKSIDECUT);
				if (m_fNoMaskSideCut != f) {
					m_fNoMaskSideCut = f;
					SetUpdateFlag(UPDATE_MASKCUTAREA);
				}
				m_FullscreenStretchMode =
					DlgCheckBox_IsChecked(hDlg, IDC_OPTIONS_FULLSCREENCUTFRAME) ?
					LibISDB::ViewerFilter::ViewStretchMode::Crop :
					LibISDB::ViewerFilter::ViewStretchMode::KeepAspectRatio;
				if (pViewer != nullptr && App.UICore.GetFullscreen())
					pViewer->SetViewStretchMode(m_FullscreenStretchMode);
				m_MaximizeStretchMode =
					DlgCheckBox_IsChecked(hDlg, IDC_OPTIONS_MAXIMIZECUTFRAME) ?
					LibISDB::ViewerFilter::ViewStretchMode::Crop :
					LibISDB::ViewerFilter::ViewStretchMode::KeepAspectRatio;
				if (pViewer != nullptr && ::IsZoomed(App.UICore.GetMainWindow()))
					pViewer->SetViewStretchMode(m_MaximizeStretchMode);
				f = DlgCheckBox_IsChecked(hDlg, IDC_OPTIONS_IGNOREDISPLAYSIZE);
				if (m_fIgnoreDisplayExtension != f) {
					m_fIgnoreDisplayExtension = f;
					SetUpdateFlag(UPDATE_IGNOREDISPLAYEXTENSION);
				}
				f = DlgCheckBox_IsChecked(hDlg, IDC_OPTIONS_CLIPTODEVICE);
				if (m_fClipToDevice != f) {
					m_fClipToDevice = f;
					SetUpdateFlag(UPDATE_CLIPTODEVICE);
				}

				m_fChanged = true;
			}
			return TRUE;
		}
		break;
	}

	return FALSE;
}


void CVideoOptions::SetVideoDecoderList(
	int ID, const GUID &SubType, BYTE StreamType, const String &DecoderName)
{
	LPCWSTR pszDefaultDecoderName =
		LibISDB::DirectShow::KnownDecoderManager::IsDecoderAvailable(SubType) ?
		LibISDB::DirectShow::KnownDecoderManager::GetDecoderName(SubType) : nullptr;

	LibISDB::DirectShow::FilterFinder FilterFinder;
	std::vector<String> FilterList;
	if (FilterFinder.FindFilters(&MEDIATYPE_Video, &SubType)) {
		FilterList.reserve(FilterFinder.GetFilterCount());
		for (int i = 0; i < FilterFinder.GetFilterCount(); i++) {
			String FilterName;

			if (FilterFinder.GetFilterInfo(i, nullptr, &FilterName)
					&& (pszDefaultDecoderName == nullptr
						|| ::lstrcmpi(FilterName.c_str(), pszDefaultDecoderName) != 0)) {
				FilterList.push_back(FilterName);
			}
		}
	}
	int Sel = 0;
	if (FilterList.empty()) {
		DlgComboBox_AddString(
			m_hDlg, ID,
			pszDefaultDecoderName != nullptr ?
			pszDefaultDecoderName :
			TEXT("<デコーダが見付かりません>"));
	} else {
		if (pszDefaultDecoderName != nullptr)
			DlgComboBox_AddString(m_hDlg, ID, pszDefaultDecoderName);
		if (FilterList.size() > 1) {
			std::ranges::sort(
				FilterList,
				[](const String &Filter1, const String &Filter2) {
					return ::CompareString(
						LOCALE_USER_DEFAULT,
						NORM_IGNORECASE | NORM_IGNORESYMBOLS,
						Filter1.data(), static_cast<int>(Filter1.length()),
						Filter2.data(), static_cast<int>(Filter2.length())) == CSTR_LESS_THAN;
				});
		}
		for (const String &e : FilterList) {
			DlgComboBox_AddString(m_hDlg, ID, e.c_str());
		}

		String Text;

		Text = TEXT("自動");
		if (!DecoderName.empty()) {
			Sel = static_cast<int>(DlgComboBox_FindStringExact(m_hDlg, ID, -1, DecoderName.c_str())) + 1;
		} else {
			const LibISDB::ViewerFilter *pViewer =
				GetAppClass().CoreEngine.GetFilter<LibISDB::ViewerFilter>();
			LibISDB::String Name;
			if (pViewer != nullptr && pViewer->IsOpen()
					&& StreamType == pViewer->GetVideoStreamType()
					&& pViewer->GetVideoDecoderName(&Name)) {
				Text += TEXT(" (");
				Text += Name;
				Text += TEXT(")");
			}
		}
		DlgComboBox_InsertString(m_hDlg, ID, 0, Text.c_str());
	}
	DlgComboBox_SetCurSel(m_hDlg, ID, Sel);
}


void CVideoOptions::GetVideoDecoderSetting(
	int ID, BYTE StreamType, String *pDecoderName)
{
	TCHAR szDecoder[MAX_VIDEO_DECODER_NAME];
	const int Sel = static_cast<int>(DlgComboBox_GetCurSel(m_hDlg, ID));
	if (Sel > 0)
		DlgComboBox_GetLBString(m_hDlg, ID, Sel, szDecoder);
	else
		szDecoder[0] = '\0';
	if (::lstrcmpi(szDecoder, pDecoderName->c_str()) != 0) {
		*pDecoderName = szDecoder;
		SetUpdateFlag(UPDATE_DECODER);
		const LibISDB::ViewerFilter *pViewer = GetAppClass().CoreEngine.GetFilter<LibISDB::ViewerFilter>();
		if (pViewer != nullptr && StreamType == pViewer->GetVideoStreamType())
			SetGeneralUpdateFlag(UPDATE_GENERAL_BUILDMEDIAVIEWER);
	}
}


} // namespace TVTest
