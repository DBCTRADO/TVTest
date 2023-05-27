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


#ifndef TVTEST_EVENT_INFO_OSD_H
#define TVTEST_EVENT_INFO_OSD_H


#include "PseudoOSD.h"
#include "Graphics.h"
#include "Theme.h"
#include "LibISDB/LibISDB/EPG/EventInfo.hpp"


namespace TVTest
{

	class CEventInfoOSD
	{
	public:
		struct ColorScheme
		{
			Theme::ThemeColor Back{0, 0, 0, 160};
			Theme::ThemeColor Text{255, 255, 255, 255};
			Theme::ThemeColor TextOutline{0, 0, 0, 255};
			Theme::ThemeColor Title{192, 224, 255, 255};
			Theme::ThemeColor TitleOutline{0, 0, 0, 255};
		};

		bool Show(HWND hwndParent, DWORD Time = 0);
		bool Hide();
		bool IsVisible() const;
		bool IsCreated() const;
		bool Update();

		bool SetEventInfo(const LibISDB::EventInfo *pEventInfo);

		bool SetPosition(int Left, int Top, int Width, int Height);
		bool SetPosition(const RECT &Pos)
		{
			return SetPosition(Pos.left, Pos.top, Pos.right - Pos.left, Pos.bottom - Pos.top);
		}
		void GetPosition(int *pLeft, int *pTop, int *pWidth, int *pHeight) const;
		bool AdjustPosition(RECT *pRect) const;

		void SetColorScheme(const ColorScheme &Colors);
		const ColorScheme & GetColorScheme() const noexcept { return m_ColorScheme; }
		void SetFont(const LOGFONT &Font);
		void SetTitleFont(const LOGFONT &Font);

		void OnParentMove();

		void SetStyle(const Style::CStyleManager *pStyleManager);
		void NormalizeStyle(
			const Style::CStyleManager *pStyleManager,
			const Style::CStyleScaling *pStyleScaling);

	private:
		struct EventInfoOSDStyle
		{
			Style::Margins Margin{50, 50, 0, 0, Style::UnitType::Undefined};
			Style::Margins Padding{4};
			Style::IntValue TextSizeRatio{22};
			Style::IntValue TextSizeMin{12};
			Style::IntValue TextSizeMax{24};
			Style::IntValue TextOutline{20};
			bool fUseHinting = true;
			bool fUsePath = true;
			bool fShowLogo = true;
		};

		LibISDB::EventInfo m_EventInfo;
		ColorScheme m_ColorScheme;
		EventInfoOSDStyle m_Style;
		LOGFONT m_Font{};
		LOGFONT m_TitleFont{};
		DrawUtil::CBitmap m_Bitmap;
		CPseudoOSD m_OSD;

		bool CreateBitmap(int Width, int Height);
		void Draw(Graphics::CCanvas &Canvas, const RECT &Rect) const;
	};

} // namespace TVTest


#endif
