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
#include "CaptureOptions.h"
#include "DialogUtil.h"
#include "resource.h"
#include "Common/DebugDef.h"


namespace TVTest
{


class CCaptureVariableStringMap
	: public CEventVariableStringMap
{
public:
	CCaptureVariableStringMap();
	CCaptureVariableStringMap(const EventInfo &Info, const CCaptureImage *pImage);
	bool GetParameterList(ParameterGroupList *pList) const override;

private:
	bool GetLocalString(LPCWSTR pszKeyword, String *pString) override;

	static const ParameterInfo m_CaptureParameterList[];

	int m_ImageWidth;
	int m_ImageHeight;
};


const CVariableStringMap::ParameterInfo CCaptureVariableStringMap::m_CaptureParameterList[] =
{
	{TEXT("width"),  TEXT("画像の幅")},
	{TEXT("height"), TEXT("画像の高さ")},
};


CCaptureVariableStringMap::CCaptureVariableStringMap()
	: m_ImageWidth(1920)
	, m_ImageHeight(1080)
{
}


CCaptureVariableStringMap::CCaptureVariableStringMap(
	const EventInfo &Info, const CCaptureImage *pImage)
	: CEventVariableStringMap(Info)
{
	BITMAPINFOHEADER bmih;

	if (pImage->GetBitmapInfoHeader(&bmih)) {
		m_ImageWidth = bmih.biWidth;
		m_ImageHeight = bmih.biHeight;
	} else {
		m_ImageWidth = 0;
		m_ImageHeight = 0;
	}
}


bool CCaptureVariableStringMap::GetLocalString(LPCWSTR pszKeyword, String *pString)
{
	if (::lstrcmpi(pszKeyword, TEXT("width")) == 0) {
		StringFormat(pString, TEXT("{}"), m_ImageWidth);
	} else if (::lstrcmpi(pszKeyword, TEXT("height")) == 0) {
		StringFormat(pString, TEXT("{}"), m_ImageHeight);
	} else {
		return CEventVariableStringMap::GetLocalString(pszKeyword, pString);
	}

	return true;
}


bool CCaptureVariableStringMap::GetParameterList(ParameterGroupList *pList) const
{
	if (!CEventVariableStringMap::GetParameterList(pList))
		return false;

	ParameterGroup &Group = pList->emplace_back();
	Group.ParameterList.insert(
		Group.ParameterList.end(),
		m_CaptureParameterList,
		m_CaptureParameterList + lengthof(m_CaptureParameterList));

	return true;
}




const SIZE CCaptureOptions::m_SizeList[SIZE_LAST + 1] = {
	// 16:9
	{1920, 1080},
	{1440,  810},
	{1280,  720},
	{1024,  576},
	{ 960,  540},
	{ 800,  450},
	{ 640,  360},
	{ 320,  180},
	// 4:3
	{1440, 1080},
	{1280,  960},
	{1024,  768},
	{ 800,  600},
	{ 720,  540},
	{ 640,  480},
	{ 320,  240},
};


const CCaptureOptions::PercentageType CCaptureOptions::m_PercentageList[PERCENTAGE_LAST + 1] = {
	{3, 4},	// 75%
	{2, 3},	// 66%
	{1, 2},	// 50%
	{1, 3},	// 33%
	{1, 4},	// 25%
};


CCaptureOptions::~CCaptureOptions()
{
	Destroy();
}


bool CCaptureOptions::ReadSettings(CSettings &Settings)
{
	Settings.Read(TEXT("CaptureFolder"), &m_SaveFolder);
	if (!Settings.Read(TEXT("CaptureFileNameFormat"), &m_FileName)) {
		// ver.0.9.0 より前との互換用
		if (Settings.Read(TEXT("CaptureFileName"), &m_FileName))
			m_FileName += TEXT("%date%-%time%");
	}
	TCHAR szFormat[32];
	if (Settings.Read(TEXT("CaptureSaveFormat"), szFormat, lengthof(szFormat))) {
		const int Format = m_ImageCodec.FormatNameToIndex(szFormat);
		if (Format >= 0)
			m_SaveFormat = Format;
	}
	Settings.Read(TEXT("CaptureIconSaveFile"), &m_fCaptureSaveToFile);
	Settings.Read(TEXT("CaptureSetComment"), &m_fSetComment);
	String CommentFormat;
	if (Settings.Read(TEXT("CaptureCommentFormat"), &CommentFormat))
		m_CommentFormat = StringUtility::Decode(CommentFormat);
	Settings.Read(TEXT("JpegQuality"), &m_JPEGQuality);
	Settings.Read(TEXT("PngCompressionLevel"), &m_PNGCompressionLevel);
	int Size;
	if (Settings.Read(TEXT("CaptureSizeType"), &Size)
			&& Size >= 0 && Size <= SIZE_TYPE_LAST) {
		if (Size == SIZE_TYPE_RAW)
			m_CaptureSizeType = SIZE_TYPE_ORIGINAL;
		else
			m_CaptureSizeType = Size;
	}
	int Width, Height;
	if (Settings.Read(TEXT("CaptureWidth"), &Width)
			&& Settings.Read(TEXT("CaptureHeight"), &Height)) {
		for (int i = 0; i <= SIZE_LAST; i++) {
			if (m_SizeList[i].cx == Width && m_SizeList[i].cy == Height) {
				m_CaptureSize = i;
				break;
			}
		}
	}
	int Num, Denom;
	if (Settings.Read(TEXT("CaptureRatioNum"), &Num)
			&& Settings.Read(TEXT("CaptureRatioDenom"), &Denom)) {
		for (int i = 0; i <= PERCENTAGE_LAST; i++) {
			if (m_PercentageList[i].Num == Num
					&& m_PercentageList[i].Denom == Denom) {
				m_CapturePercentage = i;
				break;
			}
		}
	}
	return true;
}


bool CCaptureOptions::WriteSettings(CSettings &Settings)
{
	Settings.Write(TEXT("CaptureFolder"), m_SaveFolder);
	Settings.Write(TEXT("CaptureFileNameFormat"), m_FileName);
	Settings.Write(TEXT("CaptureSaveFormat"), m_ImageCodec.EnumSaveFormat(m_SaveFormat));
	Settings.Write(TEXT("CaptureIconSaveFile"), m_fCaptureSaveToFile);
	Settings.Write(TEXT("CaptureSetComment"), m_fSetComment);
	Settings.Write(TEXT("CaptureCommentFormat"), StringUtility::Encode(m_CommentFormat));
	Settings.Write(TEXT("JpegQuality"), m_JPEGQuality);
	Settings.Write(TEXT("PngCompressionLevel"), m_PNGCompressionLevel);
	Settings.Write(TEXT("CaptureSizeType"), m_CaptureSizeType);
	Settings.Write(TEXT("CaptureWidth"), static_cast<int>(m_SizeList[m_CaptureSize].cx));
	Settings.Write(TEXT("CaptureHeight"), static_cast<int>(m_SizeList[m_CaptureSize].cy));
	Settings.Write(TEXT("CaptureRatioNum"), m_PercentageList[m_CapturePercentage].Num);
	Settings.Write(TEXT("CaptureRatioDenom"), m_PercentageList[m_CapturePercentage].Denom);
	return true;
}


bool CCaptureOptions::Create(HWND hwndOwner)
{
	return CreateDialogWindow(
		hwndOwner,
		GetAppClass().GetResourceInstance(), MAKEINTRESOURCE(IDD_OPTIONS_CAPTURE));
}


bool CCaptureOptions::SetPresetCaptureSize(int Size)
{
	if (Size < 0)
		return false;
	if (Size <= SIZE_TYPE_VIEW) {
		m_CaptureSizeType = Size;
	} else if (Size - 2 <= PERCENTAGE_LAST) {
		m_CaptureSizeType = SIZE_TYPE_PERCENTAGE;
		m_CapturePercentage = Size - 2;
	} else if (Size - (2 + PERCENTAGE_LAST + 1) <= SIZE_LAST) {
		m_CaptureSizeType = SIZE_TYPE_CUSTOM;
		m_CaptureSize = Size - (2 + PERCENTAGE_LAST + 1);
	} else {
		return false;
	}
	return true;
}


int CCaptureOptions::GetPresetCaptureSize() const
{
	int Size;

	switch (m_CaptureSizeType) {
	case SIZE_TYPE_ORIGINAL:
	case SIZE_TYPE_VIEW:
		Size = m_CaptureSizeType;
		break;
	case SIZE_TYPE_CUSTOM:
		Size = 2 + (PERCENTAGE_LAST + 1) + m_CaptureSize;
		break;
	case SIZE_TYPE_PERCENTAGE:
		Size = 2 + m_CapturePercentage;
		break;
	}
	return Size;
}


bool CCaptureOptions::GetSizePercentage(int *pNum, int *pDenom) const
{
	if (pNum)
		*pNum = m_PercentageList[m_CapturePercentage].Num;
	if (pDenom)
		*pDenom = m_PercentageList[m_CapturePercentage].Denom;
	return true;
}


bool CCaptureOptions::GetCustomSize(int *pWidth, int *pHeight) const
{
	if (pWidth)
		*pWidth = m_SizeList[m_CaptureSize].cx;
	if (pHeight)
		*pHeight = m_SizeList[m_CaptureSize].cy;
	return true;
}


bool CCaptureOptions::GenerateFileName(
	String *pFileName, const CCaptureImage *pImage) const
{
	if (pFileName == nullptr)
		return false;

	CFilePath SaveFolder;

	if (!m_SaveFolder.empty()) {
		if (!GetAbsolutePath(m_SaveFolder, &SaveFolder))
			return false;
	} else {
		if (!GetAppClass().GetAppDirectory(&SaveFolder))
			return false;
	}

	CEventVariableStringMap::EventInfo EventInfo;
	GetAppClass().Core.GetVariableStringEventInfo(&EventInfo);
	CCaptureVariableStringMap VarStrMap(EventInfo, pImage);
	String FileName;

	VarStrMap.SetCurrentTime(&pImage->GetCaptureTime());

	if (!FormatVariableString(&VarStrMap, m_FileName.c_str(), &FileName)
			|| FileName.empty())
		return false;
	SaveFolder.Append(FileName);
	FileName = SaveFolder;
	SaveFolder.RemoveFileName();
	if (SaveFolder.length() >= MAX_PATH)
		return false;
	if (!::PathIsDirectory(SaveFolder.c_str())) {
		const int Result = ::SHCreateDirectoryEx(nullptr, SaveFolder.c_str(), nullptr);
		if (Result != ERROR_SUCCESS && Result != ERROR_ALREADY_EXISTS) {
			GetAppClass().AddLog(
				CLogItem::LogType::Error,
				TEXT("キャプチャの保存先フォルダ \"{}\" を作成できません。"),
				SaveFolder);
			return false;
		}
	}

	FileName += _T('.');
	FileName += m_ImageCodec.GetExtension(m_SaveFormat);
	if (!MakeUniqueFileName(&FileName))
		return false;
	*pFileName = FileName;

	return true;
}


bool CCaptureOptions::GetOptionText(LPTSTR pszOption, int MaxLength) const
{
	LPCTSTR pszFormatName = m_ImageCodec.EnumSaveFormat(m_SaveFormat);

	if (::lstrcmpi(pszFormatName, TEXT("JPEG")) == 0) {
		if (MaxLength < 4)
			return false;
		StringFormat(pszOption, MaxLength, TEXT("{}"), m_JPEGQuality);
	} else if (::lstrcmpi(pszFormatName, TEXT("PNG")) == 0) {
		if (MaxLength < 2)
			return false;
		StringFormat(pszOption, MaxLength, TEXT("{}"), m_PNGCompressionLevel);
	} else {
		if (MaxLength < 1)
			return false;
		pszOption[0] = '\0';
	}
	return true;
}


bool CCaptureOptions::GetCommentText(
	String *pComment, const CCaptureImage *pImage) const
{
	if (pComment == nullptr || pImage == nullptr)
		return false;

	CEventVariableStringMap::EventInfo EventInfo;
	GetAppClass().Core.GetVariableStringEventInfo(&EventInfo);
	CCaptureVariableStringMap VarStrMap(EventInfo, pImage);

	VarStrMap.SetCurrentTime(&pImage->GetCaptureTime());

	return FormatVariableString(&VarStrMap, m_CommentFormat.c_str(), pComment);
}


bool CCaptureOptions::SaveImage(CCaptureImage *pImage)
{
	String FileName;
	TCHAR szOption[16];
	BITMAPINFO *pbmi;
	BYTE *pBits;

	if (!GenerateFileName(&FileName, pImage))
		return false;
	GetOptionText(szOption, lengthof(szOption));
	if (!pImage->LockData(&pbmi, &pBits))
		return false;
	const bool fOK = m_ImageCodec.SaveImage(
		FileName.c_str(), m_SaveFormat, szOption,
		pbmi, pBits, m_fSetComment ? pImage->GetComment() : nullptr);
	pImage->UnlockData();
	return fOK;
}


int CCaptureOptions::TranslateCommand(int Command)
{
	if (Command == CM_CAPTURE)
		return m_fCaptureSaveToFile ? CM_SAVEIMAGE : CM_COPYIMAGE;
	return -1;
}


bool CCaptureOptions::OpenSaveFolder() const
{
	String Folder;

	if (!m_SaveFolder.empty()) {
		if (!GetAbsolutePath(m_SaveFolder, &Folder))
			return false;
	} else {
		if (!GetAppClass().GetAppDirectory(&Folder))
			return false;
	}
	return (ULONG_PTR)::ShellExecute(nullptr, TEXT("open"), Folder.c_str(), nullptr, nullptr, SW_SHOWNORMAL) > 32;
}


INT_PTR CCaptureOptions::DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			SendDlgItemMessage(hDlg, IDC_CAPTUREOPTIONS_SAVEFOLDER, EM_LIMITTEXT, MAX_PATH - 1, 0);
			SetDlgItemText(hDlg, IDC_CAPTUREOPTIONS_SAVEFOLDER, m_SaveFolder.c_str());
			SetDlgItemText(hDlg, IDC_CAPTUREOPTIONS_FILENAME, m_FileName.c_str());
			InitDropDownButton(hDlg, IDC_CAPTUREOPTIONS_FILENAME_PARAMETERS);

