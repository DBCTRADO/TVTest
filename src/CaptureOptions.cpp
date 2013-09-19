#include "stdafx.h"
#include "TVTest.h"
#include "AppMain.h"
#include "CaptureOptions.h"
#include "DialogUtil.h"
#include "resource.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif




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
	: m_SaveFormat(0)
	, m_JPEGQuality(90)
	, m_PNGCompressionLevel(6)
	, m_fCaptureSaveToFile(true)
	, m_fSetComment(false)
	, m_CaptureSizeType(SIZE_TYPE_ORIGINAL)
	, m_CaptureSize(
#ifndef TVH264_FOR_1SEG
		SIZE_1920x1080
#else
		SIZE_320x180
#endif
		)
	, m_CapturePercentage(PERCENTAGE_50)
{
	GetAppClass().GetAppDirectory(m_szSaveFolder);
	::lstrcpy(m_szFileName,TEXT("Capture"));
}


CCaptureOptions::~CCaptureOptions()
{
	Destroy();
}


bool CCaptureOptions::ReadSettings(CSettings &Settings)
{
	Settings.Read(TEXT("CaptureFolder"),m_szSaveFolder,lengthof(m_szSaveFolder));
	Settings.Read(TEXT("CaptureFileName"),m_szFileName,lengthof(m_szFileName));
	TCHAR szFormat[32];
	if (Settings.Read(TEXT("CaptureSaveFormat"),szFormat,lengthof(szFormat))) {
		int Format=m_ImageCodec.FormatNameToIndex(szFormat);
		if (Format>=0)
			m_SaveFormat=Format;
	}
	Settings.Read(TEXT("CaptureIconSaveFile"),&m_fCaptureSaveToFile);
	Settings.Read(TEXT("CaptureSetComment"),&m_fSetComment);
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
	Settings.Write(TEXT("CaptureFileName"),m_szFileName);
	Settings.Write(TEXT("CaptureSaveFormat"),m_ImageCodec.EnumSaveFormat(m_SaveFormat));
	Settings.Write(TEXT("CaptureIconSaveFile"),m_fCaptureSaveToFile);
	Settings.Write(TEXT("CaptureSetComment"),m_fSetComment);
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


bool CCaptureOptions::GenerateFileName(LPTSTR pszFileName,int MaxLength,const SYSTEMTIME *pst) const
{
	TCHAR szSaveFolder[MAX_PATH];
	SYSTEMTIME st;

	if (m_szSaveFolder[0]!='\0') {
		::lstrcpy(szSaveFolder,m_szSaveFolder);
	} else {
		if (!GetAppClass().GetAppDirectory(szSaveFolder))
			return false;
	}
	if (::lstrlen(szSaveFolder)+1+::lstrlen(m_szFileName)>=MaxLength)
		return false;
	::PathCombine(pszFileName,szSaveFolder,m_szFileName);
	if (pst==NULL) {
		::GetLocalTime(&st);
	} else {
		st=*pst;
	}
	int Length=::lstrlen(pszFileName);
	Length+=StdUtil::snprintf(pszFileName+Length,MaxLength-Length,
							  TEXT("%04d%02d%02d-%02d%02d%02d"),
							  st.wYear,st.wMonth,st.wDay,st.wHour,st.wMinute,st.wSecond);
	LPCTSTR pszExtension=m_ImageCodec.GetExtension(m_SaveFormat);
	StdUtil::snprintf(pszFileName+Length,MaxLength-Length,TEXT(".%s"),pszExtension);
	if (::PathFileExists(pszFileName)) {
		for (int i=0;;i++) {
			StdUtil::snprintf(pszFileName+Length,MaxLength-Length,
							  TEXT("-%d.%s"),i+1,pszExtension);
			if (!::PathFileExists(pszFileName))
				break;
		}
	}
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


bool CCaptureOptions::GetCommentText(LPTSTR pszComment,int MaxComment,
									LPCTSTR pszChannelName,LPCTSTR pszEventName)
{
	SYSTEMTIME st;
	TCHAR szDate[64],szTime[64];

	::GetLocalTime(&st);
	::GetDateFormat(LOCALE_USER_DEFAULT,DATE_SHORTDATE,&st,NULL,szDate,lengthof(szDate));
	::GetTimeFormat(LOCALE_USER_DEFAULT,TIME_FORCE24HOURFORMAT,&st,NULL,szTime,lengthof(szTime));
	int Length=StdUtil::snprintf(pszComment,MaxComment,TEXT("%s %s"),szDate,szTime);
	if (!IsStringEmpty(pszChannelName))
		Length+=StdUtil::snprintf(pszComment+Length,MaxComment-Length,TEXT(" %s"),pszChannelName);
	if (!IsStringEmpty(pszEventName))
		Length+=StdUtil::snprintf(pszComment+Length,MaxComment-Length,TEXT("\r\n%s"),pszEventName);
	return true;
}


bool CCaptureOptions::SaveImage(CCaptureImage *pImage)
{
	TCHAR szFileName[MAX_PATH],szOption[16];
	BITMAPINFO *pbmi;
	BYTE *pBits;
	bool fOK;

	GenerateFileName(szFileName,lengthof(szFileName),&pImage->GetCaptureTime());
	GetOptionText(szOption,lengthof(szOption));
	if (!pImage->LockData(&pbmi,&pBits))
		return false;
	fOK=m_ImageCodec.SaveImage(szFileName,m_SaveFormat,szOption,
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
		::lstrcpy(szFolder,m_szSaveFolder);
	} else {
		::GetModuleFileName(NULL,szFolder,lengthof(szFolder));
		*(::PathFindFileName(szFolder)-1)='\0';
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
			SendDlgItemMessage(hDlg,IDC_CAPTUREOPTIONS_FILENAME,EM_LIMITTEXT,MAX_PATH-1,0);
			SetDlgItemText(hDlg,IDC_CAPTUREOPTIONS_FILENAME,m_szFileName);

			static const LPCTSTR SizeTypeText[] = {
				TEXT("元の大きさ"),
				TEXT("表示されている大きさ"),
			};
			TCHAR szText[32];
			for (i=0;i<lengthof(SizeTypeText);i++)
				DlgComboBox_AddString(hDlg,IDC_CAPTUREOPTIONS_SIZE,SizeTypeText[i]);
			for (i=0;i<=PERCENTAGE_LAST;i++) {
				StdUtil::snprintf(szText,lengthof(szText),TEXT("%d %%"),
								  m_PercentageList[i].Num*100/m_PercentageList[i].Denom);
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
		}
		return TRUE;

	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code) {
		case PSN_APPLY:
			{
				TCHAR szSaveFolder[MAX_PATH],szFileName[MAX_PATH],szMessage[256];

				GetDlgItemText(hDlg,IDC_CAPTUREOPTIONS_SAVEFOLDER,szSaveFolder,lengthof(szSaveFolder));
				if (szSaveFolder[0]!='\0' && !::PathIsDirectory(szSaveFolder)) {
					TCHAR szMessage[MAX_PATH+80];

					StdUtil::snprintf(szMessage,lengthof(szMessage),
						TEXT("キャプチャ画像の保存先フォルダ \"%s\" がありません。\n")
						TEXT("作成しますか?"),szSaveFolder);
					if (::MessageBox(hDlg,szMessage,TEXT("フォルダ作成の確認"),
									 MB_YESNO | MB_ICONQUESTION)==IDYES) {
						int Result;

						Result=::SHCreateDirectoryEx(hDlg,szSaveFolder,NULL);
						if (Result!=ERROR_SUCCESS
								&& Result!=ERROR_ALREADY_EXISTS) {
							SettingError();
							StdUtil::snprintf(szMessage,lengthof(szMessage),
								TEXT("フォルダ \"%s\" を作成できません。"),szSaveFolder);
							::MessageBox(hDlg,szMessage,
										 NULL,MB_OK | MB_ICONEXCLAMATION);
							SetDlgItemFocus(hDlg,IDC_CAPTUREOPTIONS_SAVEFOLDER);
							return TRUE;
						}
					}
				}
				GetDlgItemText(hDlg,IDC_CAPTUREOPTIONS_FILENAME,szFileName,lengthof(szFileName));
				if (!IsValidFileName(szFileName,false,szMessage,lengthof(szMessage))) {
					SettingError();
					SetDlgItemFocus(hDlg,IDC_CAPTUREOPTIONS_FILENAME);
					SendDlgItemMessage(hDlg,IDC_CAPTUREOPTIONS_FILENAME,EM_SETSEL,0,-1);
					MessageBox(hDlg,szMessage,NULL,MB_OK | MB_ICONEXCLAMATION);
					return TRUE;
				}
				lstrcpy(m_szSaveFolder,szSaveFolder);
				lstrcpy(m_szFileName,szFileName);
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

				m_fChanged=true;
			}
			break;
		}
		break;
	}

	return FALSE;
}
