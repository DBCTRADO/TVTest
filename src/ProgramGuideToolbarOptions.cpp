#include "stdafx.h"
#include "TVTest.h"
#include "AppMain.h"
#include "ProgramGuideToolbarOptions.h"
#include "DialogUtil.h"
#include "resource.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif




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

		case IDOK:
			{
				int OrderList[CProgramGuideFrameSettings::TOOLBAR_NUM];

				for (int i=0;i<CProgramGuideFrameSettings::TOOLBAR_NUM;i++) {
					int ID=static_cast<int>(m_ItemListView.GetItemParam(i));

					OrderList[i]=ID;
					m_FrameSettings.SetToolbarVisible(ID,m_ItemListView.IsItemChecked(i));
				}

				m_FrameSettings.SetToolbarOrderList(OrderList);
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
