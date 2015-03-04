#include "stdafx.h"
#include "TVTest.h"
#include "AppMain.h"
#include "ProgramGuideToolbarOptions.h"
#include "DialogUtil.h"
#include "resource.h"
#include "Common/DebugDef.h"




CProgramGuideToolbarOptions::CProgramGuideToolbarOptions(CProgramGuideFrameSettings &FrameSettings)
	: m_FrameSettings(FrameSettings)
{
}


CProgramGuideToolbarOptions::~CProgramGuideToolbarOptions()
{
}


bool CProgramGuideToolbarOptions::Show(HWND hwndOwner)
{
	return ShowDialog(hwndOwner,
					  GetAppClass().GetResourceInstance(),
					  MAKEINTRESOURCE(IDD_PROGRAMGUIDETOOLBAR))==IDOK;
}


INT_PTR CProgramGuideToolbarOptions::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			m_ItemListView.Attach(::GetDlgItem(hDlg,IDC_PROGRAMGUIDETOOLBAR_ITEMLIST));
			m_ItemListView.InitCheckList();

			int Order[CProgramGuideFrameSettings::TOOLBAR_NUM];
			m_FrameSettings.GetToolbarOrderList(Order);

			for (int i=0;i<lengthof(Order);i++) {
				const int ID=Order[i];

				m_ItemListView.InsertItem(i,m_FrameSettings.GetToolbarName(ID),ID);
				m_ItemListView.CheckItem(i,m_FrameSettings.GetToolbarVisible(ID));
			}

			for (int i=1;i<=CProgramGuideFrameSettings::DATEBAR_MAXBUTTONCOUNT;i++) {
				TCHAR szText[4];
				StdUtil::snprintf(szText,lengthof(szText),TEXT("%d"),i);
				DlgComboBox_AddString(hDlg,IDC_PROGRAMGUIDETOOLBAR_DATEBAR_BUTTONCOUNT,szText);
			}
			DlgComboBox_SetCurSel(hDlg,IDC_PROGRAMGUIDETOOLBAR_DATEBAR_BUTTONCOUNT,
								  m_FrameSettings.GetDateBarButtonCount()-1);

			const CProgramGuideFrameSettings::TimeBarSettings &TimeBarSettings=
				m_FrameSettings.GetTimeBarSettings();

			::CheckRadioButton(hDlg,
				IDC_PROGRAMGUIDETOOLBAR_TIMEBAR_TIME_INTERVAL,
				IDC_PROGRAMGUIDETOOLBAR_TIMEBAR_TIME_CUSTOM,
				IDC_PROGRAMGUIDETOOLBAR_TIMEBAR_TIME_INTERVAL+TimeBarSettings.Time);
			EnableDlgItems(hDlg,
				IDC_PROGRAMGUIDETOOLBAR_TIMEBAR_INTERVAL,
				IDC_PROGRAMGUIDETOOLBAR_TIMEBAR_INTERVAL_UNIT,
				TimeBarSettings.Time==CProgramGuideFrameSettings::TimeBarSettings::TIME_INTERVAL);
			EnableDlgItem(hDlg,
				IDC_PROGRAMGUIDETOOLBAR_TIMEBAR_CUSTOMTIME,
				TimeBarSettings.Time==CProgramGuideFrameSettings::TimeBarSettings::TIME_CUSTOM);

			for (int i=CProgramGuideFrameSettings::TimeBarSettings::INTERVAL_MIN;
					i<=CProgramGuideFrameSettings::TimeBarSettings::INTERVAL_MAX;
					i++) {
				TCHAR szText[4];
				StdUtil::snprintf(szText,lengthof(szText),TEXT("%d"),i);
				DlgComboBox_AddString(hDlg,IDC_PROGRAMGUIDETOOLBAR_TIMEBAR_INTERVAL,szText);
			}
			DlgComboBox_SetCurSel(hDlg,IDC_PROGRAMGUIDETOOLBAR_TIMEBAR_INTERVAL,
								  TimeBarSettings.Interval-CProgramGuideFrameSettings::TimeBarSettings::INTERVAL_MIN);

			DlgEdit_SetText(hDlg,IDC_PROGRAMGUIDETOOLBAR_TIMEBAR_CUSTOMTIME,
							TimeBarSettings.CustomTime.c_str());

			for (int i=CProgramGuideFrameSettings::TimeBarSettings::BUTTONCOUNT_MIN;
					i<=CProgramGuideFrameSettings::TimeBarSettings::BUTTONCOUNT_MAX;
					i++) {
				TCHAR szText[4];
				StdUtil::snprintf(szText,lengthof(szText),TEXT("%d"),i);
				DlgComboBox_AddString(hDlg,IDC_PROGRAMGUIDETOOLBAR_TIMEBAR_MAXBUTTONCOUNT,szText);
			}
			DlgComboBox_SetCurSel(hDlg,IDC_PROGRAMGUIDETOOLBAR_TIMEBAR_MAXBUTTONCOUNT,
								  TimeBarSettings.MaxButtonCount-CProgramGuideFrameSettings::TimeBarSettings::BUTTONCOUNT_MIN);
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_PROGRAMGUIDETOOLBAR_ITEMLIST_UP:
		case IDC_PROGRAMGUIDETOOLBAR_ITEMLIST_DOWN:
			{
				int From=m_ItemListView.GetSelectedItem(),To;

				if (From>=0) {
					if (LOWORD(wParam)==IDC_PROGRAMGUIDETOOLBAR_ITEMLIST_UP) {
						if (From<1)
							break;
						To=From-1;
					} else {
						if (From+1>=CProgramGuideFrameSettings::TOOLBAR_NUM)
							break;
						To=From+1;
					}

					m_ItemListView.MoveItem(From,To);
					m_ItemListView.EnsureItemVisible(To);
					UpdateItemState();
				}
			}
			return TRUE;

		case IDC_PROGRAMGUIDETOOLBAR_TIMEBAR_TIME_INTERVAL:
		case IDC_PROGRAMGUIDETOOLBAR_TIMEBAR_TIME_CUSTOM:
			EnableDlgItems(hDlg,
				IDC_PROGRAMGUIDETOOLBAR_TIMEBAR_INTERVAL,
				IDC_PROGRAMGUIDETOOLBAR_TIMEBAR_INTERVAL_UNIT,
				DlgRadioButton_IsChecked(hDlg,IDC_PROGRAMGUIDETOOLBAR_TIMEBAR_TIME_INTERVAL));
			EnableDlgItem(hDlg,
				IDC_PROGRAMGUIDETOOLBAR_TIMEBAR_CUSTOMTIME,
				DlgRadioButton_IsChecked(hDlg,IDC_PROGRAMGUIDETOOLBAR_TIMEBAR_TIME_CUSTOM));
			return TRUE;

		case IDOK:
			{
				int OrderList[CProgramGuideFrameSettings::TOOLBAR_NUM];

				for (int i=0;i<CProgramGuideFrameSettings::TOOLBAR_NUM;i++) {
					int ID=static_cast<int>(m_ItemListView.GetItemParam(i));

					OrderList[i]=ID;
					m_FrameSettings.SetToolbarVisible(ID,m_ItemListView.IsItemChecked(i));
				}

				m_FrameSettings.SetToolbarOrderList(OrderList);

				m_FrameSettings.SetDateBarButtonCount(
					(int)DlgComboBox_GetCurSel(hDlg,IDC_PROGRAMGUIDETOOLBAR_DATEBAR_BUTTONCOUNT)+1);

				CProgramGuideFrameSettings::TimeBarSettings TimeBarSettings;
				if (DlgRadioButton_IsChecked(hDlg,IDC_PROGRAMGUIDETOOLBAR_TIMEBAR_TIME_INTERVAL))
					TimeBarSettings.Time=CProgramGuideFrameSettings::TimeBarSettings::TIME_INTERVAL;
				else if (DlgRadioButton_IsChecked(hDlg,IDC_PROGRAMGUIDETOOLBAR_TIMEBAR_TIME_CUSTOM))
					TimeBarSettings.Time=CProgramGuideFrameSettings::TimeBarSettings::TIME_CUSTOM;
				TimeBarSettings.Interval=
					(int)DlgComboBox_GetCurSel(hDlg,IDC_PROGRAMGUIDETOOLBAR_TIMEBAR_INTERVAL)+
						CProgramGuideFrameSettings::TimeBarSettings::INTERVAL_MIN;
				GetDlgItemString(hDlg,IDC_PROGRAMGUIDETOOLBAR_TIMEBAR_CUSTOMTIME,&TimeBarSettings.CustomTime);
				TimeBarSettings.MaxButtonCount=
					(int)DlgComboBox_GetCurSel(hDlg,IDC_PROGRAMGUIDETOOLBAR_TIMEBAR_MAXBUTTONCOUNT)+
						CProgramGuideFrameSettings::TimeBarSettings::BUTTONCOUNT_MIN;
				m_FrameSettings.SetTimeBarSettings(TimeBarSettings);
			}
		case IDCANCEL:
			::EndDialog(hDlg,LOWORD(wParam));
			return TRUE;
		}
		return TRUE;

	case WM_NOTIFY:
		switch (reinterpret_cast<LPNMHDR>(lParam)->code) {
		case LVN_ITEMCHANGED:
			UpdateItemState();
			return TRUE;
		}
		break;

	case WM_DESTROY:
		m_ItemListView.Detach();
		return TRUE;
	}

	return FALSE;
}


void CProgramGuideToolbarOptions::UpdateItemState()
{
	int Sel=m_ItemListView.GetSelectedItem();

	EnableDlgItem(m_hDlg,IDC_PROGRAMGUIDETOOLBAR_ITEMLIST_UP,Sel>0);
	EnableDlgItem(m_hDlg,IDC_PROGRAMGUIDETOOLBAR_ITEMLIST_DOWN,
				  Sel>=0 && Sel+1<m_ItemListView.GetItemCount());
}
