#ifndef STATUS_VIEW_H
#define STATUS_VIEW_H


#include <vector>
#include "BasicWindow.h"
#include "UIBase.h"
#include "TsUtilClass.h"
#include "Theme.h"
#include "DrawUtil.h"
#include "WindowUtil.h"
#include "Aero.h"


class CStatusView;

class ABSTRACT_CLASS(CStatusItem)
{
protected:
	CStatusView *m_pStatus;
	int m_ID;
	int m_DefaultWidth;
	int m_Width;
	int m_MinWidth;
	bool m_fVisible;
	bool m_fBreak;

	bool GetMenuPos(POINT *pPos,UINT *pFlags);
	enum {
		DRAWTEXT_HCENTER = 0x00000001UL
	};
	void DrawText(HDC hdc,const RECT &Rect,LPCTSTR pszText,DWORD Flags=0) const;
	void DrawIcon(HDC hdc,const RECT &Rect,DrawUtil::CMonoColorIconList &IconList,
				  int IconIndex=0,bool fEnabled=true) const;

public:
	CStatusItem(int ID,int DefaultWidth);
	virtual ~CStatusItem() {}
	int GetIndex() const;
	bool GetRect(RECT *pRect) const;
	bool GetClientRect(RECT *pRect) const;
	int GetID() const { return m_ID; }
	int GetDefaultWidth() const { return m_DefaultWidth; }
	int GetWidth() const { return m_Width; }
	bool SetWidth(int Width);
	int GetMinWidth() const { return m_MinWidth; }
	void SetVisible(bool fVisible);
	bool GetVisible() const { return m_fVisible; }
	bool Update();
	virtual LPCTSTR GetName() const=0;
	virtual void Draw(HDC hdc,const RECT &ItemRect,const RECT &DrawRect)=0;
	virtual void DrawPreview(HDC hdc,const RECT &ItemRect,const RECT &DrawRect) { Draw(hdc,ItemRect,DrawRect); }
	virtual void OnLButtonDown(int x,int y) {}
	virtual void OnRButtonDown(int x,int y) { OnLButtonDown(x,y); }
	virtual void OnLButtonDoubleClick(int x,int y) { OnLButtonDown(x,y); }
	virtual void OnMouseMove(int x,int y) {}
	virtual void OnVisibleChange(bool fVisible) {}
	virtual void OnFocus(bool fFocus) {}
	virtual bool OnMouseHover(int x,int y) { return false; }
	virtual LRESULT OnNotifyMessage(LPNMHDR pnmh) { return 0; }
	virtual void SetStyle(const TVTest::Style::CStyleManager *pStyleManager) {}
	virtual void NormalizeStyle(const TVTest::Style::CStyleManager *pStyleManager) {}

	friend CStatusView;
};

class CIconStatusItem : public CStatusItem
{
public:
	CIconStatusItem(int ID,int DefaultWidth);
	void NormalizeStyle(const TVTest::Style::CStyleManager *pStyleManager) override;
};

class CStatusView
	: public CCustomWindow
	, public TVTest::CUIBase
	, public CTracer
{
public:
	class ABSTRACT_CLASS(CEventHandler)
	{
	protected:
		CStatusView *m_pStatusView;

	public:
		CEventHandler();
		virtual ~CEventHandler();
		virtual void OnMouseLeave() {}
		virtual void OnHeightChanged(int Height) {}
		friend CStatusView;
	};

	struct ThemeInfo {
		Theme::Style ItemStyle;
		Theme::Style HighlightItemStyle;
		Theme::Style BottomItemStyle;
		Theme::BorderInfo Border;
	};

	static bool Initialize(HINSTANCE hinst);
	CStatusView();
	~CStatusView();

// CBasicWindow
	bool Create(HWND hwndParent,DWORD Style,DWORD ExStyle=0,int ID=0) override;
	void SetVisible(bool fVisible) override;

// CUIBase
	void SetStyle(const TVTest::Style::CStyleManager *pStyleManager) override;
	void NormalizeStyle(const TVTest::Style::CStyleManager *pStyleManager) override;

// CStatusView
	int NumItems() const { return (int)m_ItemList.size(); }
	const CStatusItem *GetItem(int Index) const;
	CStatusItem *GetItem(int Index);
	const CStatusItem *GetItemByID(int ID) const;
	CStatusItem *GetItemByID(int ID);
	bool AddItem(CStatusItem *pItem);
	int IDToIndex(int ID) const;
	int IndexToID(int Index) const;
	void UpdateItem(int ID);
	bool GetItemRect(int ID,RECT *pRect) const;
	bool GetItemRectByIndex(int Index,RECT *pRect) const;
	bool GetItemClientRect(int ID,RECT *pRect) const;
	int GetItemHeight() const;
	const TVTest::Style::Margins &GetItemPadding() const;
	const TVTest::Style::Size &GetIconSize() const;
	int GetFontHeight() const { return m_FontHeight; }
	int GetIntegralWidth() const;
	void SetSingleText(LPCTSTR pszText);
	bool SetTheme(const ThemeInfo *pTheme);
	bool GetTheme(ThemeInfo *pTheme) const;
	bool SetFont(const LOGFONT *pFont);
	bool GetFont(LOGFONT *pFont) const;
	bool SetMultiRow(bool fMultiRow);
	bool SetMaxRows(int MaxRows);
	int CalcHeight(int Width) const;
	int GetCurItem() const;
	bool SetEventHandler(CEventHandler *pEventHandler);
	bool SetItemOrder(const int *pOrderList);
	bool DrawItemPreview(CStatusItem *pItem,HDC hdc,const RECT &ItemRect,
						 bool fHighlight=false,HFONT hfont=NULL) const;
	bool EnableBufferedPaint(bool fEnable);
	void EnableSizeAdjustment(bool fEnable);

// CTracer
	void OnTrace(LPCTSTR pszOutput) override;

private:
	struct StatusViewStyle
	{
		TVTest::Style::Margins ItemPadding;
		TVTest::Style::IntValue TextExtraHeight;
		TVTest::Style::Size IconSize;

		StatusViewStyle();
		void SetStyle(const TVTest::Style::CStyleManager *pStyleManager);
		void NormalizeStyle(const TVTest::Style::CStyleManager *pStyleManager);
	};

	static const LPCTSTR CLASS_NAME;
	static HINSTANCE m_hinst;

	StatusViewStyle m_Style;
	DrawUtil::CFont m_Font;
	int m_FontHeight;
	int m_ItemHeight;
	bool m_fMultiRow;
	int m_MaxRows;
	int m_Rows;
	ThemeInfo m_Theme;
	std::vector<CStatusItem*> m_ItemList;
	bool m_fSingleMode;
	CDynamicString m_SingleText;
	int m_HotItem;
	CMouseLeaveTrack m_MouseLeaveTrack;
	bool m_fOnButtonDown;
	CEventHandler *m_pEventHandler;
	DrawUtil::COffscreen m_Offscreen;
	bool m_fBufferedPaint;
	CBufferedPaint m_BufferedPaint;
	bool m_fAdjustSize;

	void SetHotItem(int Item);
	void Draw(HDC hdc,const RECT *pPaintRect);
	void AdjustSize();
	void CalcRows();
	int CalcFontHeight() const;
	int CalcItemHeight() const;

// CCustomWindow
	LRESULT OnMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam) override;
};


#endif
