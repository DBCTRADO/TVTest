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
#include "InitialSettings.h"
#include "LibISDB/LibISDB/Windows/Viewer/DirectShow/DirectShowUtilities.hpp"
#include "VideoOptions.h"
#include "DialogUtil.h"
#include "DrawUtil.h"
#include "Aero.h"
#include "Help.h"
#include "resource.h"
#include <algorithm>
#include "Common/DebugDef.h"


namespace TVTest
{


CInitialSettings::CInitialSettings(const CDriverManager *pDriverManager)
	: m_pDriverManager(pDriverManager)
{
	// ビデオレンダラのデフォルトをEVRにする
	m_VideoRenderer = LibISDB::DirectShow::VideoRenderer::RendererType::EVR;

	PWSTR pszRecFolder;
	if (::SHGetKnownFolderPath(FOLDERID_Videos, 0, nullptr, &pszRecFolder) == S_OK
			|| ::SHGetKnownFolderPath(FOLDERID_Documents, 0, nullptr, &pszRecFolder) == S_OK) {
		m_RecordFolder = pszRecFolder;
		::CoTaskMemFree(pszRecFolder);
	}
}


CInitialSettings::~CInitialSettings()
{
	Destroy();
}


bool CInitialSettings::Show(HWND hwndOwner)
{
	return ShowDialog(
		hwndOwner,
		GetAppClass().GetResourceInstance(),
		MAKEINTRESOURCE(IDD_INITIALSETTINGS)) == IDOK;
}


INT_PTR CInitialSettings::DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			{
				const HWND hwndLogo = ::GetDlgItem(hDlg, IDC_INITIALSETTINGS_LOGO);
				RECT rc;

				::GetWindowRect(hwndLogo, &rc);
				::SetRect(&rc, rc.right - rc.left, 0, 0, 0);
				if (!Util::OS::IsWindows8OrLater()
						&& m_AeroGlass.ApplyAeroGlass(hDlg, &rc)) {
					m_fDrawLogo = true;
					m_LogoImage.LoadFromResource(
						GetAppClass().GetResourceInstance(),
						MAKEINTRESOURCE(IDB_LOGO32), TEXT("PNG"));
					::ShowWindow(hwndLogo, SW_HIDE);
				} else {
					const HBITMAP hbm = static_cast<HBITMAP>(
						::LoadImage(
							GetAppClass().GetResourceInstance(),
							MAKEINTRESOURCE(IDB_LOGO),
							IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR));
					::SendMessage(
						hwndLogo, STM_SETIMAGE,
						IMAGE_BITMAP, reinterpret_cast<LPARAM>(hbm));
				}
			}

			// BonDriver
			{
				int NormalDriverCount = 0;

				DlgComboBox_LimitText(hDlg, IDC_INITIALSETTINGS_DRIVER, MAX_PATH - 1);
				for (int i = 0; i < m_pDriverManager->NumDrivers(); i++) {
					const CDriverInfo *pDriverInfo = m_pDriverManager->GetDriverInfo(i);
					CDriverManager::TunerSpec Spec;
					int Index;

					// ネットワークやファイル再生用の特殊なBonDriverは後になるようにする
					if (m_pDriverManager->GetTunerSpec(pDriverInfo->GetFileName(), &Spec)
						&& !!(Spec.Flags &
							(CDriverManager::TunerSpec::Flag::Network |
							 CDriverManager::TunerSpec::Flag::File))) {
						Index = i;
					} else {
						Index = NormalDriverCount++;
					}
					DlgComboBox_InsertString(
						hDlg, IDC_INITIALSETTINGS_DRIVER,
						Index, pDriverInfo->GetFileName());
				}
				if (m_pDriverManager->NumDrivers() > 0) {
					DlgComboBox_GetLBString(
						hDlg, IDC_INITIALSETTINGS_DRIVER,
						0, &m_DriverFileName);
					::SetDlgItemText(hDlg, IDC_INITIALSETTINGS_DRIVER, m_DriverFileName.c_str());
				}
			}

