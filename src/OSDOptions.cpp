#include "stdafx.h"
#include "TVTest.h"
#include "AppMain.h"
#include "OSDOptions.h"
#include "DialogUtil.h"
#include "DrawUtil.h"
#include "Util.h"
#include "Aero.h"
#include "resource.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


#define OSD_FLAG(type) (1U<<(type))




COSDOptions::COSDOptions()
	: m_fShowOSD(true)
	, m_fPseudoOSD(true)
	, m_TextColor(RGB(0,255,0))
	, m_Opacity(80)
	, m_FadeTime(3000)
	, m_ChannelChangeType(CHANNELCHANGE_LOGOANDTEXT)
	, m_EnabledOSD(OSD_FLAG(OSD_CHANNEL) | OSD_FLAG(OSD_VOLUME) | OSD_FLAG(OSD_CHANNELNOINPUT))

	, m_fLayeredWindow(true)
	, m_fCompositionEnabled(false)

	, m_fEnableNotificationBar(true)
	, m_NotificationBarDuration(3000)
	, m_NotificationBarFlags(NOTIFY_EVENTNAME | NOTIFY_TSPROCESSORERROR)
	, m_fDisplayFontAutoSize(false)
{
	if (Util::OS::IsWindowsVistaOrLater()) {
		CAeroGlass Aero;
		if (Aero.IsEnabled())
			m_fCompositionEnabled=true;
	}

	DrawUtil::GetSystemFont(DrawUtil::FONT_DEFAULT,&m_OSDFont);

	LOGFONT lf;
	DrawUtil::GetSystemFont(DrawUtil::FONT_MESSAGE,&lf);

	m_NotificationBarFont=lf;
	m_NotificationBarFont.lfHeight=
#ifndef TVTEST_FOR_1SEG
		-14;
#else
		-12;
#endif

	m_DisplayFont=lf;
}


COSDOptions::~COSDOptions()
{
	Destroy();
}


bool COSDOptions::ReadSettings(CSettings &Settings)
{
	int Value;

	Settings.Read(TEXT("UseOSD"),&m_fShowOSD);
	Settings.Read(TEXT("PseudoOSD"),&m_fPseudoOSD);
	Settings.ReadColor(TEXT("OSDTextColor"),&m_TextColor);
	Settings.Read(TEXT("OSDOpacity"),&m_Opacity);
	Settings.Read(TEXT("OSDFont"),&m_OSDFont);
	Settings.Read(TEXT("OSDFadeTime"),&m_FadeTime);
	unsigned int EnabledOSD=m_EnabledOSD;
	if (Settings.Read(TEXT("EnabledOSD"),&EnabledOSD)) {
		// ���ڂ����������Ƀf�t�H���g�l�����f�����悤�ɂ���
		unsigned int EnabledOSDMask;
		if (!Settings.Read(TEXT("EnabledOSDMask"),&EnabledOSDMask)
				|| EnabledOSDMask==0)
			EnabledOSDMask=OSD_FLAG(OSD_CHANNELNOINPUT)-1;
		if ((EnabledOSD & ~EnabledOSDMask)==0)
			m_EnabledOSD=(EnabledOSD & EnabledOSDMask) | (m_EnabledOSD & ~EnabledOSDMask);
		else
			m_EnabledOSD=EnabledOSD;
	}
	if (Settings.Read(TEXT("ChannelOSDType"),&Value)
			&& Value>=CHANNELCHANGE_FIRST && Value<=CHANNELCHANGE_LAST)
		m_ChannelChangeType=(ChannelChangeType)Value;

	Settings.Read(TEXT("EnableNotificationBar"),&m_fEnableNotificationBar);
	Settings.Read(TEXT("NotificationBarDuration"),&m_NotificationBarDuration);
	bool f;
	if (Settings.Read(TEXT("NotifyEventName"),&f))
		EnableNotify(NOTIFY_EVENTNAME,f);
	if (Settings.Read(TEXT("NotifyTSProcessorError"),&f))
		EnableNotify(NOTIFY_TSPROCESSORERROR,f);

	if (!Settings.Read(TEXT("NotificationBarFont"),&m_NotificationBarFont)) {
		// �ߋ��̃o�[�W�����Ƃ̌݊��p(���̂�������)
		TCHAR szFont[LF_FACESIZE];
		if (Settings.Read(TEXT("NotificationBarFontName"),szFont,LF_FACESIZE)
				&& szFont[0]!='\0') {
			lstrcpy(m_NotificationBarFont.lfFaceName,szFont);
			m_NotificationBarFont.lfEscapement=0;
			m_NotificationBarFont.lfOrientation=0;
			m_NotificationBarFont.lfUnderline=0;
			m_NotificationBarFont.lfStrikeOut=0;
			m_NotificationBarFont.lfCharSet=DEFAULT_CHARSET;
			m_NotificationBarFont.lfOutPrecision=OUT_DEFAULT_PRECIS;
			m_NotificationBarFont.lfClipPrecision=CLIP_DEFAULT_PRECIS;
			m_NotificationBarFont.lfQuality=DRAFT_QUALITY;
			m_NotificationBarFont.lfPitchAndFamily=DEFAULT_PITCH | FF_DONTCARE;
		}
		if (Settings.Read(TEXT("NotificationBarFontSize"),&Value)) {
			m_NotificationBarFont.lfHeight=Value;
			m_NotificationBarFont.lfWidth=0;
		}
		if (Settings.Read(TEXT("NotificationBarFontWeight"),&Value))
			m_NotificationBarFont.lfWeight=Value;
		if (Settings.Read(TEXT("NotificationBarFontItalic"),&Value))
			m_NotificationBarFont.lfItalic=Value;
	}

	Settings.Read(TEXT("DisplayMenuFont"),&m_DisplayFont);
	Settings.Read(TEXT("DisplayMenuFontAutoSize"),&m_fDisplayFontAutoSize);

	return true;
}


