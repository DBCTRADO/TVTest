#ifndef PANEL_OPTIONS_H
#define PANEL_OPTIONS_H


#include "Options.h"
#include "Panel.h"
#include "PanelForm.h"
#include "ListView.h"
#include <vector>


enum {
	PANEL_ID_INFORMATION,
	PANEL_ID_PROGRAMLIST,
	PANEL_ID_CHANNEL,
	PANEL_ID_CONTROL,
	PANEL_ID_CAPTION,
	NUM_PANELS
};

#define PANEL_ID_FIRST	PANEL_ID_INFORMATION
#define PANEL_ID_LAST	PANEL_ID_CAPTION

class CPanelOptions : public COptions
{
public:
	CPanelOptions(CPanelFrame *pPanelFrame);
	~CPanelOptions();

// CSettingsBase
	bool ReadSettings(CSettings &Settings) override;
	bool WriteSettings(CSettings &Settings) override;

// CBasicDialog
	bool Create(HWND hwndOwner) override;

// CPanelOptions
	bool InitializePanelForm(CPanelForm *pPanelForm);
	bool GetSnapAtMainWindow() const { return m_fSnapAtMainWindow; }
	int GetSnapMargin() const { return m_SnapMargin; }
	bool GetAttachToMainWindow() const { return m_fAttachToMainWindow; }
	const LOGFONT *GetFont() const { return &m_Font; }
	int GetInitialTab() const;
	CPanelForm::TabStyle GetTabStyle() const { return m_TabStyle; }
	bool GetProgramInfoUseRichEdit() const { return m_fProgramInfoUseRichEdit; }
	int RegisterPanelItem(LPCTSTR pszID,LPCTSTR pszTitle);
	bool SetPanelItemVisibility(int ID,bool fVisible);
	bool GetPanelItemVisibility(int ID) const;
	bool ApplyItemList(CPanelForm *pPanelForm) const;

private:
	struct PanelItemInfo {
		TVTest::String ID;
		TVTest::String Title;
		bool fVisible;
	};

	typedef std::vector<PanelItemInfo> PanelItemInfoList;

	static bool CompareID(const TVTest::String &ID1,const TVTest::String &ID2) {
		return TVTest::StringUtility::CompareNoCase(ID1,ID2)==0;
	}

	CPanelFrame *m_pPanelFrame;
	bool m_fSnapAtMainWindow;
	int m_SnapMargin;
	bool m_fAttachToMainWindow;
	int m_Opacity;
	LOGFONT m_Font;
	LOGFONT m_CurSettingFont;
	bool m_fSpecCaptionFont;
	LOGFONT m_CaptionFont;
	LOGFONT m_CurSettingCaptionFont;
	TVTest::String m_InitialTab;
	TVTest::String m_LastTab;
	PanelItemInfoList m_AvailItemList;
	PanelItemInfoList m_ItemList;
	CPanelForm::TabStyle m_TabStyle;
	bool m_fTabTooltip;
	bool m_fProgramInfoUseRichEdit;
	TVTest::CListView m_ItemListView;

// CBasicDialog
	INT_PTR DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam) override;

	void UpdateItemListControlsState();
	int GetItemIDFromIDText(const TVTest::String &IDText) const;
};


#endif
