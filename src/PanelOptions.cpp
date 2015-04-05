#include "stdafx.h"
#include "TVTest.h"
#include "AppMain.h"
#include "PanelOptions.h"
#include "DialogUtil.h"
#include "resource.h"
#include <algorithm>
#include "Common/DebugDef.h"




CPanelOptions::CPanelOptions()
	: m_fSnapAtMainWindow(true)
	, m_SnapMargin(4)
	, m_fAttachToMainWindow(true)
	, m_Opacity(100)
	, m_fSpecCaptionFont(true)
	, m_TabStyle(CPanelForm::TABSTYLE_TEXT_ONLY)
	, m_fTabTooltip(true)
	, m_fProgramInfoUseRichEdit(true)
{
	DrawUtil::GetDefaultUIFont(&m_Font);
	DrawUtil::GetSystemFont(DrawUtil::FONT_MESSAGE,&m_CaptionFont);

	static const struct {
		LPCTSTR pszID;
		LPCTSTR pszTitle;
	} DefaultItemList[] = {
		{TEXT("Information"),	TEXT("情報")},
		{TEXT("ProgramList"),	TEXT("番組表")},
		{TEXT("Channel"),		TEXT("チャンネル")},
		{TEXT("Control"),		TEXT("操作")},
		{TEXT("Caption"),		TEXT("字幕")},
	};

	m_AvailItemList.resize(lengthof(DefaultItemList));
	m_ItemList.resize(lengthof(DefaultItemList));
	for (int i=0;i<lengthof(DefaultItemList);i++) {
		m_AvailItemList[i].ID=DefaultItemList[i].pszID;
		m_AvailItemList[i].Title=DefaultItemList[i].pszTitle;
		m_AvailItemList[i].fVisible=true;
		m_ItemList[i].ID=m_AvailItemList[i].ID;
		m_ItemList[i].fVisible=true;
	}
}


CPanelOptions::~CPanelOptions()
{
	Destroy();
}


bool CPanelOptions::InitializePanelForm(CPanelForm *pPanelForm)
{
	pPanelForm->SetTabFont(&m_Font);
	pPanelForm->SetPageFont(&m_Font);
	if (m_fSpecCaptionFont) {
		CPanelForm::CPage *pCaptionPanel=pPanelForm->GetPageByID(PANEL_ID_CAPTION);
		if (pCaptionPanel!=NULL)
			pCaptionPanel->SetFont(&m_CaptionFont);
	}
	int InitialTab=GetInitialTab();
	if (InitialTab<0)
		InitialTab=pPanelForm->GetTabID(0);
	pPanelForm->SetCurPageByID(InitialTab);
	ApplyItemList(pPanelForm);
	pPanelForm->EnableTooltip(m_fTabTooltip);
	return true;
}


