#include "stdafx.h"
#include "TVTest.h"
#include "NotificationBar.h"
#include "DrawUtil.h"
#include "Common/DebugDef.h"


#define NOTIFICATION_BAR_WINDOW_CLASS APP_NAME TEXT(" Notification Bar")




HINSTANCE CNotificationBar::m_hinst = nullptr;


bool CNotificationBar::Initialize(HINSTANCE hinst)
{
	if (m_hinst == nullptr) {
		WNDCLASS wc;

		wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
		wc.lpfnWndProc = WndProc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hInstance = hinst;
		wc.hIcon = nullptr;
		wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
		wc.hbrBackground = nullptr;
		wc.lpszMenuName = nullptr;
		wc.lpszClassName = NOTIFICATION_BAR_WINDOW_CLASS;
		if (RegisterClass(&wc) == 0)
			return false;
		m_hinst = hinst;
	}
	return true;
}


CNotificationBar::CNotificationBar()
	: m_fAnimate(true)
	, m_BarHeight(0)
	, m_TimerCount(0)
{
	GetSystemFont(DrawUtil::FONT_MESSAGE, &m_StyleFont);
	m_StyleFont.LogFont.lfHeight = ::MulDiv(m_StyleFont.LogFont.lfHeight, 12, 10);
	m_StyleFont.Size.Value = ::MulDiv(m_StyleFont.Size.Value, 12, 10);
}


CNotificationBar::~CNotificationBar()
{
}


bool CNotificationBar::Create(HWND hwndParent, DWORD Style, DWORD ExStyle, int ID)
{
	return CreateBasicWindow(
		hwndParent, Style, ExStyle, ID,
		NOTIFICATION_BAR_WINDOW_CLASS, nullptr, m_hinst);
}


void CNotificationBar::SetStyle(const TVTest::Style::CStyleManager *pStyleManager)
{
	m_Style.SetStyle(pStyleManager);
}


void CNotificationBar::NormalizeStyle(
	const TVTest::Style::CStyleManager *pStyleManager,
	const TVTest::Style::CStyleScaling *pStyleScaling)
{
	m_Style.NormalizeStyle(pStyleManager, pStyleScaling);
}


void CNotificationBar::SetTheme(const TVTest::Theme::CThemeManager *pThemeManager)
{
	pThemeManager->GetBackgroundStyle(TVTest::Theme::CThemeManager::STYLE_NOTIFICATIONBAR, &m_BackStyle);

	m_TextColor[MESSAGE_INFO] =
		pThemeManager->GetColor(CColorScheme::COLOR_NOTIFICATIONBARTEXT);
	m_TextColor[MESSAGE_WARNING] =
		pThemeManager->GetColor(CColorScheme::COLOR_NOTIFICATIONBARWARNINGTEXT);
	m_TextColor[MESSAGE_ERROR] =
		pThemeManager->GetColor(CColorScheme::COLOR_NOTIFICATIONBARERRORTEXT);

	if (m_hwnd != nullptr)
		Invalidate();
}


bool CNotificationBar::Show(LPCTSTR pszText, MessageType Type, DWORD Timeout, bool fSkippable)
{
	if (m_hwnd == nullptr || pszText == nullptr)
		return false;

	MessageInfo Info;
	Info.Text = pszText;
	Info.Type = Type;
	Info.Timeout = Timeout;
	Info.fSkippable = fSkippable;
	m_MessageQueue.push_back(Info);

	EndTimer(TIMER_ID_SHOWANIMATION);
	EndTimer(TIMER_ID_FADEANIMATION);

	RECT rc;
	GetBarPosition(&rc);

	if (!GetVisible()) {
		while (m_MessageQueue.size() > 1)
			m_MessageQueue.pop_front();
		::BringWindowToTop(m_hwnd);
		if (m_fAnimate)
			GetAnimatedBarPosition(&rc, 0, SHOW_ANIMATION_COUNT);
		SetPosition(&rc);
		SetVisible(true);
		Update();
		if (m_fAnimate) {
			m_TimerCount = 0;
			BeginTimer(TIMER_ID_SHOWANIMATION, SHOW_ANIMATION_INTERVAL);
		} else if (Timeout != 0) {
			BeginTimer(TIMER_ID_HIDE, Timeout);
		}
	} else {
		if (m_MessageQueue.size() > 1) {
			auto itr = m_MessageQueue.begin() + (m_MessageQueue.size() - 2);
			if (itr->fSkippable) {
				m_MessageQueue.erase(itr);
				if (m_MessageQueue.size() == 1)
					EndTimer(TIMER_ID_HIDE);
			}
		}
		SetPosition(&rc);
		Redraw();
		Timeout = m_MessageQueue.front().Timeout;
		if (Timeout != 0 && !IsTimerEnabled(TIMER_ID_HIDE))
			BeginTimer(TIMER_ID_HIDE, Timeout);
	}

	return true;
}


