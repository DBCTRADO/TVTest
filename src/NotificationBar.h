#ifndef NOTIFICATION_BAR_H
#define NOTIFICATION_BAR_H


#include <deque>
#include "BasicWindow.h"
#include "UIBase.h"
#include "Theme.h"
#include "DrawUtil.h"
#include "WindowUtil.h"


class CNotificationBar
	: public CCustomWindow
	, public TVTest::CUIBase
	, protected CWindowTimerManager
{
public:
	enum MessageType {
		MESSAGE_INFO,
		MESSAGE_WARNING,
		MESSAGE_ERROR
	};

	CNotificationBar();
	~CNotificationBar();

// CBasicWindow
	bool Create(HWND hwndParent,DWORD Style,DWORD ExStyle=0,int ID=0) override;

// CUIBase
	void SetStyle(const TVTest::Style::CStyleManager *pStyleManager) override;
	void NormalizeStyle(const TVTest::Style::CStyleManager *pStyleManager) override;

// CNotificationBar
	bool Show(LPCTSTR pszText,MessageType Type,DWORD Timeout,bool fSkippable);
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
		bool fSkippable;
	};

	struct NotificationBarStyle {
		TVTest::Style::Margins Padding;
		TVTest::Style::Size IconSize;
		TVTest::Style::Margins IconMargin;
		TVTest::Style::Margins TextMargin;
		TVTest::Style::IntValue TextExtraHeight;

		NotificationBarStyle();
		void SetStyle(const TVTest::Style::CStyleManager *pStyleManager);
		void NormalizeStyle(const TVTest::Style::CStyleManager *pStyleManager);
	};

	enum {
		TIMER_ID_SHOWANIMATION	= 0x0001U,
		TIMER_ID_FADEANIMATION	= 0x0002U,
		TIMER_ID_HIDE			= 0x0004U
	};

	static const int SHOW_ANIMATION_COUNT=4;
	static const DWORD SHOW_ANIMATION_INTERVAL=50;
	static const int FADE_ANIMATION_COUNT=4;
	static const DWORD FADE_ANIMATION_INTERVAL=50;

	NotificationBarStyle m_Style;
	Theme::GradientInfo m_BackGradient;
	COLORREF m_TextColor[3];
	DrawUtil::CFont m_Font;
	int m_BarHeight;
	bool m_fAnimate;
	std::deque<MessageInfo> m_MessageQueue;
	int m_TimerCount;

	static HINSTANCE m_hinst;

	void CalcBarHeight();
	void GetBarPosition(RECT *pRect) const;
	void GetAnimatedBarPosition(RECT *pRect,int Frame,int NumFrames) const;
	void SetHideTimer();
// CCustomWindow
	LRESULT OnMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam) override;
};


#endif
