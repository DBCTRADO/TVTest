#include "stdafx.h"
#include "Theme.h"
#include "DrawUtil.h"
#include "Util.h"
#include <utility>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


namespace TVTest
{

namespace Theme
{


void GradientStyle::Rotate(RotateType Rotate)
{
	if (Rotate==ROTATE_LEFT || Rotate==ROTATE_RIGHT) {
		switch (Direction) {
		case DIRECTION_HORZ:		Direction=DIRECTION_VERT;		break;
		case DIRECTION_VERT:		Direction=DIRECTION_HORZ;		break;
		case DIRECTION_HORZMIRROR:	Direction=DIRECTION_VERTMIRROR;	break;
		case DIRECTION_VERTMIRROR:	Direction=DIRECTION_HORZMIRROR;	break;
		}
	}
	if ((Rotate==ROTATE_LEFT || Rotate==ROTATE_180)
			&& (Direction==DIRECTION_HORZ || Direction==DIRECTION_VERT))
		std::swap(Color1,Color2);
}




ThemeColor FillStyle::GetSolidColor() const
{
	switch (Type) {
	case FILL_SOLID:
		return Solid.Color;

	case FILL_GRADIENT:
		return MixColor(Gradient.Color1,Gradient.Color2);
	}

	return ThemeColor();
}




bool Draw(HDC hdc,const RECT &Rect,const SolidStyle &Style)
{
	if (hdc==NULL)
		return false;

	return DrawUtil::Fill(hdc,&Rect,Style.Color);
}


bool Draw(HDC hdc,const RECT &Rect,const GradientStyle &Style)
{
	if (hdc==NULL)
		return false;

	switch (Style.Type) {
	case GRADIENT_NORMAL:
		return DrawUtil::FillGradient(hdc,&Rect,Style.Color1,Style.Color2,
									  (DrawUtil::FillDirection)Style.Direction);

	case GRADIENT_GLOSSY:
		return DrawUtil::FillGlossyGradient(hdc,&Rect,Style.Color1,Style.Color2,
											(DrawUtil::FillDirection)Style.Direction);

	case GRADIENT_INTERLACED:
		return DrawUtil::FillInterlacedGradient(hdc,&Rect,Style.Color1,Style.Color2,
												(DrawUtil::FillDirection)Style.Direction);
	}

	return false;
}


bool Draw(HDC hdc,const RECT &Rect,const FillStyle &Style)
{
	if (hdc==NULL)
		return false;

	switch (Style.Type) {
	case FILL_NONE:
		return true;

	case FILL_SOLID:
		return Draw(hdc,Rect,Style.Solid);

	case FILL_GRADIENT:
		return Draw(hdc,Rect,Style.Gradient);
	}

	return false;
}


bool Draw(HDC hdc,const RECT &Rect,const BackgroundStyle &Style)
{
	if (hdc==NULL)
		return false;

	RECT rc=Rect;

	if (Style.Border.Type!=BORDER_NONE)
		Draw(hdc,&rc,Style.Border);
	Draw(hdc,rc,Style.Fill);

	return false;
}


bool Draw(HDC hdc,const RECT &Rect,const ForegroundStyle &Style,LPCTSTR pszText,UINT Flags)
{
	if (hdc==NULL)
		return false;

	ThemeColor c;
	switch (Style.Fill.Type) {
	case FILL_NONE:
		return true;
	case FILL_SOLID:
		c=Style.Fill.Solid.Color;
		break;
	case FILL_GRADIENT:
		c=MixColor(Style.Fill.Gradient.Color1,Style.Fill.Gradient.Color2);
		break;
	default:
		return false;
	}

	COLORREF OldTextColor=::SetTextColor(hdc,c);
	RECT rc=Rect;
	::DrawText(hdc,pszText,-1,&rc,Flags);
	::SetTextColor(hdc,OldTextColor);

	return true;
}


bool Draw(HDC hdc,const RECT &Rect,BorderType Border)
{
	if (hdc==NULL)
		return false;

	RECT rc=Rect;

	switch (Border) {
	case BORDER_SOLID:
		{
			HPEN hpen,hpenOld;
			HBRUSH hbrOld;

			hpen=::CreatePen(PS_SOLID,1,::GetSysColor(COLOR_3DFACE));
			hpenOld=static_cast<HPEN>(::SelectObject(hdc,hpen));
			hbrOld=static_cast<HBRUSH>(::SelectObject(hdc,::GetStockObject(NULL_BRUSH)));
			::Rectangle(hdc,rc.left,rc.top,rc.right,rc.bottom);
			::SelectObject(hdc,hbrOld);
			::SelectObject(hdc,hpenOld);
			::DeleteObject(hpen);
		}
		break;

	case BORDER_SUNKEN:
		::DrawEdge(hdc,&rc,BDR_SUNKENINNER,BF_RECT);
		break;

	case BORDER_RAISED:
		::DrawEdge(hdc,&rc,BDR_RAISEDOUTER,BF_RECT);
		break;

	default:
		return false;
	}

	return true;
}


bool Draw(HDC hdc,const RECT &Rect,const BorderStyle &Style)
{
	RECT rc=Rect;
	return Draw(hdc,&rc,Style);
}


inline BYTE RGBIntensity(const ThemeColor &Color)
{
	return (BYTE)((Color.Red*19672+Color.Green*38621+Color.Blue*7500)>>16);
}

inline ThemeColor GetHighlightColor(const ThemeColor &Color)
{
	return MixColor(ThemeColor(255,255,255),Color,48+RGBIntensity(Color)/3);
}

inline ThemeColor GetShadowColor(const ThemeColor &Color)
{
	return MixColor(Color,ThemeColor(0,0,0),96+RGBIntensity(Color)/2);
}

bool Draw(HDC hdc,RECT *pRect,const BorderStyle &Style)
{
	if (hdc==NULL || pRect==NULL)
		return false;

	if (Style.Type==BORDER_NONE)
		return true;

	RECT rc=*pRect;
	HPEN hpenOld=static_cast<HPEN>(::SelectObject(hdc,::GetStockObject(DC_PEN)));
	COLORREF OldDCPenColor=::GetDCPenColor(hdc);
	HBRUSH hbrOld=static_cast<HBRUSH>(::SelectObject(hdc,::GetStockObject(NULL_BRUSH)));

	switch (Style.Type) {
	case BORDER_SOLID:
		::SetDCPenColor(hdc,Style.Color);
		::Rectangle(hdc,rc.left,rc.top,rc.right,rc.bottom);
		break;

	case BORDER_SUNKEN:
		::SetDCPenColor(hdc,GetHighlightColor(Style.Color));
		::MoveToEx(hdc,rc.left+1,rc.bottom-1,NULL);
		::LineTo(hdc,rc.right-1,rc.bottom-1);
		::LineTo(hdc,rc.right-1,rc.top);
		::SetDCPenColor(hdc,GetShadowColor(Style.Color));
		::LineTo(hdc,rc.left,rc.top);
		::LineTo(hdc,rc.left,rc.bottom);
		break;

	case BORDER_RAISED:
		::SetDCPenColor(hdc,GetHighlightColor(Style.Color));
		::MoveToEx(hdc,rc.right-2,rc.top,NULL);
		::LineTo(hdc,rc.left,rc.top);
		::LineTo(hdc,rc.left,rc.bottom-1);
		::SetDCPenColor(hdc,GetShadowColor(Style.Color));
		::LineTo(hdc,rc.right-1,rc.bottom-1);
		::LineTo(hdc,rc.right-1,rc.top-1);
		break;

	default:
		::SelectObject(hdc,hbrOld);
		::SelectObject(hdc,hpenOld);
		return false;
	}

	::SelectObject(hdc,hbrOld);
	::SetDCPenColor(hdc,OldDCPenColor);
	::SelectObject(hdc,hpenOld);

	SubtractBorderRect(Style,pRect);

	return true;
}


ThemeColor MixColor(const ThemeColor &Color1,const ThemeColor &Color2,BYTE Ratio)
{
	return ThemeColor(
		(Color1.Red  *Ratio+Color2.Red  *(255-Ratio))/255,
		(Color1.Green*Ratio+Color2.Green*(255-Ratio))/255,
		(Color1.Blue *Ratio+Color2.Blue *(255-Ratio))/255,
		(Color1.Alpha*Ratio+Color2.Alpha*(255-Ratio))/255);
}


FillStyle MixStyle(const FillStyle &Style1,const FillStyle &Style2,BYTE Ratio)
{
	if (Ratio==0 || Style1.Type==FILL_NONE)
		return Style2;
	if (Ratio==255 || Style2.Type==FILL_NONE)
		return Style1;

	if (Style1.Type==Style2.Type) {
		switch (Style1.Type) {
		case FILL_SOLID:
			return FillStyle(SolidStyle(MixColor(Style1.Solid.Color,Style2.Solid.Color,Ratio)));

		case FILL_GRADIENT:
			if (Style1.Gradient.Type==Style2.Gradient.Type
					&& Style1.Gradient.Direction==Style2.Gradient.Direction) {
				return FillStyle(
					GradientStyle(Style1.Gradient.Type,
								  Style1.Gradient.Direction,
								  MixColor(Style1.Gradient.Color1,Style2.Gradient.Color1,Ratio),
								  MixColor(Style1.Gradient.Color2,Style2.Gradient.Color2,Ratio)));
			}
			break;
		}
	}

	if (Style1.Type==FILL_GRADIENT && Style2.Type==FILL_SOLID) {
		FillStyle Style(Style1);
		Style.Gradient.Color1=MixColor(Style1.Gradient.Color1,Style2.Solid.Color,Ratio);
		Style.Gradient.Color2=MixColor(Style1.Gradient.Color2,Style2.Solid.Color,Ratio);
		return Style;
	}
	if (Style1.Type==FILL_SOLID && Style2.Type==FILL_GRADIENT) {
		FillStyle Style(Style2);
		Style.Gradient.Color1=MixColor(Style1.Solid.Color,Style2.Gradient.Color1,Ratio);
		Style.Gradient.Color2=MixColor(Style1.Solid.Color,Style2.Gradient.Color2,Ratio);
		return Style;
	}

	return FillStyle(SolidStyle(MixColor(Style1.GetSolidColor(),Style2.GetSolidColor(),Ratio)));
}


bool AddBorderRect(const BorderStyle &Style,RECT *pRect)
{
	if (pRect==NULL)
		return false;
	if (Style.Type!=BORDER_NONE)
		::InflateRect(pRect,1,1);
	return true;
}


bool SubtractBorderRect(const BorderStyle &Style,RECT *pRect)
{
	if (pRect==NULL)
		return false;
	if (Style.Type!=BORDER_NONE)
		::InflateRect(pRect,-1,-1);
	return true;
}


bool GetBorderWidths(const BorderStyle &Style,RECT *pRect)
{
	if (pRect==NULL)
		return false;
	if (Style.Type!=BORDER_NONE) {
		pRect->left=1;
		pRect->top=1;
		pRect->right=1;
		pRect->bottom=1;
	} else {
		::SetRectEmpty(pRect);
	}
	return true;
}


}	// namespace Theme

}	// namespace TVTest
