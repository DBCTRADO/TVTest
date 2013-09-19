#ifndef PROGRAM_LIST_PANEL_H
#define PROGRAM_LIST_PANEL_H


#include "PanelForm.h"
#include "EpgProgramList.h"
#include "EpgUtil.h"
#include "Theme.h"
#include "DrawUtil.h"
#include "EventInfoPopup.h"


class CProgramItemInfo;

class CProgramItemList
{
	int m_NumItems;
	CProgramItemInfo **m_ppItemList;
	int m_ItemListLength;
	void SortSub(CProgramItemInfo **ppFirst,CProgramItemInfo **ppLast);

public:
	CProgramItemList();
	~CProgramItemList();
	int NumItems() const { return m_NumItems; }
	CProgramItemInfo *GetItem(int Index);
	const CProgramItemInfo *GetItem(int Index) const;
	bool Add(CProgramItemInfo *pItem);
	void Clear();
	void Sort();
	void Reserve(int NumItems);
	void Attach(CProgramItemList *pList);
};

class CProgramListPanel : public CPanelForm::CPage
{
public:
	struct ThemeInfo {
		Theme::Style EventNameStyle;
		Theme::Style CurEventNameStyle;
		Theme::Style EventTextStyle;
		Theme::Style CurEventTextStyle;
		COLORREF MarginColor;
	};

	static bool Initialize(HINSTANCE hinst);

	CProgramListPanel();
	~CProgramListPanel();
// CBasicWindow
	bool Create(HWND hwndParent,DWORD Style,DWORD ExStyle=0,int ID=0) override;
// CProgramListPanel
	void SetEpgProgramList(CEpgProgramList *pList) { m_pProgramList=pList; }
	bool UpdateProgramList(WORD NetworkID,WORD TransportStreamID,WORD ServiceID);
	bool OnProgramListChanged();
	void ClearProgramList();
	void SetCurrentEventID(int EventID);
	bool SetTheme(const ThemeInfo *pTheme);
	bool GetTheme(ThemeInfo *pTheme) const;
	bool SetFont(const LOGFONT *pFont);
	bool SetEventInfoFont(const LOGFONT *pFont);
	void ShowRetrievingMessage(bool fShow);
	void SetVisibleEventIcons(UINT VisibleIcons);

private:
	CEpgProgramList *m_pProgramList;
	DrawUtil::CFont m_Font;
	DrawUtil::CFont m_TitleFont;
	int m_FontHeight;
	int m_LineMargin;
	int m_TitleMargin;
	ThemeInfo m_Theme;
	CEpgIcons m_EpgIcons;
	UINT m_VisibleEventIcons;
	int m_TotalLines;
	CProgramItemList m_ItemList;
	int m_CurEventID;
	int m_ScrollPos;
	//HWND m_hwndToolTip;
	CEventInfoPopup m_EventInfoPopup;
	CEventInfoPopupManager m_EventInfoPopupManager;
	class CEventInfoPopupHandler : public CEventInfoPopupManager::CEventHandler
	{
		CProgramListPanel *m_pPanel;
	public:
		CEventInfoPopupHandler(CProgramListPanel *pPanel);
		bool HitTest(int x,int y,LPARAM *pParam);
		bool GetEventInfo(LPARAM Param,const CEventInfoData **ppInfo);
	};
	CEventInfoPopupHandler m_EventInfoPopupHandler;
	bool m_fShowRetrievingMessage;

	static const LPCTSTR m_pszClassName;
	static HINSTANCE m_hinst;

	void DrawProgramList(HDC hdc,const RECT *prcPaint);
	bool UpdateListInfo(WORD NetworkID,WORD TransportStreamID,WORD ServiceID);
	void CalcDimensions();
	void SetScrollPos(int Pos);
	void SetScrollBar();
	void CalcFontHeight();
	int HitTest(int x,int y) const;
	//void SetToolTip();
// CCustomWindow
	LRESULT OnMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam) override;
};


#endif
