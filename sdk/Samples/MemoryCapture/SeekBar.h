#pragma once


// シークバークラス
class CSeekBar
{
public:
	enum {
		Notify_PosChanged = 1
	};

	static bool Initialize(HINSTANCE hinst);

	CSeekBar();

	bool Create(HWND hwndParent, int ID, TVTest::CTVTestApp *pApp);
	void SetPosition(int Left, int Top, int Width, int Height);
	void SetDPI(int DPI);
	int GetHeight() const;
	void SetBarRange(int Min, int Max);
	void SetBarPos(int Pos);
	int GetBarPos() const { return m_Pos; }

private:
	static const LPCTSTR m_WindowClassName;
	static HINSTANCE m_hinst;

	TVTest::CTVTestApp *m_pApp;
	HWND m_hwnd;
	int m_DPI;
	int m_Margin;
	int m_BorderWidth;
	int m_BarHeight;
	int m_Min;
	int m_Max;
	int m_Pos;
	bool m_fHot;

	void CalcMetrics();
	void Draw(HDC hdc);
	RECT GetBarRect() const;
	void OnLButtonDown(int x, int y);
	void OnMouseMove(int x, int y);

	static CSeekBar *GetThis(HWND hwnd);
	static LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};
