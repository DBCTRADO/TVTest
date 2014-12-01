#include "stdafx.h"
#include "TVTest.h"
#include "AppMain.h"
#include "GeneralOptions.h"
#include "DriverManager.h"
#include "DialogUtil.h"
#include "MessageDialog.h"
#include "resource.h"
#include <algorithm>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif




CGeneralOptions::CGeneralOptions()
	: m_DefaultDriverType(DEFAULT_DRIVER_LAST)
	, m_VideoRendererType(CVideoRenderer::RENDERER_DEFAULT)
	, m_fResident(false)
	, m_fKeepSingleTask(false)
{
}


CGeneralOptions::~CGeneralOptions()
{
	Destroy();
}


bool CGeneralOptions::Apply(DWORD Flags)
{
	CAppMain &App=GetAppClass();

	if ((Flags&UPDATE_RESIDENT)!=0) {
		App.UICore.SetResident(m_fResident);
	}

	return true;
}


bool CGeneralOptions::ReadSettings(CSettings &Settings)
{
	int Value;

	TCHAR szDirectory[MAX_PATH];
	if (Settings.Read(TEXT("DriverDirectory"),szDirectory,lengthof(szDirectory))
			&& szDirectory[0]!='\0') {
		m_BonDriverDirectory=szDirectory;
		GetAppClass().CoreEngine.SetDriverDirectory(szDirectory);
	}

	if (Settings.Read(TEXT("DefaultDriverType"),&Value)
			&& Value>=DEFAULT_DRIVER_NONE && Value<=DEFAULT_DRIVER_CUSTOM)
		m_DefaultDriverType=(DefaultDriverType)Value;
	Settings.Read(TEXT("DefaultDriver"),&m_DefaultBonDriverName);
	Settings.Read(TEXT("Driver"),&m_LastBonDriverName);

	Settings.Read(TEXT("Mpeg2Decoder"),&m_Mpeg2DecoderName);
	Settings.Read(TEXT("H264Decoder"),&m_H264DecoderName);
	Settings.Read(TEXT("H265Decoder"),&m_H265DecoderName);

	TCHAR szRenderer[16];
	if (Settings.Read(TEXT("Renderer"),szRenderer,lengthof(szRenderer))) {
		if (szRenderer[0]=='\0') {
			m_VideoRendererType=CVideoRenderer::RENDERER_DEFAULT;
		} else {
			CVideoRenderer::RendererType Renderer=CVideoRenderer::ParseName(szRenderer);
			if (Renderer!=CVideoRenderer::RENDERER_UNDEFINED)
				m_VideoRendererType=Renderer;
		}
	}

	Settings.Read(TEXT("Resident"),&m_fResident);
	Settings.Read(TEXT("KeepSingleTask"),&m_fKeepSingleTask);
	return true;
}


bool CGeneralOptions::WriteSettings(CSettings &Settings)
{
	Settings.Write(TEXT("DriverDirectory"),m_BonDriverDirectory);
	Settings.Write(TEXT("DefaultDriverType"),(int)m_DefaultDriverType);
	Settings.Write(TEXT("DefaultDriver"),m_DefaultBonDriverName);
	Settings.Write(TEXT("Driver"),GetAppClass().CoreEngine.GetDriverFileName());
	Settings.Write(TEXT("Mpeg2Decoder"),m_Mpeg2DecoderName);
	Settings.Write(TEXT("H264Decoder"),m_H264DecoderName);
	Settings.Write(TEXT("H265Decoder"),m_H265DecoderName);
	Settings.Write(TEXT("Renderer"),
				   CVideoRenderer::EnumRendererName((int)m_VideoRendererType));
	Settings.Write(TEXT("Resident"),m_fResident);
	Settings.Write(TEXT("KeepSingleTask"),m_fKeepSingleTask);
	return true;
}


bool CGeneralOptions::Create(HWND hwndOwner)
{
	return CreateDialogWindow(hwndOwner,
							  GetAppClass().GetResourceInstance(),MAKEINTRESOURCE(IDD_OPTIONS_GENERAL));
}


CGeneralOptions::DefaultDriverType CGeneralOptions::GetDefaultDriverType() const
{
	return m_DefaultDriverType;
}


LPCTSTR CGeneralOptions::GetDefaultDriverName() const
{
	return m_DefaultBonDriverName.c_str();
}


bool CGeneralOptions::SetDefaultDriverName(LPCTSTR pszDriverName)
{
	if (pszDriverName==NULL)
		m_DefaultBonDriverName.clear();
	else
		m_DefaultBonDriverName=pszDriverName;
	return true;
}


