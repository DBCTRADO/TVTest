#ifndef DIALOG_H
#define DIALOG_H


#include "UIBase.h"
#include <vector>


class CBasicDialog
	: public TVTest::CUIBase
{
public:
	struct Position {
		int x,y;
		int Width,Height;
		Position() : x(0), y(0), Width(0), Height(0) {}
		void Set(const RECT *pRect) {
			x=pRect->left;
			y=pRect->top;
			Width=pRect->right-x;
			Height=pRect->bottom-y;
		}
		void Get(RECT *pRect) const {
			pRect->left=x;
			pRect->top=y;
			pRect->right=x+Width;
			pRect->bottom=y+Height;
		}
	};

	CBasicDialog();
	virtual ~CBasicDialog();
	virtual bool Show(HWND hwndOwner) { return false; }
	virtual bool Create(HWND hwndOwner) { return false; }
	bool IsCreated() const;
	bool Destroy();
	bool IsModeless() const { return m_fModeless; }
	bool ProcessMessage(LPMSG pMsg);
	bool IsVisible() const;
	bool SetVisible(bool fVisible);
	bool GetPosition(Position *pPosition) const;
	bool GetPosition(RECT *pPosition) const;
	bool GetPosition(int *pLeft,int *pTop,int *pWidth=nullptr,int *pHeight=nullptr) const;
	bool SetPosition(const Position &Pos);
	bool SetPosition(const RECT *pPosition);
	bool SetPosition(int Left,int Top,int Width,int Height);
	bool SetPosition(int Left,int Top);
	bool IsPositionSet() const { return m_fSetPosition; }
	LRESULT SendMessage(UINT uMsg,WPARAM wParam,LPARAM lParam);

protected:
	HWND m_hDlg;
	bool m_fModeless;
	bool m_fSetPosition;
	Position m_Position;
	TVTest::Style::CStyleScaling m_StyleScaling;

	struct ItemInfo {
		HWND hwnd;
		RECT rcOriginal;
	};
	std::vector<ItemInfo> m_ItemList;
	int m_OriginalDPI;
	int m_CurrentDPI;
	HFONT m_hOriginalFont;
	DrawUtil::CFont m_Font;
	bool m_fInitializing;

	static CBasicDialog *GetThis(HWND hDlg);
	static INT_PTR CALLBACK DialogProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
	int ShowDialog(HWND hwndOwner,HINSTANCE hinst,LPCTSTR pszTemplate);
	bool CreateDialogWindow(HWND hwndOwner,HINSTANCE hinst,LPCTSTR pszTemplate);
	virtual INT_PTR HandleMessage(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
	virtual INT_PTR DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
	virtual bool ApplyPosition();
	void StorePosition();
	void InitDialog();
	virtual void OnDestroyed() {}

// CUIBase
	void ApplyStyle() override;
	void RealizeStyle() override;
};

class CResizableDialog : public CBasicDialog
{
public:
	CResizableDialog();
	virtual ~CResizableDialog();

protected:
	struct LayoutItem {
		int ID;
		RECT rcOriginal;
		int DPI;
		unsigned int Align;
	};
	enum {
		ALIGN_NONE		= 0x00000000,
		ALIGN_LEFT		= 0x00000001,
		ALIGN_TOP		= 0x00000002,
		ALIGN_RIGHT		= 0x00000004,
		ALIGN_BOTTOM	= 0x00000008,
		ALIGN_BOTTOM_RIGHT	= ALIGN_RIGHT | ALIGN_BOTTOM,
		ALIGN_HORZ			= ALIGN_LEFT | ALIGN_RIGHT,
		ALIGN_VERT			= ALIGN_TOP | ALIGN_BOTTOM,
		ALIGN_HORZ_TOP		= ALIGN_HORZ | ALIGN_TOP,
		ALIGN_HORZ_BOTTOM	= ALIGN_HORZ | ALIGN_BOTTOM,
		ALIGN_VERT_LEFT		= ALIGN_LEFT | ALIGN_VERT,
		ALIGN_VERT_RIGHT	= ALIGN_RIGHT | ALIGN_VERT,
		ALIGN_ALL			= ALIGN_HORZ | ALIGN_VERT
	};

	SIZE m_MinSize;
	SIZE m_OriginalClientSize;
	SIZE m_ScaledClientSize;
	int m_BaseDPI;
	HWND m_hwndSizeGrip;
	std::vector<LayoutItem> m_ControlList;

	INT_PTR HandleMessage(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam) override;
	bool ApplyPosition() override;
	void DoLayout();
	bool AddControl(int ID,unsigned int Align);
	bool AddControls(int FirstID,int LastID,unsigned int Align);
	bool UpdateControlPosition(int ID);

// CUIBase
	void ApplyStyle() override;
	void RealizeStyle() override;
};


#endif
