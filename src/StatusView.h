#ifndef STATUS_VIEW_H
#define STATUS_VIEW_H


#include <vector>
#include <memory>
#include "BasicWindow.h"
#include "UIBase.h"
#include "Theme.h"
#include "DrawUtil.h"
#include "WindowUtil.h"
#include "Aero.h"


class CStatusView;

class ABSTRACT_CLASS(CStatusItem)
	: public TVTest::CUIBase
{
public:
	enum {
		STYLE_VARIABLEWIDTH = 0x00000001U,
		STYLE_FULLROW       = 0x00000002U,
		STYLE_FORCEFULLROW  = 0x00000004U
	};

	enum SizeUnit {
		SIZE_PIXEL,
		SIZE_EM
	};

	static const int EM_FACTOR = 1000;

	struct SizeValue
	{
		int Value;
		SizeUnit Unit;

		SizeValue(int v, SizeUnit u) : Value(v), Unit(u) {}
	};

	enum {
		DRAW_HIGHLIGHT = 0x00000001U,
		DRAW_BOTTOM    = 0x00000002U,
		DRAW_PREVIEW   = 0x00000004U
	};

	CStatusItem(int ID, const SizeValue &DefaultWidth);
	virtual ~CStatusItem() = default;
	int GetIndex() const;
	bool GetRect(RECT *pRect) const;
	bool GetClientRect(RECT *pRect) const;
	int GetID() const { return m_ID; }
	const SizeValue &GetDefaultWidth() const { return m_DefaultWidth; }
	int GetWidth() const { return m_Width; }
	bool SetWidth(int Width);
	int GetMinWidth() const { return m_MinWidth; }
	int GetMaxWidth() const { return m_MaxWidth; }
	int GetActualWidth() const { return m_ActualWidth; }
	bool SetActualWidth(int Width);
	int GetMinHeight() const { return m_MinHeight; }
	void SetVisible(bool fVisible);
	bool GetVisible() const { return m_fVisible; }
	void SetItemStyle(unsigned int Style);
	void SetItemStyle(unsigned int Mask, unsigned int Style);
	unsigned int GetItemStyle() const { return m_Style; }
	bool IsVariableWidth() const { return (m_Style & STYLE_VARIABLEWIDTH) != 0; }
	bool IsFullRow() const { return (m_Style & STYLE_FULLROW) != 0; }
	bool IsForceFullRow() const { return (m_Style & STYLE_FORCEFULLROW) != 0; }
	bool Update();
	void Redraw();
	virtual LPCTSTR GetIDText() const = 0;
	virtual LPCTSTR GetName() const = 0;
	virtual bool UpdateContent() { return true; }
	virtual void Draw(HDC hdc, const RECT &ItemRect, const RECT &DrawRect, unsigned int Flags) = 0;
	virtual void OnLButtonDown(int x, int y) {}
	virtual void OnLButtonUp(int x, int y) {}
	virtual void OnLButtonDoubleClick(int x, int y) { OnLButtonDown(x, y); }
	virtual void OnRButtonDown(int x, int y) { OnLButtonDown(x, y); }
	virtual void OnRButtonUp(int x, int y) {}
	virtual void OnRButtonDoubleClick(int x, int y) {}
	virtual void OnMButtonDown(int x, int y) {}
	virtual void OnMButtonUp(int x, int y) {}
	virtual void OnMButtonDoubleClick(int x, int y) {}
	virtual void OnMouseMove(int x, int y) {}
	virtual bool OnMouseWheel(int x, int y, bool fHorz, int Delta, int *pCommand) { return false; }
	virtual void OnVisibilityChanged() {}
	virtual void OnPresentStatusChange(bool fPresent) {}
	virtual void OnFocus(bool fFocus) {}
	virtual bool OnMouseHover(int x, int y) { return false; }
	virtual void OnSizeChanged() {}
	virtual void OnCaptureReleased() {}
	virtual LRESULT OnNotifyMessage(LPNMHDR pnmh) { return 0; }
	virtual void OnFontChanged() {}

	friend CStatusView;

protected:
	CStatusView *m_pStatus;
	int m_ID;
	SizeValue m_DefaultWidth;
	int m_Width;
	int m_MinWidth;
	int m_MaxWidth;
	int m_ActualWidth;
	int m_MinHeight;
	bool m_fVisible;
	bool m_fBreak;
	unsigned int m_Style;

	bool GetMenuPos(POINT *pPos, UINT *pFlags, RECT *pExcludeRect);
	enum {
		DRAWTEXT_HCENTER       = 0x00000001UL,
		DRAWTEXT_NOENDELLIPSIS = 0x00000002UL
	};
	void DrawText(HDC hdc, const RECT &Rect, LPCTSTR pszText, DWORD Flags = 0) const;
	void DrawIcon(
		HDC hdc, const RECT &Rect, DrawUtil::CMonoColorIconList &IconList,
		int IconIndex = 0, bool fEnabled = true) const;
};

class CIconStatusItem
	: public CStatusItem
{
public:
	CIconStatusItem(int ID, int DefaultWidth);
	void NormalizeStyle(
		const TVTest::Style::CStyleManager *pStyleManager,
		const TVTest::Style::CStyleScaling *pStyleScaling) override;
};

