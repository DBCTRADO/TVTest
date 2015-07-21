#include "stdafx.h"
#include "DrawUtil.h"
#include "Graphics.h"
#include "Util.h"
#include "Common/DebugDef.h"


#define DIVIDE_BY_255(v) ((((v)+1)*257)>>16)




namespace DrawUtil {


// 単色で塗りつぶす
bool Fill(HDC hdc,const RECT *pRect,COLORREF Color)
{
	if (hdc==NULL || pRect==NULL)
		return false;
	COLORREF OldColor=::SetDCBrushColor(hdc,Color);
	BOOL fResult=::FillRect(hdc,pRect,static_cast<HBRUSH>(::GetStockObject(DC_BRUSH)));
	::SetDCBrushColor(hdc,OldColor);
	return fResult!=FALSE;
}


// グラデーションで塗りつぶす
bool FillGradient(HDC hdc,const RECT *pRect,COLORREF Color1,COLORREF Color2,
				  FillDirection Direction)
{
	if (hdc==NULL || pRect==NULL
			|| pRect->left>=pRect->right || pRect->top>=pRect->bottom)
		return false;

	if ((pRect->right-pRect->left==1
				&& (Direction==DIRECTION_HORZ || Direction==DIRECTION_HORZMIRROR))
			|| (pRect->bottom-pRect->top==1
				&& (Direction==DIRECTION_VERT || Direction==DIRECTION_VERTMIRROR)))
		return Fill(hdc,pRect,MixColor(Color1,Color2));

	if (Direction==DIRECTION_HORZMIRROR || Direction==DIRECTION_VERTMIRROR) {
		RECT rc;

		rc=*pRect;
		if (Direction==DIRECTION_HORZMIRROR) {
			rc.right=(pRect->left+pRect->right)/2;
			if (rc.right>rc.left) {
				FillGradient(hdc,&rc,Color1,Color2,DIRECTION_HORZ);
				rc.left=rc.right;
			}
			rc.right=pRect->right;
			FillGradient(hdc,&rc,Color2,Color1,DIRECTION_HORZ);
		} else {
			rc.bottom=(pRect->top+pRect->bottom)/2;
			if (rc.bottom>rc.top) {
				FillGradient(hdc,&rc,Color1,Color2,DIRECTION_VERT);
				rc.top=rc.bottom;
			}
			rc.bottom=pRect->bottom;
			FillGradient(hdc,&rc,Color2,Color1,DIRECTION_VERT);
		}
		return true;
	}

	TRIVERTEX vert[2];
	GRADIENT_RECT rect={0,1};

	vert[0].x=pRect->left;
	vert[0].y=pRect->top;
	vert[0].Red=GetRValue(Color1)<<8;
	vert[0].Green=GetGValue(Color1)<<8;
	vert[0].Blue=GetBValue(Color1)<<8;
	vert[0].Alpha=0x0000;
	vert[1].x=pRect->right;
	vert[1].y=pRect->bottom;
	vert[1].Red=GetRValue(Color2)<<8;
	vert[1].Green=GetGValue(Color2)<<8;
	vert[1].Blue=GetBValue(Color2)<<8;
	vert[1].Alpha=0x0000;
	return ::GdiGradientFill(hdc,vert,2,&rect,1,
		Direction==DIRECTION_HORZ?GRADIENT_FILL_RECT_H:GRADIENT_FILL_RECT_V)!=FALSE;
}


static BYTE BlendAlpha(int Alpha1,int Alpha2,int Pos,int Max)
{
	if (Max<=0)
		return static_cast<BYTE>((Alpha1+Alpha2)/2);
	return static_cast<BYTE>((Alpha1*(Max-Pos)+Alpha2*Pos)/Max);
}


bool FillGradient(HDC hdc,const RECT *pRect,const RGBA &Color1,const RGBA &Color2,
				  FillDirection Direction)
{
	if (hdc==NULL || pRect==NULL
			|| pRect->left>=pRect->right || pRect->top>=pRect->bottom)
		return false;

	if (Color1.Alpha==255 && Color2.Alpha==255)
		return FillGradient(hdc,pRect,Color1.GetCOLORREF(),Color2.GetCOLORREF(),Direction);

	if (Direction==DIRECTION_HORZMIRROR || Direction==DIRECTION_VERTMIRROR) {
		RECT rc;

		rc=*pRect;
		if (Direction==DIRECTION_HORZMIRROR) {
			rc.right=(pRect->left+pRect->right)/2;
			if (rc.right>rc.left) {
				FillGradient(hdc,&rc,Color1,Color2,DIRECTION_HORZ);
				rc.left=rc.right;
			}
			rc.right=pRect->right;
			FillGradient(hdc,&rc,Color2,Color1,DIRECTION_HORZ);
		} else {
			rc.bottom=(pRect->top+pRect->bottom)/2;
			if (rc.bottom>rc.top) {
				FillGradient(hdc,&rc,Color1,Color2,DIRECTION_VERT);
				rc.top=rc.bottom;
			}
			rc.bottom=pRect->bottom;
			FillGradient(hdc,&rc,Color2,Color1,DIRECTION_VERT);
		}
		return true;
	}

	const int Width=pRect->right-pRect->left;
	const int Height=pRect->bottom-pRect->top;
	HBITMAP hbm=::CreateCompatibleBitmap(hdc,Width,Height);
	if (hbm==NULL)
		return false;
	HDC hdcMem=::CreateCompatibleDC(hdc);
	HGDIOBJ hOldBmp=::SelectObject(hdcMem,hbm);

	RECT rc={0,0,Width,Height};
	FillGradient(hdcMem,&rc,Color1.GetCOLORREF(),Color2.GetCOLORREF(),Direction);

	BLENDFUNCTION BlendFunc={AC_SRC_OVER,0,0,0};
	if (Direction==DIRECTION_HORZ) {
		for (int x=0;x<Width;x++) {
			BlendFunc.SourceConstantAlpha=
				BlendAlpha(Color1.Alpha,Color2.Alpha,x,Width-1);
			if (BlendFunc.SourceConstantAlpha!=0) {
				::GdiAlphaBlend(hdc,x+pRect->left,pRect->top,1,Height,
								hdcMem,x,0,1,Height,
								BlendFunc);
			}
		}
	} else {
		for (int y=0;y<Height;y++) {
			BlendFunc.SourceConstantAlpha=
				BlendAlpha(Color1.Alpha,Color2.Alpha,y,Height-1);
			if (BlendFunc.SourceConstantAlpha!=0) {
				::GdiAlphaBlend(hdc,pRect->left,y+pRect->top,Width,1,
								hdcMem,0,y,Width,1,
								BlendFunc);
			}
		}
	}

	::SelectObject(hdcMem,hOldBmp);
	::DeleteDC(hdcMem);
	::DeleteObject(hbm);

	return true;
}


// 光沢のあるグラデーションで塗りつぶす
bool FillGlossyGradient(HDC hdc,const RECT *pRect,
						COLORREF Color1,COLORREF Color2,
						FillDirection Direction,int GlossRatio1,int GlossRatio2)
{
	RECT rc;
	COLORREF crCenter,crEnd;
	FillDirection Dir;

	rc.left=pRect->left;
	rc.top=pRect->top;
	if (Direction==DIRECTION_HORZ || Direction==DIRECTION_HORZMIRROR) {
		rc.right=(rc.left+pRect->right)/2;
		rc.bottom=pRect->bottom;
		Dir=DIRECTION_HORZ;
	} else {
		rc.right=pRect->right;
		rc.bottom=(rc.top+pRect->bottom)/2;
		Dir=DIRECTION_VERT;
	}
	if (Direction==DIRECTION_HORZ || Direction==DIRECTION_VERT) {
		crCenter=MixColor(Color1,Color2,128);
		crEnd=Color2;
	} else {
		crCenter=Color2;
		crEnd=Color1;
	}
	DrawUtil::FillGradient(hdc,&rc,
						   MixColor(RGB(255,255,255),Color1,GlossRatio1),
						   MixColor(RGB(255,255,255),crCenter,GlossRatio2),
						   Dir);
	if (Direction==DIRECTION_HORZ || Direction==DIRECTION_HORZMIRROR) {
		rc.left=rc.right;
		rc.right=pRect->right;
	} else {
		rc.top=rc.bottom;
		rc.bottom=pRect->bottom;
	}
	DrawUtil::FillGradient(hdc,&rc,crCenter,crEnd,Dir);
	return true;
}


// 縞々のグラデーションで塗りつぶす
bool FillInterlacedGradient(HDC hdc,const RECT *pRect,
							COLORREF Color1,COLORREF Color2,FillDirection Direction,
							COLORREF LineColor,int LineOpacity)
{
	if (hdc==NULL || pRect==NULL)
		return false;

	int Width=pRect->right-pRect->left;
	int Height=pRect->bottom-pRect->top;
	if (Width<=0 || Height<=0)
		return false;
	if (Width==1 || Height==1)
		return Fill(hdc,pRect,MixColor(Color1,Color2));

	HPEN hpenOld=static_cast<HPEN>(::SelectObject(hdc,::GetStockObject(DC_PEN)));
	COLORREF OldPenColor=::GetDCPenColor(hdc);

	if (Direction==DIRECTION_HORZ || Direction==DIRECTION_HORZMIRROR) {
		int Center=pRect->left*2+Width-1;

		for (int x=pRect->left;x<pRect->right;x++) {
			COLORREF Color;

			Color=MixColor(Color1,Color2,
						   (BYTE)(Direction==DIRECTION_HORZ?
								  (pRect->right-1-x)*255/(Width-1):
								  abs(Center-x*2)*255/(Width-1)));
			if ((x-pRect->left)%2==1)
				Color=MixColor(LineColor,Color,LineOpacity);
			::SetDCPenColor(hdc,Color);
			::MoveToEx(hdc,x,pRect->top,NULL);
			::LineTo(hdc,x,pRect->bottom);
		}
	} else {
		int Center=pRect->top*2+Height-1;

		for (int y=pRect->top;y<pRect->bottom;y++) {
			COLORREF Color;

			Color=MixColor(Color1,Color2,
						   (BYTE)(Direction==DIRECTION_VERT?
								  (pRect->bottom-1-y)*255/(Height-1):
								  abs(Center-y*2)*255/(Height-1)));
			if ((y-pRect->top)%2==1)
				Color=MixColor(LineColor,Color,LineOpacity);
			::SetDCPenColor(hdc,Color);
			::MoveToEx(hdc,pRect->left,y,NULL);
			::LineTo(hdc,pRect->right,y);
		}
	}

	::SetDCPenColor(hdc,OldPenColor);
	::SelectObject(hdc,hpenOld);

	return true;
}


// 光沢を描画する
bool GlossOverlay(HDC hdc,const RECT *pRect,
				  int Highlight1,int Highlight2,int Shadow1,int Shadow2)
{
	const int Width=pRect->right-pRect->left;
	const int Height=pRect->bottom-pRect->top;
	if (Width<=0 || Height<=0)
		return false;

	BITMAPINFO bmi;
	::ZeroMemory(&bmi,sizeof(bmi));
	bmi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth=Width;
	bmi.bmiHeader.biHeight=-Height;
	bmi.bmiHeader.biPlanes=1;
	bmi.bmiHeader.biBitCount=32;
	void *pBits;
	HBITMAP hbm=::CreateDIBSection(NULL,&bmi,DIB_RGB_COLORS,&pBits,NULL,0);
	if (hbm==NULL)
		return false;

	const SIZE_T RowBytes=Width*4;
	const int Center=Height/2;
	int x,y;
	BYTE *p=static_cast<BYTE*>(pBits);
	for (y=0;y<Center;y++) {
		::FillMemory(p,RowBytes,BlendAlpha(Highlight1,Highlight2,y,Center-1));
		p+=RowBytes;
	}
	for (;y<Height;y++) {
		const BYTE Alpha=BlendAlpha(Shadow1,Shadow2,y-Center,Height-Center-1);
		::ZeroMemory(p,RowBytes);
		for (x=0;x<Width;x++) {
			p[x*4+3]=Alpha;
		}
		p+=RowBytes;
	}

	HDC hdcMemory=::CreateCompatibleDC(hdc);
	if (hdcMemory==NULL) {
		::DeleteObject(hbm);
		return false;
	}
	HBITMAP hbmOld=SelectBitmap(hdcMemory,hbm);
	BLENDFUNCTION bf={AC_SRC_OVER,0,255,AC_SRC_ALPHA};
	::GdiAlphaBlend(hdc,pRect->left,pRect->top,Width,Height,
					hdcMemory,0,0,Width,Height,bf);
	::SelectObject(hdcMemory,hbmOld);
	::DeleteDC(hdcMemory);
	::DeleteObject(hbm);
	return true;
}


// 単色を合成する
bool ColorOverlay(HDC hdc,const RECT *pRect,COLORREF Color,BYTE Opacity)
{
	const int Width=pRect->right-pRect->left;
	const int Height=pRect->bottom-pRect->top;
	if (Width<=0 || Height<=0)
		return false;

	BITMAPINFO bmi;
	::ZeroMemory(&bmi,sizeof(bmi));
	bmi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth=Width;
	bmi.bmiHeader.biHeight=-Height;
	bmi.bmiHeader.biPlanes=1;
	bmi.bmiHeader.biBitCount=32;
	void *pBits;
	HBITMAP hbm=::CreateDIBSection(NULL,&bmi,DIB_RGB_COLORS,&pBits,NULL,0);
	if (hbm==NULL)
		return false;

	const DWORD Pixel=0xFF000000|((DWORD)GetRValue(Color)<<16)|((DWORD)GetGValue(Color)<<8)|(DWORD)GetBValue(Color);
	DWORD *p=static_cast<DWORD*>(pBits);
	DWORD *pEnd=p+Width*Height;
	do {
		*p++=Pixel;
	} while (p<pEnd);

	HDC hdcMemory=::CreateCompatibleDC(hdc);
	if (hdcMemory==NULL) {
		::DeleteObject(hbm);
		return false;
	}
	HBITMAP hbmOld=SelectBitmap(hdcMemory,hbm);
	BLENDFUNCTION bf={AC_SRC_OVER,0,Opacity,0};
	::GdiAlphaBlend(hdc,pRect->left,pRect->top,Width,Height,
					hdcMemory,0,0,Width,Height,bf);
	::SelectObject(hdcMemory,hbmOld);
	::DeleteDC(hdcMemory);
	::DeleteObject(hbm);
	return true;
}


// 指定された矩形の周囲を塗りつぶす
bool FillBorder(HDC hdc,const RECT *pBorderRect,const RECT *pEmptyRect,const RECT *pPaintRect,HBRUSH hbr)
{
	RECT rc;

	if (pPaintRect->left<pBorderRect->right && pPaintRect->right>pBorderRect->left) {
		rc.left=max(pPaintRect->left,pBorderRect->left);
		rc.right=min(pPaintRect->right,pBorderRect->right);
		rc.top=max(pPaintRect->top,pBorderRect->top);
		rc.bottom=min(pPaintRect->bottom,pEmptyRect->top);
		if (rc.top<rc.bottom)
			::FillRect(hdc,&rc,hbr);
		rc.top=max(pEmptyRect->bottom,pPaintRect->top);
		rc.bottom=min(pPaintRect->bottom,pBorderRect->bottom);
		if (rc.top<rc.bottom)
			::FillRect(hdc,&rc,hbr);
	}
	if (pPaintRect->top<pEmptyRect->bottom && pPaintRect->bottom>pEmptyRect->top) {
		rc.top=max(pEmptyRect->top,pPaintRect->top);
		rc.bottom=min(pEmptyRect->bottom,pPaintRect->bottom);
		rc.left=max(pPaintRect->left,pBorderRect->left);
		rc.right=min(pEmptyRect->left,pPaintRect->right);
		if (rc.left<rc.right)
			::FillRect(hdc,&rc,hbr);
		rc.left=max(pPaintRect->left,pEmptyRect->right);
		rc.right=min(pPaintRect->right,pBorderRect->right);
		if (rc.left<rc.right)
			::FillRect(hdc,&rc,hbr);
	}
	return true;
}


bool FillBorder(HDC hdc,const RECT *pBorderRect,const RECT *pEmptyRect,const RECT *pPaintRect,COLORREF Color)
{
	COLORREF OldColor=::SetDCBrushColor(hdc,Color);
	bool fResult=FillBorder(hdc,pBorderRect,pEmptyRect,pPaintRect,
							static_cast<HBRUSH>(::GetStockObject(DC_BRUSH)));
	::SetDCBrushColor(hdc,OldColor);
	return fResult;
}


// ビットマップを描画する
bool DrawBitmap(HDC hdc,int DstX,int DstY,int DstWidth,int DstHeight,
				HBITMAP hbm,const RECT *pSrcRect,BYTE Opacity)
{
	if (hdc==NULL || hbm==NULL)
		return false;

	int SrcX,SrcY,SrcWidth,SrcHeight;
	if (pSrcRect!=NULL) {
		SrcX=pSrcRect->left;
		SrcY=pSrcRect->top;
		SrcWidth=pSrcRect->right-pSrcRect->left;
		SrcHeight=pSrcRect->bottom-pSrcRect->top;
	} else {
		BITMAP bm;
		if (::GetObject(hbm,sizeof(BITMAP),&bm)!=sizeof(BITMAP))
			return false;
		SrcX=SrcY=0;
		SrcWidth=bm.bmWidth;
		SrcHeight=bm.bmHeight;
	}

	HDC hdcMemory=::CreateCompatibleDC(hdc);
	if (hdcMemory==NULL)
		return false;
	HBITMAP hbmOld=static_cast<HBITMAP>(::SelectObject(hdcMemory,hbm));

	if (Opacity==255) {
		if (SrcWidth==DstWidth && SrcHeight==DstHeight) {
			::BitBlt(hdc,DstX,DstY,DstWidth,DstHeight,
					 hdcMemory,SrcX,SrcY,SRCCOPY);
		} else {
			int OldStretchMode=::SetStretchBltMode(hdc,STRETCH_HALFTONE);
			::StretchBlt(hdc,DstX,DstY,DstWidth,DstHeight,
						 hdcMemory,SrcX,SrcY,SrcWidth,SrcHeight,SRCCOPY);
			::SetStretchBltMode(hdc,OldStretchMode);
		}
	} else {
		BLENDFUNCTION bf={AC_SRC_OVER,0,Opacity,0};
		::GdiAlphaBlend(hdc,DstX,DstY,DstWidth,DstHeight,
						hdcMemory,SrcX,SrcY,SrcWidth,SrcHeight,bf);
	}

	::SelectObject(hdcMemory,hbmOld);
	::DeleteDC(hdcMemory);
	return true;
}


// 単色で画像を描画する
bool DrawMonoColorDIB(HDC hdcDst,int DstX,int DstY,
					  HDC hdcSrc,int SrcX,int SrcY,int Width,int Height,COLORREF Color)
{
	if (hdcDst==NULL || hdcSrc==NULL)
		return false;

	COLORREF TransColor=Color^0x00FFFFFF;
	RGBQUAD Palette[2];

	Palette[0].rgbBlue=GetBValue(Color);
	Palette[0].rgbGreen=GetGValue(Color);
	Palette[0].rgbRed=GetRValue(Color);
	Palette[0].rgbReserved=0;
	Palette[1].rgbBlue=GetBValue(TransColor);
	Palette[1].rgbGreen=GetGValue(TransColor);
	Palette[1].rgbRed=GetRValue(TransColor);
	Palette[1].rgbReserved=0;
	::SetDIBColorTable(hdcSrc,0,2,Palette);
	::GdiTransparentBlt(hdcDst,DstX,DstY,Width,Height,
						hdcSrc,SrcX,SrcY,Width,Height,TransColor);
	return true;
}


bool DrawMonoColorDIB(HDC hdcDst,int DstX,int DstY,
					  HBITMAP hbm,int SrcX,int SrcY,int Width,int Height,COLORREF Color)
{
	if (hdcDst==NULL || hbm==NULL)
		return false;

	HDC hdcMem=::CreateCompatibleDC(hdcDst);
	if (hdcMem==NULL)
		return false;

	HBITMAP hbmOld=static_cast<HBITMAP>(::SelectObject(hdcMem,hbm));
	DrawMonoColorDIB(hdcDst,DstX,DstY,
					 hdcMem,SrcX,SrcY,Width,Height,Color);
	::SelectObject(hdcMem,hbmOld);
	::DeleteDC(hdcMem);

	return true;
}


HBITMAP CreateDIB(int Width,int Height,int BitCount,void **ppBits)
{
	struct {
		BITMAPINFOHEADER bmiHeader;
		RGBQUAD bmiColors[256];
	} bmi;
	void *pBits;

	::ZeroMemory(&bmi,sizeof(bmi));
	bmi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth=Width;
	bmi.bmiHeader.biHeight=Height;
	bmi.bmiHeader.biPlanes=1;
	bmi.bmiHeader.biBitCount=BitCount;
	bmi.bmiHeader.biCompression=BI_RGB;
	HBITMAP hbm=::CreateDIBSection(NULL,(BITMAPINFO*)&bmi,DIB_RGB_COLORS,&pBits,NULL,0);
	if (hbm==NULL)
		return NULL;
	if (ppBits!=NULL)
		*ppBits=pBits;
	return hbm;
}


HBITMAP DuplicateDIB(HBITMAP hbmSrc)
{
	if (hbmSrc==NULL)
		return NULL;

	BITMAP bm;
	if (::GetObject(hbmSrc,sizeof(bm),&bm)!=sizeof(bm)
			|| bm.bmBits==NULL)
		return NULL;

	void *pBits;
	HBITMAP hbm=CreateDIB(bm.bmWidth,bm.bmHeight,bm.bmBitsPixel,&pBits);
	if (hbm==NULL)
		return NULL;

	::CopyMemory(pBits,bm.bmBits,bm.bmHeight*bm.bmWidthBytes);

	if (bm.bmBitsPixel<=8) {
		HDC hdc=::CreateCompatibleDC(NULL);
		if (hdc==NULL) {
			::DeleteObject(hbm);
			return NULL;
		}
		HGDIOBJ hOldBitmap=::SelectObject(hdc,hbmSrc);
		RGBQUAD ColorTable[256];
		::GetDIBColorTable(hdc,0,1<<bm.bmBitsPixel,ColorTable);
		::SelectObject(hdc,hbm);
		::SetDIBColorTable(hdc,0,1<<bm.bmBitsPixel,ColorTable);
		::SelectObject(hdc,hOldBitmap);
		::DeleteDC(hdc);
	}

	return hbm;
}


HBITMAP ResizeBitmap(HBITMAP hbmSrc,int Width,int Height,int BitCount,int StretchMode)
{
	if (hbmSrc==NULL || Width<1 || Height==0)
		return NULL;

	HBITMAP hbm=CreateDIB(Width,Height,BitCount);
	if (hbm==NULL)
		return NULL;

	bool fOK=false;
	HDC hdcSrc=::CreateCompatibleDC(NULL);
	HDC hdcDst=::CreateCompatibleDC(NULL);
	if (hdcSrc!=NULL && hdcDst!=NULL) {
		HBITMAP hbmSrcOld=SelectBitmap(hdcSrc,hbmSrc);
		HBITMAP hbmDstOld=SelectBitmap(hdcDst,hbm);
		int OldStretchMode=::SetStretchBltMode(hdcDst,StretchMode);
		BITMAP bm;
		::GetObject(hbmSrc,sizeof(bm),&bm);
		::StretchBlt(hdcDst,0,0,Width,abs(Height),
					 hdcSrc,0,0,bm.bmWidth,bm.bmHeight,SRCCOPY);
		::SetStretchBltMode(hdcDst,OldStretchMode);
		::SelectObject(hdcDst,hbmDstOld);
		::SelectObject(hdcSrc,hbmSrcOld);
		fOK=true;
	}
	if (hdcDst!=NULL)
		::DeleteDC(hdcDst);
	if (hdcSrc!=NULL)
		::DeleteDC(hdcSrc);

	if (!fOK) {
		::DeleteObject(hbm);
		return NULL;
	}

	return hbm;
}


// テキストを描画する
bool DrawText(HDC hdc,LPCTSTR pszText,const RECT &Rect,UINT Format,
			  const CFont *pFont,COLORREF Color)
{
	if (hdc==NULL || pszText==NULL)
		return false;

	int OldBkMode;
	COLORREF OldTextColor;
	HFONT hfontOld;

	OldBkMode=::SetBkMode(hdc,TRANSPARENT);
	if (Color!=CLR_INVALID)
		OldTextColor=::SetTextColor(hdc,Color);
	if (pFont!=NULL)
		hfontOld=DrawUtil::SelectObject(hdc,*pFont);
	RECT rc=Rect;
	::DrawText(hdc,pszText,-1,&rc,Format);
	if (pFont!=NULL)
		::SelectObject(hdc,hfontOld);
	if (Color!=CLR_INVALID)
		::SetTextColor(hdc,OldTextColor);
	::SetBkMode(hdc,OldBkMode);
	return true;
}


// システムフォントを取得する
bool GetSystemFont(FontType Type,LOGFONT *pLogFont)
{
	if (pLogFont==NULL)
		return false;
	if (Type==FONT_DEFAULT) {
		return ::GetObject(::GetStockObject(DEFAULT_GUI_FONT),sizeof(LOGFONT),pLogFont)==sizeof(LOGFONT);
	} else {
		NONCLIENTMETRICS ncm;
		LOGFONT *plf;
		ncm.cbSize=CCSIZEOF_STRUCT(NONCLIENTMETRICS,lfMessageFont);
		::SystemParametersInfo(SPI_GETNONCLIENTMETRICS,ncm.cbSize,&ncm,0);
		switch (Type) {
		case FONT_MESSAGE:		plf=&ncm.lfMessageFont;		break;
		case FONT_MENU:			plf=&ncm.lfMenuFont;		break;
		case FONT_CAPTION:		plf=&ncm.lfCaptionFont;		break;
		case FONT_SMALLCAPTION:	plf=&ncm.lfSmCaptionFont;	break;
		case FONT_STATUS:		plf=&ncm.lfStatusFont;		break;
		default:
			return false;
		}
		*pLogFont=*plf;
	}
	return true;
}


// UIに使用するデフォルトのフォントを取得する
bool GetDefaultUIFont(LOGFONT *pFont)
{
	if (pFont==NULL)
		return false;

	::ZeroMemory(pFont,sizeof(LOGFONT));

	LOGFONT MessageFont;
	if (GetSystemFont(FONT_MESSAGE,&MessageFont)) {
		// メイリオだと行間が空きすぎるのが…
		if (::lstrcmp(MessageFont.lfFaceName,TEXT("メイリオ"))==0
				|| ::lstrcmpi(MessageFont.lfFaceName,TEXT("Meiryo"))==0) {
			pFont->lfHeight=-abs(MessageFont.lfHeight);
			pFont->lfWeight=FW_NORMAL;
			::lstrcpy(pFont->lfFaceName,TEXT("Meiryo UI"));
			if (IsFontAvailable(*pFont))
				return true;
		} else {
			*pFont=MessageFont;
			return true;
		}
	}

	return GetSystemFont(FONT_DEFAULT,pFont);
}


bool IsFontAvailable(const LOGFONT &Font,HDC hdc)
{
	HFONT hfont=::CreateFontIndirect(&Font);

	if (hfont==NULL)
		return false;
	HDC hdcMem=NULL;
	if (hdc==NULL) {
		hdcMem=::CreateCompatibleDC(NULL);
		if (hdcMem==NULL)
			return false;
		hdc=hdcMem;
	}
	HFONT hfontOld=SelectFont(hdc,hfont);
	TCHAR szFaceName[LF_FACESIZE];
	bool fAvailable=
		::GetTextFace(hdc,_countof(szFaceName),szFaceName)>0
		&& ::lstrcmpi(szFaceName,Font.lfFaceName)==0;
	::SelectObject(hdc,hfontOld);
	if (hdcMem!=NULL)
		::DeleteDC(hdcMem);

	return fAvailable;
}


bool IsFontSmoothingEnabled()
{
	BOOL fEnabled=FALSE;
	return ::SystemParametersInfo(SPI_GETFONTSMOOTHING,0,&fEnabled,0) && fEnabled;
}


bool IsClearTypeEnabled()
{
	UINT Type;
	return IsFontSmoothingEnabled()
		&& ::SystemParametersInfo(SPI_GETFONTSMOOTHINGTYPE,0,&Type,0)
		&& Type==FE_FONTSMOOTHINGCLEARTYPE;
}


CFont::CFont()
	: m_hfont(NULL)
{
}

CFont::CFont(const CFont &Font)
	: m_hfont(NULL)
{
	*this=Font;
}

CFont::CFont(const LOGFONT &Font)
	: m_hfont(NULL)
{
	Create(&Font);
}

CFont::CFont(FontType Type)
	: m_hfont(NULL)
{
	Create(Type);
}

CFont::~CFont()
{
	Destroy();
}

CFont &CFont::operator=(const CFont &Font)
{
	if (Font.m_hfont) {
		LOGFONT lf;
		Font.GetLogFont(&lf);
		Create(&lf);
	} else {
		if (m_hfont)
			::DeleteObject(m_hfont);
		m_hfont=NULL;
	}
	return *this;
}

bool CFont::operator==(const CFont &Font) const
{
	if (m_hfont==NULL)
		return Font.m_hfont==NULL;
	if (Font.m_hfont==NULL)
		return m_hfont==NULL;
	LOGFONT lf1,lf2;
	GetLogFont(&lf1);
	Font.GetLogFont(&lf2);
	return CompareLogFont(&lf1,&lf2);
}

bool CFont::operator!=(const CFont &Font) const
{
	return !(*this==Font);
}

bool CFont::Create(const LOGFONT *pLogFont)
{
	if (pLogFont==NULL)
		return false;
	HFONT hfont=::CreateFontIndirect(pLogFont);
	if (hfont==NULL)
		return false;
	if (m_hfont)
		::DeleteObject(m_hfont);
	m_hfont=hfont;
	return true;
}

bool CFont::Create(FontType Type)
{
	LOGFONT lf;

	if (!GetSystemFont(Type,&lf))
		return false;
	return Create(&lf);
}

void CFont::Destroy()
{
	if (m_hfont) {
		::DeleteObject(m_hfont);
		m_hfont=NULL;
	}
}

bool CFont::GetLogFont(LOGFONT *pLogFont) const
{
	if (m_hfont==NULL || pLogFont==NULL)
		return false;
	return ::GetObject(m_hfont,sizeof(LOGFONT),pLogFont)==sizeof(LOGFONT);
}

int CFont::GetHeight(bool fCell) const
{
	if (m_hfont==NULL)
		return 0;

	HDC hdc=::CreateCompatibleDC(NULL);
	int Height;
	if (hdc==NULL) {
		LOGFONT lf;
		if (!GetLogFont(&lf))
			return 0;
		Height=abs(lf.lfHeight);
	} else {
		Height=GetHeight(hdc,fCell);
		::DeleteDC(hdc);
	}
	return Height;
}

int CFont::GetHeight(HDC hdc,bool fCell) const
{
	if (m_hfont==NULL || hdc==NULL)
		return 0;
	HGDIOBJ hOldFont=::SelectObject(hdc,m_hfont);
	TEXTMETRIC tm;
	::GetTextMetrics(hdc,&tm);
	::SelectObject(hdc,hOldFont);
	if (!fCell)
		tm.tmHeight-=tm.tmInternalLeading;
	return tm.tmHeight;
}


CBrush::CBrush()
	: m_hbr(NULL)
{
}

CBrush::CBrush(const CBrush &Brush)
	: m_hbr(NULL)
{
	*this=Brush;
}

CBrush::CBrush(COLORREF Color)
	: m_hbr(NULL)
{
	Create(Color);
}

CBrush::~CBrush()
{
	Destroy();
}

CBrush &CBrush::operator=(const CBrush &Brush)
{
	if (&Brush!=this) {
		Destroy();
		if (Brush.m_hbr!=NULL) {
			LOGBRUSH lb;

			if (::GetObject(Brush.m_hbr,sizeof(LOGBRUSH),&lb)==sizeof(LOGBRUSH))
				m_hbr=::CreateBrushIndirect(&lb);
		}
	}
	return *this;
}

bool CBrush::Create(COLORREF Color)
{
	HBRUSH hbr=::CreateSolidBrush(Color);

	if (hbr==NULL)
		return false;
	Destroy();
	m_hbr=hbr;
	return true;
}

void CBrush::Destroy()
{
	if (m_hbr!=NULL) {
		::DeleteObject(m_hbr);
		m_hbr=NULL;
	}
}


CBitmap::CBitmap()
	: m_hbm(NULL)
{
}

CBitmap::CBitmap(const CBitmap &Src)
	: m_hbm(NULL)
{
	*this=Src;
}

CBitmap::~CBitmap()
{
	Destroy();
}

CBitmap &CBitmap::operator=(const CBitmap &Src)
{
	if (&Src!=this) {
		Destroy();
		if (Src.m_hbm!=NULL) {
			if (Src.IsDIB())
				m_hbm=DuplicateDIB(Src.m_hbm);
			else
				m_hbm=static_cast<HBITMAP>(::CopyImage(Src.m_hbm,IMAGE_BITMAP,0,0,0));
		}
	}
	return *this;
}

bool CBitmap::Create(int Width,int Height,int BitCount)
{
	Destroy();
	m_hbm=CreateDIB(Width,Height,BitCount);
	return m_hbm!=NULL;
}

bool CBitmap::Load(HINSTANCE hinst,LPCTSTR pszName,UINT Flags)
{
	Destroy();
	m_hbm=static_cast<HBITMAP>(::LoadImage(hinst,pszName,IMAGE_BITMAP,0,0,Flags));
	return m_hbm!=NULL;
}

bool CBitmap::Attach(HBITMAP hbm)
{
	if (hbm==NULL)
		return false;
	Destroy();
	m_hbm=hbm;
	return true;
}

void CBitmap::Destroy()
{
	if (m_hbm!=NULL) {
		::DeleteObject(m_hbm);
		m_hbm=NULL;
	}
}

bool CBitmap::IsDIB() const
{
	if (m_hbm!=NULL) {
		DIBSECTION ds;
		if (::GetObject(m_hbm,sizeof(ds),&ds)==sizeof(ds))
			return true;
	}
	return false;
}

int CBitmap::GetWidth() const
{
	if (m_hbm!=NULL) {
		BITMAP bm;
		if (::GetObject(m_hbm,sizeof(bm),&bm)==sizeof(bm))
			return bm.bmWidth;
	}
	return 0;
}

int CBitmap::GetHeight() const
{
	if (m_hbm!=NULL) {
		BITMAP bm;
		if (::GetObject(m_hbm,sizeof(bm),&bm)==sizeof(bm))
			return bm.bmHeight;
	}
	return 0;
}


CMonoColorBitmap::CMonoColorBitmap()
	: m_hbm(NULL)
	, m_hbmPremultiplied(NULL)
{
}

CMonoColorBitmap::CMonoColorBitmap(const CMonoColorBitmap &Src)
	: m_hbm(NULL)
	, m_hbmPremultiplied(NULL)
{
	*this=Src;
}

CMonoColorBitmap::CMonoColorBitmap(CMonoColorBitmap &&Src)
	: m_hbm(NULL)
	, m_hbmPremultiplied(NULL)
{
	*this=std::move(Src);
}

CMonoColorBitmap::~CMonoColorBitmap()
{
	Destroy();
}

CMonoColorBitmap &CMonoColorBitmap::operator=(const CMonoColorBitmap &Src)
{
	if (&Src!=this) {
		Destroy();

		if (Src.m_hbm!=NULL)
			m_hbm=DuplicateDIB(Src.m_hbm);
		if (Src.m_hbmPremultiplied!=NULL)
			m_hbmPremultiplied=DuplicateDIB(Src.m_hbmPremultiplied);
		m_Color=Src.m_Color;
		m_fColorImage=Src.m_fColorImage;
	}

	return *this;
}

CMonoColorBitmap &CMonoColorBitmap::operator=(CMonoColorBitmap &&Src)
{
	if (&Src!=this) {
		Destroy();

		m_hbm=Src.m_hbm;
		Src.m_hbm=NULL;
		m_hbmPremultiplied=Src.m_hbmPremultiplied;
		Src.m_hbmPremultiplied=NULL;
		m_Color=Src.m_Color;
		m_fColorImage=Src.m_fColorImage;
	}

	return *this;
}

bool CMonoColorBitmap::Load(HINSTANCE hinst,LPCTSTR pszName)
{
	HBITMAP hbmSrc=static_cast<HBITMAP>(::LoadImage(hinst,pszName,IMAGE_BITMAP,0,0,LR_CREATEDIBSECTION));
	if (hbmSrc==NULL)
		return false;

	bool fResult=Create(hbmSrc);

	::DeleteObject(hbmSrc);

	return fResult;
}

bool CMonoColorBitmap::Create(HBITMAP hbmSrc)
{
	Destroy();

	if (hbmSrc==NULL)
		return false;

	BITMAP bm;
	::GetObject(hbmSrc,sizeof(bm),&bm);
	if ((bm.bmBitsPixel!=8 && bm.bmBitsPixel!=24 && bm.bmBitsPixel!=32)
			|| bm.bmBits==NULL)
		return false;

	void *pBits;
	m_hbm=CreateDIB(bm.bmWidth,bm.bmHeight,32,&pBits);
	if (m_hbm==NULL)
		return false;

	if (bm.bmBitsPixel==32) {
		m_fColorImage=true;
		::CopyMemory(pBits,bm.bmBits,bm.bmWidth*4*bm.bmHeight);

		void *pPremultipliedBits;
		m_hbmPremultiplied=CreateDIB(bm.bmWidth,bm.bmHeight,32,&pPremultipliedBits);
		if (m_hbmPremultiplied==NULL) {
			Destroy();
			return false;
		}
		BYTE *p=static_cast<BYTE*>(pBits);
		BYTE *q=static_cast<BYTE*>(pPremultipliedBits);
		for (int y=0;y<bm.bmHeight;y++) {
			for (int x=0;x<bm.bmWidth;x++) {
				UINT Alpha=p[3];
				q[0]=(BYTE)DIVIDE_BY_255(p[0]*Alpha);
				q[1]=(BYTE)DIVIDE_BY_255(p[1]*Alpha);
				q[2]=(BYTE)DIVIDE_BY_255(p[2]*Alpha);
				q[3]=(BYTE)Alpha;
				p+=4;
				q+=4;
			}
		}
	} else {
		m_fColorImage=false;
		m_hbmPremultiplied=m_hbm;

		const size_t RowBytes=(bm.bmWidth*bm.bmBitsPixel+31)/32*4;
		BYTE *p=static_cast<BYTE*>(bm.bmBits);
		BYTE *q=static_cast<BYTE*>(pBits);
		for (int y=0;y<bm.bmHeight;y++) {
			if (bm.bmBitsPixel==8) {
				for (int x=0;x<bm.bmWidth;x++) {
					q[3]=p[x];
					q+=4;
				}
			} else {
				for (int x=0;x<bm.bmWidth;x++) {
					q[3]=p[x*3];
					q+=4;
				}
			}
			p+=RowBytes;
		}
	}

	m_Color=CLR_INVALID;

	return true;
}

void CMonoColorBitmap::Destroy()
{
	if (m_hbmPremultiplied!=NULL) {
		if (m_hbmPremultiplied!=m_hbm)
			::DeleteObject(m_hbmPremultiplied);
		m_hbmPremultiplied=NULL;
	}

	if (m_hbm!=NULL) {
		::DeleteObject(m_hbm);
		m_hbm=NULL;
	}
}

bool CMonoColorBitmap::Draw(HDC hdc,
							int DstX,int DstY,int DstWidth,int DstHeight,
							int SrcX,int SrcY,int SrcWidth,int SrcHeight,
							COLORREF Color,BYTE Opacity)
{
	if (m_hbmPremultiplied==NULL)
		return false;

	BITMAP bm;
	if (::GetObject(m_hbmPremultiplied,sizeof(bm),&bm)!=sizeof(bm))
		return false;

	if (SrcWidth<=0)
		SrcWidth=bm.bmWidth;
	if (DstWidth<=0)
		DstWidth=SrcWidth;
	if (SrcHeight<=0)
		SrcHeight=bm.bmHeight;
	if (DstHeight<=0)
		DstHeight=SrcHeight;
	if (SrcX<0 || SrcY<0 || SrcX+SrcWidth>bm.bmWidth || SrcY+SrcHeight>bm.bmHeight)
		return false;

	if (!m_fColorImage)
		SetColor(Color);

	HDC hdcMemory=::CreateCompatibleDC(hdc);
	if (hdcMemory==NULL)
		return false;
	HBITMAP hbmOld=static_cast<HBITMAP>(::SelectObject(hdcMemory,m_hbmPremultiplied));
	BLENDFUNCTION bf={AC_SRC_OVER,0,Opacity,AC_SRC_ALPHA};
	::GdiAlphaBlend(hdc,DstX,DstY,DstWidth,DstHeight,
					hdcMemory,SrcX,SrcY,SrcWidth,SrcHeight,bf);
	::SelectObject(hdcMemory,hbmOld);
	::DeleteDC(hdcMemory);

	return true;
}

bool CMonoColorBitmap::Draw(
	HDC hdc,int DstX,int DstY,COLORREF Color,BYTE Opacity,
	int SrcX,int SrcY,int Width,int Height)
{
	return Draw(hdc,DstX,DstY,Width,Height,SrcX,SrcY,Width,Height,Color,Opacity);
}

HIMAGELIST CMonoColorBitmap::CreateImageList(int IconWidth,COLORREF Color)
{
	if (m_hbm==NULL || IconWidth<1)
		return NULL;

	BITMAP bm;
	if (::GetObject(m_hbm,sizeof(bm),&bm)!=sizeof(bm)
			|| bm.bmWidth<IconWidth)
		return NULL;

	HIMAGELIST himl=::ImageList_Create(IconWidth,bm.bmHeight,ILC_COLOR32,0,1);
	if (himl==NULL)
		return NULL;

	HBITMAP hbm=ExtractBitmap(0,0,bm.bmWidth,bm.bmHeight,Color);
	if (hbm==NULL) {
		::ImageList_Destroy(himl);
		return NULL;
	}

	::ImageList_Add(himl,hbm,NULL);

	::DeleteObject(hbm);

	return himl;
}

HBITMAP CMonoColorBitmap::ExtractBitmap(int x,int y,int Width,int Height,COLORREF Color)
{
	if (m_hbm==NULL || x<0 || y<0)
		return NULL;

	BITMAP bm;
	if (::GetObject(m_hbm,sizeof(bm),&bm)!=sizeof(bm)
			|| x+Width>bm.bmWidth
			|| y+Height>bm.bmHeight)
		return NULL;

	void *pBits;
	HBITMAP hbm=CreateDIB(Width,Height,32,&pBits);
	if (hbm==NULL)
		return NULL;

	const BYTE *p=static_cast<const BYTE*>(bm.bmBits)+
		(bm.bmHeight-(y+Height))*bm.bmWidthBytes+x*4;
	BYTE *q=static_cast<BYTE*>(pBits);
	if (m_fColorImage) {
		for (int y=0;y<Height;y++) {
			::CopyMemory(q,p,Width*4);
			p+=bm.bmWidthBytes;
			q+=Width*4;
		}
	} else {
		const BYTE Red=GetRValue(Color),Green=GetGValue(Color),Blue=GetBValue(Color);
		for (int y=0;y<Height;y++) {
			for (int x=0;x<Width;x++) {
				q[0]=Blue;
				q[1]=Green;
				q[2]=Red;
				q[3]=p[3];
				p+=4;
				q+=4;
			}
			p+=bm.bmWidthBytes-Width*4;
		}
	}

	return hbm;
}

HICON CMonoColorBitmap::ExtractIcon(int x,int y,int Width,int Height,COLORREF Color)
{
	HBITMAP hbmColor=ExtractBitmap(x,y,Width,Height,Color);
	if (hbmColor==NULL)
		return NULL;

	HBITMAP hbmMask=::CreateBitmap(Width,Height,1,1,NULL);
	if (hbmMask==NULL) {
		::DeleteObject(hbmColor);
		return NULL;
	}

	ICONINFO ii;
	ii.fIcon=TRUE;
	ii.xHotspot=0;
	ii.yHotspot=0;
	ii.hbmMask=hbmMask;
	ii.hbmColor=hbmColor;

	HICON hico=::CreateIconIndirect(&ii);

	::DeleteObject(hbmMask);
	::DeleteObject(hbmColor);

	return hico;
}

HICON CMonoColorBitmap::ExtractIcon(COLORREF Color)
{
	if (m_hbm==NULL)
		return NULL;

	BITMAP bm;
	if (::GetObject(m_hbm,sizeof(bm),&bm)!=sizeof(bm))
		return NULL;

	return ExtractIcon(0,0,bm.bmWidth,bm.bmHeight,Color);
}

void CMonoColorBitmap::SetColor(COLORREF Color)
{
	if (m_Color!=Color) {
		BITMAP bm;
		if (::GetObject(m_hbmPremultiplied,sizeof(bm),&bm)!=sizeof(bm))
			return;

		const UINT Red=GetRValue(Color),Green=GetGValue(Color),Blue=GetBValue(Color);
		BYTE *p=static_cast<BYTE*>(bm.bmBits);
		for (int y=0;y<bm.bmHeight;y++) {
			for (int x=0;x<bm.bmWidth;x++) {
				UINT Alpha=p[3];
				p[0]=(BYTE)DIVIDE_BY_255(Blue*Alpha);
				p[1]=(BYTE)DIVIDE_BY_255(Green*Alpha);
				p[2]=(BYTE)DIVIDE_BY_255(Red*Alpha);
				p+=4;
			}
		}

		m_Color=Color;
	}
}


CMonoColorIconList::CMonoColorIconList()
	: m_IconWidth(0)
	, m_IconHeight(0)
{
}

bool CMonoColorIconList::Load(HINSTANCE hinst,LPCTSTR pszName,int Width,int Height)
{
	if (!m_Bitmap.Load(hinst,pszName))
		return false;
	m_IconWidth=Width;
	m_IconHeight=Height;
	return true;
}

bool CMonoColorIconList::Create(HBITMAP hbm,int Width,int Height)
{
	if (!m_Bitmap.Create(hbm))
		return false;
	m_IconWidth=Width;
	m_IconHeight=Height;
	return true;
}

void CMonoColorIconList::Destroy()
{
	m_Bitmap.Destroy();
	m_IconWidth=0;
	m_IconHeight=0;
}

bool CMonoColorIconList::IsCreated() const
{
	return m_Bitmap.IsCreated();
}

bool CMonoColorIconList::Draw(
	HDC hdc,int DstX,int DstY,int DstWidth,int DstHeight,
	int IconIndex,COLORREF Color,BYTE Opacity)
{
	if (hdc==NULL || DstWidth<=0 || DstHeight<=0)
		return false;

	// GdiAlphaBlend() はリサイズが汚いため、GDI+ を使う
	if (DstWidth!=m_IconWidth || DstHeight!=m_IconHeight) {
		TVTest::Graphics::CCanvas Canvas(hdc);

		HBITMAP hbm=m_Bitmap.ExtractBitmap(IconIndex*m_IconWidth,0,m_IconWidth,m_IconHeight,Color);
		if (hbm!=NULL) {
			{
				TVTest::Graphics::CImage Image;
				Image.CreateFromBitmap(hbm);
				Canvas.DrawImage(DstX,DstY,DstWidth,DstHeight,
								 &Image,0,0,m_IconWidth,m_IconHeight,
								 (float)Opacity/255.0f);
			}
			::DeleteObject(hbm);
			return true;
		}
	}

	return m_Bitmap.Draw(hdc,DstX,DstY,DstWidth,DstHeight,
						 IconIndex*m_IconWidth,0,m_IconWidth,m_IconHeight,
						 Color,Opacity);
}


CMemoryDC::CMemoryDC()
	: m_hdc(NULL)
{
}

CMemoryDC::CMemoryDC(HDC hdc)
	: m_hdc(NULL)
{
	Create(hdc);
}

CMemoryDC::~CMemoryDC()
{
	Delete();
}

bool CMemoryDC::Create(HDC hdc)
{
	Delete();

	m_hdc=::CreateCompatibleDC(hdc);
	if (m_hdc==NULL)
		return false;
	m_hbmOld=static_cast<HBITMAP>(::GetCurrentObject(m_hdc,OBJ_BITMAP));
	return true;
}

void CMemoryDC::Delete()
{
	if (m_hdc!=NULL) {
		::SelectObject(m_hdc,m_hbmOld);
		::DeleteDC(m_hdc);
		m_hdc=NULL;
	}
}

bool CMemoryDC::SetBitmap(HBITMAP hbm)
{
	if (m_hdc==NULL || hbm==NULL)
		return false;
	::SelectObject(m_hdc,hbm);
	return true;
}

bool CMemoryDC::Draw(HDC hdc,int DstX,int DstY,int SrcX,int SrcY,int Width,int Height)
{
	if (m_hdc==NULL || hdc==NULL || Width<1 || Height<1)
		return false;
	return ::BitBlt(hdc,DstX,DstY,Width,Height,m_hdc,SrcX,SrcY,SRCCOPY)!=FALSE;
}

bool CMemoryDC::DrawStretch(HDC hdc,int DstX,int DstY,int DstWidth,int DstHeight,
							int SrcX,int SrcY,int SrcWidth,int SrcHeight,int Mode)
{
	if (m_hdc==NULL || hdc==NULL)
		return false;
	int OldStretchMode=::SetStretchBltMode(hdc,Mode);
	::StretchBlt(hdc,DstX,DstY,DstWidth,DstHeight,m_hdc,SrcX,SrcY,SrcWidth,SrcHeight,SRCCOPY);
	::SetStretchBltMode(hdc,OldStretchMode);
	return true;
}

bool CMemoryDC::DrawAlpha(HDC hdc,int DstX,int DstY,int SrcX,int SrcY,int Width,int Height)
{
	if (m_hdc==NULL || hdc==NULL)
		return false;
	BLENDFUNCTION bf={AC_SRC_OVER,0,255,AC_SRC_ALPHA};
	::GdiAlphaBlend(hdc,DstX,DstY,Width,Height,m_hdc,SrcX,SrcY,Width,Height,bf);
	return true;
}


COffscreen::COffscreen()
	: m_hdc(NULL)
	, m_hbm(NULL)
	, m_hbmOld(NULL)
	, m_Width(0)
	, m_Height(0)
{
}

COffscreen::~COffscreen()
{
	Destroy();
}

bool COffscreen::Create(int Width,int Height,HDC hdc)
{
	if (Width<=0 || Height<=0)
		return false;
	Destroy();
	HDC hdcScreen;
	if (hdc==NULL) {
		hdcScreen=::GetDC(NULL);
		if (hdcScreen==NULL)
			return false;
		hdc=hdcScreen;
	} else {
		hdcScreen=NULL;
	}
	m_hdc=::CreateCompatibleDC(hdc);
	if (m_hdc==NULL) {
		if (hdcScreen!=NULL)
			::ReleaseDC(NULL,hdcScreen);
		return false;
	}
	m_hbm=::CreateCompatibleBitmap(hdc,Width,Height);
	if (hdcScreen!=NULL)
		::ReleaseDC(NULL,hdcScreen);
	if (m_hbm==NULL) {
		Destroy();
		return false;
	}
	m_hbmOld=static_cast<HBITMAP>(::SelectObject(m_hdc,m_hbm));
	m_Width=Width;
	m_Height=Height;
	return true;
}

void COffscreen::Destroy()
{
	if (m_hbmOld!=NULL) {
		::SelectObject(m_hdc,m_hbmOld);
		m_hbmOld=NULL;
	}
	if (m_hdc!=NULL) {
		::DeleteDC(m_hdc);
		m_hdc=NULL;
	}
	if (m_hbm!=NULL) {
		::DeleteObject(m_hbm);
		m_hbm=NULL;
		m_Width=0;
		m_Height=0;
	}
}

bool COffscreen::CopyTo(HDC hdc,const RECT *pDstRect)
{
	int DstX,DstY,Width,Height;

	if (m_hdc==NULL || hdc==NULL)
		return false;
	if (pDstRect!=NULL) {
		DstX=pDstRect->left;
		DstY=pDstRect->top;
		Width=pDstRect->right-pDstRect->left;
		Height=pDstRect->bottom-pDstRect->top;
		if (Width<=0 || Height<=0)
			return false;
		if (Width>m_Width)
			Width=m_Width;
		if (Height>m_Height)
			Height=m_Height;
	} else {
		DstX=DstY=0;
		Width=m_Width;
		Height=m_Height;
	}
	::BitBlt(hdc,DstX,DstY,Width,Height,m_hdc,0,0,SRCCOPY);
	return true;
}


}	// namespace DrawUtil




