#include "stdafx.h"
#include "TVTest.h"
#include "AppMain.h"
#include "VideoOptions.h"
#include "DialogUtil.h"
#include "MessageDialog.h"
#include "resource.h"
#include <algorithm>
#include "Common/DebugDef.h"




const CVideoOptions::RendererInfo CVideoOptions::m_RendererList[] = {
	{CVideoRenderer::RENDERER_DEFAULT,            TEXT("システムデフォルト")},
	{CVideoRenderer::RENDERER_VMR7,               TEXT("VMR7")},
	{CVideoRenderer::RENDERER_VMR9,               TEXT("VMR9")},
	{CVideoRenderer::RENDERER_VMR7RENDERLESS,     TEXT("VMR7 Renderless")},
	{CVideoRenderer::RENDERER_VMR9RENDERLESS,     TEXT("VMR9 Renderless")},
	{CVideoRenderer::RENDERER_EVR,                TEXT("EVR")},
	{CVideoRenderer::RENDERER_EVRCUSTOMPRESENTER, TEXT("EVR (Custom Presenter)")},
	{CVideoRenderer::RENDERER_OVERLAYMIXER,       TEXT("Overlay Mixer")},
	{CVideoRenderer::RENDERER_madVR,              TEXT("madVR")},
};


bool CVideoOptions::GetRendererInfo(int Index,RendererInfo *pInfo)
{
	if (Index<0 || Index>=lengthof(m_RendererList))
		return false;
	*pInfo=m_RendererList[Index];
	return true;
}


CVideoOptions::CVideoOptions()
	: m_VideoRendererType(CVideoRenderer::RENDERER_DEFAULT)
	, m_fResetPanScanEventChange(true)
	, m_fNoMaskSideCut(true)
	, m_FullscreenStretchMode(CMediaViewer::STRETCH_KEEPASPECTRATIO)
	, m_MaximizeStretchMode(CMediaViewer::STRETCH_KEEPASPECTRATIO)
	, m_fIgnoreDisplayExtension(false)
	, m_fClipToDevice(true)
{
}


CVideoOptions::~CVideoOptions()
{
	Destroy();
}


bool CVideoOptions::Apply(DWORD Flags)
{
	CAppMain &App=GetAppClass();

	if ((Flags & UPDATE_MASKCUTAREA)!=0) {
		App.CoreEngine.m_DtvEngine.m_MediaViewer.SetNoMaskSideCut(m_fNoMaskSideCut);
	}

	if ((Flags & UPDATE_IGNOREDISPLAYEXTENSION)!=0) {
		App.CoreEngine.m_DtvEngine.m_MediaViewer.SetIgnoreDisplayExtension(m_fIgnoreDisplayExtension);
	}

	if ((Flags & UPDATE_CLIPTODEVICE)!=0) {
		App.CoreEngine.m_DtvEngine.m_MediaViewer.SetClipToDevice(m_fClipToDevice);
	}

	return true;
}


bool CVideoOptions::ReadSettings(CSettings &Settings)
{
	int Value;

	Settings.Read(TEXT("Mpeg2Decoder"),&m_Mpeg2DecoderName);
	Settings.Read(TEXT("H264Decoder"),&m_H264DecoderName);
	Settings.Read(TEXT("H265Decoder"),&m_H265DecoderName);

	TCHAR szRenderer[32];
	if (Settings.Read(TEXT("Renderer"),szRenderer,lengthof(szRenderer))) {
		if (szRenderer[0]=='\0') {
			m_VideoRendererType=CVideoRenderer::RENDERER_DEFAULT;
		} else {
			CVideoRenderer::RendererType Renderer=CVideoRenderer::ParseName(szRenderer);
			if (Renderer!=CVideoRenderer::RENDERER_UNDEFINED)
				m_VideoRendererType=Renderer;
		}
	}

	Settings.Read(TEXT("ResetPanScanEventChange"),&m_fResetPanScanEventChange);
	Settings.Read(TEXT("NoMaskSideCut"),&m_fNoMaskSideCut);
	if (Settings.Read(TEXT("FullscreenStretchMode"),&Value))
		m_FullscreenStretchMode=Value==1?CMediaViewer::STRETCH_CUTFRAME:
										 CMediaViewer::STRETCH_KEEPASPECTRATIO;
	if (Settings.Read(TEXT("MaximizeStretchMode"),&Value))
		m_MaximizeStretchMode=Value==1?CMediaViewer::STRETCH_CUTFRAME:
									   CMediaViewer::STRETCH_KEEPASPECTRATIO;
	Settings.Read(TEXT("IgnoreDisplayExtension"),&m_fIgnoreDisplayExtension);
	Settings.Read(TEXT("ClipToDevice"),&m_fClipToDevice);

	return true;
}


