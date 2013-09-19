#ifndef WINDOW_UTIL_H
#define WINDOW_UTIL_H


void SnapWindow(HWND hwnd,RECT *prc,int Margin,HWND hwndExclude=NULL);


class CMouseLeaveTrack
{
public:
	CMouseLeaveTrack();
	void Initialize(HWND hwnd);
	bool OnMouseMove();
	bool OnMouseLeave();
	bool OnNcMouseMove();
	bool OnNcMouseLeave();
	bool OnMessage(UINT Msg,WPARAM wParam,LPARAM lParam);
	bool IsClientTrack() const { return m_fClientTrack; }
	bool IsNonClientTrack() const { return m_fNonClientTrack; }

protected:
	HWND m_hwnd;
	bool m_fClientTrack;
	bool m_fNonClientTrack;

	bool IsCursorInWindow() const;
};


#endif