			// 映像デコーダ
			InitDecoderList(
				IDC_INITIALSETTINGS_MPEG2DECODER,
				MEDIASUBTYPE_MPEG2_VIDEO,
				m_Mpeg2DecoderName.c_str());
			InitDecoderList(
				IDC_INITIALSETTINGS_H264DECODER,
				MEDIASUBTYPE_H264,
				m_H264DecoderName.c_str());
			InitDecoderList(
				IDC_INITIALSETTINGS_H265DECODER,
				MEDIASUBTYPE_HEVC,
				m_H265DecoderName.c_str());

			// Video renderer
			{
				int Sel = -1;
				CVideoOptions::RendererInfo Info;

				for (int i = 0; CVideoOptions::GetRendererInfo(i, &Info); i++) {
					DlgComboBox_AddString(hDlg, IDC_INITIALSETTINGS_VIDEORENDERER, Info.pszName);
					DlgComboBox_SetItemData(hDlg, IDC_INITIALSETTINGS_VIDEORENDERER, i, static_cast<LPARAM>(Info.Renderer));
					if (Info.Renderer == m_VideoRenderer)
						Sel = i;
				}
				DlgComboBox_SetCurSel(hDlg, IDC_INITIALSETTINGS_VIDEORENDERER, Sel);
			}

			// 録画フォルダ
			::SetDlgItemText(hDlg, IDC_INITIALSETTINGS_RECORDFOLDER, m_RecordFolder.c_str());
			::SendDlgItemMessage(hDlg, IDC_INITIALSETTINGS_RECORDFOLDER, EM_LIMITTEXT, MAX_PATH - 1, 0);

			AdjustDialogPos(nullptr, hDlg);
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_INITIALSETTINGS_DRIVER_BROWSE:
			{
				OPENFILENAME ofn;
				TCHAR szFileName[MAX_PATH];
				String FileName, InitDir;

				::GetDlgItemText(hDlg, IDC_INITIALSETTINGS_DRIVER, szFileName, lengthof(szFileName));
				if (PathUtil::Split(szFileName, &InitDir, &FileName)) {
					StringCopy(szFileName, FileName.c_str());
				} else {
					GetAppClass().GetAppDirectory(&InitDir);
				}
				InitOpenFileName(&ofn);
				ofn.hwndOwner = hDlg;
				ofn.lpstrFilter =
					TEXT("BonDriver(BonDriver*.dll)\0BonDriver*.dll\0")
					TEXT("すべてのファイル\0*.*\0");
				ofn.lpstrFile = szFileName;
				ofn.nMaxFile = lengthof(szFileName);
				ofn.lpstrInitialDir = InitDir.c_str();
				ofn.lpstrTitle = TEXT("BonDriverの選択");
				ofn.Flags = OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_EXPLORER;
				if (FileOpenDialog(&ofn)) {
					::SetDlgItemText(hDlg, IDC_INITIALSETTINGS_DRIVER, szFileName);
				}
			}
			return TRUE;

		case IDC_INITIALSETTINGS_RECORDFOLDER_BROWSE:
			{
				TCHAR szFolder[MAX_PATH];

				::GetDlgItemText(hDlg, IDC_INITIALSETTINGS_RECORDFOLDER, szFolder, lengthof(szFolder));
				if (BrowseFolderDialog(
							hDlg, szFolder,
							TEXT("録画ファイルの保存先フォルダ:")))
					::SetDlgItemText(hDlg, IDC_INITIALSETTINGS_RECORDFOLDER, szFolder);
			}
			return TRUE;

		case IDC_INITIALSETTINGS_HELP:
			GetAppClass().UICore.ShowHelpContent(HELP_ID_INITIALSETTINGS);
			return TRUE;