bool COSDOptions::WriteSettings(CSettings &Settings)
{
	Settings.Write(TEXT("UseOSD"),m_fShowOSD);
	Settings.Write(TEXT("PseudoOSD"),m_fPseudoOSD);
	Settings.WriteColor(TEXT("OSDTextColor"),m_TextColor);
	Settings.Write(TEXT("OSDOpacity"),m_Opacity);
	Settings.Write(TEXT("OSDFont"),&m_OSDFont);
	Settings.Write(TEXT("OSDFadeTime"),m_FadeTime);
	Settings.Write(TEXT("EnabledOSD"),m_EnabledOSD);
	Settings.Write(TEXT("EnabledOSDMask"),OSD_FLAG(OSD_TRAILER_)-1);
	Settings.Write(TEXT("ChannelOSDType"),(int)m_ChannelChangeType);

	Settings.Write(TEXT("EnableNotificationBar"),m_fEnableNotificationBar);
	Settings.Write(TEXT("NotificationBarDuration"),m_NotificationBarDuration);
	Settings.Write(TEXT("NotifyEventName"),(m_NotificationBarFlags&NOTIFY_EVENTNAME)!=0);
	Settings.Write(TEXT("NotifyTSProcessorError"),(m_NotificationBarFlags&NOTIFY_TSPROCESSORERROR)!=0);

#if 0
	Settings.Write(TEXT("NotificationBarFontName"),m_NotificationBarFont.lfFaceName);
	Settings.Write(TEXT("NotificationBarFontSize"),(int)m_NotificationBarFont.lfHeight);
	Settings.Write(TEXT("NotificationBarFontWeight"),(int)m_NotificationBarFont.lfWeight);
	Settings.Write(TEXT("NotificationBarFontItalic"),(int)m_NotificationBarFont.lfItalic);
#else
	Settings.Write(TEXT("NotificationBarFont"),&m_NotificationBarFont);
#endif

	Settings.Write(TEXT("DisplayMenuFont"),&m_DisplayFont);
	Settings.Write(TEXT("DisplayMenuFontAutoSize"),m_fDisplayFontAutoSize);

	return true;
}


bool COSDOptions::Create(HWND hwndOwner)
{
	return CreateDialogWindow(hwndOwner,
							  GetAppClass().GetResourceInstance(),MAKEINTRESOURCE(IDD_OPTIONS_OSD));
}


bool COSDOptions::GetLayeredWindow() const
{
	return m_fLayeredWindow && m_fCompositionEnabled;
}


void COSDOptions::OnDwmCompositionChanged()
{
	CAeroGlass Aero;

	m_fCompositionEnabled=Aero.IsEnabled();
}


bool COSDOptions::IsOSDEnabled(OSDType Type) const
{
	return m_fShowOSD && (m_EnabledOSD&OSD_FLAG(Type))!=0;
}


