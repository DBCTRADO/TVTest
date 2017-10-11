#ifndef PSEUDO_OSD_H
#define PSEUDO_OSD_H


#include "DrawUtil.h"
#include "WindowUtil.h"


class CPseudoOSD
{
public:
	enum class TextStyle : unsigned int {
		None           = 0x0000U,
		Left           = 0x0000U,
		Right          = 0x0001U,
		HorzCenter     = 0x0002U,
		HorzAlignMask  = 0x0003U,
		Top            = 0x0000U,
		Bottom         = 0x0004U,
		VertCenter     = 0x0008U,
		VertAlignMask  = 0x000CU,
		Outline        = 0x0010U,
		FillBackground = 0x0020U,
	};

	enum class ImageEffect : unsigned int {
		None  = 0x0000U,
		Gloss = 0x0001U,
		Dark  = 0x0002U,
	};

	static bool Initialize(HINSTANCE hinst);
	static bool IsPseudoOSD(HWND hwnd);

	CPseudoOSD();
	~CPseudoOSD();
	bool Create(HWND hwndParent, bool fLayeredWindow = false);
	bool Destroy();
	bool Show(DWORD Time = 0, bool fAnimation = false);
	bool Hide();
	bool IsVisible() const;
	bool SetText(LPCTSTR pszText, HBITMAP hbmIcon = nullptr, int IconWidth = 0, int IconHeight = 0, ImageEffect Effect = ImageEffect::None);
	bool SetPosition(int Left, int Top, int Width, int Height);
	void GetPosition(int *pLeft, int *pTop, int *pWidth, int *pHeight) const;
	void SetTextColor(COLORREF crText);
	bool SetTextHeight(int Height);
	bool SetTextStyle(TextStyle Style);
	bool SetFont(const LOGFONT &Font);
	bool CalcTextSize(SIZE *pSize);
	bool SetImage(HBITMAP hbm, ImageEffect Effect = ImageEffect::None);
	void OnParentMove();

private:
	HWND m_hwnd;
	COLORREF m_crBackColor;
	COLORREF m_crTextColor;
	DrawUtil::CFont m_Font;
	TextStyle m_TextStyle;
	TVTest::String m_Text;
	HBITMAP m_hbmIcon;
	int m_IconWidth;
	int m_IconHeight;
	HBITMAP m_hbm;
	ImageEffect m_ImageEffect;
	struct {
		int Left, Top, Width, Height;
	} m_Position;
	CWindowTimerManager m_Timer;
	int m_AnimationCount;
	bool m_fLayeredWindow;
	bool m_fPopupLayeredWindow;
	HWND m_hwndParent;
	POINT m_ParentPosition;

	void Draw(HDC hdc, const RECT &PaintRect) const;
	void DrawImageEffect(HDC hdc, const RECT *pRect) const;
	void UpdateLayeredWindow();

	static const LPCTSTR m_pszWindowClass;
	static HINSTANCE m_hinst;
	static CPseudoOSD *GetThis(HWND hwnd);
	static LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};

TVTEST_ENUM_FLAGS(CPseudoOSD::TextStyle)
TVTEST_ENUM_FLAGS(CPseudoOSD::ImageEffect)


#endif