bool CGeneralOptions::GetFirstDriverName(LPTSTR pszDriverName) const
{
	switch (m_DefaultDriverType) {
	case DEFAULT_DRIVER_NONE:
		pszDriverName[0]='\0';
		break;
	case DEFAULT_DRIVER_LAST:
		if (m_LastBonDriverName.length()>=MAX_PATH)
			return false;
		::lstrcpy(pszDriverName,m_LastBonDriverName.c_str());
		break;
	case DEFAULT_DRIVER_CUSTOM:
		if (m_DefaultBonDriverName.length()>=MAX_PATH)
			return false;
		::lstrcpy(pszDriverName,m_DefaultBonDriverName.c_str());
		break;
	default:
		return false;
	}
	return true;
}


LPCTSTR CGeneralOptions::GetMpeg2DecoderName() const
{
	return m_Mpeg2DecoderName.c_str();
}


bool CGeneralOptions::SetMpeg2DecoderName(LPCTSTR pszDecoderName)
{
	if (pszDecoderName==NULL)
		m_Mpeg2DecoderName.clear();
	else
		m_Mpeg2DecoderName=pszDecoderName;
	return true;
}


LPCTSTR CGeneralOptions::GetH264DecoderName() const
{
	return m_H264DecoderName.c_str();
}


bool CGeneralOptions::SetH264DecoderName(LPCTSTR pszDecoderName)
{
	if (pszDecoderName==NULL)
		m_H264DecoderName.clear();
	else
		m_H264DecoderName=pszDecoderName;
	return true;
}


LPCTSTR CGeneralOptions::GetH265DecoderName() const
{
	return m_H265DecoderName.c_str();
}


bool CGeneralOptions::SetH265DecoderName(LPCTSTR pszDecoderName)
{
	if (pszDecoderName==NULL)
		m_H265DecoderName.clear();
	else
		m_H265DecoderName=pszDecoderName;
	return true;
}


CVideoRenderer::RendererType CGeneralOptions::GetVideoRendererType() const
{
	return m_VideoRendererType;
}


bool CGeneralOptions::SetVideoRendererType(CVideoRenderer::RendererType Renderer)
{
	if (CVideoRenderer::EnumRendererName((int)Renderer)==NULL)
		return false;
	m_VideoRendererType=Renderer;
	return true;
}


bool CGeneralOptions::GetResident() const
{
	return m_fResident;
}


bool CGeneralOptions::GetKeepSingleTask() const
{
	return m_fKeepSingleTask;
}