bool CNotificationBar::Hide()
{
	if (m_hwnd == nullptr)
		return false;

	EndAllTimers();

	if (m_fAnimate) {
		RECT rc;

		GetAnimatedBarPosition(&rc, FADE_ANIMATION_COUNT - 1, FADE_ANIMATION_COUNT);
		SetPosition(&rc);
		Redraw();
		m_TimerCount = 0;
		if (BeginTimer(TIMER_ID_FADEANIMATION, FADE_ANIMATION_INTERVAL))
			return true;
	}

	SetVisible(false);
	m_MessageQueue.clear();

	return true;
}


bool CNotificationBar::SetFont(const TVTest::Style::Font &Font)
{
	m_StyleFont = Font;
	if (m_hwnd != nullptr)
		ApplyStyle();
	return true;
}


void CNotificationBar::CalcBarHeight()
{
	int FontHeight = TVTest::Style::GetFontHeight(
		m_hwnd, m_Font.GetHandle(), m_Style.TextExtraHeight);
	int IconHeight = m_Style.IconSize.Height + m_Style.IconMargin.Vert();
	int TextHeight = FontHeight + m_Style.TextMargin.Vert();

	m_BarHeight = max(IconHeight, TextHeight) + m_Style.Padding.Vert();
}


void CNotificationBar::GetBarPosition(RECT *pRect) const
{
	::GetClientRect(::GetParent(m_hwnd), pRect);
	pRect->bottom = m_BarHeight;
}


void CNotificationBar::GetAnimatedBarPosition(RECT *pRect, int Frame, int NumFrames) const
{
	GetBarPosition(pRect);
	pRect->bottom = (Frame + 1) * m_BarHeight / NumFrames;
}


void CNotificationBar::SetHideTimer()
{
	if (!m_MessageQueue.empty()) {
		DWORD Timeout = m_MessageQueue.front().Timeout;
		if (Timeout != 0)
			BeginTimer(TIMER_ID_HIDE, Timeout);
		else
			EndTimer(TIMER_ID_HIDE);
	}
}


