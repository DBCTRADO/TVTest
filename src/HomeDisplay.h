#ifndef HOME_DISPLAY_H
#define HOME_DISPLAY_H


#include <vector>
#include "View.h"
#include "DrawUtil.h"
#include "Theme.h"
#include "Aero.h"
#include "ProgramSearch.h"
#include "WindowUtil.h"


class CHomeDisplay
	: public CDisplayView
	, public CDoubleBufferingDraw
	, public CSettingsBase
{
public:
	struct StyleInfo {
		TVTest::Theme::BackgroundStyle CategoriesBackStyle;
		TVTest::Theme::BackgroundStyle ContentBackStyle;
		TVTest::Theme::Style CategoryItemStyle;
		TVTest::Theme::Style CategoryItemSelStyle;
		TVTest::Theme::Style CategoryItemCurStyle;
		TVTest::Theme::Style ItemStyle[2];
		TVTest::Theme::Style ItemHotStyle;
		COLORREF BannerTextColor;
		int FontHeight;
		TVTest::Style::Margins ItemMargins;
		TVTest::Style::Margins CategoryItemMargins;
		TVTest::Style::IntValue CategoryIconMargin;
	};

	class ABSTRACT_CLASS(CHomeDisplayEventHandler) : public CDisplayView::CEventHandler
	{
	public:
		CHomeDisplayEventHandler();
		virtual void OnClose() = 0;

		friend class CHomeDisplay;

	protected:
		class CHomeDisplay *m_pHomeDisplay;
	};

	class ABSTRACT_CLASS(CCategory)
	{
	public:
		CCategory(class CHomeDisplay *pHomeDisplay);
		virtual ~CCategory() {}
		virtual int GetID() const = 0;
		virtual LPCTSTR GetTitle() const = 0;
		virtual int GetIconIndex() const = 0;
		virtual int GetHeight() const = 0;
		virtual bool Create() = 0;
		virtual void ReadSettings(CSettings &Settings) {}
		virtual void WriteSettings(CSettings &Settings) {}
		virtual void LayOut(const StyleInfo &Style,HDC hdc,const RECT &ContentRect) = 0;
		virtual void Draw(HDC hdc,const StyleInfo &Style,const RECT &ContentRect,const RECT &PaintRect) const = 0;
		virtual bool GetCurItemRect(RECT *pRect) const = 0;
		virtual bool SetFocus(bool fFocus) {}
		virtual bool IsFocused() const = 0;
		virtual bool OnDecide() { return false; }
		virtual void OnWindowCreate() {}
		virtual void OnWindowDestroy() {}
		virtual void OnCursorMove(int x,int y) {}
		virtual void OnCursorLeave() {}
		virtual void OnLButtonDown(int x,int y) {}
		virtual bool OnLButtonUp(int x,int y) { return false; }
		virtual bool OnRButtonUp(int x,int y) { return false; }
		virtual bool OnSetCursor() { return false; }
		virtual bool OnCursorKey(WPARAM KeyCode) { return false; }

	protected:
		class CHomeDisplay *m_pHomeDisplay;
	};

	CHomeDisplay();
	~CHomeDisplay();

// CBasicWindow
	bool Create(HWND hwndParent,DWORD Style,DWORD ExStyle=0,int ID=0) override;

// CDisplayView
	bool Close() override;
	bool IsMessageNeed(const MSG *pMsg) const override;
	bool OnMouseWheel(UINT Msg,WPARAM wParam,LPARAM lParam) override;

// CSettingsBase
	bool LoadSettings(CSettings &Settings) override;
	bool SaveSettings(CSettings &Settings) override;

// CHomeDisplay
	void Clear();
	bool UpdateContents();
	void SetEventHandler(CHomeDisplayEventHandler *pEventHandler);
	bool SetFont(const TVTest::Style::Font &Font,bool fAutoSize);
	int GetScrollPos() const { return m_ScrollPos; }
	bool SetScrollPos(int Pos,bool fScroll=true);
	bool SetCurCategory(int Category);
	int GetCurCategoryID() const;
	bool UpdateCurContent();
	bool OnContentChanged();

	static bool Initialize(HINSTANCE hinst);

private:
	enum PartType {
		PART_MARGIN,
		PART_CATEGORY,
		PART_CONTENT
	};

	DrawUtil::CFont m_Font;
	bool m_fAutoFontSize;
	StyleInfo m_HomeDisplayStyle;
	int m_CategoriesAreaWidth;
	int m_CategoryItemWidth;
	int m_CategoryItemHeight;
	int m_ContentHeight;

	std::vector<CCategory*> m_CategoryList;
	CHomeDisplayEventHandler *m_pHomeDisplayEventHandler;
	int m_CurCategory;
	HWND m_hwndScroll;
	int m_ScrollPos;
	CMouseWheelHandler m_MouseWheel;
	bool m_fHitCloseButton;
	PartType m_CursorPart;
	bool m_fHoverOverButton;
	HIMAGELIST m_himlIcons;

	static const LPCTSTR m_pszWindowClass;
	static HINSTANCE m_hinst;

// CCustomWindow
	LRESULT OnMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam) override;

// CDoubleBufferingDraw
	void Draw(HDC hdc,const RECT &PaintRect) override;

	void LayOut();
	void SetScrollBar();
	void GetContentAreaRect(RECT *pRect) const;
	PartType HitTest(int x,int y,int *pCategoryIndex) const;
	void ScrollToCurItem();
	bool GetCategoryItemRect(int Category,RECT *pRect) const;
	bool RedrawCategoryItem(int Category);
};


#endif
