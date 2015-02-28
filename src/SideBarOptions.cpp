#include "stdafx.h"
#include "TVTest.h"
#include "SideBarOptions.h"
#include "AppMain.h"
#include "DialogUtil.h"
#include "resource.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


#define ZOOM_ICON_FIRST		37

static const CSideBar::SideBarItem ItemList[] = {
	{CM_ZOOM_20,				ZOOM_ICON_FIRST+0},
	{CM_ZOOM_25,				ZOOM_ICON_FIRST+1},
	{CM_ZOOM_33,				ZOOM_ICON_FIRST+2},
	{CM_ZOOM_50,				ZOOM_ICON_FIRST+3},
	{CM_ZOOM_66,				ZOOM_ICON_FIRST+4},
	{CM_ZOOM_75,				ZOOM_ICON_FIRST+5},
	{CM_ZOOM_100,				ZOOM_ICON_FIRST+6},
	{CM_ZOOM_150,				ZOOM_ICON_FIRST+7},
	{CM_ZOOM_200,				ZOOM_ICON_FIRST+8},
	{CM_ZOOM_250,				ZOOM_ICON_FIRST+9},
	{CM_ZOOM_300,				ZOOM_ICON_FIRST+10},
	{CM_CUSTOMZOOM_FIRST+0,		ZOOM_ICON_FIRST+11},
	{CM_CUSTOMZOOM_FIRST+1,		ZOOM_ICON_FIRST+12},
	{CM_CUSTOMZOOM_FIRST+2,		ZOOM_ICON_FIRST+13},
	{CM_CUSTOMZOOM_FIRST+3,		ZOOM_ICON_FIRST+14},
	{CM_CUSTOMZOOM_FIRST+4,		ZOOM_ICON_FIRST+15},
	{CM_CUSTOMZOOM_FIRST+5,		ZOOM_ICON_FIRST+16},
	{CM_CUSTOMZOOM_FIRST+6,		ZOOM_ICON_FIRST+17},
	{CM_CUSTOMZOOM_FIRST+7,		ZOOM_ICON_FIRST+18},
	{CM_CUSTOMZOOM_FIRST+8,		ZOOM_ICON_FIRST+19},
	{CM_CUSTOMZOOM_FIRST+9,		ZOOM_ICON_FIRST+20},
	{CM_ASPECTRATIO_DEFAULT,	0},
	{CM_ASPECTRATIO_16x9,		1},
	{CM_ASPECTRATIO_LETTERBOX,	2},
	{CM_ASPECTRATIO_SUPERFRAME,	3},
	{CM_ASPECTRATIO_SIDECUT,	4},
	{CM_ASPECTRATIO_4x3,		5},
	{CM_FULLSCREEN,				6},
	{CM_ALWAYSONTOP,			7},
	{CM_DISABLEVIEWER,			8},
	{CM_CAPTURE,				9},
	{CM_SAVEIMAGE,				10},
	{CM_COPY,					11},
	{CM_CAPTUREPREVIEW,			12},
	{CM_RESET,					13},
	{CM_RESETVIEWER,			14},
	{CM_PANEL,					15},
	{CM_PROGRAMGUIDE,			16},
	{CM_STATUSBAR,				17},
	{CM_VIDEODECODERPROPERTY,	18},
	{CM_OPTIONS,				19},
	{CM_STREAMINFO,				20},
	{CM_SPDIF_TOGGLE,			21},
	{CM_HOMEDISPLAY,			22},
	{CM_CHANNELDISPLAY,			23},
	{CM_1SEGMODE,				24},
	{CM_CHANNELNO_1,			25},
	{CM_CHANNELNO_2,			26},
	{CM_CHANNELNO_3,			27},
	{CM_CHANNELNO_4,			28},
	{CM_CHANNELNO_5,			29},
	{CM_CHANNELNO_6,			30},
	{CM_CHANNELNO_7,			31},
	{CM_CHANNELNO_8,			32},
	{CM_CHANNELNO_9,			33},
	{CM_CHANNELNO_10,			34},
	{CM_CHANNELNO_11,			35},
	{CM_CHANNELNO_12,			36},
};

