#include "stdafx.h"
#include "Graphics.h"
#include "DrawUtil.h"
#include "Util.h"
#include <gdiplus.h>

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
		(Gdiplus::REAL)Rect.left,
		(Gdiplus::REAL)Rect.top,
		(Gdiplus::REAL)(Rect.right - Rect.left),
		(Gdiplus::REAL)(Rect.bottom - Rect.top));
}




CGraphicsCore::CGraphicsCore()
	: m_fInitialized(false)
{
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




CImage::CImage()
	: m_pBitmap(nullptr)
{
}


CImage::CImage(const CImage &Src)
	: m_pBitmap(nullptr)
{
	*this = Src;
}


CImage::~CImage()
{
	Free();
}


CImage &CImage::operator=(const CImage &Src)
{
	if (&Src != this) {
		Free();
		if (Src.m_pBitmap != nullptr)
			m_pBitmap = Src.m_pBitmap->Clone(0, 0, Src.m_pBitmap->GetWidth(), Src.m_pBitmap->GetHeight(), Src.m_pBitmap->GetPixelFormat());
	}
	return *this;
}


void CImage::Free()
{
	if (m_pBitmap != nullptr) {
		delete m_pBitmap;
		m_pBitmap = nullptr;
	}
}


bool CImage::LoadFromFile(LPCWSTR pszFileName)
{
	Free();
	m_pBitmap = Gdiplus::Bitmap::FromFile(pszFileName);
	return m_pBitmap != nullptr;
}


bool CImage::LoadFromResource(HINSTANCE hinst, LPCWSTR pszName)
{
	Free();
	m_pBitmap = Gdiplus::Bitmap::FromResource(hinst, pszName);
	return m_pBitmap != nullptr;
}


bool CImage::LoadFromResource(HINSTANCE hinst, LPCTSTR pszName, LPCTSTR pszType)
{
	Free();

	HRSRC hRes = ::FindResource(hinst, pszName, pszType);
	if (hRes == nullptr)
		return false;
	DWORD Size = ::SizeofResource(hinst, hRes);
	if (Size == 0)
		return false;
	HGLOBAL hData = ::LoadResource(hinst, hRes);
	const void *pData = ::LockResource(hData);
	if (pData == nullptr)
		return false;
	HGLOBAL hBuffer = ::GlobalAlloc(GMEM_MOVEABLE, Size);
	if (hBuffer == nullptr)
		return false;
	void *pBuffer = ::GlobalLock(hBuffer);
	if (pBuffer == nullptr) {
		::GlobalFree(hBuffer);
		return false;
	}
	::CopyMemory(pBuffer, pData, Size);
	::GlobalUnlock(hBuffer);
	IStream *pStream;
	if (::CreateStreamOnHGlobal(hBuffer, TRUE, &pStream) != S_OK) {
		::GlobalFree(hBuffer);
		return false;
	}
	m_pBitmap = Gdiplus::Bitmap::FromStream(pStream);
	pStream->Release();
	return m_pBitmap != nullptr;
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
	m_pBitmap = new Gdiplus::Bitmap(Width, Height, Format);
	if (m_pBitmap == nullptr)
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
		if (!Create(bm.bmWidth, bm.bmHeight, 32))
			return false;

		Gdiplus::Rect rc(0, 0, bm.bmWidth, bm.bmHeight);
		Gdiplus::BitmapData Data;

		if (m_pBitmap->LockBits(
					&rc, Gdiplus::ImageLockModeWrite,
					m_pBitmap->GetPixelFormat(), &Data) != Gdiplus::Ok) {
			Free();
			return false;
		}
		const BYTE *p = static_cast<const BYTE*>(bm.bmBits) + (bm.bmHeight - 1) * bm.bmWidthBytes;
		BYTE *q = static_cast<BYTE*>(Data.Scan0);
		for (UINT y = 0; y < Data.Height; y++) {
			::CopyMemory(q, p, bm.bmWidth * 4);
			p -= bm.bmWidthBytes;
			q += Data.Stride;
		}
		m_pBitmap->UnlockBits(&Data);
	} else {
		m_pBitmap = Gdiplus::Bitmap::FromHBITMAP(hbm, hpal);
	}

	return m_pBitmap != nullptr;
}


