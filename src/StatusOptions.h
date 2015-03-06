#ifndef STATUS_OPTIONS_H
#define STATUS_OPTIONS_H


#include <vector>
#include "Options.h"
#include "StatusItems.h"
#include "WindowUtil.h"


class CStatusOptions : public COptions
{
public:
	CStatusOptions(CStatusView *pStatusView);
	~CStatusOptions();
// CSettingsBase
	bool ReadSettings(CSettings &Settings) override;
	bool WriteSettings(CSettings &Settings) override;
// CBasicDialog
	bool Create(HWND hwndOwner) override;
// CStatusOptions
	bool ApplyOptions();
	bool ApplyItemList();
	int RegisterItem(LPCTSTR pszID);
	bool SetItemVisibility(int ID,bool fVisible);
	const LOGFONT &GetFont() const { return m_lfItemFont; }
	bool GetShowTOTTime() const { return m_fShowTOTTime; }
	void SetShowTOTTime(bool fShow) { m_fShowTOTTime=fShow; }
	bool IsPopupProgramInfoEnabled() const { return m_fEnablePopupProgramInfo; }
	void EnablePopupProgramInfo(bool fEnable) { m_fEnablePopupProgramInfo=fEnable; }
	bool GetShowEventProgress() const { return m_fShowEventProgress; }
	void SetShowEventProgress(bool fShow) { m_fShowEventProgress=fShow; }

private:
	struct StatusItemInfo
	{
		int ID;
		TVTest::String IDText;
		bool fVisible;
		int Width;
	};

	typedef std::vector<StatusItemInfo> StatusItemInfoList;

	class CItemListSubclass : public CWindowSubclass
	{
	public:
		CItemListSubclass(CStatusOptions *pStatusOptions);

	private:
		LRESULT OnMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam) override;

		CStatusOptions *m_pStatusOptions;
	};

	CStatusView *m_pStatusView;
	StatusItemInfoList m_AvailItemList;
	StatusItemInfoList m_ItemList;
	StatusItemInfoList m_ItemListCur;
	int m_ItemID;
	LOGFONT m_lfItemFont;
	bool m_fMultiRow;
	int m_MaxRows;

	LOGFONT m_CurSettingFont;
	CItemListSubclass m_ItemListSubclass;
	int m_ItemHeight;
	int m_TextWidth;
	TVTest::Style::Margins m_ItemMargin;
	TVTest::Style::Size m_CheckSize;
	int m_DropInsertPos;
	UINT m_DragTimerID;
	bool m_fDragResize;

	bool m_fShowTOTTime;
	bool m_fEnablePopupProgramInfo;
	bool m_fShowEventProgress;

// CBasicDialog
	INT_PTR DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam) override;

	void InitListBox(HWND hDlg);
	void CalcTextWidth(HWND hDlg);
	void SetListHExtent(HWND hDlg);
	void DrawInsertMark(HWND hwndList,int Pos);
	bool GetItemPreviewRect(HWND hwndList,int Index,RECT *pRect);
	bool IsCursorResize(HWND hwndList,int x,int y);
	void MakeItemList(StatusItemInfoList *pList) const;
};


#endif
