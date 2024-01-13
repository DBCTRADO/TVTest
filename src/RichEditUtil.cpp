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
#include "RichEditUtil.h"
#include "Common/DebugDef.h"


namespace TVTest
{


const LPCTSTR CRichEditUtil::m_pszURLChars =
	TEXT("0123456789")
	TEXT("ABCDEFGHIJKLMNOPQRSTUVWXYZ")
	TEXT("abcdefghijklmnopqrstuvwxyz")
	TEXT("!#$%&'()*+,-./:;=?@[]_~");
const LPCTSTR CRichEditUtil::m_pszURLFullWidthChars =
	TEXT("０１２３４５６７８９")
	TEXT("ＡＢＣＤＥＦＧＨＩＪＫＬＭＮＯＰＱＲＳＴＵＶＷＸＹＺ")
	TEXT("ａｂｃｄｅｆｇｈｉｊｋｌｍｎｏｐｑｒｓｔｕｖｗｘｙｚ")
	TEXT("！＃＄％＆’（）＊＋，－．／：；＝？＠［］＿～");


CRichEditUtil::~CRichEditUtil()
{
	UnloadRichEditLib();
}


bool CRichEditUtil::LoadRichEditLib()
{
	if (m_hLib == nullptr) {
		m_hLib = Util::LoadSystemLibrary(TEXT("Msftedit.dll"));
		if (m_hLib == nullptr)
			return false;
	}
	return true;
}


void CRichEditUtil::UnloadRichEditLib()
{
	if (m_hLib != nullptr) {
		::FreeLibrary(m_hLib);
		m_hLib = nullptr;
	}
}


bool CRichEditUtil::LogFontToCharFormat(HDC hdc, const LOGFONT *plf, CHARFORMAT *pcf)
{
	if (hdc == nullptr || plf == nullptr || pcf == nullptr)
		return false;

	pcf->cbSize = sizeof(CHARFORMAT);
	pcf->dwMask = CFM_BOLD | CFM_CHARSET | CFM_COLOR | CFM_FACE | CFM_ITALIC | CFM_SIZE | CFM_STRIKEOUT | CFM_UNDERLINE;
	pcf->dwEffects = 0;
	if (plf->lfWeight >= FW_BOLD)
		pcf->dwEffects |= CFE_BOLD;
	if (plf->lfItalic)
		pcf->dwEffects |= CFE_ITALIC;
	if (plf->lfUnderline)
		pcf->dwEffects |= CFE_UNDERLINE;
	if (plf->lfStrikeOut)
		pcf->dwEffects |= CFE_STRIKEOUT;
	pcf->yHeight = std::abs(plf->lfHeight) * 72 * 20 / ::GetDeviceCaps(hdc, LOGPIXELSY);
	pcf->crTextColor = ::GetSysColor(COLOR_WINDOWTEXT);
	pcf->bPitchAndFamily = plf->lfPitchAndFamily;
	pcf->bCharSet = plf->lfCharSet;
	StringCopy(pcf->szFaceName, plf->lfFaceName);

	return true;
}


bool CRichEditUtil::LogFontToCharFormat2(HDC hdc, const LOGFONT *plf, CHARFORMAT2 *pcf)
{
	if (hdc == nullptr || plf == nullptr || pcf == nullptr)
		return false;

	*pcf = CHARFORMAT2();
	pcf->cbSize = sizeof(CHARFORMAT2);
	pcf->dwMask = CFM_CHARSET | CFM_COLOR | CFM_FACE | CFM_ITALIC | CFM_SIZE | CFM_STRIKEOUT | CFM_UNDERLINE | CFM_WEIGHT;
	pcf->dwEffects = 0;
	if (plf->lfItalic)
		pcf->dwEffects |= CFE_ITALIC;
	if (plf->lfUnderline)
		pcf->dwEffects |= CFE_UNDERLINE;
	if (plf->lfStrikeOut)
		pcf->dwEffects |= CFE_STRIKEOUT;
	pcf->yHeight = std::abs(plf->lfHeight) * 72 * 20 / ::GetDeviceCaps(hdc, LOGPIXELSY);
	pcf->crTextColor = ::GetSysColor(COLOR_WINDOWTEXT);
	pcf->bPitchAndFamily = plf->lfPitchAndFamily;
	pcf->bCharSet = plf->lfCharSet;
	StringCopy(pcf->szFaceName, plf->lfFaceName);
	pcf->wWeight = static_cast<WORD>(plf->lfWeight);

	return true;
}


void CRichEditUtil::CharFormatToCharFormat2(const CHARFORMAT *pcf, CHARFORMAT2 *pcf2)
{
	std::memcpy(pcf2, pcf, sizeof(CHARFORMAT));
	pcf2->cbSize = sizeof(CHARFORMAT2);
}


bool CRichEditUtil::AppendText(HWND hwndEdit, LPCTSTR pszText, const CHARFORMAT *pcf)
{
	if (hwndEdit == nullptr || pszText == nullptr)
		return false;

	CHARRANGE cr, crOld;

	::SendMessage(hwndEdit, WM_SETREDRAW, FALSE, 0);
	::SendMessage(hwndEdit, EM_EXGETSEL, 0, reinterpret_cast<LPARAM>(&crOld));
	cr.cpMin = 0;
	cr.cpMax = -1;
	::SendMessage(hwndEdit, EM_EXSETSEL, 0, reinterpret_cast<LPARAM>(&cr));
	::SendMessage(hwndEdit, EM_EXGETSEL, 0, reinterpret_cast<LPARAM>(&cr));
	cr.cpMin = cr.cpMax;
	::SendMessage(hwndEdit, EM_EXSETSEL, 0, reinterpret_cast<LPARAM>(&cr));
	if (pcf != nullptr) {
		::SendMessage(hwndEdit, EM_SETCHARFORMAT, SCF_SELECTION, reinterpret_cast<LPARAM>(pcf));
	}
	::SendMessage(hwndEdit, EM_REPLACESEL, 0, reinterpret_cast<LPARAM>(pszText));
	::SendMessage(hwndEdit, EM_EXSETSEL, 0, reinterpret_cast<LPARAM>(&crOld));
	::SendMessage(hwndEdit, WM_SETREDRAW, TRUE, 0);
	::InvalidateRect(hwndEdit, nullptr, TRUE);

	return true;
}


bool CRichEditUtil::AppendText(HWND hwndEdit, LPCTSTR pszText, const CHARFORMAT2 *pcf)
{
	return AppendText(hwndEdit, pszText, reinterpret_cast<const CHARFORMAT*>(pcf));
}


bool CRichEditUtil::CopyAllText(HWND hwndEdit)
{
	POINT ptScroll;
	CHARRANGE cr, crOld;

	::SendMessage(hwndEdit, WM_SETREDRAW, FALSE, 0);
	::SendMessage(hwndEdit, EM_GETSCROLLPOS, 0, reinterpret_cast<LPARAM>(&ptScroll));
	::SendMessage(hwndEdit, EM_HIDESELECTION, TRUE, 0);
	::SendMessage(hwndEdit, EM_EXGETSEL, 0, reinterpret_cast<LPARAM>(&crOld));
	cr.cpMin = 0;
	cr.cpMax = -1;
	::SendMessage(hwndEdit, EM_EXSETSEL, 0, reinterpret_cast<LPARAM>(&cr));
	::SendMessage(hwndEdit, WM_COPY, 0, 0);
	::SendMessage(hwndEdit, EM_EXSETSEL, 0, reinterpret_cast<LPARAM>(&crOld));
	::SendMessage(hwndEdit, EM_HIDESELECTION, FALSE, 0);
	::SendMessage(hwndEdit, EM_SETSCROLLPOS, 0, reinterpret_cast<LPARAM>(&ptScroll));
	::SendMessage(hwndEdit, WM_SETREDRAW, TRUE, 0);
	return true;
}


void CRichEditUtil::SelectAll(HWND hwndEdit)
{
	CHARRANGE cr = {0, -1};

	::SendMessage(hwndEdit, EM_EXSETSEL, 0, reinterpret_cast<LPARAM>(&cr));
}


bool CRichEditUtil::IsSelected(HWND hwndEdit)
{
	return hwndEdit != nullptr
		&& ::SendMessage(hwndEdit, EM_SELECTIONTYPE, 0, 0) != SEL_EMPTY;
}


String CRichEditUtil::GetSelectedText(HWND hwndEdit)
{
	String Text;

	if (hwndEdit != nullptr) {
		CHARRANGE cr = {0, 0};

		::SendMessage(hwndEdit, EM_EXGETSEL, 0, reinterpret_cast<LPARAM>(&cr));
		if (cr.cpMax > cr.cpMin) {
			Text.resize(cr.cpMax - cr.cpMin + 1);
			const LRESULT Length = ::SendMessage(hwndEdit, EM_GETSELTEXT, 0, reinterpret_cast<LPARAM>(Text.data()));
			if (Length > 0)
				Text.resize(Length);
			else
				Text.clear();
		}
	}

	return Text;
}


int CRichEditUtil::GetMaxLineWidth(HWND hwndEdit)
{
	const int NumLines = static_cast<int>(::SendMessage(hwndEdit, EM_GETLINECOUNT, 0, 0));
	int MaxWidth = 0;

	for (int i = 0; i < NumLines; i++) {
		const int Index = static_cast<int>(::SendMessage(hwndEdit, EM_LINEINDEX, i, 0));
		POINTL pt;
		::SendMessage(
			hwndEdit, EM_POSFROMCHAR,
			reinterpret_cast<WPARAM>(&pt),
			Index + ::SendMessage(hwndEdit, EM_LINELENGTH, Index, 0));
		if (pt.x > MaxWidth)
			MaxWidth = pt.x;
	}
	return MaxWidth;
}


void CRichEditUtil::DisableAutoFont(HWND hwndEdit)
{
	LPARAM Options = ::SendMessage(hwndEdit, EM_GETLANGOPTIONS, 0, 0);
	Options &= ~(IMF_AUTOFONT | IMF_DUALFONT);
	::SendMessage(hwndEdit, EM_SETLANGOPTIONS, 0, Options);
}


bool CRichEditUtil::DetectURL(
	HWND hwndEdit, const CHARFORMAT *pcf, int FirstLine, int LastLine,
	DetectURLFlag Flags, CharRangeList *pCharRangeList)
{
	const int LineCount = static_cast<int>(::SendMessage(hwndEdit, EM_GETLINECOUNT, 0, 0));
	if (LastLine < 0 || LastLine > LineCount)
		LastLine = LineCount;

	CHARFORMAT2 cfLink;
	CharFormatToCharFormat2(pcf, &cfLink);
	if (!(Flags & DetectURLFlag::NoLink)) {
		// リンクの文字色は設定できない模様
		cfLink.dwMask |= CFM_UNDERLINE | CFM_LINK/* | CFM_COLOR*/;
		cfLink.dwEffects |= CFE_UNDERLINE | CFE_LINK;
		//cfLink.crTextColor = ::GetSysColor(COLOR_HOTLIGHT);
	} else {
		cfLink.dwMask |= CFM_UNDERLINE;
		cfLink.dwEffects |= CFE_UNDERLINE;
	}

	CHARRANGE cr, crOld;
	bool fDetect = false;

	if (pCharRangeList != nullptr)
		pCharRangeList->clear();

	::SendMessage(hwndEdit, WM_SETREDRAW, FALSE, 0);
	::SendMessage(hwndEdit, EM_EXGETSEL, 0, reinterpret_cast<LPARAM>(&crOld));

	for (int i = FirstLine; i < LastLine;) {
		const int LineIndex = static_cast<int>(::SendMessage(hwndEdit, EM_LINEINDEX, i, 0));
		TCHAR szText[2048];
		LPTSTR p = szText;
		int TotalLength = 0;

		while (i < LastLine) {
#ifdef UNICODE
			p[0] = static_cast<WORD>(lengthof(szText) - 2 - TotalLength);
#else
			*reinterpret_cast<WORD*>(p) = static_cast<WORD>(sizeof(szText) - sizeof(WORD) - 1 - TotalLength);
#endif
			const int Length = static_cast<int>(::SendMessage(hwndEdit, EM_GETLINE, i, reinterpret_cast<LPARAM>(p)));
			i++;
			if (Length < 1)
				break;
			p += Length;
			TotalLength += Length;
			if (*(p - 1) == _T('\r') || *(p - 1) == _T('\n'))
				break;
		}
		if (TotalLength > 0) {
			szText[TotalLength] = _T('\0');
			LPCTSTR q = szText;
			int Length = TotalLength;
			while (SearchNextURL(&q, &Length)) {
				cr.cpMin = LineIndex + static_cast<LONG>(q - szText);
				cr.cpMax = cr.cpMin + Length;
				::SendMessage(hwndEdit, EM_EXSETSEL, 0, reinterpret_cast<LPARAM>(&cr));
#ifdef UNICODE
				if (!!(Flags & DetectURLFlag::ToHalfWidth)) {
					LPWSTR pszURL = nullptr;
					for (int j = 0; j < Length; j++) {
						const LPCWSTR pFound = ::StrChr(m_pszURLFullWidthChars, q[j]);
						if (pFound != nullptr) {
							pszURL = szText + (q - szText);
							pszURL[j] = m_pszURLChars[pFound - m_pszURLFullWidthChars];
						}
					}
					if (pszURL != nullptr) {
						const WCHAR cEnd = pszURL[Length];
						pszURL[Length] = L'\0';
						::SendMessage(hwndEdit, EM_REPLACESEL, 0, reinterpret_cast<LPARAM>(pszURL));
						pszURL[Length] = cEnd;
						::SendMessage(hwndEdit, EM_EXSETSEL, 0, reinterpret_cast<LPARAM>(&cr));
					}
				}
#endif
				::SendMessage(hwndEdit, EM_SETCHARFORMAT, SCF_SELECTION, reinterpret_cast<LPARAM>(&cfLink));
				if (pCharRangeList != nullptr)
					pCharRangeList->push_back(cr);
				fDetect = true;
				q += Length;
				Length = TotalLength - static_cast<int>(q - szText);
			}
		}
	}

	::SendMessage(hwndEdit, EM_EXSETSEL, 0, reinterpret_cast<LPARAM>(&crOld));
	::SendMessage(hwndEdit, WM_SETREDRAW, TRUE, 0);
	::InvalidateRect(hwndEdit, nullptr, TRUE);

	return fDetect;
}


bool CRichEditUtil::SearchNextURL(LPCTSTR *ppszText, int *pLength)
{
	static const struct {
		LPCTSTR pszPrefix;
		int Length;
	} URLPrefixList[] = {
		{TEXT("http://"),  7},
		{TEXT("https://"), 8},
		{TEXT("www."),     4},
	};

	LPCTSTR p = *ppszText;
	const int TextLength = *pLength;

	for (int i = 0; i < TextLength - 4; i++) {
		for (const auto &Prefix : URLPrefixList) {
			int URLLength = Prefix.Length;
			if (i + URLLength < TextLength
					&& ::CompareString(
							LOCALE_USER_DEFAULT, NORM_IGNOREWIDTH,
							&p[i], URLLength, Prefix.pszPrefix, URLLength) == CSTR_EQUAL) {
				while (i + URLLength < TextLength) {
					if (p[i + URLLength] < 0x0080) {
						if (::StrChr(m_pszURLChars, p[i + URLLength]) == nullptr)
							break;
					} else {
						if (::StrChr(m_pszURLFullWidthChars, p[i + URLLength]) == nullptr)
							break;
					}
					URLLength++;
				}
#ifdef UNICODE
				const WCHAR LastChar = p[i + URLLength - 1];
				if ((i > 0 && (p[i - 1] == L'(' || p[i - 1] == L'（') && (LastChar == L')' || LastChar == L'）'))
						|| LastChar == L'('
						|| LastChar == L'（')
					URLLength--;
#endif
				*ppszText = p + i;
				*pLength = URLLength;
				return true;
			}
		}
	}

	return false;
}


bool CRichEditUtil::HandleLinkClick(const ENLINK *penl)
{
	if (penl == nullptr)
		return false;

	return OpenLink(penl->nmhdr.hwndFrom, penl->chrg);
}


bool CRichEditUtil::HandleLinkClick(HWND hwndEdit, const POINT &Pos, const CharRangeList &LinkList)
{
	const int Index = LinkHitTest(hwndEdit, Pos, LinkList);
	if (Index < 0)
		return false;

	return OpenLink(hwndEdit, LinkList[Index]);
}


int CRichEditUtil::LinkHitTest(HWND hwndEdit, const POINT &Pos, const CharRangeList &LinkList)
{
	if (hwndEdit == nullptr || LinkList.empty())
		return -1;

	POINTL pt = {Pos.x, Pos.y};
	const LONG Index = static_cast<LONG>(::SendMessage(hwndEdit, EM_CHARFROMPOS, 0, reinterpret_cast<LPARAM>(&pt)));
	for (size_t i = 0; i < LinkList.size(); i++) {
		if (LinkList[i].cpMin <= Index && LinkList[i].cpMax > Index) {
			return static_cast<int>(i);
		}
	}

	return -1;
}


bool CRichEditUtil::OpenLink(HWND hwndEdit, const CHARRANGE &Range)
{
	if (Range.cpMax - Range.cpMin >= 256)
		return false;

	TCHAR szText[256];

	TEXTRANGE tr;
	tr.chrg = Range;
	tr.lpstrText = szText;
	int Length = static_cast<int>(::SendMessage(hwndEdit, EM_GETTEXTRANGE, 0, reinterpret_cast<LPARAM>(&tr)));
	if (Length <= 0)
		return false;

	TCHAR szURL[256 + 7];
	Length = ::LCMapString(
		LOCALE_USER_DEFAULT, LCMAP_HALFWIDTH,
		szText, Length, szURL, lengthof(szURL));
	szURL[Length] = _T('\0');
	if (::StrCmpN(szURL, TEXT("www."), 4) == 0) {
		StringCopy(szText, szURL);
		StringFormat(szURL, TEXT("http://{}"), szText);
	}
	::ShellExecute(nullptr, TEXT("open"), szURL, nullptr, nullptr, SW_SHOWNORMAL);

	return true;
}




void CRichEditLinkHandler::Reset()
{
	m_LinkList.clear();
	m_ClickPos.x = -1;
	m_ClickPos.y = -1;
	m_fCursorOverLink = false;
}


bool CRichEditLinkHandler::DetectURL(
	HWND hwndEdit, const CHARFORMAT *pcf, int FirstLine, int LastLine,
	CRichEditUtil::DetectURLFlag Flags)
{
	Reset();
	m_hwndEdit = hwndEdit;
	return CRichEditUtil::DetectURL(hwndEdit, pcf, FirstLine, LastLine, Flags, &m_LinkList);
}


bool CRichEditLinkHandler::OnMsgFilter(MSGFILTER *pMsgFilter)
{
	switch (pMsgFilter->msg) {
	case WM_LBUTTONDOWN:
		m_ClickPos.x = GET_X_LPARAM(pMsgFilter->lParam);
		m_ClickPos.y = GET_Y_LPARAM(pMsgFilter->lParam);
		return true;
		break;

	case WM_MOUSEMOVE:
		m_ClickPos.x = -1;
		m_ClickPos.y = -1;
		{
			const POINT pt = {GET_X_LPARAM(pMsgFilter->lParam), GET_Y_LPARAM(pMsgFilter->lParam)};

			if (CRichEditUtil::LinkHitTest(m_hwndEdit, pt, m_LinkList) >= 0) {
				m_fCursorOverLink = true;
				::SetCursor(m_hLinkCursor);
			} else {
				m_fCursorOverLink = false;
			}
		}
		return true;

	case WM_LBUTTONUP:
		{
			const POINT pt = {GET_X_LPARAM(pMsgFilter->lParam), GET_Y_LPARAM(pMsgFilter->lParam)};

			if (m_ClickPos.x == pt.x && m_ClickPos.y == pt.y)
				CRichEditUtil::HandleLinkClick(m_hwndEdit, pt, m_LinkList);
		}
		return true;
	}

	return false;
}


bool CRichEditLinkHandler::OnSetCursor(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	if (reinterpret_cast<HWND>(wParam) == m_hwndEdit
			&& LOWORD(lParam) == HTCLIENT
			&& m_fCursorOverLink) {
		::SetCursor(m_hLinkCursor);
		return true;
	}

	return false;
}


void CRichEditLinkHandler::LeaveCursor()
{
	m_ClickPos.x = -1;
	m_ClickPos.y = -1;
	m_fCursorOverLink = false;
}


} // namespace TVTest
