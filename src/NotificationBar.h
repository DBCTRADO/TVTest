#ifndef TVTEST_NOTIFICATION_BAR_H
#define TVTEST_NOTIFICATION_BAR_H


#include <deque>
#include "BasicWindow.h"
#include "UIBase.h"
#include "Theme.h"
#include "DrawUtil.h"
#include "WindowUtil.h"
#include "GUIUtil.h"


namespace TVTest
{

	class CNotificationBar
		: public CCustomWindow
		, public CUIBase
		, protected CWindowTimerManager
	{
	public:
		enum class MessageType {
			Info,
			Warning,
			Error,
		};

		CNotificationBar();
		~CNotificationBar();

	// CBasicWindow
		bool Create(HWND hwndParent, DWORD Style, DWORD ExStyle = 0, int ID = 0) override;

	// CUIBase
		void SetStyle(const Style::CStyleManager *pStyleManager) override;
		void NormalizeStyle(
			const Style::CStyleManager *pStyleManager,
			const Style::CStyleScaling *pStyleScaling) override;
		void SetTheme(const Theme::CThemeManager *pThemeManager) override;

	// CNotificationBar
		bool Show(LPCTSTR pszText, MessageType Type, DWORD Timeout, bool fSkippable);
		bool Hide();
		bool SetFont(const Style::Font &Font);
		void SetAnimate(bool fAnimate) { m_fAnimate = fAnimate; }
		int GetBarHeight() const { return m_BarHeight; }

		static bool Initialize(HINSTANCE hinst);

	private:
		struct MessageInfo
		{
			String Text;
			MessageType Type;
			DWORD Timeout;
			bool fSkippable;
		};

		struct NotificationBarStyle
		{
			Style::Margins Padding;
			Style::Size IconSize;
			Style::Margins IconMargin;
			Style::Margins TextMargin;
			Style::IntValue TextExtraHeight;

			NotificationBarStyle();
			void SetStyle(const Style::CStyleManager *pStyleManager);
			void NormalizeStyle(
				const Style::CStyleManager *pStyleManager,
				const Style::CStyleScaling *pStyleScaling);
		};

		enum {
			TIMER_ID_SHOWANIMATION = 0x0001U,
			TIMER_ID_FADEANIMATION = 0x0002U,
			TIMER_ID_HIDE          = 0x0004U
		};

		static const int SHOW_ANIMATION_COUNT = 4;
		static const DWORD SHOW_ANIMATION_INTERVAL = 50;
		static const int FADE_ANIMATION_COUNT = 4;
		static const DWORD FADE_ANIMATION_INTERVAL = 50;

		NotificationBarStyle m_Style;
		Theme::BackgroundStyle m_BackStyle;
		COLORREF m_TextColor[3];
		Style::Font m_StyleFont;
		DrawUtil::CFont m_Font;
		int m_BarHeight;
		bool m_fAnimate;
		std::deque<MessageInfo> m_MessageQueue;
		int m_TimerCount;
		CIcon m_Icons[3];

		static HINSTANCE m_hinst;

		void CalcBarHeight();
		void GetBarPosition(RECT *pRect) const;
		void GetAnimatedBarPosition(RECT *pRect, int Frame, int NumFrames) const;
		void SetHideTimer();

	// CCustomWindow
		LRESULT OnMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

	// CUIBase
		void ApplyStyle() override;
	};

}	// namespace TVTest


#endif