bool CVideoOptions::WriteSettings(CSettings &Settings)
{
	Settings.Write(TEXT("Mpeg2Decoder"),m_Mpeg2DecoderName);
	Settings.Write(TEXT("H264Decoder"),m_H264DecoderName);
	Settings.Write(TEXT("H265Decoder"),m_H265DecoderName);
	Settings.Write(TEXT("Renderer"),
				   CVideoRenderer::EnumRendererName((int)m_VideoRendererType));
	Settings.Write(TEXT("ResetPanScanEventChange"),m_fResetPanScanEventChange);
	Settings.Write(TEXT("NoMaskSideCut"),m_fNoMaskSideCut);
	Settings.Write(TEXT("FullscreenStretchMode"),(int)m_FullscreenStretchMode);
	Settings.Write(TEXT("MaximizeStretchMode"),(int)m_MaximizeStretchMode);
	Settings.Write(TEXT("IgnoreDisplayExtension"),m_fIgnoreDisplayExtension);
	Settings.Write(TEXT("ClipToDevice"),m_fClipToDevice);

	return true;
}


bool CVideoOptions::Create(HWND hwndOwner)
{
	return CreateDialogWindow(hwndOwner,
							  GetAppClass().GetResourceInstance(),
							  MAKEINTRESOURCE(IDD_OPTIONS_VIDEO));
}


LPCTSTR CVideoOptions::GetMpeg2DecoderName() const
{
	return m_Mpeg2DecoderName.c_str();
}


void CVideoOptions::SetMpeg2DecoderName(LPCTSTR pszDecoderName)
{
	TVTest::StringUtility::Assign(m_Mpeg2DecoderName,pszDecoderName);
}


LPCTSTR CVideoOptions::GetH264DecoderName() const
{
	return m_H264DecoderName.c_str();
}


void CVideoOptions::SetH264DecoderName(LPCTSTR pszDecoderName)
{
	TVTest::StringUtility::Assign(m_H264DecoderName,pszDecoderName);
}


LPCTSTR CVideoOptions::GetH265DecoderName() const
{
	return m_H265DecoderName.c_str();
}


void CVideoOptions::SetH265DecoderName(LPCTSTR pszDecoderName)
{
	TVTest::StringUtility::Assign(m_H265DecoderName,pszDecoderName);
}


CVideoRenderer::RendererType CVideoOptions::GetVideoRendererType() const
{
	return m_VideoRendererType;
}


bool CVideoOptions::SetVideoRendererType(CVideoRenderer::RendererType Renderer)
{
	if (CVideoRenderer::EnumRendererName((int)Renderer)==NULL)
		return false;
	m_VideoRendererType=Renderer;
	return true;
}


