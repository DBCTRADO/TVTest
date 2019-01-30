/*
  TVTest
  Copyright(c) 2008-2019 DBCTRADO

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


CRichEditUtil::CRichEditUtil()
	: m_hLib(nullptr)
{
}


CRichEditUtil::~CRichEditUtil()
{
	UnloadRichEditLib();
}


bool CRichEditUtil::LoadRichEditLib()
{
	if (m_hLib == nullptr) {
		m_hLib = Util::LoadSystemLibrary(TEXT("Riched20.dll"));
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
	pcf->yHeight = abs(plf->lfHeight) * 72 * 20 / ::GetDeviceCaps(hdc, LOGPIXELSY);
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
	pcf->yHeight = abs(plf->lfHeight) * 72 * 20 / ::GetDeviceCaps(hdc, LOGPIXELSY);
	pcf->crTextColor = ::GetSysColor(COLOR_WINDOWTEXT);
	pcf->bPitchAndFamily = plf->lfPitchAndFamily;
	pcf->bCharSet = plf->lfCharSet;
	StringCopy(pcf->szFaceName, plf->lfFaceName);
	pcf->wWeight = (WORD)plf->lfWeight;

	return true;
}


void CRichEditUtil::CharFormatToCharFormat2(const CHARFORMAT *pcf, CHARFORMAT2 *pcf2)
{
	::CopyMemory(pcf2, pcf, sizeof(CHARFORMAT));
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
	return AppendText(hwndEdit, pszText, (const CHARFORMAT*)pcf);
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
	int NumLines = (int)::SendMessage(hwndEdit, EM_GETLINECOUNT, 0, 0);
	int MaxWidth = 0;

	for (int i = 0; i < NumLines; i++) {
		int Index = (int)::SendMessage(hwndEdit, EM_LINEINDEX, i, 0);
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


bool CRichEditUtil::DetectURL(
	HWND hwndEdit, const CHARFORMAT *pcf, int FirstLine, int LastLine,
	DetectURLFlag Flags, CharRangeList *pCharRangeList)
{
	const int LineCount = (int)::SendMessage(hwndEdit, EM_GETLINECOUNT, 0, 0);
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
		const int LineIndex = (int)::SendMessage(hwndEdit, EM_LINEINDEX, i, 0);
		TCHAR szText[2048], *p;
		int TotalLength = 0, Length;

		p = szText;
		while (i < LastLine) {
#ifdef UNICODE
			p[0] = (WORD)(lengthof(szText) - 2 - TotalLength);
#else
			*(WORD*)p = (WORD)(sizeof(szText) - sizeof(WORD) - 1 - TotalLength);
#endif
			Length = (int)::SendMessage(hwndEdit, EM_GETLINE, i, reinterpret_cast<LPARAM>(p));
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
			Length = TotalLength;
			while (SearchNextURL(&q, &Length)) {
				cr.cpMin = LineIndex + (LONG)(q - szText);
				cr.cpMax = cr.cpMin + Length;
				::SendMessage(hwndEdit, EM_EXSETSEL, 0, reinterpret_cast<LPARAM>(&cr));
#ifdef UNICODE
				if (!!(Flags & DetectURLFlag::ToHalfWidth) && *q >= 0xFF01) {
					LPWSTR pszURL = new WCHAR[Length + 1];
					for (int j = 0; j < Length; j++)
						pszURL[j] = q[j] - 0xFEE0;
					pszURL[Length] = L'\0';
					::SendMessage(hwndEdit, EM_REPLACESEL, 0, reinterpret_cast<LPARAM>(pszURL));
					delete [] pszURL;
					::SendMessage(hwndEdit, EM_EXSETSEL, 0, reinterpret_cast<LPARAM>(&cr));
				}
#endif
				::SendMessage(hwndEdit, EM_SETCHARFORMAT, SCF_SELECTION, reinterpret_cast<LPARAM>(&cfLink));
				if (pCharRangeList != nullptr)
					pCharRangeList->push_back(cr);
				fDetect = true;
				q += Length;
				Length = TotalLength - (int)(q - szText);
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
				if (p[i] < 0x0080) {
					while (i + URLLength < TextLength && ::StrChr(m_pszURLChars, p[i + URLLength]) != nullptr)
						URLLength++;
				} else {
					while (i + URLLength < TextLength && ::StrChr(m_pszURLFullWidthChars, p[i + URLLength]) != nullptr)
						URLLength++;
				}
#ifdef UNICODE
				if (i > 0 && p[i - 1] == L'(' && p[i + URLLength - 1] == L')')
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
	int Index = LinkHitTest(hwndEdit, Pos, LinkList);
	if (Index < 0)
		return false;

	return OpenLink(hwndEdit, LinkList[Index]);
}


int CRichEditUtil::LinkHitTest(HWND hwndEdit, const POINT &Pos, const CharRangeList &LinkList)
{
	if (hwndEdit == nullptr || LinkList.empty())
		return -1;

	POINTL pt = {Pos.x, Pos.y};
	LONG Index = (LONG)::SendMessage(hwndEdit, EM_CHARFROMPOS, 0, reinterpret_cast<LPARAM>(&pt));
	for (size_t i = 0; i < LinkList.size(); i++) {
		if (LinkList[i].cpMin <= Index && LinkList[i].cpMax > Index) {
			return (int)i;
		}
	}

	return -1;
}


bool CRichEditUtil::OpenLink(HWND hwndEdit, const CHARRANGE &Range)
{
	if (Range.cpMax - Range.cpMin >= 256)
		return false;

	TCHAR szText[256];
	int Length;

	TEXTRANGE tr;
	tr.chrg = Range;
	tr.lpstrText = szText;
	Length = (int)::SendMessage(hwndEdit, EM_GETTEXTRANGE, 0, reinterpret_cast<LPARAM>(&tr));
	if (Length <= 0)
		return false;

	TCHAR szURL[256 + 7];
	Length = ::LCMapString(
		LOCALE_USER_DEFAULT, LCMAP_HALFWIDTH,
		szText, Length, szURL, lengthof(szURL));
	szURL[Length] = _T('\0');
	if (::StrCmpN(szURL, TEXT("www."), 4) == 0) {
		StringCopy(szText, szURL);
		StringPrintf(szURL, TEXT("http://%s"), szText);
	}
	::ShellExecute(nullptr, TEXT("open"), szURL, nullptr, nullptr, SW_SHOWNORMAL);

	return true;
}


}	// namespace TVTest