bool COSDOptions::IsNotifyEnabled(unsigned int Type) const
{
	return m_fEnableNotificationBar && (m_NotificationBarFlags&Type)!=0;
}


void COSDOptions::EnableNotify(unsigned int Type,bool fEnabled)
{
	if (fEnabled)
		m_NotificationBarFlags|=Type;
	else
		m_NotificationBarFlags&=~Type;
}


static void SetFontInfo(HWND hDlg,int ID,const LOGFONT *plf)
{
	HDC hdc;
	TCHAR szText[LF_FACESIZE+16];

	hdc=GetDC(hDlg);
	if (hdc==NULL)
		return;
	wsprintf(szText,TEXT("%s, %d pt"),plf->lfFaceName,CalcFontPointHeight(hdc,plf));
	SetDlgItemText(hDlg,ID,szText);
	ReleaseDC(hDlg,hdc);
}

INT_PTR COSDOptions::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			DlgCheckBox_Check(hDlg,IDC_OSDOPTIONS_SHOWOSD,m_fShowOSD);
			DlgCheckBox_Check(hDlg,IDC_OSDOPTIONS_COMPOSITE,!m_fPseudoOSD);
			m_CurTextColor=m_TextColor;
			m_CurOSDFont=m_OSDFont;
			::SetDlgItemText(hDlg,IDC_OSDOPTIONS_OSDFONT_INFO,m_OSDFont.lfFaceName);
			::SetDlgItemInt(hDlg,IDC_OSDOPTIONS_FADETIME,m_FadeTime/1000,TRUE);
			DlgUpDown_SetRange(hDlg,IDC_OSDOPTIONS_FADETIME_UD,1,UD_MAXVAL);
			DlgCheckBox_Check(hDlg,IDC_OSDOPTIONS_SHOW_CHANNEL,(m_EnabledOSD&OSD_FLAG(OSD_CHANNEL))!=0);
			DlgCheckBox_Check(hDlg,IDC_OSDOPTIONS_SHOW_VOLUME,(m_EnabledOSD&OSD_FLAG(OSD_VOLUME))!=0);
			DlgCheckBox_Check(hDlg,IDC_OSDOPTIONS_SHOW_AUDIO,(m_EnabledOSD&OSD_FLAG(OSD_AUDIO))!=0);
			DlgCheckBox_Check(hDlg,IDC_OSDOPTIONS_SHOW_RECORDING,(m_EnabledOSD&OSD_FLAG(OSD_RECORDING))!=0);
			DlgCheckBox_Check(hDlg,IDC_OSDOPTIONS_SHOW_CHANNELNOINPUT,(m_EnabledOSD&OSD_FLAG(OSD_CHANNELNOINPUT))!=0);
			static const LPCTSTR ChannelChangeModeText[] = {
				TEXT("���S�ƃ`�����l����"),
				TEXT("�`�����l�����̂�"),
				TEXT("���S�̂�"),
			};
			SetComboBoxList(hDlg,IDC_OSDOPTIONS_CHANNELCHANGE,
							ChannelChangeModeText,lengthof(ChannelChangeModeText));
			DlgComboBox_SetCurSel(hDlg,IDC_OSDOPTIONS_CHANNELCHANGE,(int)m_ChannelChangeType);
			EnableDlgItems(hDlg,IDC_OSDOPTIONS_FIRST,IDC_OSDOPTIONS_LAST,m_fShowOSD);

			DlgCheckBox_Check(hDlg,IDC_NOTIFICATIONBAR_ENABLE,m_fEnableNotificationBar);
			DlgCheckBox_Check(hDlg,IDC_NOTIFICATIONBAR_NOTIFYEVENTNAME,(m_NotificationBarFlags&NOTIFY_EVENTNAME)!=0);
			DlgCheckBox_Check(hDlg,IDC_NOTIFICATIONBAR_NOTIFYTSPROCESSORERROR,(m_NotificationBarFlags&NOTIFY_TSPROCESSORERROR)!=0);
			::SetDlgItemInt(hDlg,IDC_NOTIFICATIONBAR_DURATION,m_NotificationBarDuration/1000,FALSE);
			DlgUpDown_SetRange(hDlg,IDC_NOTIFICATIONBAR_DURATION_UPDOWN,1,60);
			m_CurNotificationBarFont=m_NotificationBarFont;
			SetFontInfo(hDlg,IDC_NOTIFICATIONBAR_FONT_INFO,&m_CurNotificationBarFont);
			EnableDlgItems(hDlg,IDC_NOTIFICATIONBAR_FIRST,IDC_NOTIFICATIONBAR_LAST,m_fEnableNotificationBar);

			m_DisplayFontCur=m_DisplayFont;
			SetFontInfo(hDlg,IDC_DISPLAYMENU_FONT_INFO,&m_DisplayFont);
			DlgCheckBox_Check(hDlg,IDC_DISPLAYMENU_AUTOFONTSIZE,m_fDisplayFontAutoSize);
		}
		return TRUE;

