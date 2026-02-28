#pragma once


#include <vector>


// ツールバークラス
class CToolbar
{
public:
	struct ItemInfo
	{
		int ID;
		int Icon;
		unsigned int Flags;
	};

	enum {
		ItemFlag_NotifyPress = 0x00000001
	};

	enum {
		Notify_ItemPressed = 1,
		Notify_ItemReleased
	};

	static bool Initialize(HINSTANCE hinst);

	CToolbar();
	~CToolbar();

	bool Create(HWND hwndParent, int ID, TVTest::CTVTestApp *pApp);
	void SetPosition(int Left, int Top, int Width, int Height);
	void SetDPI(int DPI);
	int GetHeight() const;
	void AddItem(const ItemInfo &Item);
	void SetIconImage(HBITMAP hbm, int Width, int Height);
	int GetPressingItem() const;

private:
	static const LPCTSTR m_WindowClassName;
	static HINSTANCE m_hinst;

	TVTest::CTVTestApp *m_pApp = nullptr;
	HWND m_hwnd = nullptr;
	HWND m_hwndTooltips = nullptr;
	int m_DPI = 96;
	RECT m_Margin;
	int m_ItemPadding;
	int m_ItemWidth;
	int m_ItemHeight;
	int m_IconWidth;
	int m_IconHeight;
	HBITMAP m_hbmIcons = nullptr;
	std::vector<ItemInfo> m_ItemList;
	int m_HotItem = -1;
	int m_ClickItem = -1;

	void CalcMetrics();
	void Draw(HDC hdc, const RECT &rcPaint);
	void OnLButtonDown(int x, int y);
	void OnLButtonUp(int x, int y);
	void OnMouseMove(int x, int y);
	void GetItemRect(int Item, RECT *pRect) const;
	void RedrawItem(int Item) const;
	int GetItemFromPoint(int x, int y) const;
	void SetHotItem(int Item);
	void SetTooltips();
	void UpdateTooltips();

	static CToolbar * GetThis(HWND hwnd);
	static LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};