			static const LPCTSTR SizeTypeText[] = {
				TEXT("元の大きさ"),
				TEXT("表示されている大きさ"),
			};
			TCHAR szText[32];
			for (LPCTSTR pszText : SizeTypeText)
				DlgComboBox_AddString(hDlg, IDC_CAPTUREOPTIONS_SIZE, pszText);
			for (int i = 0; i <= PERCENTAGE_LAST; i++) {
				const PercentageType &Ratio = m_PercentageList[i];
				const size_t Length = StringFormat(
					szText, TEXT("{} %"),
					::MulDiv(Ratio.Num, 100, Ratio.Denom));
				if (Ratio.Num * 100 % Ratio.Denom != 0) {
					StringFormat(
						szText + Length, lengthof(szText) - Length, TEXT(" ({}/{})"),
						Ratio.Num, Ratio.Denom);
				}
				DlgComboBox_AddString(hDlg, IDC_CAPTUREOPTIONS_SIZE, szText);
			}
			for (int i = 0; i <= SIZE_LAST; i++) {
				StringFormat(
					szText, TEXT("{} x {}"),
					m_SizeList[i].cx, m_SizeList[i].cy);
				DlgComboBox_AddString(hDlg, IDC_CAPTUREOPTIONS_SIZE, szText);
			}
			int Sel = -1;
			switch (m_CaptureSizeType) {
			case SIZE_TYPE_ORIGINAL:
			case SIZE_TYPE_VIEW:
				Sel = m_CaptureSizeType;
				break;
			case SIZE_TYPE_CUSTOM:
				Sel = lengthof(SizeTypeText) + (PERCENTAGE_LAST + 1) + m_CaptureSize;
				break;
			case SIZE_TYPE_PERCENTAGE:
				Sel = lengthof(SizeTypeText) + m_CapturePercentage;
				break;
			}
			DlgComboBox_SetCurSel(hDlg, IDC_CAPTUREOPTIONS_SIZE, Sel);

