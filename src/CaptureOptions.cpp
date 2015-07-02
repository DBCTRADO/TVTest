#include "stdafx.h"
#include "TVTest.h"
#include "AppMain.h"
#include "CaptureOptions.h"
#include "DialogUtil.h"
#include "resource.h"
#include "Common/DebugDef.h"


namespace TVTest
{


class CCaptureVariableStringMap : public CEventVariableStringMap
{
public:
	CCaptureVariableStringMap();
	CCaptureVariableStringMap(const EventInfo &Info,const CCaptureImage *pImage);
	bool GetString(LPCWSTR pszKeyword,String *pString) override;
	bool GetParameterInfo(int Index,ParameterInfo *pInfo) const override;
	int GetParameterCount() const override;

private:
	static const ParameterInfo m_CaptureParameterList[];

	int m_ImageWidth;
	int m_ImageHeight;
};


const CVariableStringMap::ParameterInfo CCaptureVariableStringMap::m_CaptureParameterList[] =
{
	{TEXT("%width%"),	TEXT("画像の幅")},
	{TEXT("%height%"),	TEXT("画像の高さ")},
};


CCaptureVariableStringMap::CCaptureVariableStringMap()
	: m_ImageWidth(1920)
	, m_ImageHeight(1080)
{
}


CCaptureVariableStringMap::CCaptureVariableStringMap(
		const EventInfo &Info,const CCaptureImage *pImage)
	: CEventVariableStringMap(Info)
{
	BITMAPINFOHEADER bmih;

	if (pImage->GetBitmapInfoHeader(&bmih)) {
		m_ImageWidth=bmih.biWidth;
		m_ImageHeight=bmih.biHeight;
	} else {
		m_ImageWidth=0;
		m_ImageHeight=0;
	}
}


bool CCaptureVariableStringMap::GetString(LPCWSTR pszKeyword,String *pString)
{
	if (::lstrcmpi(pszKeyword,TEXT("width"))==0) {
		TVTest::StringUtility::Format(*pString,TEXT("%d"),m_ImageWidth);
	} else if (::lstrcmpi(pszKeyword,TEXT("height"))==0) {
		TVTest::StringUtility::Format(*pString,TEXT("%d"),m_ImageHeight);
	} else {
		return CEventVariableStringMap::GetString(pszKeyword,pString);
	}

	return true;
}


bool CCaptureVariableStringMap::GetParameterInfo(int Index,ParameterInfo *pInfo) const
{
	if (Index>=CEventVariableStringMap::GetParameterCount()) {
		Index-=CEventVariableStringMap::GetParameterCount();
		if (Index>=lengthof(m_CaptureParameterList))
			return false;
		*pInfo=m_CaptureParameterList[Index];
		return true;
	}

	return CEventVariableStringMap::GetParameterInfo(Index,pInfo);
}


int CCaptureVariableStringMap::GetParameterCount() const
{
	return CEventVariableStringMap::GetParameterCount()+lengthof(m_CaptureParameterList);
}


}	// namespace TVTest