INT_PTR CVideoOptions::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			CAppMain &App=GetAppClass();

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
			int Sel=-1;
			RendererInfo Info;
			for (int i=0;GetRendererInfo(i,&Info);i++) {
				DlgComboBox_AddString(hDlg,IDC_OPTIONS_RENDERER,Info.pszName);
				DlgComboBox_SetItemData(hDlg,IDC_OPTIONS_RENDERER,i,(LPARAM)Info.Renderer);
				if (Info.Renderer==m_VideoRendererType)
					Sel=i;
			}
			DlgComboBox_SetCurSel(hDlg,IDC_OPTIONS_RENDERER,Sel);

			DlgCheckBox_Check(hDlg,IDC_OPTIONS_RESETPANSCANEVENTCHANGE,
							  m_fResetPanScanEventChange);
			DlgCheckBox_Check(hDlg,IDC_OPTIONS_NOMASKSIDECUT,
							  m_fNoMaskSideCut);
			DlgCheckBox_Check(hDlg,IDC_OPTIONS_FULLSCREENCUTFRAME,
				m_FullscreenStretchMode==CMediaViewer::STRETCH_CUTFRAME);
			DlgCheckBox_Check(hDlg,IDC_OPTIONS_MAXIMIZECUTFRAME,
				m_MaximizeStretchMode==CMediaViewer::STRETCH_CUTFRAME);
			DlgCheckBox_Check(hDlg,IDC_OPTIONS_IGNOREDISPLAYSIZE,
							  m_fIgnoreDisplayExtension);
			DlgCheckBox_Check(hDlg,IDC_OPTIONS_CLIPTODEVICE,
							  m_fClipToDevice);
		}
		return TRUE;

	case WM_NOTIFY:
		switch (reinterpret_cast<LPNMHDR>(lParam)->code) {
		case PSN_APPLY:
			{
				CAppMain &App=GetAppClass();
				bool f;

				CVideoRenderer::RendererType Renderer=(CVideoRenderer::RendererType)
					DlgComboBox_GetItemData(hDlg,IDC_OPTIONS_RENDERER,
						DlgComboBox_GetCurSel(hDlg,IDC_OPTIONS_RENDERER));
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

				GetVideoDecoderSetting(IDC_OPTIONS_MPEG2DECODER,
									   STREAM_TYPE_MPEG2_VIDEO,
									   &m_Mpeg2DecoderName);
				GetVideoDecoderSetting(IDC_OPTIONS_H264DECODER,
									   STREAM_TYPE_H264,
									   &m_H264DecoderName);
				GetVideoDecoderSetting(IDC_OPTIONS_H265DECODER,
									   STREAM_TYPE_H265,
									   &m_H265DecoderName);

				m_fResetPanScanEventChange=
					DlgCheckBox_IsChecked(hDlg,IDC_OPTIONS_RESETPANSCANEVENTCHANGE);
				f=DlgCheckBox_IsChecked(hDlg,IDC_OPTIONS_NOMASKSIDECUT);
				if (m_fNoMaskSideCut!=f) {
					m_fNoMaskSideCut=f;
					SetUpdateFlag(UPDATE_MASKCUTAREA);
				}
				m_FullscreenStretchMode=
					DlgCheckBox_IsChecked(hDlg,IDC_OPTIONS_FULLSCREENCUTFRAME)?
						CMediaViewer::STRETCH_CUTFRAME:CMediaViewer::STRETCH_KEEPASPECTRATIO;
				if (App.UICore.GetFullscreen())
					App.CoreEngine.m_DtvEngine.m_MediaViewer.SetViewStretchMode(m_FullscreenStretchMode);
				m_MaximizeStretchMode=
					DlgCheckBox_IsChecked(hDlg,IDC_OPTIONS_MAXIMIZECUTFRAME)?
						CMediaViewer::STRETCH_CUTFRAME:CMediaViewer::STRETCH_KEEPASPECTRATIO;
				if (::IsZoomed(App.UICore.GetMainWindow()))
					App.CoreEngine.m_DtvEngine.m_MediaViewer.SetViewStretchMode(m_MaximizeStretchMode);
				f=DlgCheckBox_IsChecked(hDlg,IDC_OPTIONS_IGNOREDISPLAYSIZE);
				if (m_fIgnoreDisplayExtension!=f) {
					m_fIgnoreDisplayExtension=f;
					SetUpdateFlag(UPDATE_IGNOREDISPLAYEXTENSION);
				}
				f=DlgCheckBox_IsChecked(hDlg,IDC_OPTIONS_CLIPTODEVICE);
				if (m_fClipToDevice!=f) {
					m_fClipToDevice=f;
					SetUpdateFlag(UPDATE_CLIPTODEVICE);
				}

				m_fChanged=true;
			}
			return TRUE;
		}
		break;
	}

	return FALSE;
}


void CVideoOptions::SetVideoDecoderList(
	int ID,const GUID &SubType,BYTE StreamType,const TVTest::String &DecoderName)
{
	LPCWSTR pszDefaultDecoderName=
		CInternalDecoderManager::IsDecoderAvailable(SubType)?
			CInternalDecoderManager::GetDecoderName(SubType):NULL;

	CDirectShowFilterFinder FilterFinder;
	std::vector<TVTest::String> FilterList;
	if (FilterFinder.FindFilter(&MEDIATYPE_Video,&SubType)) {
		FilterList.reserve(FilterFinder.GetFilterCount());
		for (int i=0;i<FilterFinder.GetFilterCount();i++) {
			TVTest::String FilterName;

			if (FilterFinder.GetFilterInfo(i,NULL,&FilterName)
					&& (pszDefaultDecoderName==NULL
						|| ::lstrcmpi(FilterName.c_str(),pszDefaultDecoderName)!=0)) {
				FilterList.push_back(FilterName);
			}
		}
	}
	int Sel=0;
	if (FilterList.empty()) {
		DlgComboBox_AddString(m_hDlg,ID,
			pszDefaultDecoderName!=NULL?
				pszDefaultDecoderName:
				TEXT("<デコーダが見付かりません>"));
	} else {
		if (pszDefaultDecoderName!=NULL)
			DlgComboBox_AddString(m_hDlg,ID,pszDefaultDecoderName);
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


void CVideoOptions::GetVideoDecoderSetting(
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