			LPCTSTR pszFormat;
			for (int i = 0; (pszFormat = m_ImageCodec.EnumSaveFormat(i)) != nullptr; i++)
				DlgComboBox_AddString(hDlg, IDC_CAPTUREOPTIONS_FORMAT, pszFormat);
			DlgComboBox_SetCurSel(hDlg, IDC_CAPTUREOPTIONS_FORMAT, m_SaveFormat);

			// JPEG quality
			SendDlgItemMessage(hDlg, IDC_CAPTUREOPTIONS_JPEGQUALITY_TB, TBM_SETRANGE, TRUE, MAKELPARAM(0, 100));
			SendDlgItemMessage(hDlg, IDC_CAPTUREOPTIONS_JPEGQUALITY_TB, TBM_SETPOS, TRUE, m_JPEGQuality);
			SendDlgItemMessage(hDlg, IDC_CAPTUREOPTIONS_JPEGQUALITY_TB, TBM_SETPAGESIZE, 0, 10);
			SendDlgItemMessage(hDlg, IDC_CAPTUREOPTIONS_JPEGQUALITY_TB, TBM_SETTICFREQ, 10, 0);
			SendDlgItemMessage(hDlg, IDC_CAPTUREOPTIONS_JPEGQUALITY_EDIT, EM_LIMITTEXT, 3, 0);
			SetDlgItemInt(hDlg, IDC_CAPTUREOPTIONS_JPEGQUALITY_EDIT, m_JPEGQuality, TRUE);
			DlgUpDown_SetRange(hDlg, IDC_CAPTUREOPTIONS_JPEGQUALITY_UD, 0, 100);