class CStatusView
	: public CCustomWindow
	, public TVTest::CUIBase
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
		virtual void OnStyleChanged() {}
		friend CStatusView;
	};

	struct StatusViewTheme
	{
		TVTest::Theme::Style ItemStyle;
		TVTest::Theme::Style HighlightItemStyle;
		TVTest::Theme::Style BottomItemStyle;
		TVTest::Theme::BorderStyle Border;
	};

	static bool Initialize(HINSTANCE hinst);
	CStatusView();
	~CStatusView();

// CBasicWindow
	bool Create(HWND hwndParent, DWORD Style, DWORD ExStyle = 0, int ID = 0) override;
	void SetVisible(bool fVisible) override;

// CUIBase
	void SetStyle(const TVTest::Style::CStyleManager *pStyleManager) override;
	void NormalizeStyle(
		const TVTest::Style::CStyleManager *pStyleManager,
		const TVTest::Style::CStyleScaling *pStyleScaling) override;
	void SetTheme(const TVTest::Theme::CThemeManager *pThemeManager) override;

// CStatusView
	int NumItems() const { return (int)m_ItemList.size(); }
	const CStatusItem *GetItem(int Index) const;
	CStatusItem *GetItem(int Index);
	const CStatusItem *GetItemByID(int ID) const;
	CStatusItem *GetItemByID(int ID);
	bool AddItem(CStatusItem *pItem);
	int IDToIndex(int ID) const;
	int IndexToID(int Index) const;
	bool UpdateItem(int ID);
	void RedrawItem(int ID);
	bool GetItemRect(int ID, RECT *pRect) const;
	bool GetItemRectByIndex(int Index, RECT *pRect) const;
	bool GetItemClientRect(int ID, RECT *pRect) const;
	int GetItemHeight() const;
	int CalcItemHeight(const DrawUtil::CFont &Font) const;
	const TVTest::Style::Margins &GetItemPadding() const;
	const TVTest::Style::Size &GetIconSize() const;
	int GetFontHeight() const { return m_FontHeight; }
	int GetIntegralWidth() const;
	bool AdjustSize();
	void SetSingleText(LPCTSTR pszText);
	static bool GetStatusViewThemeFromThemeManager(
		const TVTest::Theme::CThemeManager *pThemeManager, StatusViewTheme *pTheme);
	bool SetStatusViewTheme(const StatusViewTheme &Theme);
	bool GetStatusViewTheme(StatusViewTheme *pTheme) const;
	void SetItemTheme(const TVTest::Theme::CThemeManager *pThemeManager);
	bool SetFont(const TVTest::Style::Font &Font);
	bool GetFont(TVTest::Style::Font *pFont) const;
	HFONT GetFont() const;
	bool SetMultiRow(bool fMultiRow);
	bool SetMaxRows(int MaxRows);
	int CalcHeight(int Width) const;
	int GetCurItem() const;
	bool SetEventHandler(CEventHandler *pEventHandler);
	bool SetItemOrder(const int *pOrderList);
	bool CreateItemPreviewFont(
		const TVTest::Style::Font &Font, DrawUtil::CFont *pDrawFont) const;
	bool DrawItemPreview(
		CStatusItem *pItem, HDC hdc, const RECT &ItemRect,
		bool fHighlight = false, HFONT hfont = nullptr) const;
	bool EnableBufferedPaint(bool fEnable);
	void EnableSizeAdjustment(bool fEnable);
	int CalcItemPixelSize(const CStatusItem::SizeValue &Size) const;

private:
	struct StatusViewStyle
	{
		TVTest::Style::Margins ItemPadding;
		TVTest::Style::IntValue TextExtraHeight;
		TVTest::Style::Size IconSize;

		StatusViewStyle();
		void SetStyle(const TVTest::Style::CStyleManager *pStyleManager);
		void NormalizeStyle(
			const TVTest::Style::CStyleManager *pStyleManager,
			const TVTest::Style::CStyleScaling *pStyleScaling);
	};

	static const LPCTSTR CLASS_NAME;
	static HINSTANCE m_hinst;

	StatusViewStyle m_Style;
	TVTest::Style::Font m_Font;
	DrawUtil::CFont m_DrawFont;
	int m_FontHeight;
	int m_TextHeight;
	int m_ItemHeight;
	bool m_fMultiRow;
	int m_MaxRows;
	int m_Rows;
	StatusViewTheme m_Theme;
	std::vector<std::unique_ptr<CStatusItem>> m_ItemList;
	bool m_fSingleMode;
	TVTest::String m_SingleText;
	int m_HotItem;
	CMouseLeaveTrack m_MouseLeaveTrack;
	bool m_fOnButtonDown;
	int m_CapturedItem;
	CEventHandler *m_pEventHandler;
	DrawUtil::COffscreen m_Offscreen;
	bool m_fBufferedPaint;
	CBufferedPaint m_BufferedPaint;
	bool m_fAdjustSize;

	void SetHotItem(int Item);
	void Draw(HDC hdc, const RECT *pPaintRect);
	void CalcLayout();
	int CalcRows(const std::vector<const CStatusItem*> &ItemList, int MaxRowWidth) const;
	int CalcRows(const std::vector<CStatusItem*> &ItemList, int MaxRowWidth);
	int CalcTextHeight(const DrawUtil::CFont &Font, int *pFontHeight = nullptr) const;
	int CalcTextHeight(int *pFontHeight = nullptr) const;
	int CalcItemHeight(int TextHeight) const;
	int CalcItemHeight() const;

// CCustomWindow
	LRESULT OnMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

// CUIBase
	void ApplyStyle() override;
	void RealizeStyle() override;
};


#endif
