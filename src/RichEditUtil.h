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


#ifndef TVTEST_RICH_EDIT_UTIL_H
#define TVTEST_RICH_EDIT_UTIL_H


#include <richedit.h>
#include <vector>


namespace TVTest
{

	class CRichEditUtil
	{
	public:
		typedef std::vector<CHARRANGE> CharRangeList;

		CRichEditUtil() = default;
		~CRichEditUtil();

		CRichEditUtil(const CRichEditUtil &) = delete;
		CRichEditUtil &operator=(const CRichEditUtil &) = delete;

		bool LoadRichEditLib();
		void UnloadRichEditLib();
		bool IsRichEditLibLoaded() const { return m_hLib != nullptr; }
		LPCTSTR GetWindowClassName() const { return TEXT("RICHEDIT50W"); }

		static bool LogFontToCharFormat(HDC hdc, const LOGFONT *plf, CHARFORMAT *pcf);
		static bool LogFontToCharFormat2(HDC hdc, const LOGFONT *plf, CHARFORMAT2 *pcf);
		static void CharFormatToCharFormat2(const CHARFORMAT *pcf, CHARFORMAT2 *pcf2);
		static bool AppendText(HWND hwndEdit, LPCTSTR pszText, const CHARFORMAT *pcf);
		static bool AppendText(HWND hwndEdit, LPCTSTR pszText, const CHARFORMAT2 *pcf);
		static bool CopyAllText(HWND hwndEdit);
		static void SelectAll(HWND hwndEdit);
		static bool IsSelected(HWND hwndEdit);
		static String GetSelectedText(HWND hwndEdit);
		static int GetMaxLineWidth(HWND hwndEdit);
		static void DisableAutoFont(HWND hwndEdit);
		enum class DetectURLFlag : unsigned int {
			None        = 0x0000U,
			NoLink      = 0x0001U,
			ToHalfWidth = 0x0002U,
			TVTEST_ENUM_FLAGS_TRAILER
		};
		static bool DetectURL(
			HWND hwndEdit, const CHARFORMAT *pcf, int FirstLine = 0, int LastLine = -1,
			DetectURLFlag Flags = DetectURLFlag::ToHalfWidth, CharRangeList *pCharRangeList = nullptr);
		static bool HandleLinkClick(const ENLINK *penl);
		static bool HandleLinkClick(HWND hwndEdit, const POINT &Pos, const CharRangeList &LinkList);
		static int LinkHitTest(HWND hwndEdit, const POINT &Pos, const CharRangeList &LinkList);

	private:
		HMODULE m_hLib = nullptr;

		static const LPCTSTR m_pszURLChars;
		static const LPCTSTR m_pszURLFullWidthChars;

		static bool SearchNextURL(LPCTSTR *ppszText, int *pLength);
		static bool OpenLink(HWND hwndEdit, const CHARRANGE &Range);
	};

	/*
		リッチエディットの標準のハイパーリンク機能では文字色を変更できないため、
		自前でリンクを処理するために使用するクラス
	*/
	class CRichEditLinkHandler
	{
	public:
		void Reset();
		bool DetectURL(
			HWND hwndEdit, const CHARFORMAT *pcf, int FirstLine = 0, int LastLine = -1,
			CRichEditUtil::DetectURLFlag Flags =
				CRichEditUtil::DetectURLFlag::NoLink |
				CRichEditUtil::DetectURLFlag::ToHalfWidth);
		bool OnMsgFilter(MSGFILTER *pMsgFilter);
		bool OnSetCursor(HWND hwnd, WPARAM wParam, LPARAM lParam);
		bool IsCursorOverLink() const noexcept { return m_fCursorOverLink; }
		void LeaveCursor();
		void SetLinkCursor(HCURSOR hCursor) { m_hLinkCursor = hCursor; }

	private:
		CRichEditUtil::CharRangeList m_LinkList;
		POINT m_ClickPos{-1, -1};
		bool m_fCursorOverLink = false;
		HWND m_hwndEdit = nullptr;
		HCURSOR m_hLinkCursor = ::LoadCursor(nullptr, IDC_HAND);
	};

}	// namespace TVTest


#endif
