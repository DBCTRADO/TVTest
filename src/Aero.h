#ifndef AERO_H
#define AERO_H


class CAeroGlass
{
public:
	CAeroGlass();
	~CAeroGlass();
	bool IsEnabled();
	bool ApplyAeroGlass(HWND hwnd,const RECT *pRect);
	bool EnableNcRendering(HWND hwnd,bool fEnable);

private:
	HMODULE m_hDwmLib;
	bool LoadDwmLib();
};

class CBufferedPaint
{
public:
	CBufferedPaint();
	~CBufferedPaint();
	HDC Begin(HDC hdc,const RECT *pRect,bool fErase=false);
	bool End(bool fUpdate=true);
	bool Clear(const RECT *pRect=NULL);
	bool SetAlpha(BYTE Alpha);
	bool SetOpaque() { return SetAlpha(255); }

	static bool Initialize();
	static bool IsSupported();

private:
	HANDLE m_hPaintBuffer;
};

class CDoubleBufferingDraw
{
public:
	virtual void Draw(HDC hdc,const RECT &PaintRect) = 0;
	void OnPaint(HWND hwnd);
};


#endif
