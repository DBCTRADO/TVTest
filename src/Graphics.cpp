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


#include "stdafx.h"
#include "TVTest.h"
#include "Graphics.h"
#include "DrawUtil.h"
#include "Util.h"

#pragma comment(lib, "gdiplus.lib")


namespace TVTest
{

namespace Graphics
{


static Gdiplus::Color GdiplusColor(COLORREF Color)
{
	return Gdiplus::Color(GetRValue(Color), GetGValue(Color), GetBValue(Color));
}

static Gdiplus::Color GdiplusColor(const CColor &Color)
{
	return Gdiplus::Color(Color.Alpha, Color.Red, Color.Green, Color.Blue);
}

static Gdiplus::Rect GdiplusRect(const RECT &Rect)
{
	return Gdiplus::Rect(Rect.left, Rect.top, Rect.right - Rect.left, Rect.bottom - Rect.top);
}

static Gdiplus::RectF GdiplusRectF(const RECT &Rect)
{
	return Gdiplus::RectF(
		static_cast<Gdiplus::REAL>(Rect.left),
		static_cast<Gdiplus::REAL>(Rect.top),
		static_cast<Gdiplus::REAL>(Rect.right - Rect.left),
		static_cast<Gdiplus::REAL>(Rect.bottom - Rect.top));
}




CGraphicsCore::~CGraphicsCore()
{
	Finalize();
}


bool CGraphicsCore::Initialize()
{
	if (!m_fInitialized) {
		Gdiplus::GdiplusStartupInput si;
		si.GdiplusVersion = 1;
		si.DebugEventCallback = nullptr;
		si.SuppressBackgroundThread = FALSE;
		si.SuppressExternalCodecs = FALSE;
		if (Gdiplus::GdiplusStartup(&m_Token, &si, nullptr) != Gdiplus::Ok)
			return false;
		m_fInitialized = true;
	}
	return true;
}


void CGraphicsCore::Finalize()
{
	if (m_fInitialized) {
		Gdiplus::GdiplusShutdown(m_Token);
		m_fInitialized = false;
	}
}




CImage::CImage(const CImage &Src)
{
	*this = Src;
}


CImage &CImage::operator=(const CImage &Src)
{
	if (&Src != this) {
		Free();
		if (Src.m_Bitmap) {
			m_Bitmap.reset(
				Src.m_Bitmap->Clone(
					0, 0,
					Src.m_Bitmap->GetWidth(),
					Src.m_Bitmap->GetHeight(),
					Src.m_Bitmap->GetPixelFormat()));
			VerifyConstruct();
		}
	}
	return *this;
}


void CImage::Free()
{
	m_Bitmap.reset();
}


bool CImage::LoadFromFile(LPCWSTR pszFileName)
{
	m_Bitmap.reset(Gdiplus::Bitmap::FromFile(pszFileName));
	return VerifyConstruct();
}


bool CImage::LoadFromResource(HINSTANCE hinst, LPCWSTR pszName)
{
	m_Bitmap.reset(Gdiplus::Bitmap::FromResource(hinst, pszName));
	return VerifyConstruct();
}


bool CImage::LoadFromResource(HINSTANCE hinst, LPCTSTR pszName, LPCTSTR pszType)
{
	Free();

	const HRSRC hRes = ::FindResource(hinst, pszName, pszType);
	if (hRes == nullptr)
		return false;
	const DWORD Size = ::SizeofResource(hinst, hRes);
	if (Size == 0)
		return false;
	const HGLOBAL hData = ::LoadResource(hinst, hRes);
	const void *pData = ::LockResource(hData);
	if (pData == nullptr)
		return false;
	const HGLOBAL hBuffer = ::GlobalAlloc(GMEM_MOVEABLE, Size);
	if (hBuffer == nullptr)
		return false;
	void *pBuffer = ::GlobalLock(hBuffer);
	if (pBuffer == nullptr) {
		::GlobalFree(hBuffer);
		return false;
	}
	std::memcpy(pBuffer, pData, Size);
	::GlobalUnlock(hBuffer);
	IStream *pStream;
	if (::CreateStreamOnHGlobal(hBuffer, TRUE, &pStream) != S_OK) {
		::GlobalFree(hBuffer);
		return false;
	}
	m_Bitmap.reset(Gdiplus::Bitmap::FromStream(pStream));
	pStream->Release();
	return VerifyConstruct();
}


bool CImage::Create(int Width, int Height, int BitsPerPixel)
{
	Free();
	if (Width <= 0 || Height <= 0)
		return false;
	Gdiplus::PixelFormat Format;
	switch (BitsPerPixel) {
	case 1:  Format = PixelFormat1bppIndexed; break;
	case 4:  Format = PixelFormat4bppIndexed; break;
	case 8:  Format = PixelFormat8bppIndexed; break;
	case 24: Format = PixelFormat24bppRGB;    break;
	case 32: Format = PixelFormat32bppARGB;   break;
	default: return false;
	}
	m_Bitmap.reset(new Gdiplus::Bitmap(Width, Height, Format));
	if (!VerifyConstruct())
		return false;
	Clear();
	return true;
}


bool CImage::CreateFromBitmap(HBITMAP hbm, HPALETTE hpal)
{
	Free();

	BITMAP bm;
	if (::GetObject(hbm, sizeof(bm), &bm) != sizeof(bm))
		return false;

	// Bitmap::FromHBITMAP() はアルファチャンネルが無視される
	if (bm.bmBitsPixel == 32) {
#if 0
		if (!Create(bm.bmWidth, bm.bmHeight, 32))
			return false;

		const Gdiplus::Rect rc(0, 0, bm.bmWidth, bm.bmHeight);
		Gdiplus::BitmapData Data;

		if (m_Bitmap->LockBits(
					&rc, Gdiplus::ImageLockModeWrite,
					m_Bitmap->GetPixelFormat(), &Data) != Gdiplus::Ok) {
			Free();
			return false;
		}
		const BYTE *p = static_cast<const BYTE*>(bm.bmBits) + (bm.bmHeight - 1) * bm.bmWidthBytes;
		BYTE *q = static_cast<BYTE*>(Data.Scan0);
		for (UINT y = 0; y < Data.Height; y++) {
			std::memcpy(q, p, bm.bmWidth * 4);
			p -= bm.bmWidthBytes;
			q += Data.Stride;
		}
		m_Bitmap->UnlockBits(&Data);
#else
		BITMAPINFO bmi = {};
		bmi.bmiHeader.biSize = sizeof(BITMAPINFO);
		bmi.bmiHeader.biWidth = bm.bmWidth;
		bmi.bmiHeader.biHeight = bm.bmHeight;
		bmi.bmiHeader.biPlanes = 1;
		bmi.bmiHeader.biBitCount = 32;
		if (!CreateFromDIB(&bmi, bm.bmBits))
			return false;
#endif
	} else {
		m_Bitmap.reset(Gdiplus::Bitmap::FromHBITMAP(hbm, hpal));
		if (!VerifyConstruct())
			return false;
	}

	return true;
}


bool CImage::CreateFromDIB(const BITMAPINFO *pbmi, void *pBits)
{
	Free();

	if (pbmi == nullptr || pBits == nullptr)
		return false;

	// Bitmap::FromBITMAPINFO() はアルファチャンネルが無視される
	if (pbmi->bmiHeader.biBitCount == 32) {
		const int Width = pbmi->bmiHeader.biWidth;
		const int Height = std::abs(pbmi->bmiHeader.biHeight);

#if 0
		if (!Create(Width, Height, 32))
			return false;

		const Gdiplus::Rect rc(0, 0, Width, Height);
		Gdiplus::BitmapData Data;

		if (m_Bitmap->LockBits(
					&rc, Gdiplus::ImageLockModeWrite,
					m_Bitmap->GetPixelFormat(), &Data) != Gdiplus::Ok) {
			Free();
			return false;
		}

		const BYTE *p = static_cast<const BYTE*>(pBits);
		std::ptrdiff_t Stride = Width * 4;
		if (pbmi->bmiHeader.biHeight > 0) {
			p += (Height - 1) * Stride;
			Stride = -Stride;
		}

		BYTE *q = static_cast<BYTE*>(Data.Scan0);
		for (UINT y = 0; y < Data.Height; y++) {
			std::memcpy(q, p, Width * 4);
			p += Stride;
			q += Data.Stride;
		}

		m_Bitmap->UnlockBits(&Data);
#else
		BYTE *p = static_cast<BYTE*>(pBits);
		int Stride = Width * 4;
		if (pbmi->bmiHeader.biHeight > 0) {
			p += (Height - 1) * Stride;
			Stride = -Stride;
		}

		m_Bitmap.reset(new Gdiplus::Bitmap(Width, Height, Stride, PixelFormat32bppARGB, p));
		if (!VerifyConstruct())
			return false;
#endif
	} else {
		m_Bitmap.reset(Gdiplus::Bitmap::FromBITMAPINFO(pbmi, pBits));
		if (!VerifyConstruct())
			return false;
	}

	return true;
}


bool CImage::IsCreated() const
{
	return static_cast<bool>(m_Bitmap);
}


int CImage::GetWidth() const
{
	if (!m_Bitmap)
		return 0;
	return m_Bitmap->GetWidth();
}


int CImage::GetHeight() const
{
	if (!m_Bitmap)
		return 0;
	return m_Bitmap->GetHeight();
}


void CImage::Clear()
{
	if (m_Bitmap) {
		const Gdiplus::Rect rc(0, 0, m_Bitmap->GetWidth(), m_Bitmap->GetHeight());
		Gdiplus::BitmapData Data;

		if (m_Bitmap->LockBits(
					&rc, Gdiplus::ImageLockModeWrite,
					m_Bitmap->GetPixelFormat(), &Data) == Gdiplus::Ok) {
			BYTE *pBits = static_cast<BYTE*>(Data.Scan0);
			for (UINT y = 0; y < Data.Height; y++) {
				::ZeroMemory(pBits, std::abs(Data.Stride));
				pBits += Data.Stride;
			}
			m_Bitmap->UnlockBits(&Data);
		}
	}
}


HBITMAP CImage::CreateBitmap()
{
	if (!m_Bitmap)
		return nullptr;

	HBITMAP hbm = nullptr;
	if (m_Bitmap->GetHBITMAP(Gdiplus::Color(0, 0, 0, 0), &hbm) != Gdiplus::Ok)
		return nullptr;

	return hbm;
}


bool CImage::VerifyConstruct()
{
	if (!m_Bitmap)
		return false;
	if (m_Bitmap->GetLastStatus() != Gdiplus::Ok) {
		m_Bitmap.reset();
		return false;
	}
	return true;
}




CBrush::CBrush(BYTE r, BYTE g, BYTE b, BYTE a)
{
	CreateSolidBrush(r, g, b, a);
}


CBrush::CBrush(const CColor &Color)
{
	CreateSolidBrush(Color);
}


void CBrush::Free()
{
	m_Brush.reset();
}


bool CBrush::CreateSolidBrush(BYTE r, BYTE g, BYTE b, BYTE a)
{
	const Gdiplus::Color Color(a, r, g, b);

	if (m_Brush) {
		if (m_Brush->SetColor(Color) != Gdiplus::Ok)
			return false;
	} else {
		m_Brush.reset(new Gdiplus::SolidBrush(Color));
		if (!VerifyConstruct())
			return false;
	}
	return true;
}


bool CBrush::CreateSolidBrush(const CColor &Color)
{
	return CreateSolidBrush(Color.Red, Color.Green, Color.Blue, Color.Alpha);
}


bool CBrush::IsCreated() const
{
	return static_cast<bool>(m_Brush);
}


bool CBrush::VerifyConstruct()
{
	if (!m_Brush)
		return false;
	if (m_Brush->GetLastStatus() != Gdiplus::Ok) {
		m_Brush.reset();
		return false;
	}
	return true;
}




CFont::CFont(const LOGFONT &lf)
{
	Create(lf);
}


void CFont::Free()
{
	m_Font.reset();
}


bool CFont::Create(const LOGFONT &lf)
{
	int FontStyle = 0;
	if (lf.lfWeight >= FW_BOLD)
		FontStyle |= Gdiplus::FontStyleBold;
	if (lf.lfItalic)
		FontStyle |= Gdiplus::FontStyleItalic;
	if (lf.lfUnderline)
		FontStyle |= Gdiplus::FontStyleUnderline;
	if (lf.lfStrikeOut)
		FontStyle |= Gdiplus::FontStyleStrikeout;

	m_Font.reset(
		new Gdiplus::Font(
			lf.lfFaceName,
			static_cast<Gdiplus::REAL>(std::abs(lf.lfHeight)),
			FontStyle,
			Gdiplus::UnitPixel));

	return VerifyConstruct();
}


bool CFont::IsCreated() const
{
	return static_cast<bool>(m_Font);
}


bool CFont::VerifyConstruct()
{
	if (!m_Font)
		return false;
	if (m_Font->GetLastStatus() != Gdiplus::Ok) {
		m_Font.reset();
		return false;
	}
	return true;
}




CCanvas::CCanvas(HDC hdc)
{
	if (hdc != nullptr) {
		m_Graphics.reset(new Gdiplus::Graphics(hdc));
		VerifyConstruct();
	}
}


CCanvas::CCanvas(CImage *pImage)
{
	if (pImage != nullptr && pImage->m_Bitmap) {
		m_Graphics.reset(new Gdiplus::Graphics(pImage->m_Bitmap.get()));
		VerifyConstruct();
	}
}


bool CCanvas::Clear(BYTE r, BYTE g, BYTE b, BYTE a)
{
	if (!m_Graphics)
		return false;
	return m_Graphics->Clear(Gdiplus::Color(a, r, g, b)) == Gdiplus::Ok;
}


bool CCanvas::SetComposition(bool fComposite)
{
	if (!m_Graphics)
		return false;
	return m_Graphics->SetCompositingMode(
		fComposite ?
			Gdiplus::CompositingModeSourceOver :
			Gdiplus::CompositingModeSourceCopy) == Gdiplus::Ok;
}


bool CCanvas::DrawImage(const CImage *pImage, int x, int y)
{
	if (!m_Graphics
			|| pImage == nullptr || !pImage->m_Bitmap)
		return false;
	return m_Graphics->DrawImage(
		pImage->m_Bitmap.get(), x, y,
		pImage->GetWidth(),
		pImage->GetHeight()) == Gdiplus::Ok;
}


bool CCanvas::DrawImage(
	int DstX, int DstY, int DstWidth, int DstHeight,
	const CImage *pImage, int SrcX, int SrcY, int SrcWidth, int SrcHeight, float Opacity)
{
	if (m_Graphics
			&& pImage != nullptr && pImage->m_Bitmap) {
		Gdiplus::ImageAttributes Attributes;
		Gdiplus::ColorMatrix Matrix = {
			1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
		};
		Matrix.m[3][3] = Opacity;
		Attributes.SetColorMatrix(&Matrix);
		return m_Graphics->DrawImage(
			pImage->m_Bitmap.get(),
			Gdiplus::Rect(DstX, DstY, DstWidth, DstHeight),
			SrcX, SrcY, SrcWidth, SrcHeight,
			Gdiplus::UnitPixel, &Attributes) == Gdiplus::Ok;
	}
	return false;
}


bool CCanvas::FillRect(const CBrush *pBrush, const RECT &Rect)
{
	if (m_Graphics
			&& pBrush != nullptr && pBrush->m_Brush) {
		return m_Graphics->FillRectangle(
			pBrush->m_Brush.get(),
			GdiplusRect(Rect)) == Gdiplus::Ok;
	}
	return false;
}


bool CCanvas::FillGradient(
	const CColor &Color1, const CColor &Color2,
	const RECT &Rect, GradientDirection Direction)
{
	if (m_Graphics) {
		const Gdiplus::RectF rect(
			static_cast<Gdiplus::REAL>(Rect.left) - 0.1f,
			static_cast<Gdiplus::REAL>(Rect.top) - 0.1f,
			static_cast<Gdiplus::REAL>(Rect.right - Rect.left) + 0.2f,
			static_cast<Gdiplus::REAL>(Rect.bottom - Rect.top) + 0.2f);
		const Gdiplus::LinearGradientBrush Brush(
			rect,
			GdiplusColor(Color1), GdiplusColor(Color2),
			Direction == GradientDirection::Horz ?
				Gdiplus::LinearGradientModeHorizontal :
				Gdiplus::LinearGradientModeVertical);
		return m_Graphics->FillRectangle(&Brush, rect) == Gdiplus::Ok;
	}
	return false;
}


bool CCanvas::DrawText(
	LPCTSTR pszText, const CFont &Font,
	const RECT &Rect, const CBrush *pBrush, TextFlag Flags)
{
	if (!m_Graphics
			|| IsStringEmpty(pszText)
			|| !Font.m_Font
			|| pBrush == nullptr || !pBrush->m_Brush)
		return false;

	Gdiplus::StringFormat Format;
	SetStringFormat(&Format, Flags);

	SetTextRenderingHint(Flags);

	if (!!(Flags & TextFlag::Draw_Path)) {
		Gdiplus::GraphicsPath Path;
		Gdiplus::FontFamily FontFamily;

		Font.m_Font->GetFamily(&FontFamily);
		if (Path.AddString(
					pszText, -1,
					&FontFamily, Font.m_Font->GetStyle(), Font.m_Font->GetSize(),
					GdiplusRect(Rect), &Format) != Gdiplus::Ok)
			return false;

		const Gdiplus::SmoothingMode OldSmoothingMode = m_Graphics->GetSmoothingMode();
		m_Graphics->SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);

		m_Graphics->FillPath(pBrush->m_Brush.get(), &Path);

		m_Graphics->SetSmoothingMode(OldSmoothingMode);

		return true;
	}