#pragma comment(lib, "uxtheme.lib")


CUxTheme::CUxTheme()
	: m_hTheme(NULL)
{
}

CUxTheme::~CUxTheme()
{
	Close();
}

bool CUxTheme::Initialize()
{
	return true;
}

bool CUxTheme::Open(HWND hwnd,LPCWSTR pszClassList)
{
	Close();
	if (!Initialize())
		return false;
	m_hTheme=::OpenThemeData(hwnd,pszClassList);
	if (m_hTheme==NULL)
		return false;
	return true;
}

void CUxTheme::Close()
{
	if (m_hTheme!=NULL) {
		::CloseThemeData(m_hTheme);
		m_hTheme=NULL;
	}
}

bool CUxTheme::IsOpen() const
{
	return m_hTheme!=NULL;
}

bool CUxTheme::IsActive()
{
	return ::IsThemeActive()!=FALSE;
}

bool CUxTheme::DrawBackground(HDC hdc,int PartID,int StateID,const RECT *pRect)
{
	if (m_hTheme==NULL)
		return false;
	return ::DrawThemeBackground(m_hTheme,hdc,PartID,StateID,pRect,NULL)==S_OK;
}

bool CUxTheme::DrawBackground(HDC hdc,int PartID,int StateID,
							  int BackgroundPartID,int BackgroundStateID,
							  const RECT *pRect)
{
	if (m_hTheme==NULL)
		return false;
	if (::IsThemeBackgroundPartiallyTransparent(m_hTheme,PartID,StateID)) {
		if (::DrawThemeBackground(m_hTheme,hdc,
								  BackgroundPartID,BackgroundStateID,
								  pRect,NULL)!=S_OK)
			return false;
	}
	return ::DrawThemeBackground(m_hTheme,hdc,PartID,StateID,pRect,NULL)==S_OK;
}