bool CImage::CreateFromDIB(const BITMAPINFO *pbmi, const void *pBits)
{
	Free();
	m_pBitmap = new Gdiplus::Bitmap(pbmi, const_cast<void*>(pBits));
	return m_pBitmap != nullptr;
}


bool CImage::IsCreated() const
{
	return m_pBitmap != nullptr;
}


int CImage::GetWidth() const
{
	if (m_pBitmap == nullptr)
		return 0;
	return m_pBitmap->GetWidth();
}


int CImage::GetHeight() const
{
	if (m_pBitmap == nullptr)
		return 0;
	return m_pBitmap->GetHeight();
}


void CImage::Clear()
{
	if (m_pBitmap != nullptr) {
		Gdiplus::Rect rc(0, 0, m_pBitmap->GetWidth(), m_pBitmap->GetHeight());
		Gdiplus::BitmapData Data;

		if (m_pBitmap->LockBits(
					&rc, Gdiplus::ImageLockModeWrite,
					m_pBitmap->GetPixelFormat(), &Data) == Gdiplus::Ok) {
			BYTE *pBits = static_cast<BYTE*>(Data.Scan0);
			for (UINT y = 0; y < Data.Height; y++) {
				::ZeroMemory(pBits, abs(Data.Stride));
				pBits += Data.Stride;
			}
			m_pBitmap->UnlockBits(&Data);
		}
	}
}




CBrush::CBrush()
	: m_pBrush(nullptr)
{
}


CBrush::CBrush(BYTE r, BYTE g, BYTE b, BYTE a)
{
	m_pBrush = new Gdiplus::SolidBrush(Gdiplus::Color(a, r, g, b));
}


CBrush::CBrush(const CColor &Color)
{
	m_pBrush = new Gdiplus::SolidBrush(GdiplusColor(Color));
}


CBrush::~CBrush()
{
	Free();
}


void CBrush::Free()
{
	if (m_pBrush != nullptr) {
		delete m_pBrush;
		m_pBrush = nullptr;
	}
}


bool CBrush::CreateSolidBrush(BYTE r, BYTE g, BYTE b, BYTE a)
{
	Gdiplus::Color Color(a, r, g, b);

	if (m_pBrush != nullptr) {
		m_pBrush->SetColor(Color);
	} else {
		m_pBrush = new Gdiplus::SolidBrush(Color);
		if (m_pBrush == nullptr)
			return false;
	}
	return true;
}


bool CBrush::CreateSolidBrush(const CColor &Color)
{
	return CreateSolidBrush(Color.Red, Color.Green, Color.Blue, Color.Alpha);
}




CFont::CFont()
	: m_pFont(nullptr)
{
}


CFont::CFont(const LOGFONT &lf)
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
	m_pFont = new Gdiplus::Font(
		lf.lfFaceName,
		(Gdiplus::REAL)abs(lf.lfHeight),
		FontStyle,
		Gdiplus::UnitPixel);
}


CFont::~CFont()
{
	Free();
}


void CFont::Free()
{
	if (m_pFont != nullptr) {
		delete m_pFont;
		m_pFont = nullptr;
	}
}




CCanvas::CCanvas(HDC hdc)
	: m_pGraphics(nullptr)
{
	if (hdc != nullptr)
		m_pGraphics = new Gdiplus::Graphics(hdc);
}


CCanvas::CCanvas(CImage *pImage)
	: m_pGraphics(nullptr)
{
	if (pImage != nullptr)
		m_pGraphics = new Gdiplus::Graphics(pImage->m_pBitmap);
}


CCanvas::~CCanvas()
{
	if (m_pGraphics != nullptr)
		delete m_pGraphics;
}


bool CCanvas::Clear(BYTE r, BYTE g, BYTE b, BYTE a)
{
	if (m_pGraphics == nullptr)
		return false;
	return m_pGraphics->Clear(Gdiplus::Color(a, r, g, b)) == Gdiplus::Ok;
}


bool CCanvas::SetComposition(bool fComposite)
{
	if (m_pGraphics == nullptr)
		return false;
	return m_pGraphics->SetCompositingMode(
		fComposite ?
			Gdiplus::CompositingModeSourceOver :
			Gdiplus::CompositingModeSourceCopy) == Gdiplus::Ok;
}


