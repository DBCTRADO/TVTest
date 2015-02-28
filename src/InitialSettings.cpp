#include "stdafx.h"
#include "TVTest.h"
#include "AppMain.h"
#include "InitialSettings.h"
#include "DirectShowFilter/DirectShowUtil.h"
#include "DialogUtil.h"
#include "DrawUtil.h"
#include "Aero.h"
#include "Help.h"
#include "resource.h"
#include <algorithm>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif




CInitialSettings::CInitialSettings(const CDriverManager *pDriverManager)
	: m_pDriverManager(pDriverManager)
	, m_VideoRenderer(CVideoRenderer::RENDERER_DEFAULT)
	, m_fDrawLogo(false)
{
	m_szDriverFileName[0]='\0';
#if 0
	// Vista�ł̓r�f�I�����_���̃f�t�H���g��EVR�ɂ���
	// ...�Ɩ�肪�o���������݂���
	if (Util::OS::IsWindowsVistaOrLater())
		m_VideoRenderer=CVideoRenderer::RENDERER_EVR;
#endif

#ifdef WIN_XP_SUPPORT
	TCHAR szRecFolder[MAX_PATH];
	if (::SHGetSpecialFolderPath(NULL,szRecFolder,CSIDL_MYVIDEO,FALSE)
			|| ::SHGetSpecialFolderPath(NULL,szRecFolder,CSIDL_PERSONAL,FALSE))
		m_RecordFolder=szRecFolder;
#else
	PWSTR pszRecFolder;
	if (::SHGetKnownFolderPath(FOLDERID_Videos,0,NULL,&pszRecFolder)==S_OK
			|| ::SHGetKnownFolderPath(FOLDERID_Documents,0,NULL,&pszRecFolder)==S_OK) {
		m_RecordFolder=pszRecFolder;
		::CoTaskMemFree(pszRecFolder);
	}
#endif
}


CInitialSettings::~CInitialSettings()
{
	Destroy();
}


bool CInitialSettings::Show(HWND hwndOwner)
{
	return ShowDialog(hwndOwner,
					  GetAppClass().GetResourceInstance(),
					  MAKEINTRESOURCE(IDD_INITIALSETTINGS))==IDOK;
}


bool CInitialSettings::GetDriverFileName(LPTSTR pszFileName,int MaxLength) const
{
	if (::lstrlen(m_szDriverFileName)>=MaxLength)
		return false;
	::lstrcpy(pszFileName,m_szDriverFileName);
	return true;
}


