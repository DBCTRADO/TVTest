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

class CMouseWheelHandler
{
public:
	CMouseWheelHandler();
	void Reset();
	void ResetDelta();
	int OnWheel(int Delta);
	int OnMouseWheel(WPARAM wParam,int ScrollLines=0);
	int OnMouseHWheel(WPARAM wParam,int ScrollChars=0);
	int GetDeltaSum() const { return m_DeltaSum; }
	int GetLastDelta() const { return m_LastDelta; }
	DWORD GetLastTime() const { return m_LastTime; }
	int GetDefaultScrollLines() const;
	int GetDefaultScrollChars() const;

protected:
	int m_DeltaSum;
	int m_LastDelta;
	DWORD m_LastTime;
};


#endif