bool CUxTheme::DrawText(HDC hdc,int PartID,int StateID,LPCWSTR pszText,
						DWORD TextFlags,const RECT *pRect)
{
	if (m_hTheme==NULL)
		return false;
	return ::DrawThemeText(m_hTheme,hdc,PartID,StateID,pszText,lstrlenW(pszText),
						   TextFlags,0,pRect)==S_OK;
}

bool CUxTheme::GetTextExtent(HDC hdc,int PartID,int StateID,LPCWSTR pszText,
							 DWORD TextFlags,RECT *pExtentRect)
{
	if (m_hTheme==NULL)
		return false;
	return ::GetThemeTextExtent(m_hTheme,hdc,PartID,StateID,
								pszText,lstrlenW(pszText),TextFlags,
								NULL,pExtentRect)==S_OK;
}

bool CUxTheme::GetMargins(int PartID,int StateID,int PropID,MARGINS *pMargins)
{
	if (m_hTheme==NULL)
		return false;
	return ::GetThemeMargins(m_hTheme,NULL,PartID,StateID,PropID,NULL,pMargins)==S_OK;
}

bool CUxTheme::GetColor(int PartID,int StateID,int PropID,COLORREF *pColor)
{
	if (m_hTheme==NULL)
		return false;
	return ::GetThemeColor(m_hTheme,PartID,StateID,PropID,pColor)==S_OK;
}

bool CUxTheme::GetFont(int PartID,int StateID,int PropID,LOGFONT *pFont)
{
	if (m_hTheme==NULL)
		return false;
	return ::GetThemeFont(m_hTheme,NULL,PartID,StateID,PropID,pFont)==S_OK;
}

bool CUxTheme::GetInt(int PartID,int StateID,int PropID,int *pValue)
{
	if (m_hTheme==NULL)
		return false;
	return ::GetThemeInt(m_hTheme,PartID,StateID,PropID,pValue)==S_OK;
}


bool CUxTheme::GetPartSize(HDC hdc,int PartID,int StateID,SIZE *pSize)
{
	if (m_hTheme==NULL)
		return false;
	return ::GetThemePartSize(m_hTheme,hdc,PartID,StateID,NULL,TS_TRUE,pSize)==S_OK;
}