INT_PTR CInitialSettings::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			{
				HWND hwndLogo=::GetDlgItem(hDlg,IDC_INITIALSETTINGS_LOGO);
				RECT rc;

				::GetWindowRect(hwndLogo,&rc);
				::SetRect(&rc,rc.right-rc.left,0,0,0);
				if (m_AeroGlass.ApplyAeroGlass(hDlg,&rc)) {
					m_fDrawLogo=true;
					m_LogoImage.LoadFromResource(GetAppClass().GetResourceInstance(),
						MAKEINTRESOURCE(IDB_LOGO32),TEXT("PNG"));
					::ShowWindow(hwndLogo,SW_HIDE);
				} else {
					HBITMAP hbm=::LoadBitmap(GetAppClass().GetResourceInstance(),
											 MAKEINTRESOURCE(IDB_LOGO));
					::SendMessage(hwndLogo,STM_SETIMAGE,
								  IMAGE_BITMAP,reinterpret_cast<LPARAM>(hbm));
				}
			}

			// BonDriver
			{
				int NormalDriverCount=0;

				DlgComboBox_LimitText(hDlg,IDC_INITIALSETTINGS_DRIVER,MAX_PATH-1);
				for (int i=0;i<m_pDriverManager->NumDrivers();i++) {
					const CDriverInfo *pDriverInfo=m_pDriverManager->GetDriverInfo(i);
					int Index;

					if (CCoreEngine::IsNetworkDriverFileName(pDriverInfo->GetFileName())) {
						Index=i;
					} else {
						Index=NormalDriverCount++;
					}
					DlgComboBox_InsertString(hDlg,IDC_INITIALSETTINGS_DRIVER,
											 Index,pDriverInfo->GetFileName());
				}
				if (m_pDriverManager->NumDrivers()>0) {
					DlgComboBox_GetLBString(hDlg,IDC_INITIALSETTINGS_DRIVER,
											0,m_szDriverFileName);
					::SetDlgItemText(hDlg,IDC_INITIALSETTINGS_DRIVER,m_szDriverFileName);
				}
			}

			// �f���f�R�[�_
			InitDecoderList(IDC_INITIALSETTINGS_MPEG2DECODER,
							MEDIASUBTYPE_MPEG2_VIDEO,
							m_Mpeg2DecoderName.c_str());
			InitDecoderList(IDC_INITIALSETTINGS_H264DECODER,
							MEDIASUBTYPE_H264,
							m_H264DecoderName.c_str());
			InitDecoderList(IDC_INITIALSETTINGS_H265DECODER,
							MEDIASUBTYPE_HEVC,
							m_H265DecoderName.c_str());

			// Video renderer
			{
				LPCTSTR pszName;

				DlgComboBox_AddString(hDlg,IDC_INITIALSETTINGS_VIDEORENDERER,TEXT("�f�t�H���g"));
				for (int i=1;(pszName=CVideoRenderer::EnumRendererName(i))!=NULL;i++)
					DlgComboBox_AddString(hDlg,IDC_INITIALSETTINGS_VIDEORENDERER,pszName);
				DlgComboBox_SetCurSel(hDlg,IDC_INITIALSETTINGS_VIDEORENDERER,
									  m_VideoRenderer);
			}

			// �^��t�H���_
			::SetDlgItemText(hDlg,IDC_INITIALSETTINGS_RECORDFOLDER,m_RecordFolder.c_str());
			::SendDlgItemMessage(hDlg,IDC_INITIALSETTINGS_RECORDFOLDER,EM_LIMITTEXT,MAX_PATH-1,0);

			AdjustDialogPos(NULL,hDlg);
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_INITIALSETTINGS_DRIVER_BROWSE:
			{
				OPENFILENAME ofn;
				TCHAR szFileName[MAX_PATH],szInitDir[MAX_PATH];
				CFilePath FilePath;

				::GetDlgItemText(hDlg,IDC_INITIALSETTINGS_DRIVER,szFileName,lengthof(szFileName));
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
					TEXT("���ׂẴt�@�C��\0*.*\0");
				ofn.lpstrFile=szFileName;
				ofn.nMaxFile=lengthof(szFileName);
				ofn.lpstrInitialDir=szInitDir;
				ofn.lpstrTitle=TEXT("BonDriver�̑I��");
				ofn.Flags=OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_EXPLORER;
				if (::GetOpenFileName(&ofn)) {
					::SetDlgItemText(hDlg,IDC_INITIALSETTINGS_DRIVER,szFileName);
				}
			}
			return TRUE;

		case IDC_INITIALSETTINGS_RECORDFOLDER_BROWSE:
			{
				TCHAR szFolder[MAX_PATH];

				::GetDlgItemText(hDlg,IDC_INITIALSETTINGS_RECORDFOLDER,szFolder,lengthof(szFolder));
				if (BrowseFolderDialog(hDlg,szFolder,
										TEXT("�^��t�@�C���̕ۑ���t�H���_:")))
					::SetDlgItemText(hDlg,IDC_INITIALSETTINGS_RECORDFOLDER,szFolder);
			}
			return TRUE;

		case IDC_INITIALSETTINGS_HELP:
			GetAppClass().UICore.ShowHelpContent(HELP_ID_INITIALSETTINGS);
			return TRUE;

		case IDOK:
			{
				bool fMpeg2Decoder=
					DlgComboBox_GetCount(hDlg,IDC_INITIALSETTINGS_MPEG2DECODER)>1;
				bool fH264Decoder=
					DlgComboBox_GetCount(hDlg,IDC_INITIALSETTINGS_H264DECODER)>1;
				bool fH265Decoder=
					DlgComboBox_GetCount(hDlg,IDC_INITIALSETTINGS_H265DECODER)>1;
				if (!fMpeg2Decoder || !fH264Decoder || !fH265Decoder) {
					TCHAR szCodecs[64],szMessage[256];
					szCodecs[0]=_T('\0');
					if (!fMpeg2Decoder)
						::lstrcat(szCodecs,TEXT("MPEG-2"));
					if (!fH264Decoder) {
						if (szCodecs[0]!=_T('\0'))
							::lstrcat(szCodecs,TEXT("/"));
						::lstrcat(szCodecs,TEXT("H.264(AVC)"));
					}
					if (!fH265Decoder) {
						if (szCodecs[0]!=_T('\0'))
							::lstrcat(szCodecs,TEXT("/"));
						::lstrcat(szCodecs,TEXT("H.265(HEVC)"));
					}
					StdUtil::snprintf(szMessage,lengthof(szMessage),
						TEXT("%s �̃f�R�[�_�����t����Ȃ����߁A%s �̉f���͍Đ��ł��܂���B\n")
						TEXT("�f�����Đ�����ɂ̓f�R�[�_���C���X�g�[�����Ă��������B"),
						szCodecs,szCodecs);
					::MessageBox(hDlg,szMessage,TEXT("���m�点"),MB_OK | MB_ICONINFORMATION);
				}

				TVTest::String Mpeg2DecoderName,H264DecoderName,H265DecoderName;
				if (fMpeg2Decoder)
					GetDecoderSetting(IDC_INITIALSETTINGS_MPEG2DECODER,&Mpeg2DecoderName);
				if (fH264Decoder)
					GetDecoderSetting(IDC_INITIALSETTINGS_H264DECODER,&H264DecoderName);
				if (fH265Decoder)
					GetDecoderSetting(IDC_INITIALSETTINGS_H265DECODER,&H265DecoderName);

				CVideoRenderer::RendererType VideoRenderer=(CVideoRenderer::RendererType)
					DlgComboBox_GetCurSel(hDlg,IDC_INITIALSETTINGS_VIDEORENDERER);

				// �����̈����g�ݍ��킹�ɑ΂��Ē��ӂ�\������
				static const struct {
					LPCTSTR pszDecoder;
					CVideoRenderer::RendererType Renderer;
					LPCTSTR pszMessage;
				} ConflictList[] = {
					{TEXT("CyberLink"),	CVideoRenderer::RENDERER_DEFAULT,
						TEXT("CyberLink �̃f�R�[�_�ƃf�t�H���g�����_���̑g�ݍ��킹�ŁA\n")
						TEXT("�ꕔ�̔ԑg�Ŕ䗦�����������Ȃ錻�ۂ��o�鎖�����邽�߁A\n")
						TEXT("�����_�����f�t�H���g�ȊO�ɂ��邱�Ƃ������߂��܂��B\n")
						TEXT("���݂̐ݒ��ύX���܂���?")},
				};
				for (int i=0;i<lengthof(ConflictList);i++) {
					int Length=::lstrlen(ConflictList[i].pszDecoder);
					if (VideoRenderer==ConflictList[i].Renderer
							&& (::StrCmpNI(Mpeg2DecoderName.c_str(),ConflictList[i].pszDecoder,Length)==0
								|| ::StrCmpNI(H264DecoderName.c_str(),ConflictList[i].pszDecoder,Length)==0)
								|| ::StrCmpNI(H265DecoderName.c_str(),ConflictList[i].pszDecoder,Length)==0) {
						if (::MessageBox(hDlg,ConflictList[i].pszMessage,TEXT("����"),
										 MB_YESNO | MB_ICONINFORMATION)==IDYES)
							return TRUE;
						break;
					}
				}

				if (!CVideoRenderer::IsAvailable(VideoRenderer)) {
					::MessageBox(hDlg,TEXT("�I�����ꂽ�����_���͂��̊��ŗ��p�\�ɂȂ��Ă��܂���B"),
								 NULL,MB_OK | MB_ICONEXCLAMATION);
					return TRUE;
				}

				::GetDlgItemText(hDlg,IDC_INITIALSETTINGS_DRIVER,
								 m_szDriverFileName,MAX_PATH);

				m_Mpeg2DecoderName=Mpeg2DecoderName;
				m_H264DecoderName=H264DecoderName;
				m_H265DecoderName=H265DecoderName;

				m_VideoRenderer=VideoRenderer;

				TCHAR szRecordFolder[MAX_PATH];
				::GetDlgItemText(hDlg,IDC_INITIALSETTINGS_RECORDFOLDER,
								 szRecordFolder,lengthof(szRecordFolder));
				CAppMain::CreateDirectoryResult CreateDirResult=
					GetAppClass().CreateDirectory(
						hDlg,szRecordFolder,
						TEXT("�^��t�@�C���̕ۑ���t�H���_ \"%s\" ������܂���B\n")
						TEXT("�쐬���܂���?"));
				if (CreateDirResult==CAppMain::CREATEDIRECTORY_RESULT_ERROR) {
					SetDlgItemFocus(hDlg,IDC_INITIALSETTINGS_RECORDFOLDER);
					return TRUE;
				}
				m_RecordFolder=szRecordFolder;
			}
		case IDCANCEL:
			::EndDialog(hDlg,LOWORD(wParam));
			return TRUE;
		}
		return TRUE;

	case WM_PAINT:
		if (m_fDrawLogo) {
			PAINTSTRUCT ps;

			::BeginPaint(hDlg,&ps);
			{
				TVTest::Graphics::CCanvas Canvas(ps.hdc);
				TVTest::Graphics::CBrush Brush(::GetSysColor(COLOR_3DFACE));
				RECT rc,rcClient;

				::GetWindowRect(::GetDlgItem(hDlg,IDC_INITIALSETTINGS_LOGO),&rc);
				::OffsetRect(&rc,-rc.left,-rc.top);
				Canvas.Clear(0,0,0,0);
				::GetClientRect(hDlg,&rcClient);
				rcClient.left=rc.right;
				Canvas.FillRect(&Brush,rcClient);
				Canvas.DrawImage(&m_LogoImage,
					(rc.right-m_LogoImage.GetWidth())/2,
					(rc.bottom-m_LogoImage.GetHeight())/2);
			}
			::EndPaint(hDlg,&ps);
			return TRUE;
		}
		break;

	case WM_CTLCOLORSTATIC:
		if (reinterpret_cast<HWND>(lParam)==::GetDlgItem(hDlg,IDC_INITIALSETTINGS_LOGO))
			return reinterpret_cast<INT_PTR>(::GetStockObject(WHITE_BRUSH));
		break;

	case WM_DESTROY:
		{
			HBITMAP hbm=reinterpret_cast<HBITMAP>(::SendDlgItemMessage(hDlg,IDC_INITIALSETTINGS_LOGO,
				STM_SETIMAGE,IMAGE_BITMAP,reinterpret_cast<LPARAM>((HBITMAP)NULL)));

			if (hbm!=NULL) {
				::DeleteObject(hbm);
			} else {
				m_LogoImage.Free();
			}
		}
		return TRUE;
	}

	return FALSE;
}