	return m_Graphics->DrawString(
		pszText, -1,
		Font.m_Font.get(),
		Gdiplus::RectF(
			static_cast<Gdiplus::REAL>(Rect.left),
			static_cast<Gdiplus::REAL>(Rect.top),
			static_cast<Gdiplus::REAL>(Rect.right - Rect.left),
			static_cast<Gdiplus::REAL>(Rect.bottom - Rect.top)),
		&Format,
		pBrush->m_Brush.get()) == Gdiplus::Ok;
}


bool CCanvas::GetTextSize(
	LPCTSTR pszText, const CFont &Font, TextFlag Flags, SIZE *pSize)
{
	if (pSize == nullptr)
		return false;

	const SIZE Size = *pSize;

	pSize->cx = 0;
	pSize->cy = 0;

	if (!m_Graphics || !Font.m_Font)
		return false;

	if (IsStringEmpty(pszText))
		return true;

	Gdiplus::StringFormat Format;
	SetStringFormat(&Format, Flags);

	SetTextRenderingHint(Flags);

#if 0
	Gdiplus::CharacterRange Range(0, ::lstrlen(pszText));
	Format.SetMeasurableCharacterRanges(1, &Range);
	Gdiplus::Region Region;
	Gdiplus::RectF Bounds;

	if (m_Graphics->MeasureCharacterRanges(
				pszText, -1, Font.m_Font.get(),
				Gdiplus::RectF(0.0f, 0.0f, 10000.0f, 10000.0f),
				&Format, 1, &Region) != Gdiplus::Ok
			|| Region.GetBounds(&Bounds, m_Graphics.get()))
		return false;

	pSize->cx = static_cast<int>(Bounds.GetRight() + 1.0f);
	pSize->cy = static_cast<int>(Bounds.GetBottom() + 1.0f);
#else
	Gdiplus::RectF Bounds;

	if (!!(Flags & TextFlag::Format_NoWrap)) {
		if (m_Graphics->MeasureString(
					pszText, -1, Font.m_Font.get(),
					Gdiplus::PointF(0.0f, 0.0f),
					&Format, &Bounds) != Gdiplus::Ok)
			return false;
	} else {
		if (m_Graphics->MeasureString(
					pszText, -1, Font.m_Font.get(),
					Gdiplus::RectF(0.0f, 0.0f, static_cast<float>(Size.cx), static_cast<float>(Size.cy)),
					&Format, &Bounds) != Gdiplus::Ok)
			return false;
	}

	pSize->cx = static_cast<int>(Bounds.GetRight() + 1.0f);
	pSize->cy = static_cast<int>(Bounds.GetBottom() + 1.0f);
#endif

	return true;
}


bool CCanvas::DrawOutlineText(
	LPCTSTR pszText, const CFont &Font,
	const RECT &Rect, const CBrush *pBrush,
	const CColor &OutlineColor, float OutlineWidth,
	TextFlag Flags)
{
	if (!m_Graphics
			|| IsStringEmpty(pszText)
			|| !Font.m_Font
			|| pBrush == nullptr || !pBrush->m_Brush)
		return false;

	Gdiplus::StringFormat Format;
	SetStringFormat(&Format, Flags);

	SetTextRenderingHint(Flags);

	Gdiplus::GraphicsPath Path;
	Gdiplus::FontFamily FontFamily;

	Font.m_Font->GetFamily(&FontFamily);
	if (Path.AddString(
			pszText, -1,
			&FontFamily, Font.m_Font->GetStyle(), Font.m_Font->GetSize(),
			GdiplusRect(Rect), &Format) != Gdiplus::Ok)
		return false;

	const Gdiplus::SmoothingMode OldSmoothingMode = m_Graphics->GetSmoothingMode();
	m_Graphics->SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);