bool CPanelOptions::ReadSettings(CSettings &Settings)
{
	int Value;

	Settings.Read(TEXT("InfoCurTab"),&m_LastTab);
	Settings.Read(TEXT("PanelFirstTab"),&m_InitialTab);
	Settings.Read(TEXT("PanelSnapAtMainWindow"),&m_fSnapAtMainWindow);
	Settings.Read(TEXT("PanelAttachToMainWindow"),&m_fAttachToMainWindow);
	if (Settings.Read(TEXT("PanelOpacity"),&m_Opacity))
		GetAppClass().Panel.Frame.SetPanelOpacity(m_Opacity*255/100);

	// Font
	TCHAR szFont[LF_FACESIZE];
	if (Settings.Read(TEXT("PanelFontName"),szFont,LF_FACESIZE) && szFont[0]!='\0') {
		::lstrcpy(m_Font.lfFaceName,szFont);
		m_Font.lfEscapement=0;
		m_Font.lfOrientation=0;
		m_Font.lfUnderline=0;
		m_Font.lfStrikeOut=0;
		m_Font.lfCharSet=DEFAULT_CHARSET;
		m_Font.lfOutPrecision=OUT_DEFAULT_PRECIS;
		m_Font.lfClipPrecision=CLIP_DEFAULT_PRECIS;
		m_Font.lfQuality=DRAFT_QUALITY;
		m_Font.lfPitchAndFamily=DEFAULT_PITCH | FF_DONTCARE;
	}
	if (Settings.Read(TEXT("PanelFontSize"),&Value)) {
		m_Font.lfHeight=Value;
		m_Font.lfWidth=0;
	}
	if (Settings.Read(TEXT("PanelFontWeight"),&Value))
		m_Font.lfWeight=Value;
	if (Settings.Read(TEXT("PanelFontItalic"),&Value))
		m_Font.lfItalic=Value;
	Settings.Read(TEXT("CaptionPanelFontSpec"),&m_fSpecCaptionFont);
	if (!Settings.Read(TEXT("CaptionPanelFont"),&m_CaptionFont))
		m_CaptionFont=m_Font;

	if (Settings.Read(TEXT("PanelTabStyle"),&Value)
			&& Value>=CPanelForm::TABSTYLE_FIRST && Value<=CPanelForm::TABSTYLE_LAST)
		m_TabStyle=static_cast<CPanelForm::TabStyle>(Value);

	Settings.Read(TEXT("PanelTabTooltip"),&m_fTabTooltip);

	int TabCount;
	if (Settings.Read(TEXT("PanelTabCount"),&TabCount) && TabCount>0) {
		PanelItemInfoList ItemList;
		TVTest::String ID;

		for (int i=0;i<TabCount;i++) {
			TCHAR szName[32];

			StdUtil::snprintf(szName,lengthof(szName),TEXT("PanelTab%d_ID"),i);
			if (!Settings.Read(szName,&ID) || ID.empty())
				continue;

			PanelItemInfo Item;
			LPTSTR p;
			long IDNum=std::_tcstol(ID.c_str(),&p,10);
			if (*p==_T('\0')) {
				if (IDNum<PANEL_ID_FIRST || IDNum>PANEL_ID_LAST)
					continue;
				Item.ID=m_AvailItemList[IDNum].ID;
			} else {
				Item.ID=ID;
			}
			size_t j;
			for (j=0;j<ItemList.size();j++) {
				if (CompareID(ItemList[j].ID,Item.ID))
					break;
			}
			if (j<ItemList.size())
				continue;

			StdUtil::snprintf(szName,lengthof(szName),TEXT("PanelTab%d_Visible"),i);
			if (!Settings.Read(szName,&Item.fVisible))
				Item.fVisible=true;

			ItemList.push_back(Item);
		}

#if 0
		for (size_t i=0;i<m_AvailItemList.size();i++) {
			const TVTest::String &ID=m_AvailItemList[i].ID;
			if (std::find_if(ItemList.begin(),ItemList.end(),
					[&](const PanelItemInfo &Item) -> bool {
						return CompareID(Item.ID,ID); })==ItemList.end()) {
				PanelItemInfo Item;
				Item.ID=ID;
				Item.fVisible=false;
				ItemList.push_back(Item);
			}
		}
#endif

		m_ItemList=ItemList;
	}

	Settings.Read(TEXT("InfoPanelUseRichEdit"),&m_fProgramInfoUseRichEdit);

	return true;
}


bool CPanelOptions::WriteSettings(CSettings &Settings)
{
	int CurPage=GetAppClass().Panel.Form.GetCurPageID();
	if (CurPage>=0 && (size_t)CurPage<m_AvailItemList.size())
		Settings.Write(TEXT("InfoCurTab"),m_AvailItemList[CurPage].ID);
	Settings.Write(TEXT("PanelFirstTab"),m_InitialTab);
	Settings.Write(TEXT("PanelSnapAtMainWindow"),m_fSnapAtMainWindow);
	Settings.Write(TEXT("PanelAttachToMainWindow"),m_fAttachToMainWindow);
	Settings.Write(TEXT("PanelOpacity"),m_Opacity);
	Settings.Write(TEXT("PanelTabStyle"),(int)m_TabStyle);
	Settings.Write(TEXT("PanelTabTooltip"),m_fTabTooltip);

	// Font
	Settings.Write(TEXT("PanelFontName"),m_Font.lfFaceName);
	Settings.Write(TEXT("PanelFontSize"),(int)m_Font.lfHeight);
	Settings.Write(TEXT("PanelFontWeight"),(int)m_Font.lfWeight);
	Settings.Write(TEXT("PanelFontItalic"),(int)m_Font.lfItalic);
	Settings.Write(TEXT("CaptionPanelFontSpec"),m_fSpecCaptionFont);
	Settings.Write(TEXT("CaptionPanelFont"),&m_CaptionFont);

	// アイテムリスト
	Settings.Write(TEXT("PanelTabCount"),(int)m_ItemList.size());
	for (int i=0;i<(int)m_ItemList.size();i++) {
		const PanelItemInfo &Item=m_ItemList[i];
		TCHAR szName[32];

		StdUtil::snprintf(szName,lengthof(szName),TEXT("PanelTab%d_ID"),i);
		Settings.Write(szName,Item.ID);
		StdUtil::snprintf(szName,lengthof(szName),TEXT("PanelTab%d_Visible"),i);
		Settings.Write(szName,Item.fVisible);
	}

	// Information panel
	// UI未実装
	//Settings.Write(TEXT("InfoPanelUseRichEdit"),m_fProgramInfoUseRichEdit);

	return true;
}


