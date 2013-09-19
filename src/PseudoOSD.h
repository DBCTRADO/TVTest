#ifndef PSEUDO_OSD_H
#define PSEUDO_OSD_H


#include "DrawUtil.h"


class CPseudoOSD
{
public:
	enum {
		IMAGEEFFECT_GLOSS	= 0x01,
		IMAGEEFFECT_DARK	= 0x02
	};

	static bool Initialize(HINSTANCE hinst);
	static bool IsPseudoOSD(HWND hwnd);

	CPseudoOSD();
	~CPseudoOSD();
	bool Create(HWND hwndParent,bool fLayeredWindow=false);
	bool Destroy();
	bool Show(DWORD Time=0,bool fAnimation=false);
	bool Hide();
	bool IsVisible() const;
	bool SetText(LPCTSTR pszText,HBITMAP hbmIcon=NULL,int IconWidth=0,int IconHeight=0,unsigned int ImageEffect=0);
	bool SetPosition(int Left,int Top,int Width,int Height);
	void GetPosition(int *pLeft,int *pTop,int *pWidth,int *pHeight) const;
	void SetTextColor(COLORREF crText);
	bool SetTextHeight(int Height);
	bool SetFont(const LOGFONT &Font);
	bool CalcTextSize(SIZE *pSize);
	bool SetImage(HBITMAP hbm,unsigned int ImageEffect=0);
	bool SetOpacity(int Opacity);
	void OnParentMove();

private:
	HWND m_hwnd;
	COLORREF m_crBackColor;
	COLORREF m_crTextColor;
	DrawUtil::CFont m_Font;
	CDynamicString m_Text;
	HBITMAP m_hbmIcon;
	int m_IconWidth;
	int m_IconHeight;
	HBITMAP m_hbm;
	unsigned int m_ImageEffect;
	struct {
		int Left,Top,Width,Height;
	} m_Position;
	UINT_PTR m_TimerID;
	int m_AnimationCount;
	int m_Opacity;
	bool m_fLayeredWindow;
	HWND m_hwndParent;
	POINT m_ParentPosition;

	void Draw(HDC hdc,const RECT &PaintRect) const;
	void DrawImageEffect(HDC hdc,const RECT *pRect) const;
	void UpdateLayeredWindow();

	static const LPCTSTR m_pszWindowClass;
	static HINSTANCE m_hinst;
	static CPseudoOSD *GetThis(HWND hwnd);
	static LRESULT CALLBACK WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
};


#endif
