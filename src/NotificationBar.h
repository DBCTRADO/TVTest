#ifndef NOTIFICATION_BAR_H
#define NOTIFICATION_BAR_H


#include <deque>
#include "BasicWindow.h"
#include "Theme.h"
#include "DrawUtil.h"


class CNotificationBar : public CBasicWindow
{
public:
	enum MessageType {
		MESSAGE_INFO,
		MESSAGE_WARNING,
		MESSAGE_ERROR
	};

	CNotificationBar();
	~CNotificationBar();
	bool Create(HWND hwndParent,DWORD Style,DWORD ExStyle=0,int ID=0);
	bool Show(LPCTSTR pszText,MessageType Type,DWORD Timeout=0);
	bool Hide();
	bool SetColors(const Theme::GradientInfo *pBackGradient,
				   COLORREF crTextColor,COLORREF crWarningTextColor,COLORREF crErrorTextColor);
	bool SetFont(const LOGFONT *pFont);
	void SetAnimate(bool fAnimate) { m_fAnimate=fAnimate; }
	int GetBarHeight() const { return m_BarHeight; }

	static bool Initialize(HINSTANCE hinst);

private:
	struct MessageInfo {
		CDynamicString Text;
		MessageType Type;
		HICON hIcon;
		DWORD Timeout;
	};

	Theme::GradientInfo m_BackGradient;
	COLORREF m_TextColor[3];
	DrawUtil::CFont m_Font;
	int m_BarHeight;
	bool m_fAnimate;
	std::deque<MessageInfo> m_MessageQueue;

	void CalcBarHeight();

	static HINSTANCE m_hinst;
	static CNotificationBar *GetThis(HWND hwnd);
	static LRESULT CALLBACK WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
};


#endif