bool CPanelOptions::Create(HWND hwndOwner)
{
	return CreateDialogWindow(hwndOwner,
							  GetAppClass().GetResourceInstance(),MAKEINTRESOURCE(IDD_OPTIONS_PANEL));
}


int CPanelOptions::GetInitialTab() const
{
	int ID=-1;

	if (!m_InitialTab.empty()) {
		LPTSTR p;
		long IDNum=std::_tcstol(m_InitialTab.c_str(),&p,10);
		if (*p==_T('\0')) {
			if (IDNum>=0 && (size_t)IDNum<m_AvailItemList.size())
				ID=IDNum;
		} else {
			ID=GetItemIDFromIDText(m_InitialTab);
		}
	}
	if (ID<0 && !m_LastTab.empty()) {
		LPTSTR p;
		long IDNum=std::_tcstol(m_LastTab.c_str(),&p,10);
		if (*p==_T('\0')) {
			if (IDNum>=0 && (size_t)IDNum<m_AvailItemList.size())
				ID=IDNum;
		} else {
			ID=GetItemIDFromIDText(m_LastTab);
		}
	}

	return ID;
}


int CPanelOptions::RegisterPanelItem(LPCTSTR pszID,LPCTSTR pszTitle)
{
	if (IsStringEmpty(pszID) || IsStringEmpty(pszTitle))
		return -1;

	PanelItemInfo Item;

	Item.ID=pszID;
	Item.Title=pszTitle;
	Item.fVisible=true;

	if (GetItemIDFromIDText(Item.ID)>=0)
		return -1;	// ID重複

	m_AvailItemList.push_back(Item);

	auto it=std::find_if(m_ItemList.begin(),m_ItemList.end(),
		[&](const PanelItemInfo &Info) -> bool { return CompareID(Info.ID,Item.ID); });
	if (it==m_ItemList.end())
		m_ItemList.push_back(Item);

	return (int)m_AvailItemList.size()-1;
}


bool CPanelOptions::SetPanelItemVisibility(int ID,bool fVisible)
{
	if (ID<0 || ID>=(int)m_AvailItemList.size())
		return false;

	m_AvailItemList[ID].fVisible=fVisible;

	const TVTest::String &IDText=m_AvailItemList[ID].ID;
	for (auto it=m_ItemList.begin();it!=m_ItemList.end();++it) {
		if (CompareID(it->ID,IDText)) {
			it->fVisible=fVisible;
			break;
		}
	}

	GetAppClass().Panel.Form.SetTabVisible(ID,fVisible);

	return true;
}


bool CPanelOptions::GetPanelItemVisibility(int ID) const
{
	if (ID<0 || ID>=(int)m_AvailItemList.size())
		return false;

	const TVTest::String &IDText=m_AvailItemList[ID].ID;
	for (auto it=m_ItemList.begin();it!=m_ItemList.end();++it) {
		if (CompareID(it->ID,IDText))
			return it->fVisible;
	}

	return m_AvailItemList[ID].fVisible;
}


