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
#include <cmath>
#include "TVTest.h"
#include "TextDraw.h"
#include "Util.h"
#include "Common/DebugDef.h"


namespace TVTest
{


// 行頭禁則文字
const LPCWSTR CTextDraw::m_pszStartProhibitChars =
	L")）]］｣」』】〉》”、。,，.．!！?？ー〜～…・ァィゥェォッャュョヮヵヶぁぃぅぇぉっゃゅょゎゝゞ々";
// 行末禁則文字
const LPCWSTR CTextDraw::m_pszEndProhibitChars =
	L"(（[［｢「『【〈《“#＃▽▼";


bool CTextDraw::SetEngine(CTextDrawEngine *pEngine)
{
	if (pEngine != nullptr) {
		m_pEngine = pEngine;
	} else {
		if (!m_DefaultEngine)
			m_DefaultEngine = std::make_unique<CTextDrawEngine_GDI>();
		m_pEngine = m_DefaultEngine.get();
	}
	return true;
}


bool CTextDraw::Begin(HDC hdc, const RECT &Rect, Flag Flags)
{
	if (m_pEngine == nullptr)
		SetEngine(nullptr);
	m_Flags = Flags;
	return m_pEngine->BeginDraw(hdc, Rect);
}


void CTextDraw::End()
{
	if (m_pEngine != nullptr)
		m_pEngine->EndDraw();
}


bool CTextDraw::BindDC(HDC hdc, const RECT &Rect)
{
	if (m_pEngine == nullptr)
		return false;

	return m_pEngine->BindDC(hdc, Rect);
}


bool CTextDraw::SetFont(HFONT hfont)
{
	if (m_pEngine == nullptr)
		return false;

	return m_pEngine->SetFont(hfont);
}


bool CTextDraw::SetFont(const LOGFONT &Font)
{
	if (m_pEngine == nullptr)
		return false;

	const HFONT hfont = ::CreateFontIndirect(&Font);
	if (hfont == nullptr)
		return false;

	const bool fOK = m_pEngine->SetFont(hfont);

	::DeleteObject(hfont);

	return fOK;
}


bool CTextDraw::SetTextColor(COLORREF Color)
{
	if (m_pEngine == nullptr)
		return false;

	return m_pEngine->SetTextColor(Color);
}


int CTextDraw::CalcLineCount(LPCWSTR pszText, int Width)
{
	if (m_pEngine == nullptr || pszText == nullptr || Width <= 0)
		return 0;

	int Lines = 0;
	LPCWSTR p = pszText;

	while (*p != L'\0') {
		if (*p == L'\r' || *p == L'\n') {
			p++;
			if (*p == L'\n')
				p++;
			if (*p == L'\0')
				break;
			Lines++;
			continue;
		}

		int Length = GetLineLength(p);
		if (Length == 0)
			break;

		int Fit = GetFitCharCount(p, Length, Width);
		Fit = AdjustLineLength(p, Fit);
		Length -= Fit;
		p += Fit;
		Lines++;

		if (*p == L'\r')
			p++;
		if (*p == L'\n')
			p++;
	}

	return Lines;
}


bool CTextDraw::Draw(LPCWSTR pszText, const RECT &Rect, int LineHeight, DrawFlag Flags)
{
	if (m_pEngine == nullptr || pszText == nullptr
			|| Rect.left >= Rect.right || Rect.top >= Rect.bottom)
		return false;

	const int Width = Rect.right - Rect.left;

	LPCWSTR p = pszText;
	int y = Rect.top;

	while (*p != L'\0' && y < Rect.bottom) {
		if (*p == L'\r' || *p == L'\n') {
			p++;
			if (*p == L'\n')
				p++;
			y += LineHeight;
			continue;
		}

		int Length = GetLineLength(p);
		if (Length == 0)
			break;

		int Fit = GetFitCharCount(p, Length, Width);

		if (!!(m_Flags & Flag::EndEllipsis) && Fit < Length && y + LineHeight >= Rect.bottom) {
			static const WCHAR szEllipses[] = L"...";
			const size_t BufferLength = Fit + lengthof(szEllipses);
			m_StringBuffer.clear();
			m_StringBuffer.resize(std::max(BufferLength, 256_z));
			const LPWSTR pszBuffer = &m_StringBuffer[0];
			std::memcpy(pszBuffer, p, Fit * sizeof(WCHAR));
			LPWSTR pszCur = pszBuffer + Fit;
			for (;;) {
				StringCopy(pszCur, szEllipses);
				Length = static_cast<int>(pszCur - pszBuffer) + (lengthof(szEllipses) - 1);
				Fit = GetFitCharCount(pszBuffer, Length, Width);
				if (Fit >= Length || pszCur == pszBuffer)
					break;
				pszCur = StringPrevChar(pszBuffer, pszCur);
			}
			RECT rc = {Rect.left, y, Rect.right, y + LineHeight};
			m_pEngine->DrawText(pszBuffer, Fit, rc, Flags);
			return true;
		}

		Fit = AdjustLineLength(p, Fit);
		RECT rc = {Rect.left, y, Rect.right, y + LineHeight};
		DrawFlag LineFlags = Flags;
		if (!!(Flags & DrawFlag::JustifyMultiLine)) {
			if (p[Fit] != L'\0' && p[Fit] != L'\r' && p[Fit] != L'\n')
				LineFlags |= DrawFlag::Align_Justified;
		}
		m_pEngine->DrawText(p, Fit, rc, LineFlags);
		Length -= Fit;
		p += Fit;
		y += LineHeight;

		if (*p == L'\r')
			p++;
		if (*p == L'\n')
			p++;
	}

	return true;
}


bool CTextDraw::GetFontMetrics(FontMetrics *pMetrics)
{
	if (m_pEngine == nullptr)
		return false;

	return m_pEngine->GetFontMetrics(pMetrics);
}


bool CTextDraw::GetTextMetrics(LPCWSTR pText, int Length, TextMetrics *pMetrics)
{
	if (m_pEngine == nullptr)
		return false;

	return m_pEngine->GetTextMetrics(pText, Length, pMetrics);
}


bool CTextDraw::SetClippingRect(const RECT &Rect)
{
	if (m_pEngine == nullptr)
		return false;

	return m_pEngine->SetClippingRect(Rect);
}


bool CTextDraw::ResetClipping()
{
	if (m_pEngine == nullptr)
		return false;

	return m_pEngine->ResetClipping();
}


int CTextDraw::GetLineLength(LPCWSTR pszText)
{
	LPCWSTR p = pszText;
	while (*p != L'\0' && *p != L'\r' && *p != '\n')
		p++;
	return static_cast<int>(p - pszText);
}


int CTextDraw::AdjustLineLength(LPCWSTR pszText, int Length)
{
	if (Length < 1) {
		Length = StringCharLength(pszText);
		if (Length < 1)
			Length = 1;
	} else if (!!(m_Flags & Flag::JapaneseHyphnation) && Length > 1) {
		LPCWSTR p = pszText + Length - 1;
		if (IsEndProhibitChar(*p)) {
			p--;
			while (p >= pszText) {
				if (!IsEndProhibitChar(*p)) {
					Length = static_cast<int>(p - pszText) + 1;
					break;
				}
				p--;
			}
		} else if (IsStartProhibitChar(*(p + 1))) {
			while (p > pszText) {
				if (!IsStartProhibitChar(*p)) {
					if (IsEndProhibitChar(*(p - 1))) {
						p--;
						while (p >= pszText) {
							if (!IsEndProhibitChar(*p)) {
								Length = static_cast<int>(p - pszText) + 1;
								break;
							}
							p--;
						}
					} else {
						Length = static_cast<int>(p - pszText);
					}
					break;
				}
				p--;
			}
		}
	}

	return Length;
}


int CTextDraw::GetFitCharCount(LPCWSTR pText, int Length, int Width)
{
	if (m_pEngine == nullptr)
		return 0;

	return m_pEngine->GetFitCharCount(pText, Length, Width);
}


bool CTextDraw::IsStartProhibitChar(WCHAR Char)
{
	return ::StrChrW(m_pszStartProhibitChars, Char) != nullptr;
}


bool CTextDraw::IsEndProhibitChar(WCHAR Char)
{
	return ::StrChrW(m_pszEndProhibitChars, Char) != nullptr;
}




void CTextDrawEngine::Finalize()
{
}


bool CTextDrawEngine::BeginDraw(HDC hdc, const RECT &Rect)
{
	return true;
}


bool CTextDrawEngine::EndDraw()
{
	return true;
}


bool CTextDrawEngine::BindDC(HDC hdc, const RECT &Rect)
{
	return true;
}


bool CTextDrawEngine::SetClippingRect(const RECT &Rect)
{
	return true;
}


bool CTextDrawEngine::ResetClipping()
{
	return true;
}


bool CTextDrawEngine::OnWindowPosChanged()
{
	return true;
}




CTextDrawEngine_GDI::~CTextDrawEngine_GDI()
{
	Finalize();
}


void CTextDrawEngine_GDI::Finalize()
{
	UnbindDC();
}


bool CTextDrawEngine_GDI::BeginDraw(HDC hdc, const RECT &Rect)
{
	UnbindDC();
	return BindDC(hdc, Rect);
}


bool CTextDrawEngine_GDI::EndDraw()
{
	if (m_hdc == nullptr)
		return false;
	UnbindDC();
	return true;
}


bool CTextDrawEngine_GDI::BindDC(HDC hdc, const RECT &Rect)
{
	UnbindDC();

	if (hdc == nullptr)
		return false;

	m_hdc = hdc;
	m_hfontOld = static_cast<HFONT>(::GetCurrentObject(m_hdc, OBJ_FONT));
	m_OldTextColor = ::GetTextColor(m_hdc);

	return true;
}


bool CTextDrawEngine_GDI::SetFont(HFONT hfont)
{
	if (hfont == nullptr || m_hdc == nullptr)
		return false;

	::SelectObject(m_hdc, hfont);

	return true;
}


bool CTextDrawEngine_GDI::SetTextColor(COLORREF Color)
{
	if (m_hdc == nullptr)
		return false;

	::SetTextColor(m_hdc, Color);

	return true;
}


bool CTextDrawEngine_GDI::DrawText(LPCWSTR pText, int Length, const RECT &Rect, CTextDraw::DrawFlag Flags)
{
	if (m_hdc == nullptr)
		return false;

	if (!!(Flags & CTextDraw::DrawFlag::Align_Justified)) {
		Util::CTempBuffer<int, 256> CharPos(Length);
		SIZE sz;

		::GetTextExtentExPointW(m_hdc, pText, Length, 0, nullptr, CharPos.GetBuffer(), &sz);
		if (sz.cx == 0)
			return false;
		int Prev = ::MulDiv(CharPos[0], Rect.right - Rect.left, sz.cx);
		for (int i = 1; i < Length; i++) {
			const int Pos = ::MulDiv(CharPos[i], Rect.right - Rect.left, sz.cx);
			CharPos[i] = Pos - Prev;
			Prev = Pos;
		}
		::ExtTextOutW(m_hdc, Rect.left, Rect.top, 0, &Rect, pText, Length, CharPos.GetBuffer());
	} else {
		::TextOutW(m_hdc, Rect.left, Rect.top, pText, Length);
	}

	return true;
}


int CTextDrawEngine_GDI::GetFitCharCount(LPCWSTR pText, int Length, int Width)
{
	if (m_hdc == nullptr)
		return 0;

	/*
		GetCharacterPlacement はフォントにグリフが無く他のフォントで代替される場合に正しく計算されない模様。
		GetTextExtentExPoint は常に文字列全体の幅が計算されるため効率が悪いが…。
	*/
#if 0

	GCP_RESULTSW Results = {sizeof(GCP_RESULTSW)};

	Results.nGlyphs = Length;

	if (::GetCharacterPlacementW(m_hdc, pText, Length, Width, &Results, GCP_MAXEXTENT) == 0)
		return 0;

	return Results.nMaxFit;

#else

	int Fit;
	SIZE Size;

	if (!::GetTextExtentExPoint(m_hdc, pText, Length, Width, &Fit, nullptr, &Size))
		return 0;

	return Fit;

#endif
}


bool CTextDrawEngine_GDI::GetFontMetrics(FontMetrics *pMetrics)
{
	if (m_hdc == nullptr || pMetrics == nullptr)
		return false;

	TEXTMETRIC tm;

	if (!::GetTextMetrics(m_hdc, &tm))
		return false;

	pMetrics->Height = tm.tmHeight;
	pMetrics->LineGap = tm.tmExternalLeading;

	return true;
}


bool CTextDrawEngine_GDI::GetTextMetrics(LPCWSTR pText, int Length, TextMetrics *pMetrics)
{
	if (m_hdc == nullptr || pText == nullptr || pMetrics == nullptr)
		return false;

	SIZE sz;

	if (!::GetTextExtentPoint32W(m_hdc, pText, Length < 0 ?::lstrlenW(pText) : Length, &sz))
		return false;

	pMetrics->Width = sz.cx;
	pMetrics->Height = sz.cy;

	return true;
}


void CTextDrawEngine_GDI::UnbindDC()
{
	if (m_hdc != nullptr) {
		::SelectObject(m_hdc, m_hfontOld);
		m_hfontOld = nullptr;
		::SetTextColor(m_hdc, m_OldTextColor);
		m_hdc = nullptr;
	}
}




CTextDrawEngine_DirectWrite::CTextDrawEngine_DirectWrite(CDirectWriteRenderer &Renderer)
	: m_Renderer(Renderer)
{
}


CTextDrawEngine_DirectWrite::~CTextDrawEngine_DirectWrite()
{
	Finalize();
}


void CTextDrawEngine_DirectWrite::Finalize()
{
	ClearFontCache();
}


bool CTextDrawEngine_DirectWrite::BeginDraw(HDC hdc, const RECT &Rect)
{
	return m_Renderer.BeginDraw(hdc, Rect);
}


bool CTextDrawEngine_DirectWrite::EndDraw()
{
	const bool fOK = m_Renderer.EndDraw();
	m_pFont = nullptr;
	m_Brush.Destroy();
	if (m_Renderer.IsNeedRecreate())
		ClearFontCache();
	return fOK;
}


bool CTextDrawEngine_DirectWrite::BindDC(HDC hdc, const RECT &Rect)
{
	return m_Renderer.BindDC(hdc, Rect);
}


bool CTextDrawEngine_DirectWrite::SetFont(HFONT hfont)
{
	if (hfont == nullptr)
		return false;

	LOGFONT lf;
	if (::GetObject(hfont, sizeof(LOGFONT), &lf) != sizeof(LOGFONT))
		return false;

	for (const auto &e : m_FontCache) {
		LOGFONT lfCache;

		if (e->GetLogFont(&lfCache) && CompareLogFont(&lf, &lfCache)) {
			m_pFont = e.get();
			return true;
		}
	}

	CDirectWriteFont *pFont = new CDirectWriteFont;
	if (!pFont->Create(m_Renderer, lf)) {
		delete pFont;
		return false;
	}

	if (m_FontCache.size() >= m_MaxFontCache) {
		m_FontCache.pop_back();
	}

	m_FontCache.emplace_front(pFont);
	m_pFont = pFont;

	return true;
}


bool CTextDrawEngine_DirectWrite::SetTextColor(COLORREF Color)
{
	if (m_Brush.IsCreated())
		return m_Brush.SetColor(GetRValue(Color), GetGValue(Color), GetBValue(Color));
	return m_Brush.Create(m_Renderer, GetRValue(Color), GetGValue(Color), GetBValue(Color));
}


bool CTextDrawEngine_DirectWrite::DrawText(LPCWSTR pText, int Length, const RECT &Rect, CTextDraw::DrawFlag Flags)
{
	if (m_pFont == nullptr)
		return false;

	CDirectWriteRenderer::DrawTextFlag DrawTextFlags = CDirectWriteRenderer::DrawTextFlag::None;
	if (!!(Flags & CTextDraw::DrawFlag::Align_HorzCenter))
		DrawTextFlags |= CDirectWriteRenderer::DrawTextFlag::Align_HorzCenter;
	if (!!(Flags & CTextDraw::DrawFlag::Align_Right))
		DrawTextFlags |= CDirectWriteRenderer::DrawTextFlag::Align_Right;
	if (!!(Flags & CTextDraw::DrawFlag::Align_Justified))
		DrawTextFlags |= CDirectWriteRenderer::DrawTextFlag::Align_Justified;
	if (!!(Flags & CTextDraw::DrawFlag::Align_VertCenter))
		DrawTextFlags |= CDirectWriteRenderer::DrawTextFlag::Align_VertCenter;
	if (!!(Flags & CTextDraw::DrawFlag::Align_Bottom))
		DrawTextFlags |= CDirectWriteRenderer::DrawTextFlag::Align_Bottom;

	return m_Renderer.DrawText(pText, Length, Rect, *m_pFont, m_Brush, DrawTextFlags);
}


int CTextDrawEngine_DirectWrite::GetFitCharCount(LPCWSTR pText, int Length, int Width)
{
	if (m_pFont == nullptr)
		return false;

	return m_Renderer.GetFitCharCount(pText, Length, Width, *m_pFont);
}


bool CTextDrawEngine_DirectWrite::GetFontMetrics(FontMetrics *pMetrics)
{
	if (m_pFont == nullptr || pMetrics == nullptr)
		return false;

	CDirectWriteRenderer::FontMetrics Metrics;

	if (!m_Renderer.GetFontMetrics(*m_pFont, &Metrics))
		return false;

	pMetrics->Height = static_cast<int>(std::ceil(Metrics.Ascent + Metrics.Descent));
	pMetrics->LineGap = static_cast<int>(std::round(Metrics.LineGap));

	return true;
}


bool CTextDrawEngine_DirectWrite::GetTextMetrics(LPCWSTR pText, int Length, TextMetrics *pMetrics)
{
	if (m_pFont == nullptr || pText == nullptr || pMetrics == nullptr)
		return false;

	CDirectWriteRenderer::TextMetrics Metrics;

	if (!m_Renderer.GetTextMetrics(pText, Length, *m_pFont, &Metrics))
		return false;

	pMetrics->Width = static_cast<int>(std::ceil(Metrics.WidthIncludingTrailingWhitespace));
	pMetrics->Height = static_cast<int>(std::ceil(Metrics.Height));

	return true;
}


bool CTextDrawEngine_DirectWrite::SetClippingRect(const RECT &Rect)
{
	return m_Renderer.SetClippingRect(Rect);
}


bool CTextDrawEngine_DirectWrite::ResetClipping()
{
	return m_Renderer.ResetClipping();
}


bool CTextDrawEngine_DirectWrite::OnWindowPosChanged()
{
	return m_Renderer.OnWindowPosChanged();
}


void CTextDrawEngine_DirectWrite::ClearFontCache()
{
	m_FontCache.clear();
}


bool CTextDrawEngine_DirectWrite::SetMaxFontCache(std::size_t MaxCache)
{
	if (MaxCache < 1)
		return false;

	while (m_FontCache.size() > MaxCache) {
		m_FontCache.pop_back();
	}

	m_MaxFontCache = MaxCache;

	return true;
}


}	// namespace TVTest