		case IDOK:
			{
				const bool fMpeg2Decoder =
					DlgComboBox_GetCount(hDlg, IDC_INITIALSETTINGS_MPEG2DECODER) > 1;
				const bool fH264Decoder =
					DlgComboBox_GetCount(hDlg, IDC_INITIALSETTINGS_H264DECODER) > 1;
				const bool fH265Decoder =
					DlgComboBox_GetCount(hDlg, IDC_INITIALSETTINGS_H265DECODER) > 1;
				if (!fMpeg2Decoder || !fH264Decoder || !fH265Decoder) {
					String Codecs, Message;
					if (!fMpeg2Decoder)
						Codecs = TEXT("MPEG-2");
					if (!fH264Decoder) {
						if (!Codecs.empty())
							Codecs += TEXT("/");
						Codecs += TEXT("H.264(AVC)");
					}
					if (!fH265Decoder) {
						if (!Codecs.empty())
							Codecs += TEXT("/");
						Codecs += TEXT("H.265(HEVC)");
					}
					StringFormat(
						&Message,
						TEXT("{0} のデコーダが見付からないため、{0} の映像は再生できません。\n")
						TEXT("映像を再生するにはデコーダをインストールしてください。"),
						Codecs);
					::MessageBox(hDlg, Message.c_str(), TEXT("お知らせ"), MB_OK | MB_ICONINFORMATION);
				}

				String Mpeg2DecoderName, H264DecoderName, H265DecoderName;
				if (fMpeg2Decoder)
					GetDecoderSetting(IDC_INITIALSETTINGS_MPEG2DECODER, &Mpeg2DecoderName);
				if (fH264Decoder)
					GetDecoderSetting(IDC_INITIALSETTINGS_H264DECODER, &H264DecoderName);
				if (fH265Decoder)
					GetDecoderSetting(IDC_INITIALSETTINGS_H265DECODER, &H265DecoderName);

				const LibISDB::DirectShow::VideoRenderer::RendererType VideoRenderer =
					static_cast<LibISDB::DirectShow::VideoRenderer::RendererType>(
						DlgComboBox_GetItemData(
							hDlg, IDC_INITIALSETTINGS_VIDEORENDERER,
							DlgComboBox_GetCurSel(hDlg, IDC_INITIALSETTINGS_VIDEORENDERER)));

				// 相性の悪い組み合わせに対して注意を表示する
				static const struct {
					LPCTSTR pszDecoder;
					LibISDB::DirectShow::VideoRenderer::RendererType Renderer;
					LPCTSTR pszMessage;
				} ConflictList[] = {
					{
						TEXT("CyberLink"), LibISDB::DirectShow::VideoRenderer::RendererType::Default,
						TEXT("CyberLink のデコーダとデフォルトレンダラの組み合わせで、\n")
						TEXT("一部の番組で比率がおかしくなる現象が出る事があるため、\n")
						TEXT("レンダラをデフォルト以外にすることをお奨めします。\n")
						TEXT("現在の設定を変更しますか?")
					},
				};
				for (const auto &e : ConflictList) {
					const int Length = ::lstrlen(e.pszDecoder);
					if (VideoRenderer == e.Renderer
							&& (::StrCmpNI(Mpeg2DecoderName.c_str(), e.pszDecoder, Length) == 0
								|| ::StrCmpNI(H264DecoderName.c_str(), e.pszDecoder, Length) == 0)
							|| ::StrCmpNI(H265DecoderName.c_str(), e.pszDecoder, Length) == 0) {
						if (::MessageBox(
									hDlg, e.pszMessage, TEXT("注意"),
									MB_YESNO | MB_ICONINFORMATION) == IDYES)
							return TRUE;
						break;
					}
				}

				if (!LibISDB::DirectShow::VideoRenderer::IsAvailable(VideoRenderer)) {
					::MessageBox(
						hDlg, TEXT("選択されたレンダラはこの環境で利用可能になっていません。"),
						nullptr, MB_OK | MB_ICONEXCLAMATION);
					return TRUE;
				}

				GetDlgItemString(hDlg, IDC_INITIALSETTINGS_DRIVER, &m_DriverFileName);

				m_Mpeg2DecoderName = Mpeg2DecoderName;
				m_H264DecoderName = H264DecoderName;
				m_H265DecoderName = H265DecoderName;

				m_VideoRenderer = VideoRenderer;

				TCHAR szRecordFolder[MAX_PATH];
				::GetDlgItemText(
					hDlg, IDC_INITIALSETTINGS_RECORDFOLDER,
					szRecordFolder, lengthof(szRecordFolder));
				const CAppMain::CreateDirectoryResult CreateDirResult =
					GetAppClass().CreateDirectory(
						hDlg, szRecordFolder,
						TEXT("録画ファイルの保存先フォルダ \"{}\" がありません。\n")
						TEXT("作成しますか?"));
				if (CreateDirResult == CAppMain::CreateDirectoryResult::Error) {
					SetDlgItemFocus(hDlg, IDC_INITIALSETTINGS_RECORDFOLDER);
					return TRUE;
				}
				m_RecordFolder = szRecordFolder;
			}
			[[fallthrough]];
		case IDCANCEL:
			::EndDialog(hDlg, LOWORD(wParam));
			return TRUE;
		}
		return TRUE;

	case WM_PAINT:
		if (m_fDrawLogo) {
			PAINTSTRUCT ps;

			::BeginPaint(hDlg, &ps);
			{
				Graphics::CCanvas Canvas(ps.hdc);
				Graphics::CBrush Brush(::GetSysColor(COLOR_3DFACE));
				RECT rc, rcClient;

				::GetWindowRect(::GetDlgItem(hDlg, IDC_INITIALSETTINGS_LOGO), &rc);
				::OffsetRect(&rc, -rc.left, -rc.top);
				Canvas.Clear(0, 0, 0, 0);
				::GetClientRect(hDlg, &rcClient);
				rcClient.left = rc.right;
				Canvas.FillRect(&Brush, rcClient);
				Canvas.DrawImage(
					&m_LogoImage,
					(rc.right - m_LogoImage.GetWidth()) / 2,
					(rc.bottom - m_LogoImage.GetHeight()) / 2);
			}
			::EndPaint(hDlg, &ps);
			return TRUE;
		}
		break;

	case WM_CTLCOLORSTATIC:
		if (reinterpret_cast<HWND>(lParam) == ::GetDlgItem(hDlg, IDC_INITIALSETTINGS_LOGO))
			return reinterpret_cast<INT_PTR>(::GetStockObject(WHITE_BRUSH));
		break;

	case WM_DESTROY:
		{
			const HBITMAP hbm = reinterpret_cast<HBITMAP>(
				::SendDlgItemMessage(
					hDlg, IDC_INITIALSETTINGS_LOGO,
					STM_SETIMAGE, IMAGE_BITMAP, reinterpret_cast<LPARAM>(nullptr)));

			if (hbm != nullptr) {
				::DeleteObject(hbm);
			} else {
				m_LogoImage.Free();
			}
		}
		return TRUE;
	}

	return FALSE;
}