	Gdiplus::Pen Pen(GdiplusColor(OutlineColor), OutlineWidth);
	Pen.SetLineJoin(Gdiplus::LineJoinRound);
	m_Graphics->DrawPath(&Pen, &Path);

	m_Graphics->FillPath(pBrush->m_Brush.get(), &Path);

	m_Graphics->SetSmoothingMode(OldSmoothingMode);

	return true;
}


bool CCanvas::GetOutlineTextSize(
	LPCTSTR pszText, const CFont &Font, float OutlineWidth, TextFlag Flags, SIZE *pSize)
{
	if (pSize == nullptr)
		return false;

	const SIZE Size = *pSize;

	pSize->cx = 0;
	pSize->cy = 0;

	if (!m_Graphics || !Font.m_Font)
		return false;

	if (IsStringEmpty(pszText))
		return true;

	Gdiplus::StringFormat Format;
	SetStringFormat(&Format, Flags);

	SetTextRenderingHint(Flags);

	Gdiplus::GraphicsPath Path;
	Gdiplus::FontFamily FontFamily;

	Font.m_Font->GetFamily(&FontFamily);
	if (Path.AddString(
				pszText, -1,
				&FontFamily, Font.m_Font->GetStyle(), Font.m_Font->GetSize(),
				!!(Flags & TextFlag::Format_NoWrap) ?
					Gdiplus::Rect(0, 0, 10000, 10000) :
					Gdiplus::Rect(0, 0, Size.cx, Size.cy),
				&Format) != Gdiplus::Ok)
		return false;

	Gdiplus::Pen Pen(Gdiplus::Color(), OutlineWidth);
	Pen.SetLineJoin(Gdiplus::LineJoinRound);

	Gdiplus::RectF Bounds;
	if (Path.GetBounds(&Bounds, nullptr, &Pen) != Gdiplus::Ok)
		return false;

#if 0
	TRACE(
		TEXT("Outline text bounds : [{} x {}] [{} {} {} {}] \"{}\"\n"),
		Size.cx, Size.cy, Bounds.X, Bounds.Y, Bounds.Width, Bounds.Height, pszText);
#endif

	pSize->cx = static_cast<long>(Bounds.GetRight() + 1.0f);
	pSize->cy = static_cast<long>(Bounds.GetBottom() + 1.0f);

	return true;
}