#if 0
	case WM_DRAWITEM:
		{
			LPDRAWITEMSTRUCT pdis=reinterpret_cast<LPDRAWITEMSTRUCT>(lParam);
			RECT rc;

			rc=pdis->rcItem;
			::DrawEdge(pdis->hDC,&rc,BDR_SUNKENOUTER,BF_RECT | BF_ADJUST);
			DrawUtil::Fill(pdis->hDC,&rc,m_CurTextColor);
		}
		return TRUE;
#endif

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_OSDOPTIONS_SHOWOSD:
			EnableDlgItemsSyncCheckBox(hDlg,IDC_OSDOPTIONS_FIRST,IDC_OSDOPTIONS_LAST,
									   IDC_OSDOPTIONS_SHOWOSD);
			return TRUE;

		case IDC_OSDOPTIONS_TEXTCOLOR:
			if (ChooseColorDialog(hDlg,&m_CurTextColor))
				InvalidateDlgItem(hDlg,IDC_OSDOPTIONS_TEXTCOLOR);
			return TRUE;

		case IDC_OSDOPTIONS_OSDFONT_CHOOSE:
			{
				CHOOSEFONT cf;
				LOGFONT lf;

				lf=m_CurOSDFont;
				lf.lfWidth=0;
				lf.lfHeight=-PointsToPixels(11);
				cf.lStructSize=sizeof(CHOOSEFONT);
				cf.hwndOwner=hDlg;
				cf.lpLogFont=&lf;
				cf.Flags=CF_FORCEFONTEXIST | CF_INITTOLOGFONTSTRUCT | CF_SCREENFONTS
					| CF_ENABLEHOOK;
				cf.lpfnHook=ChooseFontHookProc;
				if (::ChooseFont(&cf)) {
					m_CurOSDFont=lf;
					::SetDlgItemText(hDlg,IDC_OSDOPTIONS_OSDFONT_INFO,lf.lfFaceName);
				}
			}
			return TRUE;

		case IDC_NOTIFICATIONBAR_ENABLE:
			EnableDlgItemsSyncCheckBox(hDlg,IDC_NOTIFICATIONBAR_FIRST,IDC_NOTIFICATIONBAR_LAST,
									   IDC_NOTIFICATIONBAR_ENABLE);
			return TRUE;

		case IDC_NOTIFICATIONBAR_FONT_CHOOSE:
			if (ChooseFontDialog(hDlg,&m_CurNotificationBarFont))
				SetFontInfo(hDlg,IDC_NOTIFICATIONBAR_FONT_INFO,&m_CurNotificationBarFont);
			return TRUE;

		case IDC_DISPLAYMENU_FONT_CHOOSE:
			if (ChooseFontDialog(hDlg,&m_DisplayFontCur))
				SetFontInfo(hDlg,IDC_DISPLAYMENU_FONT_INFO,&m_DisplayFontCur);
			return TRUE;
		}
		return TRUE;

	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code) {
		case PSN_APPLY:
			{
				m_fShowOSD=DlgCheckBox_IsChecked(hDlg,IDC_OSDOPTIONS_SHOWOSD);
				m_fPseudoOSD=!DlgCheckBox_IsChecked(hDlg,IDC_OSDOPTIONS_COMPOSITE);
				m_TextColor=m_CurTextColor;
				m_OSDFont=m_CurOSDFont;
				m_FadeTime=::GetDlgItemInt(hDlg,IDC_OSDOPTIONS_FADETIME,NULL,FALSE)*1000;
				unsigned int EnabledOSD=0;
				if (DlgCheckBox_IsChecked(hDlg,IDC_OSDOPTIONS_SHOW_CHANNEL))
					EnabledOSD|=OSD_FLAG(OSD_CHANNEL);
				if (DlgCheckBox_IsChecked(hDlg,IDC_OSDOPTIONS_SHOW_VOLUME))
					EnabledOSD|=OSD_FLAG(OSD_VOLUME);
				if (DlgCheckBox_IsChecked(hDlg,IDC_OSDOPTIONS_SHOW_AUDIO))
					EnabledOSD|=OSD_FLAG(OSD_AUDIO);
				if (DlgCheckBox_IsChecked(hDlg,IDC_OSDOPTIONS_SHOW_RECORDING))
					EnabledOSD|=OSD_FLAG(OSD_RECORDING);
				if (DlgCheckBox_IsChecked(hDlg,IDC_OSDOPTIONS_SHOW_CHANNELNOINPUT))
					EnabledOSD|=OSD_FLAG(OSD_CHANNELNOINPUT);
				m_EnabledOSD=EnabledOSD;
				m_ChannelChangeType=(ChannelChangeType)DlgComboBox_GetCurSel(hDlg,IDC_OSDOPTIONS_CHANNELCHANGE);

				m_fEnableNotificationBar=
					DlgCheckBox_IsChecked(hDlg,IDC_NOTIFICATIONBAR_ENABLE);
				EnableNotify(NOTIFY_EVENTNAME,
					DlgCheckBox_IsChecked(hDlg,IDC_NOTIFICATIONBAR_NOTIFYEVENTNAME));
				EnableNotify(NOTIFY_TSPROCESSORERROR,
					DlgCheckBox_IsChecked(hDlg,IDC_NOTIFICATIONBAR_NOTIFYTSPROCESSORERROR));
				m_NotificationBarDuration=
					::GetDlgItemInt(hDlg,IDC_NOTIFICATIONBAR_DURATION,NULL,FALSE)*1000;
				m_NotificationBarFont=m_CurNotificationBarFont;

				m_DisplayFont=m_DisplayFontCur;
				m_fDisplayFontAutoSize=DlgCheckBox_IsChecked(hDlg,IDC_DISPLAYMENU_AUTOFONTSIZE);

				m_fChanged=true;
			}
			return TRUE;

		case NM_CUSTOMDRAW:
			{
				LPNMCUSTOMDRAW pnmcd=reinterpret_cast<LPNMCUSTOMDRAW>(lParam);

				if (pnmcd->hdr.idFrom==IDC_OSDOPTIONS_TEXTCOLOR) {
					switch (pnmcd->dwDrawStage) {
					case CDDS_PREPAINT:
						::SetWindowLongPtr(hDlg,DWLP_MSGRESULT,CDRF_NOTIFYPOSTPAINT);
						return TRUE;

					case CDDS_POSTPAINT:
						{
							HDC hdc=pnmcd->hdc;
							RECT rc=pnmcd->rc;

							bool fMargins=false;
							CUxTheme Theme;
							if (Theme.IsActive()) {
								if (Theme.Open(pnmcd->hdr.hwndFrom,L"BUTTON")) {
									MARGINS margins;
									if (Theme.GetMargins(BP_PUSHBUTTON,PBS_NORMAL,
														 TMT_CONTENTMARGINS,&margins)) {
										rc.left+=margins.cxLeftWidth+1;
										rc.top+=margins.cyTopHeight+1;
										rc.right-=margins.cxRightWidth+1;
										rc.bottom-=margins.cyBottomHeight+1;
										fMargins=true;
									}
									Theme.Close();
								}
							}
							if (!fMargins)
								::InflateRect(&rc,-6,-4);
							HGDIOBJ hOldPen=::SelectObject(hdc,::GetStockObject(BLACK_PEN));
							::Rectangle(hdc,rc.left,rc.top,rc.right,rc.bottom);
							::InflateRect(&rc,-1,-1);
							::SelectObject(hdc,::GetStockObject(WHITE_PEN));
							::Rectangle(hdc,rc.left,rc.top,rc.right,rc.bottom);
							::SelectObject(hdc,hOldPen);
							::InflateRect(&rc,-1,-1);
							DrawUtil::Fill(hdc,&rc,m_CurTextColor);
						}
						return 0;
					}
				}
			}
			break;
		}
		break;
	}

	return FALSE;
}


UINT_PTR CALLBACK COSDOptions::ChooseFontHookProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		// �T�C�Y�̍��ڂ��\���ɂ���
		ShowDlgItem(hDlg,stc3,false);
		ShowDlgItem(hDlg,cmb3,false);
		break;
	}

	return FALSE;
}