void CInitialSettings::InitDecoderList(int ID, const GUID &SubType, LPCTSTR pszDecoderName)
{
	const LPCWSTR pszDefaultDecoderName =
		LibISDB::DirectShow::KnownDecoderManager::IsDecoderAvailable(SubType) ?
		LibISDB::DirectShow::KnownDecoderManager::GetDecoderName(SubType) : nullptr;
	LibISDB::DirectShow::FilterFinder FilterFinder;
	std::vector<String> FilterList;
	int Sel = 0;

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
		if (FilterList.size() > 1) {
			std::ranges::sort(
				FilterList,
				[](const String Filter1, const String & Filter2) {
					return ::CompareString(
						LOCALE_USER_DEFAULT,
						NORM_IGNORECASE | NORM_IGNORESYMBOLS,
						Filter1.data(), static_cast<int>(Filter1.length()),
						Filter2.data(), static_cast<int>(Filter2.length())) == CSTR_LESS_THAN;
				});
		}
		for (const String &e :FilterList) {
			DlgComboBox_AddString(m_hDlg, ID, e.c_str());
		}
	}

	if (pszDefaultDecoderName != nullptr)
		DlgComboBox_InsertString(m_hDlg, ID, 0, pszDefaultDecoderName);

	if (!IsStringEmpty(pszDecoderName))
		Sel = static_cast<int>(DlgComboBox_FindStringExact(m_hDlg, ID, -1, pszDecoderName)) + 1;

	DlgComboBox_InsertString(
		m_hDlg, ID,
		0, !FilterList.empty() || pszDefaultDecoderName != nullptr ? TEXT("自動") : TEXT("<デコーダが見付かりません>"));
	DlgComboBox_SetCurSel(m_hDlg, ID, Sel);
}


void CInitialSettings::GetDecoderSetting(int ID, String *pDecoderName) const
{
	const int Sel = static_cast<int>(DlgComboBox_GetCurSel(m_hDlg, ID));
	if (Sel > 0) {
		TCHAR szDecoder[MAX_DECODER_NAME];
		DlgComboBox_GetLBString(m_hDlg, ID, Sel, szDecoder);
		*pDecoderName = szDecoder;
	} else {
		pDecoderName->clear();
	}
}


}	// namespace TVTest
