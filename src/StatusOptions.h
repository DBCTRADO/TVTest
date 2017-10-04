#ifndef STATUS_OPTIONS_H
#define STATUS_OPTIONS_H


#include <vector>
#include "Options.h"
#include "StatusItems.h"
#include "WindowUtil.h"


class CStatusOptions
	: public COptions
{
public:
	static const int OPACITY_MIN = 20;
	static const int OPACITY_MAX = 100;

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
	void ApplyItemWidth();
	int RegisterItem(LPCTSTR pszID);
	bool SetItemVisibility(int ID, bool fVisible);
	const TVTest::Style::Font &GetFont() const { return m_ItemFont; }
	bool GetShowPopup() const { return m_fShowPopup; }
	int GetPopupOpacity() const { return m_PopupOpacity; }
	bool GetShowTOTTime() const { return m_fShowTOTTime; }
	void SetShowTOTTime(bool fShow) { m_fShowTOTTime = fShow; }
	bool GetInterpolateTOTTime() const { return m_fInterpolateTOTTime; }
	void SetInterpolateTOTTime(bool fInterpolate) { m_fInterpolateTOTTime = fInterpolate; }
	bool IsPopupProgramInfoEnabled() const { return m_fEnablePopupProgramInfo; }
	void EnablePopupProgramInfo(bool fEnable) { m_fEnablePopupProgramInfo = fEnable; }
	bool GetShowEventProgress() const { return m_fShowEventProgress; }
	void SetShowEventProgress(bool fShow) { m_fShowEventProgress = fShow; }
	int GetPopupEventInfoWidth() const { return m_PopupEventInfoWidth; }
	int GetPopupEventInfoHeight() const { return m_PopupEventInfoHeight; }
	void SetPopupEventInfoSize(int Width, int Height) { m_PopupEventInfoWidth = Width; m_PopupEventInfoHeight = Height; }

private:
	struct StatusItemInfo
	{
		int ID;
		TVTest::String IDText;
		bool fVisible;
		int Width;
	};

	typedef std::vector<StatusItemInfo> StatusItemInfoList;

	class CItemListSubclass
		: public CWindowSubclass
	{
	public:
		CItemListSubclass(CStatusOptions *pStatusOptions);

	private:
		LRESULT OnMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

		CStatusOptions *m_pStatusOptions;
	};

	CStatusView *m_pStatusView;
	StatusItemInfoList m_AvailItemList;
	StatusItemInfoList m_ItemList;
	StatusItemInfoList m_ItemListCur;
	int m_ItemID;
	int m_DPI;
	int m_StatusBarDPI;
	TVTest::Style::Font m_ItemFont;
	bool m_fMultiRow;
	int m_MaxRows;
	bool m_fShowPopup;
	int m_PopupOpacity;

	TVTest::Style::Font m_CurSettingFont;
	CItemListSubclass m_ItemListSubclass;
	int m_ItemHeight;
	int m_TextWidth;
	TVTest::Style::Margins m_ItemMargin;
	TVTest::Style::Size m_CheckSize;
	int m_DropInsertPos;
	UINT m_DragTimerID;
	bool m_fDragResize;

	bool m_fShowTOTTime;
	bool m_fInterpolateTOTTime;
	bool m_fEnablePopupProgramInfo;
	bool m_fShowEventProgress;
	int m_PopupEventInfoWidth;
	int m_PopupEventInfoHeight;

// CBasicDialog
	INT_PTR DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

// CUIBase
	void ApplyStyle() override;
	void RealizeStyle() override;

	void InitListBox();
	void CalcTextWidth();
	void SetListHExtent();
	void DrawInsertMark(HWND hwndList, int Pos);
	bool GetItemPreviewRect(HWND hwndList, int Index, RECT *pRect);
	bool IsCursorResize(HWND hwndList, int x, int y);
	void MakeItemList(StatusItemInfoList *pList) const;
};


#endif