bool CPanelOptions::ApplyItemList(CPanelForm *pPanelForm) const
{
	if (pPanelForm==NULL)
		return false;

	std::vector<int> TabOrder;

	TabOrder.reserve(m_AvailItemList.size());

	for (size_t i=0;i<m_ItemList.size();i++) {
		int ID=GetItemIDFromIDText(m_ItemList[i].ID);
		if (ID>=0) {
			TabOrder.push_back(ID);
			pPanelForm->SetTabVisible(ID,m_ItemList[i].fVisible);
		}
	}

	if (TabOrder.size()<m_AvailItemList.size()) {
		for (int i=0;i<(int)m_AvailItemList.size();i++) {
			if (std::find(TabOrder.begin(),TabOrder.end(),i)==TabOrder.end()) {
				TabOrder.push_back(i);
				pPanelForm->SetTabVisible(i,m_AvailItemList[i].fVisible);
			}
		}
	}

	pPanelForm->SetTabOrder(TabOrder.data(),(int)TabOrder.size());

	return true;
}


static void SetFontInfo(HWND hDlg,int ID,const LOGFONT *plf)
{
	HDC hdc;
	TCHAR szText[LF_FACESIZE+16];

	hdc=GetDC(hDlg);
	if (hdc==NULL)
		return;
	StdUtil::snprintf(szText,lengthof(szText),TEXT("%s, %d pt"),
					  plf->lfFaceName,CalcFontPointHeight(hdc,plf));
	SetDlgItemText(hDlg,ID,szText);
	ReleaseDC(hDlg,hdc);
}