static const int DefaultItemList[] = {
#ifndef TVTEST_FOR_1SEG
	CM_ZOOM_25,
	CM_ZOOM_33,
#endif
	CM_ZOOM_50,
	CM_ZOOM_100,
#ifdef TVTEST_FOR_1SEG
	CM_ZOOM_150,
	CM_ZOOM_200,
#endif
	0,
	CM_FULLSCREEN,
	CM_ALWAYSONTOP,
	CM_DISABLEVIEWER,
	0,
	CM_PANEL,
	CM_PROGRAMGUIDE,
	CM_CHANNELDISPLAY,
	CM_OPTIONS,
};


CSideBarOptions::CSideBarOptions(CSideBar *pSideBar,const CZoomOptions *pZoomOptions)
	: COptions(TEXT("SideBar"))
	, m_pSideBar(pSideBar)
	, m_pZoomOptions(pZoomOptions)
	, m_fShowPopup(true)
	, m_fShowToolTips(true)
	, m_fShowChannelLogo(true)
	, m_Place(PLACE_LEFT)
	, m_himlIcons(NULL)
{
	m_AvailItemList.resize(lengthof(ItemList));
	for (int i=0;i<lengthof(ItemList);i++)
		m_AvailItemList[i]=ItemList[i];

	m_ItemList.resize(lengthof(DefaultItemList));
	for (int i=0;i<lengthof(DefaultItemList);i++)
		m_ItemList[i]=DefaultItemList[i];
}


CSideBarOptions::~CSideBarOptions()
{
	Destroy();
}


bool CSideBarOptions::ReadSettings(CSettings &Settings)
{
	int Value,NumItems;

	Settings.Read(TEXT("ShowPopup"),&m_fShowPopup);
	Settings.Read(TEXT("ShowToolTips"),&m_fShowToolTips);
	Settings.Read(TEXT("ShowChannelLogo"),&m_fShowChannelLogo);
	if (Settings.Read(TEXT("Place"),&Value)
			&& Value>=PLACE_FIRST && Value<=PLACE_LAST)
		m_Place=(PlaceType)Value;

	if (Settings.Read(TEXT("ItemCount"),&NumItems) && NumItems>0) {
		// �͂܂�̂�h�����߂ɁA200������ɂ��Ă���
		if (NumItems>=200)
			NumItems=200;

		m_ItemNameList.clear();

		TVTest::String Command;

		for (int i=0;i<NumItems;i++) {
			TCHAR szName[32];

			StdUtil::snprintf(szName,lengthof(szName),TEXT("Item%d"),i);
			if (Settings.Read(szName,&Command)) {
				m_ItemNameList.push_back(Command);
				/*
				if (szCommand[0]=='\0') {
					m_ItemNameList.push_back(TVTest::String());
				} else {
					m_ItemNameList.push_back(TVTest::String(szCommand));
					int Command=m_pSideBar->GetCommandList()->ParseText(szCommand);

					if (Command!=0) {
						for (int j=0;j<lengthof(ItemList);j++) {
							if (ItemList[j].Command==Command) {
								m_ItemList.push_back(Command);
								break;
							}
						}
					}
				}
				*/
			}
		}
	}

	return true;
}