const SIZE CCaptureOptions::m_SizeList[SIZE_LAST+1] = {
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


const CCaptureOptions::PercentageType CCaptureOptions::m_PercentageList[PERCENTAGE_LAST+1] = {
	{3, 4},	// 75%
	{2, 3},	// 66%
	{1, 2},	// 50%
	{1, 3},	// 33%
	{1, 4},	// 25%
};


CCaptureOptions::CCaptureOptions()
	: m_FileName(TEXT("Capture_%date%-%time%"))
	, m_SaveFormat(0)
	, m_JPEGQuality(90)
	, m_PNGCompressionLevel(6)
	, m_fCaptureSaveToFile(true)
	, m_fSetComment(false)
	, m_CommentFormat(TEXT("%year%/%month%/%day% %hour%:%minute%:%second% %channel-name%\r\n%event-title%"))
	, m_CaptureSizeType(SIZE_TYPE_ORIGINAL)
	, m_CaptureSize(
#ifndef TVTEST_FOR_1SEG
		SIZE_1920x1080
#else
		SIZE_320x180
#endif
		)
	, m_CapturePercentage(PERCENTAGE_50)
{
	GetAppClass().GetAppDirectory(m_szSaveFolder);
}


CCaptureOptions::~CCaptureOptions()
{
	Destroy();
}


bool CCaptureOptions::ReadSettings(CSettings &Settings)
{
	Settings.Read(TEXT("CaptureFolder"),m_szSaveFolder,lengthof(m_szSaveFolder));
	if (!Settings.Read(TEXT("CaptureFileNameFormat"),&m_FileName)) {
		// ver.0.9.0 より前との互換用
		if (Settings.Read(TEXT("CaptureFileName"),&m_FileName))
			m_FileName+=TEXT("%date%-%time%");
	}
	TCHAR szFormat[32];
	if (Settings.Read(TEXT("CaptureSaveFormat"),szFormat,lengthof(szFormat))) {
		int Format=m_ImageCodec.FormatNameToIndex(szFormat);
		if (Format>=0)
			m_SaveFormat=Format;
	}
	Settings.Read(TEXT("CaptureIconSaveFile"),&m_fCaptureSaveToFile);
	Settings.Read(TEXT("CaptureSetComment"),&m_fSetComment);
	TVTest::String CommentFormat;
	if (Settings.Read(TEXT("CaptureCommentFormat"),&CommentFormat))
		m_CommentFormat=TVTest::StringUtility::Decode(CommentFormat);
	Settings.Read(TEXT("JpegQuality"),&m_JPEGQuality);
	Settings.Read(TEXT("PngCompressionLevel"),&m_PNGCompressionLevel);
	int Size;
	if (Settings.Read(TEXT("CaptureSizeType"),&Size)
			&& Size>=0 && Size<=SIZE_TYPE_LAST) {
		if (Size==SIZE_TYPE_RAW)
			m_CaptureSizeType=SIZE_TYPE_ORIGINAL;
		else
			m_CaptureSizeType=Size;
	}
	int Width,Height;
	if (Settings.Read(TEXT("CaptureWidth"),&Width)
			&& Settings.Read(TEXT("CaptureHeight"),&Height)) {
		for (int i=0;i<=SIZE_LAST;i++) {
			if (m_SizeList[i].cx==Width && m_SizeList[i].cy==Height) {
				m_CaptureSize=i;
				break;
			}
		}
	}
	int Num,Denom;
	if (Settings.Read(TEXT("CaptureRatioNum"),&Num)
			&& Settings.Read(TEXT("CaptureRatioDenom"),&Denom)) {
		for (int i=0;i<=PERCENTAGE_LAST;i++) {
			if (m_PercentageList[i].Num==Num
					&& m_PercentageList[i].Denom==Denom) {
				m_CapturePercentage=i;
				break;
			}
		}
	}
	return true;
}


bool CCaptureOptions::WriteSettings(CSettings &Settings)
{
	Settings.Write(TEXT("CaptureFolder"),m_szSaveFolder);
	Settings.Write(TEXT("CaptureFileNameFormat"),m_FileName);
	Settings.Write(TEXT("CaptureSaveFormat"),m_ImageCodec.EnumSaveFormat(m_SaveFormat));
	Settings.Write(TEXT("CaptureIconSaveFile"),m_fCaptureSaveToFile);
	Settings.Write(TEXT("CaptureSetComment"),m_fSetComment);
	Settings.Write(TEXT("CaptureCommentFormat"),TVTest::StringUtility::Encode(m_CommentFormat));
	Settings.Write(TEXT("JpegQuality"),m_JPEGQuality);
	Settings.Write(TEXT("PngCompressionLevel"),m_PNGCompressionLevel);
	Settings.Write(TEXT("CaptureSizeType"),m_CaptureSizeType);
	Settings.Write(TEXT("CaptureWidth"),m_SizeList[m_CaptureSize].cx);
	Settings.Write(TEXT("CaptureHeight"),m_SizeList[m_CaptureSize].cy);
	Settings.Write(TEXT("CaptureRatioNum"),m_PercentageList[m_CapturePercentage].Num);
	Settings.Write(TEXT("CaptureRatioDenom"),m_PercentageList[m_CapturePercentage].Denom);
	return true;
}


bool CCaptureOptions::Create(HWND hwndOwner)
{
	return CreateDialogWindow(hwndOwner,
							  GetAppClass().GetResourceInstance(),MAKEINTRESOURCE(IDD_OPTIONS_CAPTURE));
}


bool CCaptureOptions::SetPresetCaptureSize(int Size)
{
	if (Size<0)
		return false;
	if (Size<=SIZE_TYPE_VIEW) {
		m_CaptureSizeType=Size;
	} else if (Size-2<=PERCENTAGE_LAST) {
		m_CaptureSizeType=SIZE_TYPE_PERCENTAGE;
		m_CapturePercentage=Size-2;
	} else if (Size-(2+PERCENTAGE_LAST+1)<=SIZE_LAST) {
		m_CaptureSizeType=SIZE_TYPE_CUSTOM;
		m_CaptureSize=Size-(2+PERCENTAGE_LAST+1);
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
		Size=m_CaptureSizeType;
		break;
	case SIZE_TYPE_CUSTOM:
		Size=2+(PERCENTAGE_LAST+1)+m_CaptureSize;
		break;
	case SIZE_TYPE_PERCENTAGE:
		Size=2+m_CapturePercentage;
		break;
	}
	return Size;
}


bool CCaptureOptions::GetSizePercentage(int *pNum,int *pDenom) const
{
	if (pNum)
		*pNum=m_PercentageList[m_CapturePercentage].Num;
	if (pDenom)
		*pDenom=m_PercentageList[m_CapturePercentage].Denom;
	return true;
}


bool CCaptureOptions::GetCustomSize(int *pWidth,int *pHeight) const
{
	if (pWidth)
		*pWidth=m_SizeList[m_CaptureSize].cx;
	if (pHeight)
		*pHeight=m_SizeList[m_CaptureSize].cy;
	return true;
}


bool CCaptureOptions::GenerateFileName(
	TVTest::String *pFileName,const CCaptureImage *pImage) const
{
	if (pFileName==NULL)
		return false;

	TCHAR szSaveFolder[MAX_PATH];

	if (m_szSaveFolder[0]!='\0') {
		if (!GetAbsolutePath(m_szSaveFolder,szSaveFolder,lengthof(szSaveFolder)))
			return false;
	} else {
		if (!GetAppClass().GetAppDirectory(szSaveFolder))
			return false;
	}

	TVTest::CEventVariableStringMap::EventInfo EventInfo;
	GetAppClass().Core.GetVariableStringEventInfo(&EventInfo);
	TVTest::CCaptureVariableStringMap VarStrMap(EventInfo,pImage);
	TVTest::String FileName;
	TCHAR szPath[MAX_PATH];

	VarStrMap.SetCurrentTime(&pImage->GetCaptureTime());

	if (!TVTest::FormatVariableString(&VarStrMap,m_FileName.c_str(),&FileName)
			|| FileName.empty())
		return false;
	if (::lstrlen(szSaveFolder)+1+(int)FileName.length()>=MAX_PATH)
		return false;
	::PathCombine(szPath,szSaveFolder,FileName.c_str());
	::lstrcpy(szSaveFolder,szPath);
	::PathRemoveFileSpec(szSaveFolder);
	if (!::PathIsDirectory(szSaveFolder)) {
		int Result=::SHCreateDirectoryEx(NULL,szSaveFolder,NULL);
		if (Result!=ERROR_SUCCESS && Result!=ERROR_ALREADY_EXISTS) {
			GetAppClass().AddLog(
				CLogItem::TYPE_ERROR,
				TEXT("キャプチャの保存先フォルダ \"%s\" を作成できません。"),
				szSaveFolder);
			return false;
		}
	}

	FileName=szPath;
	FileName+=_T('.');
	FileName+=m_ImageCodec.GetExtension(m_SaveFormat);
	if (!MakeUniqueFileName(&FileName))
		return false;
	*pFileName=FileName;

	return true;
}


bool CCaptureOptions::GetOptionText(LPTSTR pszOption,int MaxLength) const
{
	LPCTSTR pszFormatName=m_ImageCodec.EnumSaveFormat(m_SaveFormat);

	if (::lstrcmpi(pszFormatName,TEXT("JPEG"))==0) {
		if (MaxLength<4)
			return false;
		::wsprintf(pszOption,TEXT("%d"),m_JPEGQuality);
	} else if (::lstrcmpi(pszFormatName,TEXT("PNG"))==0) {
		if (MaxLength<2)
			return false;
		::wsprintf(pszOption,TEXT("%d"),m_PNGCompressionLevel);
	} else {
		if (MaxLength<1)
			return false;
		pszOption[0]='\0';
	}
	return true;
}


bool CCaptureOptions::GetCommentText(
	TVTest::String *pComment,const CCaptureImage *pImage) const
{
	if (pComment==NULL || pImage==NULL)
		return false;

	TVTest::CEventVariableStringMap::EventInfo EventInfo;
	GetAppClass().Core.GetVariableStringEventInfo(&EventInfo);
	TVTest::CCaptureVariableStringMap VarStrMap(EventInfo,pImage);

	VarStrMap.SetCurrentTime(&pImage->GetCaptureTime());

	return TVTest::FormatVariableString(&VarStrMap,m_CommentFormat.c_str(),pComment);
}


bool CCaptureOptions::SaveImage(CCaptureImage *pImage)
{
	TVTest::String FileName;
	TCHAR szOption[16];
	BITMAPINFO *pbmi;
	BYTE *pBits;
	bool fOK;

	if (!GenerateFileName(&FileName,pImage))
		return false;
	GetOptionText(szOption,lengthof(szOption));
	if (!pImage->LockData(&pbmi,&pBits))
		return false;
	fOK=m_ImageCodec.SaveImage(FileName.c_str(),m_SaveFormat,szOption,
						pbmi,pBits,m_fSetComment?pImage->GetComment():NULL);
	pImage->UnlockData();
	return fOK;
}


int CCaptureOptions::TranslateCommand(int Command)
{
	if (Command==CM_CAPTURE)
		return m_fCaptureSaveToFile?CM_SAVEIMAGE:CM_COPY;
	return -1;
}


bool CCaptureOptions::OpenSaveFolder() const
{
	TCHAR szFolder[MAX_PATH];

	if (m_szSaveFolder[0]!='\0') {
		if (!GetAbsolutePath(m_szSaveFolder,szFolder,lengthof(szFolder)))
			return false;
	} else {
		if (!GetAppClass().GetAppDirectory(szFolder))
			return false;
	}
	return (ULONG_PTR)::ShellExecute(NULL,TEXT("open"),szFolder,NULL,NULL,SW_SHOWNORMAL)>32;
}


INT_PTR CCaptureOptions::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			int i;

			SendDlgItemMessage(hDlg,IDC_CAPTUREOPTIONS_SAVEFOLDER,EM_LIMITTEXT,MAX_PATH-1,0);
			SetDlgItemText(hDlg,IDC_CAPTUREOPTIONS_SAVEFOLDER,m_szSaveFolder);
			SetDlgItemText(hDlg,IDC_CAPTUREOPTIONS_FILENAME,m_FileName.c_str());
			InitDropDownButton(hDlg,IDC_CAPTUREOPTIONS_FILENAME_PARAMETERS);

			static const LPCTSTR SizeTypeText[] = {
				TEXT("元の大きさ"),
				TEXT("表示されている大きさ"),
			};
			TCHAR szText[32];
			for (i=0;i<lengthof(SizeTypeText);i++)
				DlgComboBox_AddString(hDlg,IDC_CAPTUREOPTIONS_SIZE,SizeTypeText[i]);
			for (i=0;i<=PERCENTAGE_LAST;i++) {
				const PercentageType &Ratio=m_PercentageList[i];
				int Length=StdUtil::snprintf(
					szText,lengthof(szText),TEXT("%d %%"),
					::MulDiv(Ratio.Num,100,Ratio.Denom));
				if (Ratio.Num*100%Ratio.Denom!=0) {
					StdUtil::snprintf(
						szText+Length,lengthof(szText)-Length,TEXT(" (%d/%d)"),
						Ratio.Num,Ratio.Denom);
				}
				DlgComboBox_AddString(hDlg,IDC_CAPTUREOPTIONS_SIZE,szText);
			}
			for (i=0;i<=SIZE_LAST;i++) {
				StdUtil::snprintf(szText,lengthof(szText),TEXT("%ld x %ld"),
								  m_SizeList[i].cx,m_SizeList[i].cy);
				DlgComboBox_AddString(hDlg,IDC_CAPTUREOPTIONS_SIZE,szText);
			}
			int Sel=-1;
			switch (m_CaptureSizeType) {
			case SIZE_TYPE_ORIGINAL:
			case SIZE_TYPE_VIEW:
				Sel=m_CaptureSizeType;
				break;
			case SIZE_TYPE_CUSTOM:
				Sel=lengthof(SizeTypeText)+(PERCENTAGE_LAST+1)+m_CaptureSize;
				break;
			case SIZE_TYPE_PERCENTAGE:
				Sel=lengthof(SizeTypeText)+m_CapturePercentage;
				break;
			}
			DlgComboBox_SetCurSel(hDlg,IDC_CAPTUREOPTIONS_SIZE,Sel);

			LPCTSTR pszFormat;
			for (i=0;(pszFormat=m_ImageCodec.EnumSaveFormat(i))!=NULL;i++)
				DlgComboBox_AddString(hDlg,IDC_CAPTUREOPTIONS_FORMAT,pszFormat);
			DlgComboBox_SetCurSel(hDlg,IDC_CAPTUREOPTIONS_FORMAT,m_SaveFormat);

			// JPEG quality
			SendDlgItemMessage(hDlg,IDC_CAPTUREOPTIONS_JPEGQUALITY_TB,
								TBM_SETRANGE,TRUE,MAKELPARAM(0,100));
			SendDlgItemMessage(hDlg,IDC_CAPTUREOPTIONS_JPEGQUALITY_TB,
								TBM_SETPOS,TRUE,m_JPEGQuality);
			SendDlgItemMessage(hDlg,IDC_CAPTUREOPTIONS_JPEGQUALITY_TB,
								TBM_SETPAGESIZE,0,10);
			SendDlgItemMessage(hDlg,IDC_CAPTUREOPTIONS_JPEGQUALITY_TB,
								TBM_SETTICFREQ,10,0);
			SendDlgItemMessage(hDlg,IDC_CAPTUREOPTIONS_JPEGQUALITY_EDIT,
								EM_LIMITTEXT,3,0);
			SetDlgItemInt(hDlg,IDC_CAPTUREOPTIONS_JPEGQUALITY_EDIT,m_JPEGQuality,TRUE);
			DlgUpDown_SetRange(hDlg,IDC_CAPTUREOPTIONS_JPEGQUALITY_UD,0,100);

			// PNG compression level
			SendDlgItemMessage(hDlg,IDC_CAPTUREOPTIONS_PNGLEVEL_TB,
								TBM_SETRANGE,TRUE,MAKELPARAM(0,9));
			SendDlgItemMessage(hDlg,IDC_CAPTUREOPTIONS_PNGLEVEL_TB,
								TBM_SETPOS,TRUE,m_PNGCompressionLevel);
			SendDlgItemMessage(hDlg,IDC_CAPTUREOPTIONS_PNGLEVEL_TB,
								TBM_SETPAGESIZE,0,1);
			SendDlgItemMessage(hDlg,IDC_CAPTUREOPTIONS_PNGLEVEL_TB,
								TBM_SETTICFREQ,1,0);
			SendDlgItemMessage(hDlg,IDC_CAPTUREOPTIONS_PNGLEVEL_EDIT,
								EM_LIMITTEXT,0,1);
			SetDlgItemInt(hDlg,IDC_CAPTUREOPTIONS_PNGLEVEL_EDIT,m_PNGCompressionLevel,FALSE);
			DlgUpDown_SetRange(hDlg,IDC_CAPTUREOPTIONS_PNGLEVEL_UD,0,9);

			DlgCheckBox_Check(hDlg,IDC_CAPTUREOPTIONS_ICONSAVEFILE,m_fCaptureSaveToFile);
			DlgCheckBox_Check(hDlg,IDC_CAPTUREOPTIONS_SETCOMMENT,m_fSetComment);
			::SetDlgItemText(hDlg,IDC_CAPTUREOPTIONS_COMMENT,m_CommentFormat.c_str());
			InitDropDownButton(hDlg,IDC_CAPTUREOPTIONS_COMMENT_PARAMETERS);
			EnableDlgItems(hDlg,
						   IDC_CAPTUREOPTIONS_COMMENT,
						   IDC_CAPTUREOPTIONS_COMMENT_PARAMETERS,
						   m_fSetComment);
		}
		return TRUE;

	case WM_HSCROLL:
		if (reinterpret_cast<HWND>(lParam)==
						GetDlgItem(hDlg,IDC_CAPTUREOPTIONS_JPEGQUALITY_TB)) {
			SyncEditWithTrackBar(hDlg,IDC_CAPTUREOPTIONS_JPEGQUALITY_TB,
									  IDC_CAPTUREOPTIONS_JPEGQUALITY_EDIT);
		} else if (reinterpret_cast<HWND>(lParam)==
						GetDlgItem(hDlg,IDC_CAPTUREOPTIONS_PNGLEVEL_TB)) {
			SyncEditWithTrackBar(hDlg,IDC_CAPTUREOPTIONS_PNGLEVEL_TB,
									  IDC_CAPTUREOPTIONS_PNGLEVEL_EDIT);
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_CAPTUREOPTIONS_SAVEFOLDER_BROWSE:
			{
				TCHAR szFolder[MAX_PATH];

				GetDlgItemText(hDlg,IDC_CAPTUREOPTIONS_SAVEFOLDER,szFolder,lengthof(szFolder));
				if (BrowseFolderDialog(hDlg,szFolder,TEXT("画像の保存先フォルダ:")))
					SetDlgItemText(hDlg,IDC_CAPTUREOPTIONS_SAVEFOLDER,szFolder);
			}
			return TRUE;

		case IDC_CAPTUREOPTIONS_FILENAME:
			if (HIWORD(wParam)==EN_CHANGE)
				UpdateFileNamePreview();
			return TRUE;

		case IDC_CAPTUREOPTIONS_FILENAME_PARAMETERS:
			{
				RECT rc;
				POINT pt;

				::GetWindowRect(::GetDlgItem(hDlg,IDC_CAPTUREOPTIONS_FILENAME_PARAMETERS),&rc);
				pt.x=rc.left;
				pt.y=rc.bottom;
				TVTest::CCaptureVariableStringMap VarStrMap;
				VarStrMap.InputParameter(hDlg,IDC_CAPTUREOPTIONS_FILENAME,pt);
			}
			return TRUE;

		case IDC_CAPTUREOPTIONS_FORMAT:
			if (HIWORD(wParam)==CBN_SELCHANGE)
				UpdateFileNamePreview();
			return TRUE;

		case IDC_CAPTUREOPTIONS_JPEGQUALITY_EDIT:
			if (HIWORD(wParam)==EN_CHANGE)
				SyncTrackBarWithEdit(hDlg,IDC_CAPTUREOPTIONS_JPEGQUALITY_EDIT,
										  IDC_CAPTUREOPTIONS_JPEGQUALITY_TB);
			return TRUE;

		case IDC_CAPTUREOPTIONS_PNGLEVEL_EDIT:
			if (HIWORD(wParam)==EN_CHANGE)
				SyncTrackBarWithEdit(hDlg,IDC_CAPTUREOPTIONS_PNGLEVEL_EDIT,
										  IDC_CAPTUREOPTIONS_PNGLEVEL_TB);
			return TRUE;

		case IDC_CAPTUREOPTIONS_SETCOMMENT:
			EnableDlgItemsSyncCheckBox(hDlg,
				IDC_CAPTUREOPTIONS_COMMENT,
				IDC_CAPTUREOPTIONS_COMMENT_PARAMETERS,
				IDC_CAPTUREOPTIONS_SETCOMMENT);
			return TRUE;

		case IDC_CAPTUREOPTIONS_COMMENT_PARAMETERS:
			{
				RECT rc;
				POINT pt;

				::GetWindowRect(::GetDlgItem(hDlg,IDC_CAPTUREOPTIONS_COMMENT_PARAMETERS),&rc);
				pt.x=rc.left;
				pt.y=rc.bottom;
				TVTest::CCaptureVariableStringMap VarStrMap;
				VarStrMap.InputParameter(hDlg,IDC_CAPTUREOPTIONS_COMMENT,pt);
			}
			return TRUE;
		}
		return TRUE;

	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code) {
		case PSN_APPLY:
			{
				TCHAR szSaveFolder[MAX_PATH];

				GetDlgItemText(hDlg,IDC_CAPTUREOPTIONS_SAVEFOLDER,szSaveFolder,lengthof(szSaveFolder));
				CAppMain::CreateDirectoryResult CreateDirResult=
					GetAppClass().CreateDirectory(
						hDlg,szSaveFolder,
						TEXT("キャプチャ画像の保存先フォルダ \"%s\" がありません。\n")
						TEXT("作成しますか?"));
				if (CreateDirResult==CAppMain::CREATEDIRECTORY_RESULT_ERROR) {
					SettingError();
					SetDlgItemFocus(hDlg,IDC_CAPTUREOPTIONS_SAVEFOLDER);
					return TRUE;
				}

				TVTest::String FileName,Message;
				GetDlgItemString(hDlg,IDC_CAPTUREOPTIONS_FILENAME,&FileName);
				if (!IsValidFileName(FileName.c_str(),FILENAME_VALIDATE_ALLOWDELIMITER,&Message)) {
					SettingError();
					::SendDlgItemMessage(hDlg,IDC_CAPTUREOPTIONS_FILENAME,EM_SETSEL,0,-1);
					::MessageBox(hDlg,Message.c_str(),NULL,MB_OK | MB_ICONEXCLAMATION);
					SetDlgItemFocus(hDlg,IDC_CAPTUREOPTIONS_FILENAME);
					return TRUE;
				}

				lstrcpy(m_szSaveFolder,szSaveFolder);
				m_FileName=FileName;

				SetPresetCaptureSize(
					(int)DlgComboBox_GetCurSel(hDlg,IDC_CAPTUREOPTIONS_SIZE));
				m_SaveFormat=
					(int)DlgComboBox_GetCurSel(hDlg,IDC_CAPTUREOPTIONS_FORMAT);
				m_JPEGQuality=
					(int)SendDlgItemMessage(hDlg,IDC_CAPTUREOPTIONS_JPEGQUALITY_TB,TBM_GETPOS,0,0);
				m_PNGCompressionLevel=
					(int)SendDlgItemMessage(hDlg,IDC_CAPTUREOPTIONS_PNGLEVEL_TB,TBM_GETPOS,0,0);
				m_fCaptureSaveToFile=
					DlgCheckBox_IsChecked(hDlg,IDC_CAPTUREOPTIONS_ICONSAVEFILE);
				m_fSetComment=
					DlgCheckBox_IsChecked(hDlg,IDC_CAPTUREOPTIONS_SETCOMMENT);
				GetDlgItemString(hDlg,IDC_CAPTUREOPTIONS_COMMENT,&m_CommentFormat);

				m_fChanged=true;
			}
			break;
		}
		break;
	}

	return FALSE;
}


void CCaptureOptions::UpdateFileNamePreview()
{
	TVTest::String Format;
	TVTest::String FileName;

	GetDlgItemString(m_hDlg,IDC_CAPTUREOPTIONS_FILENAME,&Format);
	if (!Format.empty()) {
		TVTest::CCaptureVariableStringMap VarStrMap;
		VarStrMap.SetSampleEventInfo();
		TVTest::FormatVariableString(&VarStrMap,Format.c_str(),&FileName);
		if (!FileName.empty()) {
			LPCTSTR pszExtension=m_ImageCodec.GetExtension(
				(int)DlgComboBox_GetCurSel(m_hDlg,IDC_CAPTUREOPTIONS_FORMAT));
			if (pszExtension!=nullptr) {
				FileName+=_T('.');
				FileName+=pszExtension;
			}
		}
	}
	::SetDlgItemText(m_hDlg,IDC_CAPTUREOPTIONS_FILENAME_PREVIEW,FileName.c_str());
}
