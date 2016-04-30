#ifndef TVTEST_VIEW_H
#define TVTEST_VIEW_H


#include "BasicWindow.h"
#include "UIBase.h"
#include "DtvEngine.h"
#include "Theme.h"


class ABSTRACT_CLASS(CDisplayView)
	: public CCustomWindow
	, public TVTest::CUIBase
{
public:
	class ABSTRACT_CLASS(CEventHandler)
	{
	public:
		virtual ~CEventHandler() = 0;
		virtual void OnMouseMessage(UINT Msg,int x,int y) {}
	};

	CDisplayView();
	virtual ~CDisplayView() = 0;
	virtual bool Close() = 0;
	virtual bool IsMessageNeed(const MSG *pMsg) const;
	virtual bool OnMouseWheel(UINT Msg,WPARAM wParam,LPARAM lParam) { return false; }

// CBasicWindow
	void SetVisible(bool fVisible) override;

// CUIBase
	void SetStyle(const TVTest::Style::CStyleManager *pStyleManager) override;
	void NormalizeStyle(const TVTest::Style::CStyleManager *pStyleManager) override;

protected:
	enum ItemType {
		ITEM_STYLE_NORMAL,
		ITEM_STYLE_NORMAL_1,
		ITEM_STYLE_NORMAL_2,
		ITEM_STYLE_HOT,
		ITEM_STYLE_SELECTED,
		ITEM_STYLE_CURRENT
	};

	enum BackgroundType {
		BACKGROUND_STYLE_CONTENT,
		BACKGROUND_STYLE_CATEGORIES
	};

	struct DisplayViewStyle
	{
		TVTest::Style::IntValue TextSizeRatioHorz;
		TVTest::Style::IntValue TextSizeRatioVert;
		TVTest::Style::IntValue TextSizeMin;
		TVTest::Style::Margins ContentMargin;
		TVTest::Style::Margins CategoriesMargin;
		TVTest::Style::Size CloseButtonSize;
		TVTest::Style::Margins CloseButtonMargin;

		DisplayViewStyle();
		void SetStyle(const TVTest::Style::CStyleManager *pStyleManager);
		void NormalizeStyle(const TVTest::Style::CStyleManager *pStyleManager);
	};

	class CDisplayBase *m_pDisplayBase;
	DisplayViewStyle m_Style;
	CEventHandler *m_pEventHandler;

	virtual bool OnVisibleChange(bool fVisible);
	virtual bool GetCloseButtonRect(RECT *pRect) const;
	bool CloseButtonHitTest(int x,int y) const;
	void DrawCloseButton(HDC hdc) const;
	bool GetItemStyle(ItemType Type,TVTest::Theme::Style *pStyle) const;
	bool GetBackgroundStyle(BackgroundType Type,TVTest::Theme::BackgroundStyle *pStyle) const;
	int GetDefaultFontSize(int Width,int Height) const;
	void SetEventHandler(CEventHandler *pEventHandler);
	bool HandleMessage(HWND hwnd,UINT Msg,WPARAM wParam,LPARAM lParam,LRESULT *pResult);

// CCustomWindow
	LRESULT OnMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam) override;

private:
	void SetDisplayVisible(bool fVisible);

	friend class CDisplayBase;
};

class CDisplayBase
{
public:
	class ABSTRACT_CLASS(CEventHandler) {
	public:
		virtual ~CEventHandler() = 0;
		virtual bool OnVisibleChange(bool fVisible) { return true; }
	};

	CDisplayBase();
	~CDisplayBase();
	void SetEventHandler(CEventHandler *pHandler);
	void SetParent(CBasicWindow *pWindow);
	CBasicWindow *GetParent() const { return m_pParentWindow; }
	void SetDisplayView(CDisplayView *pView);
	CDisplayView *GetDisplayView() const { return m_pDisplayView; }
	bool SetVisible(bool fVisible);
	bool IsVisible() const;
	void AdjustPosition();
	void SetPosition(int Left,int Top,int Width,int Height);
	void SetPosition(const RECT *pRect);
	void SetFocus();

private:
	CBasicWindow *m_pParentWindow;
	CDisplayView *m_pDisplayView;
	CEventHandler *m_pEventHandler;
	bool m_fVisible;
};

class CDisplayEventHandlerBase
{
protected:
	void RelayMouseMessage(CDisplayView *pView,UINT Message,int x,int y);
};

class CVideoContainerWindow : public CCustomWindow
{
public:
	class ABSTRACT_CLASS(CEventHandler) {
	protected:
		CVideoContainerWindow *m_pVideoContainer;
	public:
		CEventHandler();
		virtual ~CEventHandler();
		virtual void OnSizeChanged(int Width,int Height) {}
		friend class CVideoContainerWindow;
	};

	CVideoContainerWindow();
	~CVideoContainerWindow();
	bool Create(HWND hwndParent,DWORD Style,DWORD ExStyle,int ID,CDtvEngine *pDtvEngine);
	CDtvEngine *GetDtvEngine() { return m_pDtvEngine; }
	const CDtvEngine *GetDtvEngine() const { return m_pDtvEngine; }
	void SetDisplayBase(CDisplayBase *pDisplayBase);
	void SetEventHandler(CEventHandler *pEventHandler);

	static bool Initialize(HINSTANCE hinst);

private:
	static HINSTANCE m_hinst;

	CDtvEngine *m_pDtvEngine;
	CDisplayBase *m_pDisplayBase;
	CEventHandler *m_pEventHandler;
	SIZE m_ClientSize;

// CBasicWindow
	bool Create(HWND hwndParent,DWORD Style,DWORD ExStyle=0,int ID=0) override;
// CCustomWindow
	LRESULT OnMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam) override;
};

class CViewWindow
	: public CCustomWindow
	, public TVTest::CUIBase
{
public:
	class ABSTRACT_CLASS(CEventHandler) {
	protected:
		CViewWindow *m_pView;
	public:
		CEventHandler();
		virtual ~CEventHandler();
		virtual void OnSizeChanged(int Width,int Height) {}
		friend class CViewWindow;
	};

	CViewWindow();
	~CViewWindow();
// CBasicWindow
	bool Create(HWND hwndParent,DWORD Style,DWORD ExStyle=0,int ID=0) override;
// CViewWindow
	void SetVideoContainer(CVideoContainerWindow *pVideoContainer);
	void SetMessageWindow(HWND hwnd);
	void SetEventHandler(CEventHandler *pEventHandler);
	bool SetLogo(HBITMAP hbm);
	void SetBorder(const TVTest::Theme::BorderStyle &Style);
	void SetMargin(const TVTest::Style::Margins &Margin);
	void ShowCursor(bool fShow);
	bool CalcClientRect(RECT *pRect) const;
	bool CalcWindowRect(RECT *pRect) const;

	static bool Initialize(HINSTANCE hinst);

private:
	static HINSTANCE m_hinst;

	CVideoContainerWindow *m_pVideoContainer;
	HWND m_hwndMessage;
	CEventHandler *m_pEventHandler;
	HBITMAP m_hbmLogo;
	TVTest::Theme::BorderStyle m_BorderStyle;
	TVTest::Style::Margins m_Margin;
	bool m_fShowCursor;

// CCustomWindow
	LRESULT OnMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam) override;
};


#endif