			// PNG compression level
			SendDlgItemMessage(hDlg, IDC_CAPTUREOPTIONS_PNGLEVEL_TB, TBM_SETRANGE, TRUE, MAKELPARAM(0, 9));
			SendDlgItemMessage(hDlg, IDC_CAPTUREOPTIONS_PNGLEVEL_TB, TBM_SETPOS, TRUE, m_PNGCompressionLevel);
			SendDlgItemMessage(hDlg, IDC_CAPTUREOPTIONS_PNGLEVEL_TB, TBM_SETPAGESIZE, 0, 1);
			SendDlgItemMessage(hDlg, IDC_CAPTUREOPTIONS_PNGLEVEL_TB, TBM_SETTICFREQ, 1, 0);
			SendDlgItemMessage(hDlg, IDC_CAPTUREOPTIONS_PNGLEVEL_EDIT, EM_LIMITTEXT, 0, 1);
			SetDlgItemInt(hDlg, IDC_CAPTUREOPTIONS_PNGLEVEL_EDIT, m_PNGCompressionLevel, FALSE);
			DlgUpDown_SetRange(hDlg, IDC_CAPTUREOPTIONS_PNGLEVEL_UD, 0, 9);

			DlgCheckBox_Check(hDlg, IDC_CAPTUREOPTIONS_ICONSAVEFILE, m_fCaptureSaveToFile);
			DlgCheckBox_Check(hDlg, IDC_CAPTUREOPTIONS_SETCOMMENT, m_fSetComment);
			::SetDlgItemText(hDlg, IDC_CAPTUREOPTIONS_COMMENT, m_CommentFormat.c_str());
			InitDropDownButton(hDlg, IDC_CAPTUREOPTIONS_COMMENT_PARAMETERS);
			EnableDlgItems(
				hDlg,
				IDC_CAPTUREOPTIONS_COMMENT,
				IDC_CAPTUREOPTIONS_COMMENT_PARAMETERS,
				m_fSetComment);