bool CCanvas::DrawImage(CImage *pImage, int x, int y)
{
	if (m_pGraphics == nullptr
			|| pImage == nullptr || pImage->m_pBitmap == nullptr)
		return false;
	return m_pGraphics->DrawImage(
		pImage->m_pBitmap, x, y,
		pImage->GetWidth(),
		pImage->GetHeight()) == Gdiplus::Ok;
}


bool CCanvas::DrawImage(
	int DstX, int DstY, int DstWidth, int DstHeight,
	CImage *pImage, int SrcX, int SrcY, int SrcWidth, int SrcHeight, float Opacity)
{
	if (m_pGraphics != nullptr
			&& pImage != nullptr && pImage->m_pBitmap != nullptr) {
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
		return m_pGraphics->DrawImage(
			pImage->m_pBitmap,
			Gdiplus::Rect(DstX, DstY, DstWidth, DstHeight),
			SrcX, SrcY, SrcWidth, SrcHeight,
			Gdiplus::UnitPixel, &Attributes) == Gdiplus::Ok;
	}
	return false;
}


bool CCanvas::FillRect(CBrush *pBrush, const RECT &Rect)
{
	if (m_pGraphics != nullptr
			&& pBrush != nullptr && pBrush->m_pBrush != nullptr) {
		return m_pGraphics->FillRectangle(
			pBrush->m_pBrush,
			Rect.left, Rect.top,
			Rect.right - Rect.left,
			Rect.bottom - Rect.top) == Gdiplus::Ok;
	}
	return false;
}


bool CCanvas::FillGradient(
	const CColor &Color1, const CColor &Color2,
	const RECT &Rect, GradientDirection Direction)
{
	if (m_pGraphics != nullptr) {
		Gdiplus::RectF rect(
			Gdiplus::REAL(Rect.left) - 0.1f,
			Gdiplus::REAL(Rect.top) - 0.1f,
			Gdiplus::REAL(Rect.right - Rect.left) + 0.2f,
			Gdiplus::REAL(Rect.bottom - Rect.top) + 0.2f);
		Gdiplus::LinearGradientBrush Brush(
			rect,
			GdiplusColor(Color1), GdiplusColor(Color2),
			Direction == GRADIENT_DIRECTION_HORZ ?
			Gdiplus::LinearGradientModeHorizontal :
			Gdiplus::LinearGradientModeVertical);
		return m_pGraphics->FillRectangle(&Brush, rect) == Gdiplus::Ok;
	}
	return false;
}


bool CCanvas::DrawText(
	LPCTSTR pszText, const LOGFONT &lf,
	const RECT &Rect, CBrush *pBrush, UINT Flags)
{
	if (m_pGraphics == nullptr
			|| IsStringEmpty(pszText)
			|| pBrush == nullptr || pBrush->m_pBrush == nullptr)
		return false;

	CFont Font(lf);

	Gdiplus::StringFormat Format;
	SetStringFormat(&Format, Flags);

	SetTextRenderingHint(Flags);

	if ((Flags & TEXT_DRAW_PATH) != 0) {
		Gdiplus::GraphicsPath Path;
		Gdiplus::FontFamily FontFamily;

		Font.m_pFont->GetFamily(&FontFamily);
		if (Path.AddString(
					pszText, -1,
					&FontFamily, Font.m_pFont->GetStyle(), Font.m_pFont->GetSize(),
					GdiplusRect(Rect), &Format) != Gdiplus::Ok)
			return false;

		Gdiplus::SmoothingMode OldSmoothingMode = m_pGraphics->GetSmoothingMode();
		m_pGraphics->SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);

		m_pGraphics->FillPath(pBrush->m_pBrush, &Path);

		m_pGraphics->SetSmoothingMode(OldSmoothingMode);

		return true;
	}

	return m_pGraphics->DrawString(
		pszText, -1,
		Font.m_pFont,
		Gdiplus::RectF(
			(Gdiplus::REAL)Rect.left,
			(Gdiplus::REAL)Rect.top,
			(Gdiplus::REAL)(Rect.right - Rect.left),
			(Gdiplus::REAL)(Rect.bottom - Rect.top)),
		&Format,
		pBrush->m_pBrush) == Gdiplus::Ok;
}