bool CSideBarOptions::WriteSettings(CSettings &Settings)
{
	Settings.Clear();

	Settings.Write(TEXT("ShowPopup"),m_fShowPopup);
	Settings.Write(TEXT("ShowToolTips"),m_fShowToolTips);
	Settings.Write(TEXT("ShowChannelLogo"),m_fShowChannelLogo);
	Settings.Write(TEXT("Place"),(int)m_Place);

	Settings.Write(TEXT("ItemCount"),(int)m_ItemNameList.size());
	for (size_t i=0;i<m_ItemNameList.size();i++) {
		TCHAR szName[32];
		StdUtil::snprintf(szName,lengthof(szName),TEXT("Item%d"),(int)i);
		Settings.Write(szName,m_ItemNameList[i]);
	}
	/*
	const CCommandList *pCommandList=m_pSideBar->GetCommandList();
	for (size_t i=0;i<m_ItemList.size();i++) {
		TCHAR szName[32];

		StdUtil::snprintf(szName,lengthof(szName),TEXT("Item%d"),i);
		if (m_ItemList[i]==ITEM_SEPARATOR)
			Settings.Write(szName,TEXT(""));
		else
			Settings.Write(szName,pCommandList->GetCommandTextByID(m_ItemList[i]));
	}
	*/

	return true;
}


bool CSideBarOptions::Create(HWND hwndOwner)
{
	return CreateDialogWindow(hwndOwner,
							  GetAppClass().GetResourceInstance(),MAKEINTRESOURCE(IDD_OPTIONS_SIDEBAR));
}


