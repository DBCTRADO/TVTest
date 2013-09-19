#include "stdafx.h"
#include "Theme.h"
#include "DrawUtil.h"
#include "Util.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


namespace Theme
{


bool FillGradient(HDC hdc,const RECT *pRect,const GradientInfo *pInfo)
{
	if (hdc==NULL || pRect==NULL || pInfo==NULL)
		return false;

	bool fResult;

	switch (pInfo->Type) {
	case GRADIENT_NORMAL:
		fResult=DrawUtil::FillGradient(hdc,pRect,pInfo->Color1,pInfo->Color2,
									   (DrawUtil::FillDirection)pInfo->Direction);
		break;
	case GRADIENT_GLOSSY:
		fResult=DrawUtil::FillGlossyGradient(hdc,pRect,pInfo->Color1,pInfo->Color2,
											 (DrawUtil::FillDirection)pInfo->Direction);
		break;
	case GRADIENT_INTERLACED:
		fResult=DrawUtil::FillInterlacedGradient(hdc,pRect,pInfo->Color1,pInfo->Color2,
												 (DrawUtil::FillDirection)pInfo->Direction);
		break;
	default:
		fResult=false;
	}
	return fResult;
}


bool DrawBorder(HDC hdc,const RECT &Rect,BorderType Type)
{
	if (hdc==NULL)
		return false;

	RECT rc=Rect;

	switch (Type) {
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


bool DrawBorder(HDC hdc,const RECT &Rect,const BorderInfo *pInfo)
{
	RECT rc=Rect;
	return DrawBorder(hdc,&rc,pInfo);
}


inline BYTE RGBIntensity(COLORREF Color)
{
	return (BYTE)((GetRValue(Color)*19672+GetGValue(Color)*38621+GetBValue(Color)*7500)>>16);
}

inline COLORREF GetHighlightColor(COLORREF Color)
{
	return MixColor(RGB(255,255,255),Color,48+RGBIntensity(Color)/3);
}

inline COLORREF GetShadowColor(COLORREF Color)
{
	return MixColor(Color,RGB(0,0,0),96+RGBIntensity(Color)/2);
}

bool DrawBorder(HDC hdc,RECT *pRect,const BorderInfo *pInfo)
{
	if (hdc==NULL || pRect==NULL || pInfo==NULL)
		return false;

	if (pInfo->Type==BORDER_NONE)
		return true;

	RECT rc=*pRect;
	HPEN hpenOld=static_cast<HPEN>(::SelectObject(hdc,::GetStockObject(DC_PEN)));
	COLORREF OldDCPenColor=::GetDCPenColor(hdc);
	HBRUSH hbrOld=static_cast<HBRUSH>(::SelectObject(hdc,::GetStockObject(NULL_BRUSH)));

	switch (pInfo->Type) {
	case BORDER_SOLID:
		::SetDCPenColor(hdc,pInfo->Color);
		::Rectangle(hdc,rc.left,rc.top,rc.right,rc.bottom);
		break;
	case BORDER_SUNKEN:
		::SetDCPenColor(hdc,GetHighlightColor(pInfo->Color));
		::MoveToEx(hdc,rc.left+1,rc.bottom-1,NULL);
		::LineTo(hdc,rc.right-1,rc.bottom-1);
		::LineTo(hdc,rc.right-1,rc.top);
		::SetDCPenColor(hdc,GetShadowColor(pInfo->Color));
		::LineTo(hdc,rc.left,rc.top);
		::LineTo(hdc,rc.left,rc.bottom);
		break;
	case BORDER_RAISED:
		::SetDCPenColor(hdc,GetHighlightColor(pInfo->Color));
		::MoveToEx(hdc,rc.right-2,rc.top,NULL);
		::LineTo(hdc,rc.left,rc.top);
		::LineTo(hdc,rc.left,rc.bottom-1);
		::SetDCPenColor(hdc,GetShadowColor(pInfo->Color));
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

	::InflateRect(pRect,-1,-1);

	return true;
}


bool AddBorderRect(const BorderInfo *pInfo,RECT *pRect)
{
	if (pInfo==NULL || pRect==NULL)
		return false;
	if (pInfo->Type!=BORDER_NONE)
		::InflateRect(pRect,1,1);
	return true;
}


bool SubtractBorderRect(const BorderInfo *pInfo,RECT *pRect)
{
	if (pInfo==NULL || pRect==NULL)
		return false;
	if (pInfo->Type!=BORDER_NONE)
		::InflateRect(pRect,-1,-1);
	return true;
}


bool GetBorderWidths(const BorderInfo *pInfo,RECT *pRect)
{
	if (pInfo==NULL || pRect==NULL)
		return false;
	if (pInfo->Type!=BORDER_NONE) {
		pRect->left=1;
		pRect->top=1;
		pRect->right=1;
		pRect->bottom=1;
	} else {
		::SetRectEmpty(pRect);
	}
	return true;
}


bool DrawStyleBackground(HDC hdc,const RECT *pRect,const Style *pStyle)
{
	if (hdc==NULL || pRect==NULL || pStyle==NULL)
		return false;

	RECT rc=*pRect;

	if (pStyle->Border.Type!=BORDER_NONE)
		DrawBorder(hdc,&rc,&pStyle->Border);
	FillGradient(hdc,&rc,&pStyle->Gradient);
	return true;
}


}	// namespace Theme