bool CCanvas::GetTextSize(
	LPCTSTR pszText, const LOGFONT &lf, UINT Flags, SIZE *pSize)
{
	if (pSize == nullptr)
		return false;

	pSize->cx = 0;
	pSize->cy = 0;

	if (m_pGraphics == nullptr)
		return false;

	if (IsStringEmpty(pszText))
		return true;

	CFont Font(lf);

	Gdiplus::StringFormat Format(
		Gdiplus::StringFormatFlagsNoWrap |
		Gdiplus::StringFormatFlagsNoClip);
	Format.SetTrimming(Gdiplus::StringTrimmingNone);

	SetTextRenderingHint(Flags);

#if 0
	Gdiplus::CharacterRange Range(0, ::lstrlen(pszText));
	Format.SetMeasurableCharacterRanges(1, &Range);
	Gdiplus::Region Region;
	Gdiplus::RectF Bounds;

	if (m_pGraphics->MeasureCharacterRanges(
				pszText, -1, Font.m_pFont,
				Gdiplus::RectF(0.0f, 0.0f, 10000.0f, 10000.0f),
				&Format, 1, &Region) != Gdiplus::Ok
			|| Region.GetBounds(&Bounds, m_pGraphics))
		return false;

	pSize->cx = (int)(Bounds.GetRight() + 1.0f);
	pSize->cy = (int)(Bounds.GetBottom() + 1.0f);
#else
	Gdiplus::RectF Bounds;

	if (m_pGraphics->MeasureString(
				pszText, -1, Font.m_pFont,
				Gdiplus::PointF(0.0f, 0.0f),
				&Format, &Bounds) != Gdiplus::Ok)
		return false;

	pSize->cx = (int)(Bounds.GetRight() + 1.0f);
	pSize->cy = (int)(Bounds.GetBottom() + 1.0f);
#endif

	return true;
}


bool CCanvas::DrawOutlineText(
	LPCTSTR pszText, const LOGFONT &lf,
	const RECT &Rect, CBrush *pBrush,
	const CColor &OutlineColor, float OutlineWidth,
	UINT Flags)
{
	if (m_pGraphics == nullptr
			|| IsStringEmpty(pszText)
			|| pBrush == nullptr || pBrush->m_pBrush == nullptr)
		return false;

	CFont Font(lf);

	Gdiplus::StringFormat Format;
	SetStringFormat(&Format, Flags);

	SetTextRenderingHint(Flags);

	Gdiplus::GraphicsPath Path;
	Gdiplus::FontFamily FontFamily;

	Font.m_pFont->GetFamily(&FontFamily);
	if (Path.AddString(
			pszText, -1,
			&FontFamily, Font.m_pFont->GetStyle(), Font.m_pFont->GetSize(),
			GdiplusRect(Rect), &Format) != Gdiplus::Ok)
		return false;

	Gdiplus::SmoothingMode OldSmoothingMode = m_pGraphics->GetSmoothingMode();
	m_pGraphics->SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);

	Gdiplus::Pen Pen(GdiplusColor(OutlineColor), OutlineWidth);
	Pen.SetLineJoin(Gdiplus::LineJoinRound);
	m_pGraphics->DrawPath(&Pen, &Path);

	m_pGraphics->FillPath(pBrush->m_pBrush, &Path);

	m_pGraphics->SetSmoothingMode(OldSmoothingMode);

	return true;
}