HBITMAP CSideBarOptions::CreateImage(IconSizeType SizeType,SIZE *pIconSize)
{
	LPCTSTR pszImageName,pszZoomImageName;
	int IconWidth,IconHeight;
	int NumWidth1,NumWidth2,NumWidth3,NumHeight3;
	int NumMargin1,NumMargin2,NumMargin3;
	int PercentWidth1,PercentWidth2;

	if (SizeType==ICON_SIZE_SMALL) {
		pszImageName=MAKEINTRESOURCE(IDB_SIDEBAR);
		pszZoomImageName=MAKEINTRESOURCE(IDB_SIDEBARZOOM);
		IconWidth=16;
		IconHeight=16;
		NumWidth1=4;
		NumWidth2=3;
		NumWidth3=3;
		NumHeight3=8;
		NumMargin1=1;
		NumMargin2=1;
		NumMargin3=1;
		PercentWidth1=6;
		PercentWidth2=4;
	} else {
		pszImageName=MAKEINTRESOURCE(IDB_SIDEBAR32);
		pszZoomImageName=MAKEINTRESOURCE(IDB_SIDEBARZOOM32);
		IconWidth=32;
		IconHeight=32;
		NumWidth1=8;
		NumWidth2=6;
		NumWidth3=6;
		NumHeight3=16;
		NumMargin1=2;
		NumMargin2=2;
		NumMargin3=2;
		PercentWidth1=12;
		PercentWidth2=8;
	}

	if (pIconSize!=NULL) {
		pIconSize->cx=IconWidth;
		pIconSize->cy=IconHeight;
	}

	HINSTANCE hinst=GetAppClass().GetResourceInstance();
	HBITMAP hbm=(HBITMAP)::LoadImage(hinst,pszImageName,
									 IMAGE_BITMAP,0,0,LR_CREATEDIBSECTION);

	if (hbm!=NULL) {
		// �\���{���̃A�C�R����`�悷��
		HBITMAP hbmZoom=(HBITMAP)::LoadImage(hinst,pszZoomImageName,
											 IMAGE_BITMAP,0,0,LR_CREATEDIBSECTION);

		if (hbmZoom!=NULL) {
			HDC hdcDst=::CreateCompatibleDC(NULL);
			HBITMAP hbmDstOld=static_cast<HBITMAP>(::SelectObject(hdcDst,hbm));
			HDC hdcSrc=::CreateCompatibleDC(NULL);
			HBITMAP hbmSrcOld=static_cast<HBITMAP>(::SelectObject(hdcSrc,hbmZoom));

			for (int i=0;i<CZoomOptions::NUM_ZOOM_COMMANDS;i++) {
				CZoomOptions::ZoomInfo Zoom;
				if (m_pZoomOptions->GetZoomInfoByCommand(ItemList[i].Command,&Zoom)) {
					RECT rc;
					rc.left=(ZOOM_ICON_FIRST+i)*IconWidth;
					rc.top=0;
					rc.right=rc.left+IconWidth;
					rc.bottom=IconHeight;
					if (Zoom.Type==CZoomOptions::ZOOM_RATE) {
						int Percentage=Zoom.Rate.GetPercentage();
						if (Percentage<1000) {
							::FillRect(hdcDst,&rc,static_cast<HBRUSH>(::GetStockObject(BLACK_BRUSH)));
							if (Percentage<100) {
								if (Percentage>=10)
									::BitBlt(hdcDst,rc.left,0,NumWidth1,IconHeight,
											 hdcSrc,(Percentage/10)*NumWidth1,0,SRCCOPY);
								::BitBlt(hdcDst,rc.left+NumWidth1+NumMargin1,0,NumWidth1,IconHeight,
										 hdcSrc,(Percentage%10)*NumWidth1,0,SRCCOPY);
								// %
								::BitBlt(hdcDst,
										 (ZOOM_ICON_FIRST+i)*IconWidth+(NumWidth1+NumMargin1)*2,0,
										 PercentWidth1,IconHeight,
										 hdcSrc,NumWidth1*10,0,SRCCOPY);
							} else {
								for (int j=0;j<3;j++) {
									::BitBlt(hdcDst,
											 rc.left+(2-j)*(NumWidth2+NumMargin2),0,
											 NumWidth2,IconHeight,
											 hdcSrc,(Percentage%10)*NumWidth2,IconHeight,SRCCOPY);
									Percentage/=10;
								}
								// %
								::BitBlt(hdcDst,
										 rc.left+(NumWidth2+NumMargin2)*3,0,
										 PercentWidth2,IconHeight,
										 hdcSrc,NumWidth2*10,IconHeight,SRCCOPY);
							}
						}
					} else if (Zoom.Type==CZoomOptions::ZOOM_SIZE) {
						if (Zoom.Size.Width<10000 && Zoom.Size.Height<10000) {
							::FillRect(hdcDst,&rc,static_cast<HBRUSH>(::GetStockObject(BLACK_BRUSH)));
							for (int j=0;j<2;j++) {
								int Size=j==0?Zoom.Size.Width:Zoom.Size.Height;
								for (int k=0;k<4 && Size!=0;k++) {
									::BitBlt(hdcDst,
											 rc.left+(3-k)*(NumWidth3+NumMargin3),j*IconHeight/2,
											 NumWidth3,NumHeight3,
											 hdcSrc,(Size%10)*NumWidth3,IconHeight*2,SRCCOPY);
									Size/=10;
								}
							}
						}
					}
				}
			}

			::SelectObject(hdcSrc,hbmSrcOld);
			::DeleteDC(hdcSrc);
			::SelectObject(hdcDst,hbmDstOld);
			::DeleteDC(hdcDst);
			::DeleteObject(hbmZoom);
		}
	}

	return hbm;
}


bool CSideBarOptions::ApplySideBarOptions()
{
	m_pSideBar->ShowToolTips(m_fShowToolTips);
	m_pSideBar->SetVertical(m_Place==PLACE_LEFT || m_Place==PLACE_RIGHT);
	return true;
}


