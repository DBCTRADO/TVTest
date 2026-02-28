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


#ifndef TVTEST_THEME_H
#define TVTEST_THEME_H


#include "DrawUtil.h"
#include "Style.h"


namespace TVTest
{

	namespace Theme
	{

		typedef DrawUtil::RGBA ThemeColor;
		typedef DrawUtil::CMonoColorBitmap ThemeBitmap;
		typedef DrawUtil::CMonoColorIconList IconList;

		struct SolidStyle
		{
			ThemeColor Color;

			SolidStyle() = default;
			SolidStyle(const ThemeColor &color) : Color(color) {}

			bool operator==(const SolidStyle &Op) const noexcept = default;
		};

		enum class GradientType {
			Normal,
			Glossy,
			Interlaced,
		};

		enum class GradientDirection {
			Horz,
			Vert,
			HorzMirror,
			VertMirror,
		};

		struct GradientStyle
		{
			enum class RotateType {
				Left,
				Right,
				OneEighty,
			};

			GradientType Type = GradientType::Normal;
			GradientDirection Direction = GradientDirection::Horz;
			ThemeColor Color1;
			ThemeColor Color2;

			GradientStyle() = default;
			GradientStyle(
				GradientType type, GradientDirection dir,
				const ThemeColor &color1, const ThemeColor &color2)
				: Type(type)
				, Direction(dir)
				, Color1(color1)
				, Color2(color2)
			{
			}

			bool operator==(const GradientStyle &Op) const noexcept = default;

			bool IsSolid() const { return Type == GradientType::Normal && Color1 == Color2; }
			void Rotate(RotateType Rotate);
		};

		enum class FillType {
			None,
			Solid,
			Gradient,
		};

		struct FillStyle
		{
			FillType Type = FillType::None;
			SolidStyle Solid;
			GradientStyle Gradient;

			FillStyle() = default;
			FillStyle(const SolidStyle &solid) : Type(FillType::Solid), Solid(solid) {}
			FillStyle(const GradientStyle &gradient) : Type(FillType::Gradient), Gradient(gradient) {}

			bool operator==(const FillStyle &Op) const noexcept = default;

			ThemeColor GetSolidColor() const;
		};

		enum class BorderType {
			None,
			Solid,
			Sunken,
			Raised,
		};

		struct BorderWidth
		{
			Style::IntValue Left   {1};
			Style::IntValue Top    {1};
			Style::IntValue Right  {1};
			Style::IntValue Bottom {1};

			BorderWidth() = default;
			BorderWidth(int Width) : Left(Width), Top(Width), Right(Width), Bottom(Width) {}

			bool operator==(const BorderWidth &Op) const noexcept = default;
		};

		struct BorderStyle
		{
			BorderType Type = BorderType::None;
			ThemeColor Color;
			BorderWidth Width;

			BorderStyle() = default;
			BorderStyle(BorderType type, const ThemeColor &color) : Type(type), Color(color) {}

			bool operator==(const BorderStyle &Op) const noexcept = default;
		};

		struct BackgroundStyle
		{
			FillStyle Fill;
			BorderStyle Border;

			BackgroundStyle() = default;
			BackgroundStyle(const FillStyle &fill, const BorderStyle &border) : Fill(fill), Border(border) {}
			BackgroundStyle(const FillStyle &fill) : Fill(fill) {}

			bool operator==(const BackgroundStyle &Op) const noexcept = default;
		};

		struct ForegroundStyle
		{
			FillStyle Fill;

			ForegroundStyle() = default;
			ForegroundStyle(const FillStyle &fill) : Fill(fill) {}

			bool operator==(const ForegroundStyle &Op) const noexcept = default;
		};

		struct Style
		{
			BackgroundStyle Back;
			ForegroundStyle Fore;

			Style() = default;
			Style(const BackgroundStyle &back, const ForegroundStyle &fore) : Back(back), Fore(fore) {}

			bool operator==(const Style &Op) const noexcept = default;
		};

		bool Draw(HDC hdc, const RECT &Rect, const SolidStyle &Style);
		bool Draw(HDC hdc, const RECT &Rect, const GradientStyle &Style);
		bool Draw(HDC hdc, const RECT &Rect, const FillStyle &Style);
		bool Draw(HDC hdc, const RECT &Rect, const BackgroundStyle &Style);
		bool Draw(HDC hdc, const RECT &Rect, const ForegroundStyle &Style, LPCTSTR pszText, UINT Flags);
		bool Draw(HDC hdc, const RECT &Rect, const BorderStyle &Style);
		bool Draw(HDC hdc, RECT *pRect, const BorderStyle &Style);
		ThemeColor MixColor(const ThemeColor &Color1, const ThemeColor &Color2, BYTE Ratio = 128);
		FillStyle MixStyle(const FillStyle &Style1, const FillStyle &Style2, BYTE Ratio = 128);
		bool AddBorderRect(const BorderStyle &Style, RECT *pRect);
		bool SubtractBorderRect(const BorderStyle &Style, RECT *pRect);
		bool GetBorderWidths(const BorderStyle &Style, RECT *pRect);

	} // namespace Theme

} // namespace TVTest


#endif