INT_PTR CPanelOptions::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			DlgCheckBox_Check(hDlg,IDC_PANELOPTIONS_SNAPATMAINWINDOW,
												m_fSnapAtMainWindow);
			DlgCheckBox_Check(hDlg,IDC_PANELOPTIONS_ATTACHTOMAINWINDOW,
												m_fAttachToMainWindow);

			// Opacity
			::SendDlgItemMessage(hDlg,IDC_PANELOPTIONS_OPACITY_TB,
										TBM_SETRANGE,TRUE,MAKELPARAM(20,100));
			::SendDlgItemMessage(hDlg,IDC_PANELOPTIONS_OPACITY_TB,
										TBM_SETPOS,TRUE,m_Opacity);
			::SendDlgItemMessage(hDlg,IDC_PANELOPTIONS_OPACITY_TB,
										TBM_SETPAGESIZE,0,10);
			::SendDlgItemMessage(hDlg,IDC_PANELOPTIONS_OPACITY_TB,
										TBM_SETTICFREQ,10,0);
			::SetDlgItemInt(hDlg,IDC_PANELOPTIONS_OPACITY_EDIT,m_Opacity,TRUE);
			::SendDlgItemMessage(hDlg,IDC_PANELOPTIONS_OPACITY_UD,
										UDM_SETRANGE,0,MAKELPARAM(100,20));

			m_CurSettingFont=m_Font;
			SetFontInfo(hDlg,IDC_PANELOPTIONS_FONTINFO,&m_Font);
			DlgCheckBox_Check(hDlg,IDC_PANELOPTIONS_SPECCAPTIONFONT,m_fSpecCaptionFont);
			EnableDlgItems(hDlg,IDC_PANELOPTIONS_CAPTIONFONT_INFO,
						   IDC_PANELOPTIONS_CAPTIONFONT_CHOOSE,
						   m_fSpecCaptionFont);
			m_CurSettingCaptionFont=m_CaptionFont;
			SetFontInfo(hDlg,IDC_PANELOPTIONS_CAPTIONFONT_INFO,&m_CaptionFont);

			m_ItemListView.Attach(::GetDlgItem(hDlg,IDC_PANELOPTIONS_ITEMLIST));
			m_ItemListView.InitCheckList();
			PanelItemInfoList ItemList(m_ItemList);
			for (size_t i=0;i<m_AvailItemList.size();i++) {
				const TVTest::String &ID=m_AvailItemList[i].ID;
				if (std::find_if(ItemList.begin(),ItemList.end(),
						[&](const PanelItemInfo &Item) -> bool {
							return CompareID(Item.ID,ID); })==ItemList.end()) {
					PanelItemInfo Item;
					Item.ID=ID;
					Item.fVisible=m_AvailItemList[i].fVisible;
					ItemList.push_back(Item);
				}
			}
			int ItemCount=0;
			for (int i=0;i<(int)ItemList.size();i++) {
				const PanelItemInfo &Item=ItemList[i];
				int ID=GetItemIDFromIDText(Item.ID);
				if (ID>=0) {
					m_ItemListView.InsertItem(ItemCount,m_AvailItemList[ID].Title.c_str(),ID);
					m_ItemListView.CheckItem(ItemCount,Item.fVisible);
					ItemCount++;
				}
			}
			UpdateItemListControlsState();

			DlgComboBox_AddString(hDlg,IDC_PANELOPTIONS_FIRSTTAB,TEXT("最後に表示したタブ"));
			int Sel=0;
			if (!m_InitialTab.empty()) {
				LPTSTR p;
				long IDNum=std::_tcstol(m_InitialTab.c_str(),&p,10);
				if (*p==_T('\0')) {
					if (IDNum>=0 && (size_t)IDNum<m_AvailItemList.size())
						Sel=IDNum+1;
				}
			}
			for (size_t i=0;i<m_AvailItemList.size();i++) {
				DlgComboBox_AddString(hDlg,IDC_PANELOPTIONS_FIRSTTAB,m_AvailItemList[i].Title.c_str());
				if (Sel==0 && CompareID(m_AvailItemList[i].ID,m_InitialTab))
					Sel=(int)i+1;
			}
			DlgComboBox_SetCurSel(hDlg,IDC_PANELOPTIONS_FIRSTTAB,Sel);

			static const LPCTSTR TabStyleList[] = {
				TEXT("文字のみ"),
				TEXT("アイコンのみ"),
				TEXT("アイコンと文字"),
			};
			for (int i=0;i<lengthof(TabStyleList);i++)
				DlgComboBox_AddString(hDlg,IDC_PANELOPTIONS_TABSTYLE,TabStyleList[i]);
			DlgComboBox_SetCurSel(hDlg,IDC_PANELOPTIONS_TABSTYLE,(int)m_TabStyle);

			DlgCheckBox_Check(hDlg,IDC_PANELOPTIONS_TABTOOLTIP,m_fTabTooltip);
		}
		return TRUE;

	case WM_HSCROLL:
		if (reinterpret_cast<HWND>(lParam)==
				::GetDlgItem(hDlg,IDC_PANELOPTIONS_OPACITY_TB)) {
			SyncEditWithTrackBar(hDlg,IDC_PANELOPTIONS_OPACITY_TB,
									  IDC_PANELOPTIONS_OPACITY_EDIT);
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_PANELOPTIONS_OPACITY_EDIT:
			if (HIWORD(wParam)==EN_CHANGE)
				SyncTrackBarWithEdit(hDlg,IDC_PANELOPTIONS_OPACITY_EDIT,
										  IDC_PANELOPTIONS_OPACITY_TB);
			return TRUE;

		case IDC_PANELOPTIONS_ITEMLIST_UP:
		case IDC_PANELOPTIONS_ITEMLIST_DOWN:
			{
				int From=m_ItemListView.GetSelectedItem(),To;

				if (LOWORD(wParam)==IDC_PANELOPTIONS_ITEMLIST_UP)
					To=From-1;
				else
					To=From+1;
				if (To<0 || To>=m_ItemListView.GetItemCount())
					return TRUE;
				m_ItemListView.MoveItem(From,To);
				UpdateItemListControlsState();
			}
			return TRUE;

		case IDC_PANELOPTIONS_CHOOSEFONT:
			if (ChooseFontDialog(hDlg,&m_CurSettingFont))
				SetFontInfo(hDlg,IDC_PANELOPTIONS_FONTINFO,&m_CurSettingFont);
			return TRUE;

		case IDC_PANELOPTIONS_SPECCAPTIONFONT:
			EnableDlgItemsSyncCheckBox(hDlg,IDC_PANELOPTIONS_CAPTIONFONT_INFO,
									   IDC_PANELOPTIONS_CAPTIONFONT_CHOOSE,
									   IDC_PANELOPTIONS_SPECCAPTIONFONT);
			return TRUE;

		case IDC_PANELOPTIONS_CAPTIONFONT_CHOOSE:
			if (ChooseFontDialog(hDlg,&m_CurSettingCaptionFont))
				SetFontInfo(hDlg,IDC_PANELOPTIONS_CAPTIONFONT_INFO,&m_CurSettingCaptionFont);
			return TRUE;
		}
		return TRUE;

	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code) {
		case PSN_APPLY:
			{
				CMainPanel &Panel=GetAppClass().Panel;

				m_fSnapAtMainWindow=
					DlgCheckBox_IsChecked(hDlg,IDC_PANELOPTIONS_SNAPATMAINWINDOW);
				m_fAttachToMainWindow=
					DlgCheckBox_IsChecked(hDlg,IDC_PANELOPTIONS_ATTACHTOMAINWINDOW);
				m_Opacity=::GetDlgItemInt(hDlg,IDC_PANELOPTIONS_OPACITY_EDIT,NULL,TRUE);
				Panel.Frame.SetPanelOpacity(m_Opacity*255/100);

				bool fFontChanged=!CompareLogFont(&m_Font,&m_CurSettingFont);
				if (fFontChanged) {
					m_Font=m_CurSettingFont;
					Panel.Form.SetTabFont(&m_Font);
					Panel.Form.SetPageFont(&m_Font);
				}

				bool fChangeCaptionFont=false;
				bool fSpecCaptionFont=DlgCheckBox_IsChecked(hDlg,IDC_PANELOPTIONS_SPECCAPTIONFONT);
				if (m_fSpecCaptionFont!=fSpecCaptionFont) {
					m_fSpecCaptionFont=fSpecCaptionFont;
					fChangeCaptionFont=true;
				}
				if (!CompareLogFont(&m_CaptionFont,&m_CurSettingCaptionFont)) {
					m_CaptionFont=m_CurSettingCaptionFont;
					if (m_fSpecCaptionFont)
						fChangeCaptionFont=true;
				} else if (m_fSpecCaptionFont && fFontChanged) {
					fChangeCaptionFont=true;
				}
				if (fChangeCaptionFont) {
					CPanelForm::CPage *pCaptionPanel=Panel.Form.GetPageByID(PANEL_ID_CAPTION);
					if (pCaptionPanel!=NULL)
						pCaptionPanel->SetFont(m_fSpecCaptionFont?
											   &m_CaptionFont:&m_Font);
				}

				const int ItemCount=m_ItemListView.GetItemCount();
				m_ItemList.clear();
				m_ItemList.resize(ItemCount);
				for (int i=0;i<ItemCount;i++) {
					const PanelItemInfo &ItemInfo=m_AvailItemList[m_ItemListView.GetItemParam(i)];
					m_ItemList[i].ID=ItemInfo.ID;
					m_ItemList[i].fVisible=m_ItemListView.IsItemChecked(i);
				}
				ApplyItemList(&Panel.Form);

				int InitialTab=(int)DlgComboBox_GetCurSel(hDlg,IDC_PANELOPTIONS_FIRSTTAB);
				if (InitialTab==0) {
					m_InitialTab.clear();
				} else if (InitialTab>0) {
					m_InitialTab=m_AvailItemList[InitialTab-1].ID;
				}

				int TabStyleSel=(int)DlgComboBox_GetCurSel(hDlg,IDC_PANELOPTIONS_TABSTYLE);
				if (TabStyleSel>=0) {
					m_TabStyle=static_cast<CPanelForm::TabStyle>(TabStyleSel);
					Panel.Form.SetTabStyle(m_TabStyle);
				}

				m_fTabTooltip=
					DlgCheckBox_IsChecked(hDlg,IDC_PANELOPTIONS_TABTOOLTIP);
				Panel.Form.EnableTooltip(m_fTabTooltip);

				m_fChanged=true;
			}
			break;

		case LVN_ITEMCHANGED:
			UpdateItemListControlsState();
			return TRUE;
		}
		break;

	case WM_DESTROY:
		m_ItemListView.Detach();
		return TRUE;
	}

	return FALSE;
}


void CPanelOptions::UpdateItemListControlsState()
{
	int Sel=m_ItemListView.GetSelectedItem();

	::EnableDlgItem(m_hDlg,IDC_PANELOPTIONS_ITEMLIST_UP,Sel>0);
	::EnableDlgItem(m_hDlg,IDC_PANELOPTIONS_ITEMLIST_DOWN,
					Sel>=0 && Sel+1<m_ItemListView.GetItemCount());
}


int CPanelOptions::GetItemIDFromIDText(const TVTest::String &IDText) const
{
	for (int i=0;i<(int)m_AvailItemList.size();i++) {
		if (CompareID(m_AvailItemList[i].ID,IDText))
			return i;
	}

	return -1;
}
