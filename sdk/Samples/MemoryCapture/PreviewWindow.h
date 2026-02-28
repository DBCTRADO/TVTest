#pragma once


#include "Image.h"


// プレビューウィンドウクラス
class CPreviewWindow
{
public:
	static bool Initialize(HINSTANCE hinst);

	bool Create(HWND hwndParent);
	void SetPosition(int Left, int Top, int Width, int Height);
	void SetImage(const CImage *pImage);
	void SetZoomRate(int Num, int Denom);
	void GetZoomRate(int *pNum, int *pDenom) const;
	void SetFitImageToWindow(bool fFit);
	bool GetFitImageToWindow() const { return m_fFitImageToWindow; }
	bool GetDisplaySize(int *pWidth, int *pHeight) const;

private:
	static const LPCTSTR m_WindowClassName;
	static HINSTANCE m_hinst;

	HWND m_hwnd = nullptr;
	const CImage *m_pImage = nullptr;
	bool m_fFitImageToWindow = true;
	int m_ZoomNum = 50;
	int m_ZoomDenom = 100;

	void Draw(HDC hdc);

	static CPreviewWindow * GetThis(HWND hwnd);
	static LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};
