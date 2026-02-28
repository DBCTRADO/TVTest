/*
  TVTest
  Copyright(c) 2008-2020 DBCTRADO

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


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
			Style::Margins Padding{4, 2, 4, 2};
			Style::Size IconSize{::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON)};
			Style::Margins IconMargin{0, 0, 4, 0};
			Style::Margins TextMargin{0};
			Style::IntValue TextExtraHeight{4};

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

		static constexpr int SHOW_ANIMATION_COUNT = 4;
		static constexpr DWORD SHOW_ANIMATION_INTERVAL = 50;
		static constexpr int FADE_ANIMATION_COUNT = 4;
		static constexpr DWORD FADE_ANIMATION_INTERVAL = 50;

		NotificationBarStyle m_Style;
		Theme::BackgroundStyle m_BackStyle;
		COLORREF m_TextColor[3];
		Style::Font m_StyleFont;
		DrawUtil::CFont m_Font;
		int m_BarHeight = 0;
		bool m_fAnimate = true;
		std::deque<MessageInfo> m_MessageQueue;
		int m_TimerCount = 0;
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

} // namespace TVTest


#endif