void CSideBarOptions::ApplyItemList()
{
	const CCommandList *pCommandList=m_pSideBar->GetCommandList();

	if (!m_ItemNameList.empty()) {
		m_ItemList.clear();

		for (size_t i=0;i<m_ItemNameList.size();i++) {
			if (m_ItemNameList[i].empty()) {
				m_ItemList.push_back(ITEM_SEPARATOR);
			} else {
				int ID=pCommandList->ParseText(m_ItemNameList[i].c_str());
				if (ID!=0)
					m_ItemList.push_back(ID);
			}
		}
	}

	m_pSideBar->DeleteAllItems();

	for (size_t i=0;i<m_ItemList.size();i++) {
		const int Command=m_ItemList[i];

		if (Command==ITEM_SEPARATOR) {
			m_pSideBar->AddSeparator();
		} else {
			for (size_t j=0;j<m_AvailItemList.size();j++) {
				if (m_AvailItemList[j].Command==Command) {
					CSideBar::SideBarItem Item;

					Item.Command=Command;
					Item.Icon=m_AvailItemList[j].Icon;
					Item.State=0;
					unsigned int State=pCommandList->GetCommandStateByID(Command);
					if ((State & CCommandList::COMMAND_STATE_DISABLED)!=0)
						Item.State|=CSideBar::ITEM_STATE_DISABLED;
					if ((State & CCommandList::COMMAND_STATE_CHECKED)!=0)
						Item.State|=CSideBar::ITEM_STATE_CHECKED;

					m_pSideBar->AddItem(&Item);
					break;
				}
			}
		}
	}

	m_pSideBar->Invalidate();
}


bool CSideBarOptions::SetSideBarImage()
{
	TVTest::Style::Size IconSize=m_pSideBar->GetIconDrawSize();
	SIZE sz;
	HBITMAP hbm=CreateImage(
		IconSize.Width<=16 && IconSize.Height<=16?ICON_SIZE_SMALL:ICON_SIZE_BIG,
		&sz);
	if (hbm==NULL)
		return false;
	bool fResult=m_pSideBar->SetIconImage(hbm,sz.cx,sz.cy);
	::DeleteObject(hbm);
	return fResult;
}


bool CSideBarOptions::RegisterCommand(int ID)
{
	if (ID<=0 || IsAvailableItem(ID))
		return false;

	CSideBar::SideBarItem Item;

	Item.Command=ID;
	Item.Icon=-1;
	Item.State=0;

	m_AvailItemList.push_back(Item);

	return true;
}


bool CSideBarOptions::SetPlace(PlaceType Place)
{
	if (Place<PLACE_FIRST || Place>PLACE_LAST)
		return false;
	m_Place=Place;
	return true;
}