INT_PTR CGeneralOptions::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			CAppMain &App=GetAppClass();

			::SendDlgItemMessage(hDlg,IDC_OPTIONS_DRIVERDIRECTORY,EM_LIMITTEXT,MAX_PATH-1,0);
			::SetDlgItemText(hDlg,IDC_OPTIONS_DRIVERDIRECTORY,m_BonDriverDirectory.c_str());

			// BonDriver
			::CheckRadioButton(hDlg,IDC_OPTIONS_DEFAULTDRIVER_NONE,
									IDC_OPTIONS_DEFAULTDRIVER_CUSTOM,
							   (int)m_DefaultDriverType+IDC_OPTIONS_DEFAULTDRIVER_NONE);
			EnableDlgItems(hDlg,IDC_OPTIONS_DEFAULTDRIVER,
								IDC_OPTIONS_DEFAULTDRIVER_BROWSE,
						   m_DefaultDriverType==DEFAULT_DRIVER_CUSTOM);

			const CDriverManager &DriverManager=App.DriverManager;
			DlgComboBox_LimitText(hDlg,IDC_OPTIONS_DEFAULTDRIVER,MAX_PATH-1);
			for (int i=0;i<DriverManager.NumDrivers();i++) {
				DlgComboBox_AddString(hDlg,IDC_OPTIONS_DEFAULTDRIVER,
									  DriverManager.GetDriverInfo(i)->GetFileName());
			}
			::SetDlgItemText(hDlg,IDC_OPTIONS_DEFAULTDRIVER,m_DefaultBonDriverName.c_str());

			// MPEG-2 デコーダ
			SetVideoDecoderList(IDC_OPTIONS_MPEG2DECODER,
								MEDIASUBTYPE_MPEG2_VIDEO,
								STREAM_TYPE_MPEG2_VIDEO,
								m_Mpeg2DecoderName);
			// H.264 デコーダ
			SetVideoDecoderList(IDC_OPTIONS_H264DECODER,
								MEDIASUBTYPE_H264,
								STREAM_TYPE_H264,
								m_H264DecoderName);
			// H.265 デコーダ
			SetVideoDecoderList(IDC_OPTIONS_H265DECODER,
								MEDIASUBTYPE_HEVC,
								STREAM_TYPE_H265,
								m_H265DecoderName);

			// 映像レンダラ
			LPCTSTR pszRenderer;
			DlgComboBox_AddString(hDlg,IDC_OPTIONS_RENDERER,TEXT("デフォルト"));
			for (int i=1;(pszRenderer=CVideoRenderer::EnumRendererName(i))!=NULL;i++)
				DlgComboBox_AddString(hDlg,IDC_OPTIONS_RENDERER,pszRenderer);
			DlgComboBox_SetCurSel(hDlg,IDC_OPTIONS_RENDERER,m_VideoRendererType);

			DlgCheckBox_Check(hDlg,IDC_OPTIONS_RESIDENT,m_fResident);
			DlgCheckBox_Check(hDlg,IDC_OPTIONS_KEEPSINGLETASK,m_fKeepSingleTask);
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_OPTIONS_DRIVERDIRECTORY_BROWSE:
			{
				TCHAR szDirectory[MAX_PATH];

				if (::GetDlgItemText(hDlg,IDC_OPTIONS_DRIVERDIRECTORY,szDirectory,lengthof(szDirectory))>0) {
					if (::PathIsRelative(szDirectory)) {
						TCHAR szTemp[MAX_PATH];

						GetAppClass().GetAppDirectory(szTemp);
						::PathAppend(szTemp,szDirectory);
						::PathCanonicalize(szDirectory,szTemp);
					}
				} else {
					GetAppClass().GetAppDirectory(szDirectory);
				}
				if (BrowseFolderDialog(hDlg,szDirectory,TEXT("BonDriver の検索フォルダを選択してください。")))
					::SetDlgItemText(hDlg,IDC_OPTIONS_DRIVERDIRECTORY,szDirectory);
			}
			return TRUE;

		case IDC_OPTIONS_DEFAULTDRIVER_NONE:
		case IDC_OPTIONS_DEFAULTDRIVER_LAST:
		case IDC_OPTIONS_DEFAULTDRIVER_CUSTOM:
			EnableDlgItemsSyncCheckBox(hDlg,IDC_OPTIONS_DEFAULTDRIVER,
									   IDC_OPTIONS_DEFAULTDRIVER_BROWSE,
									   IDC_OPTIONS_DEFAULTDRIVER_CUSTOM);
			return TRUE;

		case IDC_OPTIONS_DEFAULTDRIVER_BROWSE:
			{
				OPENFILENAME ofn;
				TCHAR szFileName[MAX_PATH],szInitDir[MAX_PATH];
				CFilePath FilePath;

				::GetDlgItemText(hDlg,IDC_OPTIONS_DEFAULTDRIVER,szFileName,lengthof(szFileName));
				FilePath.SetPath(szFileName);
				if (FilePath.GetDirectory(szInitDir)) {
					::lstrcpy(szFileName,FilePath.GetFileName());
				} else {
					GetAppClass().GetAppDirectory(szInitDir);
				}
				InitOpenFileName(&ofn);
				ofn.hwndOwner=hDlg;
				ofn.lpstrFilter=
					TEXT("BonDriver(BonDriver*.dll)\0BonDriver*.dll\0")
					TEXT("すべてのファイル\0*.*\0");
				ofn.lpstrFile=szFileName;
				ofn.nMaxFile=lengthof(szFileName);
				ofn.lpstrInitialDir=szInitDir;
				ofn.lpstrTitle=TEXT("BonDriverの選択");
				ofn.Flags=OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_EXPLORER;
				if (::GetOpenFileName(&ofn))
					::SetDlgItemText(hDlg,IDC_OPTIONS_DEFAULTDRIVER,szFileName);
			}
			return TRUE;
		}
		return TRUE;

	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code) {
		case PSN_APPLY:
			{
				CVideoRenderer::RendererType Renderer=(CVideoRenderer::RendererType)
					DlgComboBox_GetCurSel(hDlg,IDC_OPTIONS_RENDERER);
				if (Renderer!=m_VideoRendererType) {
					if (!CVideoRenderer::IsAvailable(Renderer)) {
						SettingError();
						::MessageBox(hDlg,TEXT("選択されたレンダラはこの環境で利用可能になっていません。"),
									 NULL,MB_OK | MB_ICONEXCLAMATION);
						return TRUE;
					}
					m_VideoRendererType=Renderer;
					SetUpdateFlag(UPDATE_RENDERER);
					SetGeneralUpdateFlag(UPDATE_GENERAL_BUILDMEDIAVIEWER);
				}

				TCHAR szDirectory[MAX_PATH];
				::GetDlgItemText(hDlg,IDC_OPTIONS_DRIVERDIRECTORY,
								 szDirectory,lengthof(szDirectory));
				m_BonDriverDirectory=szDirectory;

				m_DefaultDriverType=(DefaultDriverType)
					(GetCheckedRadioButton(hDlg,IDC_OPTIONS_DEFAULTDRIVER_NONE,
										   IDC_OPTIONS_DEFAULTDRIVER_CUSTOM)-
					IDC_OPTIONS_DEFAULTDRIVER_NONE);

				TCHAR szDefaultBonDriver[MAX_PATH];
				::GetDlgItemText(hDlg,IDC_OPTIONS_DEFAULTDRIVER,
								 szDefaultBonDriver,lengthof(szDefaultBonDriver));
				m_DefaultBonDriverName=szDefaultBonDriver;

				GetVideoDecoderSetting(IDC_OPTIONS_MPEG2DECODER,
									   STREAM_TYPE_MPEG2_VIDEO,
									   &m_Mpeg2DecoderName);
				GetVideoDecoderSetting(IDC_OPTIONS_H264DECODER,
									   STREAM_TYPE_H264,
									   &m_H264DecoderName);
				GetVideoDecoderSetting(IDC_OPTIONS_H265DECODER,
									   STREAM_TYPE_H265,
									   &m_H265DecoderName);

				bool fResident=DlgCheckBox_IsChecked(hDlg,IDC_OPTIONS_RESIDENT);
				if (fResident!=m_fResident) {
					m_fResident=fResident;
					SetUpdateFlag(UPDATE_RESIDENT);
				}

				m_fKeepSingleTask=
					DlgCheckBox_IsChecked(hDlg,IDC_OPTIONS_KEEPSINGLETASK);

				m_fChanged=true;
			}
			return TRUE;
		}
		break;
	}

	return FALSE;
}


