#ifndef DIALOG_H
#define DIALOG_H


#include <vector>


class CBasicDialog
{
public:
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
	bool GetPosition(RECT *pPosition) const;
	bool GetPosition(int *pLeft,int *pTop,int *pWidth,int *pHeight) const;
	bool SetPosition(const RECT *pPosition);
	bool SetPosition(int Left,int Top,int Width,int Height);
	LRESULT SendMessage(UINT uMsg,WPARAM wParam,LPARAM lParam);

protected:
	HWND m_hDlg;
	bool m_fModeless;
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
	Position m_Position;

	static CBasicDialog *GetThis(HWND hDlg);
	static INT_PTR CALLBACK DialogProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
	int ShowDialog(HWND hwndOwner,HINSTANCE hinst,LPCTSTR pszTemplate);
	bool CreateDialogWindow(HWND hwndOwner,HINSTANCE hinst,LPCTSTR pszTemplate);
	virtual INT_PTR DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
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
	HWND m_hwndSizeGrip;
	std::vector<LayoutItem> m_ControlList;

	virtual INT_PTR DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
	void DoLayout();
	bool AddControl(int ID,unsigned int Align);
	bool AddControls(int FirstID,int LastID,unsigned int Align);
	void ApplyPosition();
};


#endif