INT_PTR CSideBarOptions::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			DlgCheckBox_Check(hDlg,IDC_SIDEBAR_SHOW,m_fShowPopup);
			DlgCheckBox_Check(hDlg,IDC_SIDEBAR_SHOWTOOLTIPS,m_fShowToolTips);
			DlgCheckBox_Check(hDlg,IDC_SIDEBAR_SHOWCHANNELLOGO,m_fShowChannelLogo);

			const COLORREF IconColor=::GetSysColor(COLOR_WINDOWTEXT);
			SIZE sz;
			HBITMAP hbmIcons=CreateImage(ICON_SIZE_SMALL,&sz);
			DrawUtil::CMonoColorBitmap Bitmap;
			Bitmap.Create(hbmIcons);
			::DeleteObject(hbmIcons);
			m_himlIcons=Bitmap.CreateImageList(sz.cx,IconColor);
			Bitmap.Destroy();

			m_IconIDMap.clear();
			for (int i=0;i<lengthof(ItemList);i++)
				m_IconIDMap.insert(std::pair<int,int>(ItemList[i].Command,ItemList[i].Icon));
			const CCommandList *pCommandList=m_pSideBar->GetCommandList();
			for (size_t i=lengthof(ItemList);i<m_AvailItemList.size();i++) {
				const int ID=m_AvailItemList[i].Command;
				if (ID>=CM_PLUGIN_FIRST && ID<=CM_PLUGIN_LAST) {
					CPlugin *pPlugin=GetAppClass().PluginManager.GetPluginByCommand(ID);
					if (pPlugin!=NULL && pPlugin->GetIcon().IsCreated()) {
						HICON hIcon=pPlugin->GetIcon().ExtractIcon(IconColor);
						if (hIcon!=NULL) {
							int Icon=ImageList_AddIcon(m_himlIcons,hIcon);
							::DestroyIcon(hIcon);
							m_IconIDMap.insert(std::pair<int,int>(ID,Icon));
						}
					}
				} else if (ID>=CM_PLUGINCOMMAND_FIRST && ID<=CM_PLUGINCOMMAND_LAST) {
					LPCTSTR pszCommand;
					CPlugin *pPlugin=GetAppClass().PluginManager.GetPluginByPluginCommand(
						pCommandList->GetCommandTextByID(ID),&pszCommand);
					if (pPlugin!=NULL) {
						CPlugin::CPluginCommandInfo *pCommandInfo=
							pPlugin->GetPluginCommandInfo(pszCommand);
						if (pCommandInfo!=NULL && pCommandInfo->GetIcon().IsCreated()) {
							HICON hIcon=pCommandInfo->GetIcon().ExtractIcon(IconColor);
							if (hIcon!=NULL) {
								int Icon=ImageList_AddIcon(m_himlIcons,hIcon);
								::DestroyIcon(hIcon);
								m_IconIDMap.insert(std::pair<int,int>(ID,Icon));
							}
						}
					}
				}
			}

			HWND hwndList=::GetDlgItem(hDlg,IDC_SIDEBAR_ITEMLIST);
			ListView_SetExtendedListViewStyle(hwndList,LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP);
			ListView_SetImageList(hwndList,m_himlIcons,LVSIL_SMALL);
			RECT rc;
			::GetClientRect(hwndList,&rc);
			rc.right-=::GetSystemMetrics(SM_CXVSCROLL);
			LVCOLUMN lvc;
			lvc.mask=LVCF_FMT | LVCF_WIDTH | LVCF_SUBITEM;
			lvc.fmt=LVCFMT_LEFT;
			lvc.cx=rc.right;
			lvc.iSubItem=0;
			ListView_InsertColumn(hwndList,0,&lvc);
			SetItemList(hwndList,&m_ItemList[0],(int)m_ItemList.size());
			hwndList=::GetDlgItem(hDlg,IDC_SIDEBAR_COMMANDLIST);
			ListView_SetExtendedListViewStyle(hwndList,LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP);
			ListView_SetImageList(hwndList,m_himlIcons,LVSIL_SMALL);
			::GetClientRect(hwndList,&rc);
			rc.right-=::GetSystemMetrics(SM_CXVSCROLL);
			lvc.cx=rc.right;
			ListView_InsertColumn(hwndList,0,&lvc);
			std::vector<int> List;
			List.resize(m_AvailItemList.size());
			for (size_t i=0;i<m_AvailItemList.size();i++)
				List[i]=m_AvailItemList[i].Command;
			SetItemList(hwndList,List.data(),(int)List.size());
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_SIDEBAR_UP:
		case IDC_SIDEBAR_DOWN:
			{
				bool fUp=LOWORD(wParam)==IDC_SIDEBAR_UP;
				HWND hwndList=::GetDlgItem(hDlg,IDC_SIDEBAR_ITEMLIST);
				int Sel=ListView_GetNextItem(hwndList,-1,LVNI_SELECTED);

				if ((fUp && Sel>0) || (!fUp && Sel<ListView_GetItemCount(hwndList)-1)) {
					LVITEM lvi;
					TCHAR szText[CCommandList::MAX_COMMAND_TEXT];

					lvi.mask=LVIF_STATE | LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
					lvi.iItem=Sel;
					lvi.iSubItem=0;
					lvi.stateMask=~0U;
					lvi.pszText=szText;
					lvi.cchTextMax=lengthof(szText);
					ListView_GetItem(hwndList,&lvi);
					ListView_DeleteItem(hwndList,Sel);
					if (fUp)
						lvi.iItem--;
					else
						lvi.iItem++;
					ListView_InsertItem(hwndList,&lvi);
				}
			}
			return TRUE;

		case IDC_SIDEBAR_REMOVE:
			{
				HWND hwndList=::GetDlgItem(hDlg,IDC_SIDEBAR_ITEMLIST);
				int Sel=ListView_GetNextItem(hwndList,-1,LVNI_SELECTED);

				if (Sel>=0)
					ListView_DeleteItem(hwndList,Sel);
			}
			return TRUE;

		case IDC_SIDEBAR_DEFAULT:
			SetItemList(::GetDlgItem(hDlg,IDC_SIDEBAR_ITEMLIST),
						DefaultItemList,lengthof(DefaultItemList));
			return TRUE;

		case IDC_SIDEBAR_ADD:
			{
				HWND hwndList=::GetDlgItem(hDlg,IDC_SIDEBAR_COMMANDLIST);
				int Sel=ListView_GetNextItem(hwndList,-1,LVNI_SELECTED);

				if (Sel>=0) {
					LVITEM lvi;
					TCHAR szText[CCommandList::MAX_COMMAND_TEXT];

					lvi.mask=LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
					lvi.iItem=Sel;
					lvi.iSubItem=0;
					lvi.pszText=szText;
					lvi.cchTextMax=lengthof(szText);
					ListView_GetItem(hwndList,&lvi);
					hwndList=::GetDlgItem(hDlg,IDC_SIDEBAR_ITEMLIST);
					Sel=ListView_GetNextItem(hwndList,-1,LVNI_SELECTED);
					lvi.iItem=Sel>=0?Sel:ListView_GetItemCount(hwndList);
					lvi.state=LVIS_SELECTED | LVIS_FOCUSED;
					lvi.stateMask=LVIS_SELECTED | LVIS_FOCUSED;
					ListView_InsertItem(hwndList,&lvi);
				}
			}
			return TRUE;

		case IDC_SIDEBAR_SEPARATOR:
			{
				HWND hwndList=::GetDlgItem(hDlg,IDC_SIDEBAR_ITEMLIST);
				int Sel=ListView_GetNextItem(hwndList,-1,LVNI_SELECTED);
				LVITEM lvi;

				lvi.mask=LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
				lvi.iItem=Sel>=0?Sel:ListView_GetItemCount(hwndList);
				lvi.iSubItem=0;
				lvi.pszText=TEXT("(��؂�)");
				lvi.iImage=-1;
				lvi.lParam=ITEM_SEPARATOR;
				ListView_InsertItem(hwndList,&lvi);
			}
			return TRUE;
		}
		return TRUE;

	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code) {
		case LVN_ITEMCHANGED:
			{
				LPNMLISTVIEW pnmlv=reinterpret_cast<LPNMLISTVIEW>(lParam);
				int Sel=ListView_GetNextItem(pnmlv->hdr.hwndFrom,-1,LVNI_SELECTED);

				if (pnmlv->hdr.idFrom==IDC_SIDEBAR_ITEMLIST) {
					EnableDlgItem(hDlg,IDC_SIDEBAR_UP,Sel>0);
					EnableDlgItem(hDlg,IDC_SIDEBAR_DOWN,Sel>=0 && Sel<ListView_GetItemCount(pnmlv->hdr.hwndFrom)-1);
					EnableDlgItem(hDlg,IDC_SIDEBAR_REMOVE,Sel>=0);
				} else {
					EnableDlgItem(hDlg,IDC_SIDEBAR_ADD,Sel>=0);
				}
			}
			break;

		case NM_RCLICK:
			{
				LPNMITEMACTIVATE pnmia=reinterpret_cast<LPNMITEMACTIVATE>(lParam);

				if (pnmia->hdr.idFrom==IDC_SIDEBAR_ITEMLIST) {
					if (pnmia->iItem>=0) {
						static const int MenuIDs[] = {
							IDC_SIDEBAR_UP,
							IDC_SIDEBAR_DOWN,
							0,
							IDC_SIDEBAR_REMOVE,
						};
						PopupMenuFromControls(hDlg,MenuIDs,lengthof(MenuIDs),TPM_RIGHTBUTTON);
					}
				} else {
					if (pnmia->iItem>=0) {
						static const int MenuIDs[] = {
							IDC_SIDEBAR_ADD,
						};
						PopupMenuFromControls(hDlg,MenuIDs,lengthof(MenuIDs),TPM_RIGHTBUTTON);
					}
				}
			}
			break;

		case PSN_APPLY:
			{
				m_fShowPopup=DlgCheckBox_IsChecked(hDlg,IDC_SIDEBAR_SHOW);
				m_fShowToolTips=DlgCheckBox_IsChecked(hDlg,IDC_SIDEBAR_SHOWTOOLTIPS);
				m_pSideBar->ShowToolTips(m_fShowToolTips);
				bool fShowChannelLogo=DlgCheckBox_IsChecked(hDlg,IDC_SIDEBAR_SHOWCHANNELLOGO);
				if (m_fShowChannelLogo!=fShowChannelLogo) {
					m_fShowChannelLogo=fShowChannelLogo;
					m_pSideBar->Invalidate();
				}

				HWND hwndList=::GetDlgItem(hDlg,IDC_SIDEBAR_ITEMLIST);
				std::vector<int> ItemList;
				int i,Count;
				LVITEM lvi;

				Count=ListView_GetItemCount(hwndList);
				lvi.mask=LVIF_PARAM;
				lvi.iSubItem=0;
				for (i=0;i<Count;i++) {
					lvi.iItem=i;
					ListView_GetItem(hwndList,&lvi);
					ItemList.push_back((int)lvi.lParam);
				}
				if (ItemList!=m_ItemList) {
					m_ItemList=ItemList;

					const CCommandList *pCommandList=m_pSideBar->GetCommandList();
					m_ItemNameList.clear();
					for (int i=0;i<Count;i++) {
						const int ID=m_ItemList[i];
						if (ID==ITEM_SEPARATOR) {
							m_ItemNameList.push_back(TVTest::String());
						} else {
							m_ItemNameList.push_back(TVTest::String(pCommandList->GetCommandTextByID(ID)));
						}
					}

					ApplyItemList();
				}

				m_fChanged=true;
			}
			break;
		}
		break;

	case WM_DESTROY:
		if (m_himlIcons!=NULL) {
			::ImageList_Destroy(m_himlIcons);
			m_himlIcons=NULL;
		}
		m_IconIDMap.clear();
		return TRUE;
	}

	return FALSE;
}