void CInitialSettings::InitDecoderList(int ID,const GUID &SubType,LPCTSTR pszDecoderName)
{
	CDirectShowFilterFinder FilterFinder;
	std::vector<TVTest::String> FilterList;
	int Sel=0;

	if (FilterFinder.FindFilter(&MEDIATYPE_Video,&SubType)) {
		FilterList.reserve(FilterFinder.GetFilterCount());
		for (int i=0;i<FilterFinder.GetFilterCount();i++) {
			WCHAR szFilterName[MAX_DECODER_NAME];

			if (FilterFinder.GetFilterInfo(i,NULL,szFilterName,lengthof(szFilterName))) {
				FilterList.push_back(TVTest::String(szFilterName));
			}
		}
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

		Sel=(int)DlgComboBox_FindStringExact(m_hDlg,ID,-1,pszDecoderName)+1;
	}

	DlgComboBox_InsertString(m_hDlg,ID,
		0,!FilterList.empty()?TEXT("����"):TEXT("<�f�R�[�_�����t����܂���>"));
	DlgComboBox_SetCurSel(m_hDlg,ID,Sel);
}


void CInitialSettings::GetDecoderSetting(int ID,TVTest::String *pDecoderName) const
{
	int Sel=(int)DlgComboBox_GetCurSel(m_hDlg,ID);
	if (Sel>0) {
		TCHAR szDecoder[MAX_DECODER_NAME];
		DlgComboBox_GetLBString(m_hDlg,ID,Sel,szDecoder);
		*pDecoderName=szDecoder;
	} else {
		pDecoderName->clear();
	}
}
