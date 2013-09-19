#include "stdafx.h"
#include "DrawUtil.h"
#include "Util.h"

// このマクロを使うとGDI+のヘッダでエラーが出る
/*
#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif
*/


namespace DrawUtil {


// 単色で塗りつぶす
bool Fill(HDC hdc,const RECT *pRect,COLORREF Color)
{
	HBRUSH hbr=::CreateSolidBrush(Color);

	if (hbr==NULL)
		return false;
	::FillRect(hdc,pRect,hbr);
	::DeleteObject(hbr);
	return true;
}


// グラデーションで塗りつぶす
bool FillGradient(HDC hdc,const RECT *pRect,COLORREF Color1,COLORREF Color2,
				  FillDirection Direction)
{
	if (hdc==NULL || pRect==NULL
			|| pRect->left>pRect->right || pRect->top>pRect->bottom)
		return false;

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


bool FillGradient(HDC hdc,const RECT *pRect,const RGBA &Color1,const RGBA &Color2,
				  FillDirection Direction)
{
	if (hdc==NULL || pRect==NULL
			|| pRect->left>pRect->right || pRect->top>pRect->bottom)
		return false;

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
				(BYTE)(((Width-1-x)*Color1.Alpha+x*Color2.Alpha)/(Width-1));
			if (BlendFunc.SourceConstantAlpha!=0) {
				::GdiAlphaBlend(hdc,x+pRect->left,pRect->top,1,Height,
								hdcMem,x,0,1,Height,
								BlendFunc);
			}
		}
	} else {
		for (int y=0;y<Height;y++) {
			BlendFunc.SourceConstantAlpha=
				(BYTE)(((Height-1-y)*Color1.Alpha+y*Color2.Alpha)/(Height-1));
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
		::FillMemory(p,RowBytes,
					 (BYTE)(((y*Highlight2)+(Center-1-y)*Highlight1)/(Center-1)));
		p+=RowBytes;
	}
	for (;y<Height;y++) {
		BYTE Alpha=(BYTE)(((y-Center)*Shadow2+(Height-1-y)*Shadow1)/(Height-Center-1));
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


// テキストを指定幅で折り返して何行になるか計算する
int CalcWrapTextLines(HDC hdc,LPCTSTR pszText,int Width)
{
	if (hdc==NULL || pszText==NULL)
		return 0;

	TEXTMETRIC tm;
	::GetTextMetrics(hdc,&tm);
	int MaxLength=max((Width*3)/(tm.tmAveCharWidth*2),8);

	LPCTSTR p;
	int Lines=0;

	p=pszText;
	while (*p!=_T('\0')) {
		if (*p==_T('\r') || *p==_T('\n')) {
			p++;
			if (*p==_T('\n'))
				p++;
			if (*p==_T('\0'))
				break;
			Lines++;
			continue;
		}
		int Length;
		for (Length=0;p[Length]!=_T('\0') && p[Length]!=_T('\r') && p[Length]!=_T('\n');Length++);
#if 0
		int Fit=0;
		SIZE sz;
		::GetTextExtentExPoint(hdc,p,Length,Width,&Fit,NULL,&sz);
		if (Fit<1) {
			Fit=StringCharLength(p);
			if (Fit==0)
				Fit=1;
		}
		p+=Fit;
		Lines++;
#else
		/*
			GetTextExtentExPoint は常に文字列全体の幅を計算してしまうため、
			高速化のために渡す文字列長を制限する
		*/
		do {
			int CalcLen;
			if (Length<=MaxLength) {
				CalcLen=Length;
			} else {
				LPCTSTR pEnd=p;
				do {
					LPCTSTR pNext=StringNextChar(pEnd);
					if (pNext==pEnd)
						break;
					pEnd=pNext;
				} while ((int)(pEnd-p)<MaxLength);
				CalcLen=(int)(pEnd-p);
				if (CalcLen==0) {
					p+=Length;
					break;
				}
			}
			int Fit=0;
			SIZE sz;
			::GetTextExtentExPoint(hdc,p,CalcLen,Width,&Fit,NULL,&sz);
			if (Fit<CalcLen || Fit==Length) {
				if (Fit<1) {
					Fit=StringCharLength(p);
					if (Fit==0)
						Fit=1;
				}
				Length-=Fit;
				p+=Fit;
				Lines++;
			} else {
				if (Fit>=MaxLength)
					MaxLength*=2;
			}
		} while (Length>0);
#endif
		if (*p==_T('\r'))
			p++;
		if (*p==_T('\n'))
			p++;
	}

	return Lines;
}


// テキストを指定幅で折り返して描画する
bool DrawWrapText(HDC hdc,LPCTSTR pszText,const RECT *pRect,int LineHeight,unsigned int Flags)
{
	if (hdc==NULL || pszText==NULL || pRect==NULL)
		return false;

	const int Width=pRect->right-pRect->left;
	TEXTMETRIC tm;
	::GetTextMetrics(hdc,&tm);
	int MaxLength=max((Width*3)/(tm.tmAveCharWidth*2),8);

	LPCTSTR p;
	int y;

	p=pszText;
	y=pRect->top;
	while (*p!=_T('\0') && y<pRect->bottom) {
		if (*p==_T('\r') || *p==_T('\n')) {
			p++;
			if (*p==_T('\n'))
				p++;
			y+=LineHeight;
			continue;
		}
		int Length;
		for (Length=0;p[Length]!=_T('\0') && p[Length]!=_T('\r') && p[Length]!=_T('\n');Length++);
#if 0
		int Fit=0;
		SIZE sz;
		::GetTextExtentExPoint(hdc,p,Length,Width,&Fit,NULL,&sz);
		if (Fit<1) {
			Fit=StringCharLength(p);
			if (Fit==0)
				Fit=1;
		}

		if ((Flags&DRAW_TEXT_ELLIPSIS)!=0 && Fit<Length && y+LineHeight>=pRect->bottom) {
			LPTSTR pszBuffer=new TCHAR[Fit+4];
			::lstrcpyn(pszBuffer,p,Fit+1);
			LPTSTR pszCur=pszBuffer+Fit;
			while (true) {
				::lstrcpy(pszCur,TEXT("..."));
				Length=(int)((pszCur-pszBuffer)+3);
				::GetTextExtentExPoint(hdc,pszBuffer,Length,Width,&Fit,NULL,&sz);
				if (Fit>=Length || pszCur==pszBuffer)
					break;
				pszCur=StringPrevChar(pszBuffer,pszCur);
			}
			::TextOut(hdc,pRect->left,y,pszBuffer,Fit);
			delete [] pszBuffer;
			return true;
		}

		::TextOut(hdc,pRect->left,y,p,Fit);
		p+=Fit;
		y+=LineHeight;
#else
		do {
			int CalcLen;
			if (Length<=MaxLength) {
				CalcLen=Length;
			} else {
				LPCTSTR pEnd=p;
				do {
					LPCTSTR pNext=StringNextChar(pEnd);
					if (pNext==pEnd)
						break;
					pEnd=pNext;
				} while ((int)(pEnd-p)<MaxLength);
				CalcLen=(int)(pEnd-p);
				if (CalcLen==0) {
					p+=Length;
					break;
				}
			}
			int Fit=0;
			SIZE sz;
			::GetTextExtentExPoint(hdc,p,CalcLen,Width,&Fit,NULL,&sz);

			if ((Flags&DRAW_TEXT_ELLIPSIS)!=0 && Fit<Length && y+LineHeight>=pRect->bottom) {
				LPTSTR pszBuffer=new TCHAR[Fit+4];
				::lstrcpyn(pszBuffer,p,Fit+1);
				LPTSTR pszCur=pszBuffer+Fit;
				while (true) {
					::lstrcpy(pszCur,TEXT("..."));
					Length=(int)((pszCur-pszBuffer)+3);
					::GetTextExtentExPoint(hdc,pszBuffer,Length,Width,&Fit,NULL,&sz);
					if (Fit>=Length || pszCur==pszBuffer)
						break;
					pszCur=StringPrevChar(pszBuffer,pszCur);
				}
				::TextOut(hdc,pRect->left,y,pszBuffer,Fit);
				delete [] pszBuffer;
				return true;
			}

			if (Fit<CalcLen || Fit==Length) {
				if (Fit<1) {
					Fit=StringCharLength(p);
					if (Fit==0)
						Fit=1;
				}
				::TextOut(hdc,pRect->left,y,p,Fit);
				Length-=Fit;
				p+=Fit;
				y+=LineHeight;
			} else {
				if (Fit>=MaxLength)
					MaxLength*=2;
			}
		} while (Length>0 && y<pRect->bottom);
#endif
		if (*p==_T('\r'))
			p++;
		if (*p==_T('\n'))
			p++;
	}

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
			//pFont->lfHeight=MessageFont.lfHeight;
			pFont->lfHeight=-12;
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
		if (Src.m_hbm!=NULL)
			m_hbm=static_cast<HBITMAP>(::CopyImage(Src.m_hbm,IMAGE_BITMAP,0,0,
												   Src.IsDIB()?LR_CREATEDIBSECTION:0));
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
{
}

CMonoColorBitmap::~CMonoColorBitmap()
{
	Destroy();
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
		::CopyMemory(pBits,bm.bmBits,bm.bmWidth*4*bm.bmHeight);
		m_fColorImage=true;
	} else {
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
			p+=(bm.bmWidth*bm.bmBitsPixel+31)/32*4;
		}
		m_fColorImage=false;
	}

	m_Color=CLR_INVALID;

	return true;
}

void CMonoColorBitmap::Destroy()
{
	if (m_hbm!=NULL) {
		::DeleteObject(m_hbm);
		m_hbm=NULL;
	}
}

bool CMonoColorBitmap::Draw(HDC hdc,int DstX,int DstY,COLORREF Color,int SrcX,int SrcY,int Width,int Height)
{
	if (m_hbm==NULL)
		return false;

	BITMAP bm;
	if (::GetObject(m_hbm,sizeof(bm),&bm)!=sizeof(bm))
		return false;

	if (Width<=0)
		Width=bm.bmWidth;
	if (Height<=0)
		Height=bm.bmHeight;
	if (SrcX<0 || SrcY<0 || SrcX+Width>bm.bmWidth || SrcY+Height>bm.bmHeight)
		return false;

	if (!m_fColorImage)
		SetColor(Color);

	HDC hdcMemory=::CreateCompatibleDC(hdc);
	if (hdcMemory==NULL)
		return false;
	HBITMAP hbmOld=static_cast<HBITMAP>(::SelectObject(hdcMemory,m_hbm));
	BLENDFUNCTION bf={AC_SRC_OVER,0,255,AC_SRC_ALPHA};
	::GdiAlphaBlend(hdc,DstX,DstY,Width,Height,
					hdcMemory,SrcX,SrcY,Width,Height,bf);
	::SelectObject(hdcMemory,hbmOld);
	::DeleteDC(hdcMemory);

	return true;
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

	if (!m_fColorImage)
		SetColor(Color);

	::ImageList_Add(himl,m_hbm,NULL);

	return himl;
}

void CMonoColorBitmap::SetColor(COLORREF Color)
{
	if (m_Color!=Color) {
		BITMAP bm;
		if (::GetObject(m_hbm,sizeof(bm),&bm)!=sizeof(bm))
			return;

		const UINT Red=GetRValue(Color),Green=GetGValue(Color),Blue=GetBValue(Color);
		BYTE *p=static_cast<BYTE*>(bm.bmBits);
		for (int y=0;y<bm.bmHeight;y++) {
			for (int x=0;x<bm.bmWidth;x++) {
#define DIVIDE_BY_255(v) ((((v)+1)*257)>>16)
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




// GDI+のヘッダで整数型の引数にNULLを渡しているので
// #define NULL nullptr するとエラーが出る…
#if _MSC_VER >= 1600	// VC2010
#undef NULL
#define NULL 0
#endif
#include <gdiplus.h>

#pragma comment(lib, "gdiplus.lib")
#ifdef WINDOWS2000_SUPPORT
//#pragma comment(linker, "/DELAYLOAD:gdiplus.dll")
#endif


class CGdiPlusInitializer
{
	bool m_fInitialized;
	ULONG_PTR m_Token;

public:
	CGdiPlusInitializer()
		: m_fInitialized(false)
	{
	}

	~CGdiPlusInitializer()
	{
		Finalize();
	}

	bool Initialize()
	{
		if (!m_fInitialized) {
#ifdef WINDOWS2000_SUPPORT
			// GDI+ の DLL がロードできるか調べる
			// (gdiplus.dllが無くても起動するように遅延ロードの指定をしている)
			HMODULE hLib=::LoadLibrary(TEXT("gdiplus.dll"));
			if (hLib==NULL)
				return false;
			::FreeLibrary(hLib);
#endif

			Gdiplus::GdiplusStartupInput si;
			si.GdiplusVersion=1;
			si.DebugEventCallback=NULL;
			si.SuppressBackgroundThread=FALSE;
			si.SuppressExternalCodecs=FALSE;
			if (Gdiplus::GdiplusStartup(&m_Token,&si,NULL)!=Gdiplus::Ok)
				return false;
			m_fInitialized=true;
		}
		return true;
	}

	void Finalize()
	{
		if (m_fInitialized) {
			Gdiplus::GdiplusShutdown(m_Token);
			m_fInitialized=false;
		}
	}
};

static CGdiPlusInitializer GdiPlusInitializer;


CGdiPlus::CGdiPlus()
	: m_fInitialized(false)
{
}

CGdiPlus::~CGdiPlus()
{
	Finalize();
}

bool CGdiPlus::Initialize()
{
	if (!GdiPlusInitializer.Initialize())
		return false;
	m_fInitialized=true;
	return true;
}

void CGdiPlus::Finalize()
{
	m_fInitialized=false;
}

bool CGdiPlus::DrawImage(CCanvas *pCanvas,CImage *pImage,int x,int y)
{
	if (pCanvas!=NULL && pCanvas->m_pGraphics!=NULL
			 && pImage!=NULL && pImage->m_pBitmap!=NULL) {
		return pCanvas->m_pGraphics->DrawImage(pImage->m_pBitmap,x,y,
											   pImage->GetWidth(),
											   pImage->GetHeight())==Gdiplus::Ok;
	}
	return false;
}

bool CGdiPlus::DrawImage(CCanvas *pCanvas,int DstX,int DstY,int DstWidth,int DstHeight,
	CImage *pImage,int SrcX,int SrcY,int SrcWidth,int SrcHeight,float Opacity)
{
	if (pCanvas!=NULL && pCanvas->m_pGraphics!=NULL
			 && pImage!=NULL && pImage->m_pBitmap!=NULL) {
		Gdiplus::ImageAttributes Attributes;
		Gdiplus::ColorMatrix Matrix = {
			1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
 		};
		Matrix.m[3][3]=Opacity;
		Attributes.SetColorMatrix(&Matrix);
		return pCanvas->m_pGraphics->DrawImage(pImage->m_pBitmap,
			Gdiplus::Rect(DstX,DstY,DstWidth,DstHeight),
			SrcX,SrcY,SrcWidth,SrcHeight,
			Gdiplus::UnitPixel,&Attributes)==Gdiplus::Ok;
	}
	return false;
}

bool CGdiPlus::FillRect(CCanvas *pCanvas,CBrush *pBrush,const RECT *pRect)
{
	if (pCanvas!=NULL && pCanvas->m_pGraphics!=NULL
			&& pBrush!=NULL && pBrush->m_pBrush!=NULL && pRect!=NULL) {
		return pCanvas->m_pGraphics->FillRectangle(pBrush->m_pBrush,
												   pRect->left,pRect->top,
												   pRect->right-pRect->left,
												   pRect->bottom-pRect->top)==Gdiplus::Ok;
	}
	return false;
}

bool CGdiPlus::FillGradient(CCanvas *pCanvas,COLORREF Color1,COLORREF Color2,
							const RECT &Rect,GradientDirection Direction)
{
	if (pCanvas!=NULL && pCanvas->m_pGraphics!=NULL) {
		Gdiplus::RectF rect(
			Gdiplus::REAL(Rect.left)-0.1f,
			Gdiplus::REAL(Rect.top)-0.1f,
			Gdiplus::REAL(Rect.right-Rect.left)+0.2f,
			Gdiplus::REAL(Rect.bottom-Rect.top)+0.2f);
		Gdiplus::LinearGradientBrush Brush(
			rect,
			Gdiplus::Color(GetRValue(Color1),GetGValue(Color1),GetBValue(Color1)),
			Gdiplus::Color(GetRValue(Color2),GetGValue(Color2),GetBValue(Color2)),
			Direction==GRADIENT_DIRECTION_HORZ?
				Gdiplus::LinearGradientModeHorizontal:
				Gdiplus::LinearGradientModeVertical);
		return pCanvas->m_pGraphics->FillRectangle(&Brush,rect)==Gdiplus::Ok;
	}
	return false;
}


CGdiPlus::CImage::CImage()
	: m_pBitmap(NULL)
{
}

CGdiPlus::CImage::CImage(const CImage &Src)
	: m_pBitmap(NULL)
{
	*this=Src;
}

CGdiPlus::CImage::~CImage()
{
	Free();
}

CGdiPlus::CImage &CGdiPlus::CImage::operator=(const CImage &Src)
{
	if (&Src!=this) {
		Free();
		if (Src.m_pBitmap!=NULL)
			m_pBitmap=Src.m_pBitmap->Clone(0,0,Src.m_pBitmap->GetWidth(),Src.m_pBitmap->GetHeight(),Src.m_pBitmap->GetPixelFormat());
	}
	return *this;
}

void CGdiPlus::CImage::Free()
{
	if (m_pBitmap!=NULL) {
		delete m_pBitmap;
		m_pBitmap=NULL;
	}
}

bool CGdiPlus::CImage::LoadFromFile(LPCWSTR pszFileName)
{
	Free();
	m_pBitmap=Gdiplus::Bitmap::FromFile(pszFileName);
	return m_pBitmap!=NULL;
}

bool CGdiPlus::CImage::LoadFromResource(HINSTANCE hinst,LPCWSTR pszName)
{
	Free();
	m_pBitmap=Gdiplus::Bitmap::FromResource(hinst,pszName);
	return m_pBitmap!=NULL;
}

bool CGdiPlus::CImage::LoadFromResource(HINSTANCE hinst,LPCTSTR pszName,LPCTSTR pszType)
{
	Free();

	HRSRC hRes=::FindResource(hinst,pszName,pszType);
	if (hRes==NULL)
		return false;
	DWORD Size=::SizeofResource(hinst,hRes);
	if (Size==0)
		return false;
	HGLOBAL hData=::LoadResource(hinst,hRes);
	const void *pData=::LockResource(hData);
	if (pData==NULL)
		return false;
	HGLOBAL hBuffer=::GlobalAlloc(GMEM_MOVEABLE,Size);
	if (hBuffer==NULL)
		return false;
	void *pBuffer=::GlobalLock(hBuffer);
	if (pBuffer==NULL) {
		::GlobalFree(hBuffer);
		return false;
	}
	::CopyMemory(pBuffer,pData,Size);
	::GlobalUnlock(hBuffer);
	IStream *pStream;
	if (::CreateStreamOnHGlobal(hBuffer,TRUE,&pStream)!=S_OK) {
		::GlobalFree(hBuffer);
		return false;
	}
	m_pBitmap=Gdiplus::Bitmap::FromStream(pStream);
	pStream->Release();
	return m_pBitmap!=NULL;
}

bool CGdiPlus::CImage::Create(int Width,int Height,int BitsPerPixel)
{
	Free();
	if (Width<=0 || Height<=0)
		return false;
	Gdiplus::PixelFormat Format;
	switch (BitsPerPixel) {
	case 1:		Format=PixelFormat1bppIndexed;	break;
	case 4:		Format=PixelFormat4bppIndexed;	break;
	case 8:		Format=PixelFormat8bppIndexed;	break;
	case 24:	Format=PixelFormat24bppRGB;	break;
	case 32:	Format=PixelFormat32bppARGB;	break;
	default:	return false;
	}
	m_pBitmap=new Gdiplus::Bitmap(Width,Height,Format);
	if (m_pBitmap==NULL)
		return false;
	Clear();
	return true;
}

bool CGdiPlus::CImage::CreateFromBitmap(HBITMAP hbm,HPALETTE hpal)
{
	Free();
	m_pBitmap=Gdiplus::Bitmap::FromHBITMAP(hbm,hpal);
	return m_pBitmap!=NULL;
}

bool CGdiPlus::CImage::CreateFromDIB(const BITMAPINFO *pbmi,const void *pBits)
{
	Free();
	m_pBitmap=new Gdiplus::Bitmap(pbmi,const_cast<void*>(pBits));
	return m_pBitmap!=NULL;
}

bool CGdiPlus::CImage::IsCreated() const
{
	return m_pBitmap!=NULL;
}

int CGdiPlus::CImage::GetWidth() const
{
	if (m_pBitmap==NULL)
		return 0;
	return m_pBitmap->GetWidth();
}

int CGdiPlus::CImage::GetHeight() const
{
	if (m_pBitmap==NULL)
		return 0;
	return m_pBitmap->GetHeight();
}

void CGdiPlus::CImage::Clear()
{
	if (m_pBitmap!=NULL) {
		Gdiplus::Rect rc(0,0,m_pBitmap->GetWidth(),m_pBitmap->GetHeight());
		Gdiplus::BitmapData Data;

		if (m_pBitmap->LockBits(&rc,Gdiplus::ImageLockModeWrite,
								m_pBitmap->GetPixelFormat(),&Data)==Gdiplus::Ok) {
			BYTE *pBits=static_cast<BYTE*>(Data.Scan0);
			for (UINT y=0;y<Data.Height;y++) {
				::ZeroMemory(pBits,abs(Data.Stride));
				pBits+=Data.Stride;
			}
			m_pBitmap->UnlockBits(&Data);
		}
	}
}


CGdiPlus::CBrush::CBrush()
	: m_pBrush(NULL)
{
}

CGdiPlus::CBrush::CBrush(BYTE r,BYTE g,BYTE b,BYTE a)
{
	m_pBrush=new Gdiplus::SolidBrush(Gdiplus::Color(a,r,g,b));
}

CGdiPlus::CBrush::CBrush(COLORREF Color)
{
	m_pBrush=new Gdiplus::SolidBrush(Gdiplus::Color(255,GetRValue(Color),GetGValue(Color),GetBValue(Color)));
}

CGdiPlus::CBrush::~CBrush()
{
	Free();
}

void CGdiPlus::CBrush::Free()
{
	if (m_pBrush!=NULL) {
		delete m_pBrush;
		m_pBrush=NULL;
	}
}

bool CGdiPlus::CBrush::CreateSolidBrush(BYTE r,BYTE g,BYTE b,BYTE a)
{
	Gdiplus::Color Color(a,r,g,b);

	if (m_pBrush!=NULL) {
		m_pBrush->SetColor(Color);
	} else {
		m_pBrush=new Gdiplus::SolidBrush(Color);
		if (m_pBrush==NULL)
			return false;
	}
	return true;
}


CGdiPlus::CCanvas::CCanvas(HDC hdc)
{
	m_pGraphics=new Gdiplus::Graphics(hdc);
}

CGdiPlus::CCanvas::CCanvas(CImage *pImage)
	: m_pGraphics(NULL)
{
	if (pImage!=NULL)
		m_pGraphics=new Gdiplus::Graphics(pImage->m_pBitmap);
}

CGdiPlus::CCanvas::~CCanvas()
{
	if (m_pGraphics!=NULL)
		delete m_pGraphics;
}

bool CGdiPlus::CCanvas::Clear(BYTE r,BYTE g,BYTE b,BYTE a)
{
	if (m_pGraphics==NULL)
		return false;
	return m_pGraphics->Clear(Gdiplus::Color(a,r,g,b))==Gdiplus::Ok;
}




#pragma comment(lib, "uxtheme.lib")
#ifdef WINDOWS2000_SUPPORT
//#pragma comment(linker, "/DELAYLOAD:uxtheme.dll")
#endif


CUxTheme::CUxTheme()
	: m_hTheme(NULL)
#ifdef WINDOWS2000_SUPPORT
	, m_hLib(NULL)
#endif
{
}

CUxTheme::~CUxTheme()
{
	Close();
#ifdef WINDOWS2000_SUPPORT
	if (m_hLib!=NULL)
		::FreeLibrary(m_hLib);
#endif
}

bool CUxTheme::Initialize()
{
#ifdef WINDOWS2000_SUPPORT
	if (m_hLib==NULL) {
		// uxtheme.dll がロードできるか調べる
		// (uxtheme.dllが無くても起動するように遅延ロードの指定をしている)
		m_hLib=::LoadLibrary(TEXT("uxtheme.dll"));
		if (m_hLib==NULL)
			return false;
	}
#endif
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
#ifdef WINDOWS2000_SUPPORT
	if (m_hLib==NULL)
		return false;
#endif
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