void CSideBarOptions::SetItemList(HWND hwndList,const int *pList,int NumItems)
{
	const CCommandList *pCommandList=m_pSideBar->GetCommandList();
	LVITEM lvi;
	TCHAR szText[CCommandList::MAX_COMMAND_TEXT];

	ListView_DeleteAllItems(hwndList);
	lvi.mask=LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
	lvi.iSubItem=0;
	lvi.pszText=szText;

	for (int i=0;i<NumItems;i++) {
		const int ID=pList[i];

		lvi.iImage=-1;
		if (ID==ITEM_SEPARATOR) {
			::lstrcpy(szText,TEXT("(��؂�)"));
		} else {
			pCommandList->GetCommandName(pCommandList->IDToIndex(ID),szText,lengthof(szText));
			auto itr=m_IconIDMap.find(ID);
			if (itr!=m_IconIDMap.end())
				lvi.iImage=itr->second;
		}
		lvi.iItem=i;
		lvi.lParam=pList[i];
		ListView_InsertItem(hwndList,&lvi);
	}
}


bool CSideBarOptions::IsAvailableItem(int ID) const
{
	for (size_t i=0;i<m_AvailItemList.size();i++) {
		if (m_AvailItemList[i].Command==ID)
			return true;
	}

	return false;
}
