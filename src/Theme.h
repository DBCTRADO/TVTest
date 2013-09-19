#ifndef THEME_H
#define THEME_H


namespace Theme {

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

	struct GradientInfo {
		GradientType Type;
		GradientDirection Direction;
		COLORREF Color1;
		COLORREF Color2;

		GradientInfo()
			: Type(GRADIENT_NORMAL)
			, Direction(DIRECTION_HORZ)
			, Color1(RGB(0,0,0))
			, Color2(RGB(0,0,0))
		{
		}
		GradientInfo(GradientType type,GradientDirection dir,COLORREF color1,COLORREF color2)
			: Type(type)
			, Direction(dir)
			, Color1(color1)
			, Color2(color2)
		{
		}
	};

	bool FillGradient(HDC hdc,const RECT *pRect,const GradientInfo *pInfo);

	enum BorderType {
		BORDER_NONE,
		BORDER_SOLID,
		BORDER_SUNKEN,
		BORDER_RAISED
	};

	struct BorderInfo {
		BorderType Type;
		COLORREF Color;
		BorderInfo() : Type(BORDER_NONE), Color(RGB(0,0,0)) {}
		BorderInfo(BorderType type,COLORREF color) : Type(type), Color(color) {}
		bool operator==(const BorderInfo &Info) const {
			return Type==Info.Type && Color==Info.Color;
		}
		bool operator!=(const BorderInfo &Info) const {
			return !(*this==Info);
		}
	};

	bool DrawBorder(HDC hdc,const RECT &Rect,BorderType Type);
	bool DrawBorder(HDC hdc,const RECT &Rect,const BorderInfo *pInfo);
	bool DrawBorder(HDC hdc,RECT *pRect,const BorderInfo *pInfo);
	bool AddBorderRect(const BorderInfo *pInfo,RECT *pRect);
	bool SubtractBorderRect(const BorderInfo *pInfo,RECT *pRect);
	bool GetBorderWidths(const BorderInfo *pInfo,RECT *pRect);

	struct Style {
		GradientInfo Gradient;
		BorderInfo Border;
		COLORREF TextColor;
	};

	bool DrawStyleBackground(HDC hdc,const RECT *pRect,const Style *pStyle);

}	// namespace Theme


#endif
