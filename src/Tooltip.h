#ifndef TOOLTIP_H
#define TOOLTIP_H


class CTooltip
{
protected:
	HWND m_hwndTooltip;
	HWND m_hwndParent;

public:
	CTooltip();
	~CTooltip();
	bool Create(HWND hwnd);
	void Destroy();
	bool IsCreated() const { return m_hwndTooltip!=NULL; }
	HWND GetHandle() const { return m_hwndTooltip; }
	void DeleteAllTools();
	bool Enable(bool fEnable);
	bool IsVisible() const;
	bool SetMaxWidth(int Width);
	bool SetPopDelay(int Delay);
	int NumTools() const;
	bool AddTool(UINT ID,const RECT &Rect,LPCTSTR pszText=LPSTR_TEXTCALLBACK,LPARAM lParam=0);
	bool AddTool(HWND hwnd,LPCTSTR pszText=LPSTR_TEXTCALLBACK,LPARAM lParam=0);
	bool DeleteTool(UINT ID);
	bool SetToolRect(UINT ID,const RECT &Rect);
	bool SetText(UINT ID,LPCTSTR pszText);
	bool AddTrackingTip(UINT ID,LPCTSTR pszText=LPSTR_TEXTCALLBACK,LPARAM lParam=0);
	bool TrackActivate(UINT ID,bool fActivate);
	bool TrackPosition(int x,int y);
};

class CBalloonTip
{
	HWND m_hwndToolTips;
	HWND m_hwndOwner;

public:
	CBalloonTip();
	~CBalloonTip();
	bool Initialize(HWND hwnd);
	void Finalize();
	enum {
		ICON_NONE,
		ICON_INFO,
		ICON_WARNING,
		ICON_ERROR
	};
	bool Show(LPCTSTR pszText,LPCTSTR pszTitle,const POINT *pPos,int Icon=ICON_NONE);
	bool Hide();
};


#endif