LRESULT CNotificationBar::OnMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		{
			InitializeUI();

			InitializeTimer(hwnd);
			m_TimerCount = 0;
		}
		return 0;

	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			RECT rc;

			::BeginPaint(hwnd, &ps);

			::GetClientRect(hwnd, &rc);

			{
				TVTest::Theme::CThemeDraw ThemeDraw(BeginThemeDraw(ps.hdc));
				ThemeDraw.Draw(m_BackStyle, rc);
			}

			if (!m_MessageQueue.empty()) {
				const MessageInfo &Info = m_MessageQueue.front();

				TVTest::Style::Subtract(&rc, m_Style.Padding);
				if (rc.left < rc.right) {
					if (Info.Type >= 0 && Info.Type < lengthof(m_Icons) && m_Icons[Info.Type]) {
						rc.left += m_Style.IconMargin.Left;
						::DrawIconEx(
							ps.hdc,
							rc.left,
							rc.top + m_Style.IconMargin.Top +
							((rc.bottom - rc.top) - m_Style.IconMargin.Vert() - m_Style.IconSize.Height) / 2,
							m_Icons[Info.Type],
							m_Style.IconSize.Width, m_Style.IconSize.Height,
							0, nullptr, DI_NORMAL);
						rc.left += m_Style.IconSize.Width + m_Style.IconMargin.Right;
					}
					TVTest::Style::Subtract(&rc, m_Style.TextMargin);
					DrawUtil::DrawText(
						ps.hdc, Info.Text.c_str(), rc,
						DT_SINGLELINE | DT_LEFT | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS,
						&m_Font, m_TextColor[Info.Type]);
				}
			}

			::EndPaint(hwnd, &ps);
		}
		return 0;

	case WM_TIMER:
		switch (wParam) {
		case TIMER_ID_SHOWANIMATION:
			{
				RECT rc;

				GetAnimatedBarPosition(&rc, m_TimerCount + 1, SHOW_ANIMATION_COUNT);
				SetPosition(&rc);
				Update();
				if (m_TimerCount < SHOW_ANIMATION_COUNT - 2) {
					m_TimerCount++;
				} else {
					EndTimer(TIMER_ID_SHOWANIMATION);
					m_TimerCount = 0;
					SetHideTimer();
				}
			}
			break;

		case TIMER_ID_FADEANIMATION:
			if (m_TimerCount < FADE_ANIMATION_COUNT - 1) {
				RECT rc;

				GetAnimatedBarPosition(
					&rc, FADE_ANIMATION_COUNT - 2 - m_TimerCount, FADE_ANIMATION_COUNT);
				SetPosition(&rc);
				Update();
				m_TimerCount++;
			} else {
				SetVisible(false);
				EndTimer(TIMER_ID_FADEANIMATION);
				m_TimerCount = 0;
				m_MessageQueue.clear();
			}
			break;

		case TIMER_ID_HIDE:
			if (!m_MessageQueue.empty())
				m_MessageQueue.pop_front();
			if (m_MessageQueue.empty()) {
				Hide();
			} else {
				Redraw();
				SetHideTimer();
			}
			break;
		}
		return 0;

	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_LBUTTONDBLCLK:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_RBUTTONDBLCLK:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
	case WM_MBUTTONDBLCLK:
	case WM_MOUSEMOVE:
		{
			POINT pt;
			HWND hwndParent = ::GetParent(hwnd);

			pt.x = GET_X_LPARAM(lParam);
			pt.y = GET_Y_LPARAM(lParam);
			::MapWindowPoints(hwnd, hwndParent, &pt, 1);
			return ::SendMessage(hwndParent, uMsg, wParam, MAKELPARAM(pt.x, pt.y));
		}
	}

	return CCustomWindow::OnMessage(hwnd, uMsg, wParam, lParam);
}


void CNotificationBar::ApplyStyle()
{
	if (m_hwnd == nullptr)
		return;

	CreateDrawFont(m_StyleFont, &m_Font);

	/*
	m_Icons[MESSAGE_INFO].Attach(
		LoadSystemIcon(IDI_INFORMATION, m_Style.IconSize.Width, m_Style.IconSize.Height));
	*/
	m_Icons[MESSAGE_WARNING].Attach(
		LoadSystemIcon(IDI_WARNING, m_Style.IconSize.Width, m_Style.IconSize.Height));
	m_Icons[MESSAGE_ERROR].Attach(
		LoadSystemIcon(IDI_ERROR, m_Style.IconSize.Width, m_Style.IconSize.Height));

	CalcBarHeight();
}




CNotificationBar::NotificationBarStyle::NotificationBarStyle()
	: Padding(4, 2, 4, 2)
	, IconSize(::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON))
	, IconMargin(0, 0, 4, 0)
	, TextMargin(0)
	, TextExtraHeight(4)
{
}


void CNotificationBar::NotificationBarStyle::SetStyle(const TVTest::Style::CStyleManager *pStyleManager)
{
	*this = NotificationBarStyle();
	pStyleManager->Get(TEXT("notification-bar.padding"), &Padding);
	pStyleManager->Get(TEXT("notification-bar.icon"), &IconSize);
	pStyleManager->Get(TEXT("notification-bar.icon.margin"), &IconMargin);
	pStyleManager->Get(TEXT("notification-bar.text.margin"), &TextMargin);
	pStyleManager->Get(TEXT("notification-bar.text.extra-height"), &TextExtraHeight);
}


void CNotificationBar::NotificationBarStyle::NormalizeStyle(
	const TVTest::Style::CStyleManager *pStyleManager,
	const TVTest::Style::CStyleScaling *pStyleScaling)
{
	pStyleScaling->ToPixels(&Padding);
	pStyleScaling->ToPixels(&IconSize);
	pStyleScaling->ToPixels(&IconMargin);
	pStyleScaling->ToPixels(&TextMargin);
	pStyleScaling->ToPixels(&TextExtraHeight);
}