float CCanvas::GetLineSpacing(const CFont &Font) const
{
	if (!m_Graphics || !Font.m_Font)
		return 0.0f;
	return Font.m_Font->GetHeight(m_Graphics.get());
}


float CCanvas::GetFontAscent(const CFont &Font) const
{
	if (!m_Graphics || !Font.m_Font)
		return 0.0f;

	Gdiplus::FontFamily Family;
	if (Font.m_Font->GetFamily(&Family) != Gdiplus::Ok)
		return 0.0f;

	const INT Style = Font.m_Font->GetStyle();
	const UINT16 EmHeight = Family.GetEmHeight(Style);
	if (EmHeight == 0)
		return 0.0f;

	return Font.m_Font->GetSize() * static_cast<float>(Family.GetCellAscent(Style)) / static_cast<float>(EmHeight);
}


float CCanvas::GetFontDescent(const CFont &Font) const
{
	if (!m_Graphics || !Font.m_Font)
		return 0.0f;

	Gdiplus::FontFamily Family;
	if (Font.m_Font->GetFamily(&Family) != Gdiplus::Ok)
		return 0.0f;

	const INT Style = Font.m_Font->GetStyle();
	const UINT16 EmHeight = Family.GetEmHeight(Style);
	if (EmHeight == 0)
		return 0.0f;

	return Font.m_Font->GetSize() * static_cast<float>(Family.GetCellDescent(Style)) / static_cast<float>(EmHeight);
}