			AddControls({
				{IDC_CAPTUREOPTIONS_SAVEFOLDER,          AlignFlag::Horz},
				{IDC_CAPTUREOPTIONS_SAVEFOLDER_BROWSE,   AlignFlag::Right},
				{IDC_CAPTUREOPTIONS_FILENAME,            AlignFlag::Horz},
				{IDC_CAPTUREOPTIONS_FILENAME_PARAMETERS, AlignFlag::Right},
				{IDC_CAPTUREOPTIONS_FILENAME_PREVIEW,    AlignFlag::Horz},
				{IDC_CAPTUREOPTIONS_COMMENT,             AlignFlag::Horz},
				{IDC_CAPTUREOPTIONS_COMMENT_PARAMETERS,  AlignFlag::Right},
			});
		}
		return TRUE;

	case WM_HSCROLL:
		if (reinterpret_cast<HWND>(lParam) == GetDlgItem(hDlg, IDC_CAPTUREOPTIONS_JPEGQUALITY_TB)) {
			SyncEditWithTrackBar(hDlg, IDC_CAPTUREOPTIONS_JPEGQUALITY_TB, IDC_CAPTUREOPTIONS_JPEGQUALITY_EDIT);
		} else if (reinterpret_cast<HWND>(lParam) == GetDlgItem(hDlg, IDC_CAPTUREOPTIONS_PNGLEVEL_TB)) {
			SyncEditWithTrackBar(hDlg, IDC_CAPTUREOPTIONS_PNGLEVEL_TB, IDC_CAPTUREOPTIONS_PNGLEVEL_EDIT);
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_CAPTUREOPTIONS_SAVEFOLDER_BROWSE:
			{
				TCHAR szFolder[MAX_PATH];

				GetDlgItemText(hDlg, IDC_CAPTUREOPTIONS_SAVEFOLDER, szFolder, lengthof(szFolder));
				if (BrowseFolderDialog(hDlg, szFolder, TEXT("画像の保存先フォルダ:")))
					SetDlgItemText(hDlg, IDC_CAPTUREOPTIONS_SAVEFOLDER, szFolder);
			}
			return TRUE;

		case IDC_CAPTUREOPTIONS_FILENAME:
			if (HIWORD(wParam) == EN_CHANGE)
				UpdateFileNamePreview();
			return TRUE;

		case IDC_CAPTUREOPTIONS_FILENAME_PARAMETERS:
			{
				RECT rc;

				::GetWindowRect(::GetDlgItem(hDlg, IDC_CAPTUREOPTIONS_FILENAME_PARAMETERS), &rc);
				const POINT pt = {rc.left, rc.bottom};
				CCaptureVariableStringMap VarStrMap;
				VarStrMap.InputParameter(hDlg, IDC_CAPTUREOPTIONS_FILENAME, pt);
			}
			return TRUE;

		case IDC_CAPTUREOPTIONS_FORMAT:
			if (HIWORD(wParam) == CBN_SELCHANGE)
				UpdateFileNamePreview();
			return TRUE;

		case IDC_CAPTUREOPTIONS_JPEGQUALITY_EDIT:
			if (HIWORD(wParam) == EN_CHANGE)
				SyncTrackBarWithEdit(hDlg, IDC_CAPTUREOPTIONS_JPEGQUALITY_EDIT, IDC_CAPTUREOPTIONS_JPEGQUALITY_TB);
			return TRUE;

		case IDC_CAPTUREOPTIONS_PNGLEVEL_EDIT:
			if (HIWORD(wParam) == EN_CHANGE)
				SyncTrackBarWithEdit(hDlg, IDC_CAPTUREOPTIONS_PNGLEVEL_EDIT, IDC_CAPTUREOPTIONS_PNGLEVEL_TB);
			return TRUE;

		case IDC_CAPTUREOPTIONS_SETCOMMENT:
			EnableDlgItemsSyncCheckBox(
				hDlg,
				IDC_CAPTUREOPTIONS_COMMENT,
				IDC_CAPTUREOPTIONS_COMMENT_PARAMETERS,
				IDC_CAPTUREOPTIONS_SETCOMMENT);
			return TRUE;

		case IDC_CAPTUREOPTIONS_COMMENT_PARAMETERS:
			{
				RECT rc;

				::GetWindowRect(::GetDlgItem(hDlg, IDC_CAPTUREOPTIONS_COMMENT_PARAMETERS), &rc);
				const POINT pt = {rc.left, rc.bottom};
				CCaptureVariableStringMap VarStrMap;
				VarStrMap.InputParameter(hDlg, IDC_CAPTUREOPTIONS_COMMENT, pt);
			}
			return TRUE;
		}
		return TRUE;

	case WM_NOTIFY:
		switch (reinterpret_cast<LPNMHDR>(lParam)->code) {
		case PSN_APPLY:
			{
				String SaveFolder;
				GetDlgItemString(hDlg, IDC_CAPTUREOPTIONS_SAVEFOLDER, &SaveFolder);
				const CAppMain::CreateDirectoryResult CreateDirResult =
					GetAppClass().CreateDirectory(
						hDlg, SaveFolder.c_str(),
						TEXT("キャプチャ画像の保存先フォルダ \"{}\" がありません。\n")
						TEXT("作成しますか?"));
				if (CreateDirResult == CAppMain::CreateDirectoryResult::Error) {
					SettingError();
					SetDlgItemFocus(hDlg, IDC_CAPTUREOPTIONS_SAVEFOLDER);
					return TRUE;
				}

				String FileName, Message;
				GetDlgItemString(hDlg, IDC_CAPTUREOPTIONS_FILENAME, &FileName);
				if (!IsValidFileName(FileName.c_str(), FileNameValidateFlag::AllowDelimiter, &Message)) {
					SettingError();
					::SendDlgItemMessage(hDlg, IDC_CAPTUREOPTIONS_FILENAME, EM_SETSEL, 0, -1);
					::MessageBox(hDlg, Message.c_str(), nullptr, MB_OK | MB_ICONEXCLAMATION);
					SetDlgItemFocus(hDlg, IDC_CAPTUREOPTIONS_FILENAME);
					return TRUE;
				}

				m_SaveFolder = SaveFolder;
				m_FileName = FileName;

				SetPresetCaptureSize(
					static_cast<int>(DlgComboBox_GetCurSel(hDlg, IDC_CAPTUREOPTIONS_SIZE)));
				m_SaveFormat =
					static_cast<int>(DlgComboBox_GetCurSel(hDlg, IDC_CAPTUREOPTIONS_FORMAT));
				m_JPEGQuality =
					static_cast<int>(SendDlgItemMessage(hDlg, IDC_CAPTUREOPTIONS_JPEGQUALITY_TB, TBM_GETPOS, 0, 0));
				m_PNGCompressionLevel =
					static_cast<int>(SendDlgItemMessage(hDlg, IDC_CAPTUREOPTIONS_PNGLEVEL_TB, TBM_GETPOS, 0, 0));
				m_fCaptureSaveToFile =
					DlgCheckBox_IsChecked(hDlg, IDC_CAPTUREOPTIONS_ICONSAVEFILE);
				m_fSetComment =
					DlgCheckBox_IsChecked(hDlg, IDC_CAPTUREOPTIONS_SETCOMMENT);
				GetDlgItemString(hDlg, IDC_CAPTUREOPTIONS_COMMENT, &m_CommentFormat);

				m_fChanged = true;
			}
			break;
		}
		break;
	}

	return FALSE;
}


void CCaptureOptions::UpdateFileNamePreview()
{
	String Format;
	String FileName;

	GetDlgItemString(m_hDlg, IDC_CAPTUREOPTIONS_FILENAME, &Format);
	if (!Format.empty()) {
		CCaptureVariableStringMap VarStrMap;
		VarStrMap.SetSampleEventInfo();
		FormatVariableString(&VarStrMap, Format.c_str(), &FileName);
		if (!FileName.empty()) {
			LPCTSTR pszExtension = m_ImageCodec.GetExtension(
				static_cast<int>(DlgComboBox_GetCurSel(m_hDlg, IDC_CAPTUREOPTIONS_FORMAT)));
			if (pszExtension != nullptr) {
				FileName += _T('.');
				FileName += pszExtension;
			}
		}
	}
	::SetDlgItemText(m_hDlg, IDC_CAPTUREOPTIONS_FILENAME_PREVIEW, FileName.c_str());
}


}	// namespace TVTest
