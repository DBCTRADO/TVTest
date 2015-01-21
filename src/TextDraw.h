#ifndef TVTEST_TEXT_DRAW_H
#define TVTEST_TEXT_DRAW_H


#include <vector>


namespace TVTest
{

	class CTextDraw
	{
	public:
		enum {
			FLAG_END_ELLIPSIS			=0x0001U,	// é˚Ç‹ÇËÇ´ÇÁÇ»Ç¢èÍçáè»ó™ãLçÜÇïtâ¡
			FLAG_JAPANESE_HYPHNATION	=0x0002U	// ã÷ë•èàóù
		};

		CTextDraw();
		~CTextDraw();
		bool Begin(HDC hdc,unsigned int Flags=0);
		void End();
		HDC GetHDC() const { return m_hdc; }
		void SetFlags(unsigned int Flags) { m_Flags=Flags; }
		unsigned int GetFlags() const { return m_Flags; }
		int CalcLineCount(LPCWSTR pszText,int Width);
		bool Draw(LPCWSTR pszText,const RECT &Rect,int LineHeight);

	private:
		HDC m_hdc;
		unsigned int m_Flags;
		std::vector<WCHAR> m_StringBuffer;

		int GetLineLength(LPCWSTR pszText);
		int AdjustLineLength(LPCWSTR pszText,int Length);
		int GetFitCharCount(LPCWSTR pText,int Length,int Width);

		static const LPCWSTR m_pszStartProhibitChars;
		static const LPCWSTR m_pszEndProhibitChars;

		static bool IsStartProhibitChar(WCHAR Char);
		static bool IsEndProhibitChar(WCHAR Char);
	};

}	// namespace TVTest


#endif
