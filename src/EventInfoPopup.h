#ifndef EVENT_INFO_POPUP_H
#define EVENT_INFO_POPUP_H


#include "BasicWindow.h"
#include "EpgProgramList.h"
#include "DrawUtil.h"
#include "RichEditUtil.h"
#include "GUIUtil.h"


class CEventInfoPopup : protected CCustomWindow
{
public:
	class ABSTRACT_CLASS(CEventHandler)
	{
	protected:
		CEventInfoPopup *m_pPopup;
	public:
		enum { COMMAND_FIRST=100 };

		CEventHandler();
		virtual ~CEventHandler();
		virtual bool OnMenuPopup(HMENU hmenu) { return true; }
		virtual void OnMenuSelected(int Command) {}
		friend class CEventInfoPopup;
	};

	CEventInfoPopup();
	~CEventInfoPopup();
	bool Show(const CEventInfoData *pEventInfo,const RECT *pPos=NULL,
			  HICON hIcon=NULL,LPCTSTR pszTitle=NULL);
	bool Hide();
	bool IsVisible();
	bool IsOwnWindow(HWND hwnd) const;
	void GetSize(int *pWidth,int *pHeight);
	bool SetSize(int Width,int Height);
	void SetColor(COLORREF BackColor,COLORREF TextColor);
	void SetTitleColor(COLORREF BackColor,COLORREF TextColor);
	bool SetFont(const LOGFONT *pFont);
	void SetEventHandler(CEventHandler *pEventHandler);
	bool IsSelected() const;
	LPTSTR GetSelectedText() const;
	void GetPreferredIconSize(int *pWidth,int *pHeight) const;
	bool GetPopupPosition(int x,int y,RECT *pPos) const;
	bool AdjustPopupPosition(POINT *pPos) const;
	bool GetDefaultPopupPosition(RECT *pPos) const;
	bool GetDefaultPopupPosition(POINT *pPos) const;

	static bool Initialize(HINSTANCE hinst);

private:
	CEventInfoData m_EventInfo;
	HWND m_hwndEdit;
	CRichEditUtil m_RichEditUtil;
	COLORREF m_BackColor;
	COLORREF m_TextColor;
	COLORREF m_TitleBackColor;
	COLORREF m_TitleTextColor;
	DrawUtil::CFont m_Font;
	DrawUtil::CFont m_TitleFont;
	int m_TitleHeight;
	int m_TitleLeftMargin;
	int m_TitleIconTextMargin;
	int m_ButtonSize;
	int m_ButtonMargin;
	bool m_fDetailInfo;
	TVTest::String m_TitleText;
	TVTest::CIcon m_TitleIcon;
	CEventHandler *m_pEventHandler;

	static const LPCTSTR m_pszWindowClass;
	static HINSTANCE m_hinst;

// CCustomWindow
	LRESULT OnMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam) override;

	bool Create(HWND hwndParent,DWORD Style,DWORD ExStyle,int ID) override;
	void SetEventInfo(const CEventInfoData *pEventInfo);
	void FormatAudioInfo(const CEventInfoData::AudioInfo *pAudioInfo,
						 LPTSTR pszText,int MaxLength) const;
	void CalcTitleHeight();
	void GetCloseButtonRect(RECT *pRect) const;
	void SetNcRendering();
};

class CEventInfoPopupManager
{
public:
	class ABSTRACT_CLASS(CEventHandler)
	{
	protected:
		CEventInfoPopup *m_pPopup;
	public:
		CEventHandler();
		virtual ~CEventHandler();
		virtual bool HitTest(int x,int y,LPARAM *pParam)=0;
		virtual bool ShowPopup(LPARAM Param,CEventInfoPopup *pPopup)=0;
		friend class CEventInfoPopupManager;
	};

	CEventInfoPopupManager(CEventInfoPopup *pPopup);
	~CEventInfoPopupManager();
	bool Initialize(HWND hwnd,CEventHandler *pEventHandler);
	void Finalize();
	bool SetEnable(bool fEnable);
	bool GetEnable() const { return m_fEnable; }
	bool Popup(int x,int y);

private:
	static const LPCTSTR m_pszPropName;
	CEventInfoPopup *m_pPopup;
	bool m_fEnable;
	HWND m_hwnd;
	WNDPROC m_pOldWndProc;
	CEventHandler *m_pEventHandler;
	bool m_fTrackMouseEvent;
	LPARAM m_HitTestParam;

	static LRESULT CALLBACK HookWndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
};


#endif