void CGeneralOptions::SetVideoDecoderList(
	int ID,const GUID &SubType,BYTE StreamType,const TVTest::String &DecoderName)
{
	CDirectShowFilterFinder FilterFinder;
	std::vector<TVTest::String> FilterList;
	if (FilterFinder.FindFilter(&MEDIATYPE_Video,&SubType)) {
		FilterList.reserve(FilterFinder.GetFilterCount());
		for (int i=0;i<FilterFinder.GetFilterCount();i++) {
			WCHAR szFilterName[MAX_VIDEO_DECODER_NAME];

			if (FilterFinder.GetFilterInfo(i,NULL,szFilterName,lengthof(szFilterName))) {
				FilterList.push_back(TVTest::String(szFilterName));
			}
		}
	}
	int Sel=0;
	if (FilterList.empty()) {
		DlgComboBox_AddString(m_hDlg,ID,TEXT("<デコーダが見付かりません>"));
	} else {
		if (FilterList.size()>1) {
			std::sort(FilterList.begin(),FilterList.end(),
				[](const TVTest::String Filter1,const TVTest::String &Filter2) {
					return ::CompareString(LOCALE_USER_DEFAULT,
										   NORM_IGNORECASE | NORM_IGNORESYMBOLS,
										   Filter1.data(),(int)Filter1.length(),
										   Filter2.data(),(int)Filter2.length())==CSTR_LESS_THAN;
				});
		}
		for (size_t i=0;i<FilterList.size();i++) {
			DlgComboBox_AddString(m_hDlg,ID,FilterList[i].c_str());
		}

		CMediaViewer &MediaViewer=GetAppClass().CoreEngine.m_DtvEngine.m_MediaViewer;
		TCHAR szText[32+MAX_VIDEO_DECODER_NAME];

		::lstrcpy(szText,TEXT("自動"));
		if (!DecoderName.empty()) {
			Sel=(int)DlgComboBox_FindStringExact(m_hDlg,ID,-1,DecoderName.c_str())+1;
		} else if (MediaViewer.IsOpen()
				&& StreamType==MediaViewer.GetVideoStreamType()) {
			::lstrcat(szText,TEXT(" ("));
			MediaViewer.GetVideoDecoderName(szText+::lstrlen(szText),MAX_VIDEO_DECODER_NAME);
			::lstrcat(szText,TEXT(")"));
		}
		DlgComboBox_InsertString(m_hDlg,ID,0,szText);
	}
	DlgComboBox_SetCurSel(m_hDlg,ID,Sel);
}


void CGeneralOptions::GetVideoDecoderSetting(
	int ID,BYTE StreamType,TVTest::String *pDecoderName)
{
	TCHAR szDecoder[MAX_VIDEO_DECODER_NAME];
	int Sel=(int)DlgComboBox_GetCurSel(m_hDlg,ID);
	if (Sel>0)
		DlgComboBox_GetLBString(m_hDlg,ID,Sel,szDecoder);
	else
		szDecoder[0]='\0';
	if (::lstrcmpi(szDecoder,pDecoderName->c_str())!=0) {
		*pDecoderName=szDecoder;
		SetUpdateFlag(UPDATE_DECODER);
		if (StreamType==GetAppClass().CoreEngine.m_DtvEngine.m_MediaViewer.GetVideoStreamType())
			SetGeneralUpdateFlag(UPDATE_GENERAL_BUILDMEDIAVIEWER);
	}
}