bool CCanvas::VerifyConstruct()
{
	if (!m_Graphics)
		return false;
	if (m_Graphics->GetLastStatus() != Gdiplus::Ok) {
		m_Graphics.reset();
		return false;
	}
	return true;
}


void CCanvas::SetStringFormat(Gdiplus::StringFormat *pFormat, TextFlag Flags)
{
	INT FormatFlags = pFormat->GetFormatFlags();
	if (!!(Flags & TextFlag::Format_NoWrap))
		FormatFlags |= Gdiplus::StringFormatFlagsNoWrap;
	else
		FormatFlags &= ~Gdiplus::StringFormatFlagsNoWrap;
	if (!!(Flags & TextFlag::Format_NoClip))
		FormatFlags |= Gdiplus::StringFormatFlagsNoClip;
	else
		FormatFlags &= ~Gdiplus::StringFormatFlagsNoClip;
	if (!!(Flags & TextFlag::Format_ClipLastLine))
		FormatFlags |= Gdiplus::StringFormatFlagsLineLimit;
	else
		FormatFlags &= ~Gdiplus::StringFormatFlagsLineLimit;
	pFormat->SetFormatFlags(FormatFlags);

	switch (Flags & TextFlag::Format_HorzAlignMask) {
	case TextFlag::Format_Left:       pFormat->SetAlignment(Gdiplus::StringAlignmentNear);   break;
	case TextFlag::Format_Right:      pFormat->SetAlignment(Gdiplus::StringAlignmentFar);    break;
	case TextFlag::Format_HorzCenter: pFormat->SetAlignment(Gdiplus::StringAlignmentCenter); break;
	}

	switch (Flags & TextFlag::Format_VertAlignMask) {
	case TextFlag::Format_Top:        pFormat->SetLineAlignment(Gdiplus::StringAlignmentNear);   break;
	case TextFlag::Format_Bottom:     pFormat->SetLineAlignment(Gdiplus::StringAlignmentFar);    break;
	case TextFlag::Format_VertCenter: pFormat->SetLineAlignment(Gdiplus::StringAlignmentCenter); break;
	}

	pFormat->SetTrimming(
		!!(Flags & TextFlag::Format_EndEllipsis ) ? Gdiplus::StringTrimmingEllipsisCharacter :
		!!(Flags & TextFlag::Format_WordEllipsis) ? Gdiplus::StringTrimmingEllipsisWord :
		!!(Flags & TextFlag::Format_TrimChar    ) ? Gdiplus::StringTrimmingCharacter :
		Gdiplus::StringTrimmingNone);
}


void CCanvas::SetTextRenderingHint(TextFlag Flags)
{
	m_Graphics->SetTextRenderingHint(
		!!(Flags & TextFlag::Draw_Antialias) ?
			(!!(Flags & TextFlag::Draw_Hinting) ?
				Gdiplus::TextRenderingHintAntiAliasGridFit :
				Gdiplus::TextRenderingHintAntiAlias) :
		!!(Flags & TextFlag::Draw_NoAntialias) ?
			(!!(Flags & TextFlag::Draw_Hinting) ?
				Gdiplus::TextRenderingHintSingleBitPerPixelGridFit :
				Gdiplus::TextRenderingHintSingleBitPerPixel) :
		!!(Flags & TextFlag::Draw_ClearType) ?
			Gdiplus::TextRenderingHintClearTypeGridFit :
			Gdiplus::TextRenderingHintSystemDefault);
}


}	// namespace TVTest

}	// namespace Graphics
