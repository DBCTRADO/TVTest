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

			SolidStyle() {}
			SolidStyle(const ThemeColor &color) : Color(color) {}
			bool operator==(const SolidStyle &Op) const { return Color==Op.Color; }
			bool operator!=(const SolidStyle &Op) const { return Color!=Op.Color; }
		};

		enum GradientType {
			GRADIENT_NORMAL,
			GRADIENT_GLOSSY,
			GRADIENT_INTERLACED
		};

		enum GradientDirection {
			DIRECTION_HORZ,
			DIRECTION_VERT,
			DIRECTION_HORZMIRROR,
			DIRECTION_VERTMIRROR
		};

		struct GradientStyle
		{
			enum RotateType {
				ROTATE_LEFT,
				ROTATE_RIGHT,
				ROTATE_180
			};

			GradientType Type;
			GradientDirection Direction;
			ThemeColor Color1;
			ThemeColor Color2;

			GradientStyle()
				: Type(GRADIENT_NORMAL)
				, Direction(DIRECTION_HORZ)
			{
			}
			GradientStyle(GradientType type,GradientDirection dir,
						  const ThemeColor &color1,const ThemeColor &color2)
				: Type(type)
				, Direction(dir)
				, Color1(color1)
				, Color2(color2)
			{
			}
			bool operator==(const GradientStyle &Op) const
			{
				return Type==Op.Type && Direction==Op.Direction
					&& Color1==Op.Color1 && Color2==Op.Color2;
			}
			bool operator!=(const GradientStyle &Op) const { return !(*this==Op); }
			bool IsSolid() const { return Type==GRADIENT_NORMAL && Color1==Color2; }
			void Rotate(RotateType Rotate);
		};

		enum FillType {
			FILL_NONE,
			FILL_SOLID,
			FILL_GRADIENT
		};

		struct FillStyle
		{
			FillType Type;
			SolidStyle Solid;
			GradientStyle Gradient;

			FillStyle() : Type(FILL_NONE) {}
			FillStyle(const SolidStyle &solid) : Type(FILL_SOLID), Solid(solid) {}
			FillStyle(const GradientStyle &gradient) : Type(FILL_GRADIENT), Gradient(gradient) {}
			bool operator==(const FillStyle &Op) const {
				return Type==Op.Type && Solid==Op.Solid && Gradient==Op.Gradient;
			}
			bool operator!=(const FillStyle &Op) const { return !(*this==Op); }
			ThemeColor GetSolidColor() const;
		};

		enum BorderType {
			BORDER_NONE,
			BORDER_SOLID,
			BORDER_SUNKEN,
			BORDER_RAISED
		};

		struct BorderWidth
		{
			TVTest::Style::IntValue Left;
			TVTest::Style::IntValue Top;
			TVTest::Style::IntValue Right;
			TVTest::Style::IntValue Bottom;

			BorderWidth() : Left(1), Top(1), Right(1), Bottom(1) {}
			BorderWidth(int Width) : Left(Width), Top(Width), Right(Width), Bottom(Width) {}
			bool operator==(const BorderWidth &Op) const {
				return Left==Op.Left && Top==Op.Top && Right==Op.Right && Bottom==Op.Bottom;
			}
			bool operator!=(const BorderWidth &Op) const { return !(*this==Op); }
		};

		struct BorderStyle
		{
			BorderType Type;
			ThemeColor Color;
			BorderWidth Width;

			BorderStyle() : Type(BORDER_NONE) {}
			BorderStyle(BorderType type,const ThemeColor &color) : Type(type), Color(color) {}
			bool operator==(const BorderStyle &Op) const {
				return Type==Op.Type && Color==Op.Color && Width==Op.Width;
			}
			bool operator!=(const BorderStyle &Op) const { return !(*this==Op); }
		};

		struct BackgroundStyle
		{
			FillStyle Fill;
			BorderStyle Border;

			BackgroundStyle() {}
			BackgroundStyle(const FillStyle &fill,const BorderStyle &border) : Fill(fill), Border(border) {}
			BackgroundStyle(const FillStyle &fill) : Fill(fill) {}
			bool operator==(const BackgroundStyle &Op) const { return Fill==Op.Fill && Border==Op.Border; }
			bool operator!=(const BackgroundStyle &Op) const { return !(*this==Op); }
		};

		struct ForegroundStyle
		{
			FillStyle Fill;

			ForegroundStyle() {}
			ForegroundStyle(const FillStyle &fill) : Fill(fill) {}
			bool operator==(const ForegroundStyle &Op) const { return Fill==Op.Fill; }
			bool operator!=(const ForegroundStyle &Op) const { return !(*this==Op); }
		};

		struct Style
		{
			BackgroundStyle Back;
			ForegroundStyle Fore;

			Style() {}
			Style(const BackgroundStyle &back,const ForegroundStyle &fore) : Back(back), Fore(fore) {}
			bool operator==(const Style &Op) const { return Back==Op.Back && Fore==Op.Fore; }
			bool operator!=(const Style &Op) const { return !(*this==Op); }
		};

		bool Draw(HDC hdc,const RECT &Rect,const SolidStyle &Style);
		bool Draw(HDC hdc,const RECT &Rect,const GradientStyle &Style);
		bool Draw(HDC hdc,const RECT &Rect,const FillStyle &Style);
		bool Draw(HDC hdc,const RECT &Rect,const BackgroundStyle &Style);
		bool Draw(HDC hdc,const RECT &Rect,const ForegroundStyle &Style,LPCTSTR pszText,UINT Flags);
		bool Draw(HDC hdc,const RECT &Rect,const BorderStyle &Style);
		bool Draw(HDC hdc,RECT *pRect,const BorderStyle &Style);
		ThemeColor MixColor(const ThemeColor &Color1,const ThemeColor &Color2,BYTE Ratio=128);
		FillStyle MixStyle(const FillStyle &Style1,const FillStyle &Style2,BYTE Ratio=128);
		bool AddBorderRect(const BorderStyle &Style,RECT *pRect);
		bool SubtractBorderRect(const BorderStyle &Style,RECT *pRect);
		bool GetBorderWidths(const BorderStyle &Style,RECT *pRect);

	}	// namespace Theme

}	// namespace TVTest


#endif