bool CCanvas::GetOutlineTextSize(
	LPCTSTR pszText, const LOGFONT &lf, float OutlineWidth, UINT Flags, SIZE *pSize)
{
	if (pSize == nullptr)
		return false;

	pSize->cx = 0;
	pSize->cy = 0;

	if (m_pGraphics == nullptr)
		return false;

	if (IsStringEmpty(pszText))
		return true;

	CFont Font(lf);

	Gdiplus::StringFormat Format(
		Gdiplus::StringFormatFlagsNoWrap |
		Gdiplus::StringFormatFlagsNoClip);
	Format.SetTrimming(Gdiplus::StringTrimmingNone);

	SetTextRenderingHint(Flags);

	Gdiplus::GraphicsPath Path;
	Gdiplus::FontFamily FontFamily;

	Font.m_pFont->GetFamily(&FontFamily);
	if (Path.AddString(
				pszText, -1,
				&FontFamily, Font.m_pFont->GetStyle(), Font.m_pFont->GetSize(),
				Gdiplus::Rect(0, 0, 10000, 10000), &Format) != Gdiplus::Ok)
		return false;

	Gdiplus::Pen Pen(Gdiplus::Color(), OutlineWidth);
	Pen.SetLineJoin(Gdiplus::LineJoinRound);

	Gdiplus::RectF Bounds;
	if (Path.GetBounds(&Bounds, nullptr, &Pen) != Gdiplus::Ok)
		return false;

#if 0
	TRACE(
		TEXT("Outline text bounds : %f %f %f %f \"%s\"\n"),
		Bounds.X, Bounds.Y, Bounds.Width, Bounds.Height, pszText);
#endif

	pSize->cx = (long)(Bounds.GetRight() + 1.0f);
	pSize->cy = (long)(Bounds.GetBottom() + 1.0f);

	return true;
}


void CCanvas::SetStringFormat(Gdiplus::StringFormat *pFormat, UINT Flags)
{
	INT FormatFlags = pFormat->GetFormatFlags();
	if ((Flags & TEXT_FORMAT_NO_WRAP) != 0)
		FormatFlags |= Gdiplus::StringFormatFlagsNoWrap;
	if ((Flags & TEXT_FORMAT_NO_CLIP) != 0)
		FormatFlags |= Gdiplus::StringFormatFlagsNoClip;
	pFormat->SetFormatFlags(FormatFlags);

	switch (Flags & TEXT_FORMAT_HORZ_ALIGN_MASK) {
	case TEXT_FORMAT_LEFT:        pFormat->SetAlignment(Gdiplus::StringAlignmentNear);   break;
	case TEXT_FORMAT_RIGHT:       pFormat->SetAlignment(Gdiplus::StringAlignmentFar);    break;
	case TEXT_FORMAT_HORZ_CENTER: pFormat->SetAlignment(Gdiplus::StringAlignmentCenter); break;
	}

	switch (Flags & TEXT_FORMAT_VERT_ALIGN_MASK) {
	case TEXT_FORMAT_TOP:         pFormat->SetLineAlignment(Gdiplus::StringAlignmentNear);   break;
	case TEXT_FORMAT_BOTTOM:      pFormat->SetLineAlignment(Gdiplus::StringAlignmentFar);    break;
	case TEXT_FORMAT_VERT_CENTER: pFormat->SetLineAlignment(Gdiplus::StringAlignmentCenter); break;
	}

	pFormat->SetTrimming(
		(Flags & TEXT_FORMAT_END_ELLIPSIS ) != 0 ? Gdiplus::StringTrimmingEllipsisCharacter :
		(Flags & TEXT_FORMAT_WORD_ELLIPSIS) != 0 ? Gdiplus::StringTrimmingEllipsisWord :
		(Flags & TEXT_FORMAT_TRIM_CHAR    ) != 0 ? Gdiplus::StringTrimmingCharacter :
		Gdiplus::StringTrimmingNone);
}


void CCanvas::SetTextRenderingHint(UINT Flags)
{
	m_pGraphics->SetTextRenderingHint(
		(Flags & TEXT_DRAW_ANTIALIAS) != 0 ?
			((Flags & TEXT_DRAW_HINTING) != 0 ?
				Gdiplus::TextRenderingHintAntiAliasGridFit :
				Gdiplus::TextRenderingHintAntiAlias) :
		(Flags & TEXT_DRAW_NO_ANTIALIAS) != 0 ?
			((Flags & TEXT_DRAW_HINTING) != 0 ?
				Gdiplus::TextRenderingHintSingleBitPerPixelGridFit :
				Gdiplus::TextRenderingHintSingleBitPerPixel) :
		(Flags & TEXT_DRAW_CLEARTYPE) != 0 ?
			Gdiplus::TextRenderingHintClearTypeGridFit :
			Gdiplus::TextRenderingHintSystemDefault);
}


}	// namespace TVTest

}	// namespace Graphics
