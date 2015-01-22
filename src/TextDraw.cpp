#include "stdafx.h"
#include "TVTest.h"
#include "TextDraw.h"
#include "Util.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


namespace TVTest
{


// 行頭禁則文字
const LPCWSTR CTextDraw::m_pszStartProhibitChars=
	L")）]］｣」』】〉》”、。,，.．!！?？ー～…・ァィゥェォッャュョヮヵヶぁぃぅぇぉっゃゅょゎゝゞ々";
// 行末禁則文字
const LPCWSTR CTextDraw::m_pszEndProhibitChars=
	L"(（[［｢「『【〈《“#＃";


CTextDraw::CTextDraw()
	: m_hdc(nullptr)
	, m_Flags(0)
{
}


CTextDraw::~CTextDraw()
{
}


bool CTextDraw::Begin(HDC hdc,unsigned int Flags)
{
	m_hdc=hdc;
	m_Flags=Flags;
	return hdc!=nullptr;
}


void CTextDraw::End()
{
	m_hdc=nullptr;
}


int CTextDraw::CalcLineCount(LPCWSTR pszText,int Width)
{
	if (m_hdc==nullptr || pszText==nullptr || Width<=0)
		return 0;

	int Lines=0;
	LPCWSTR p=pszText;

	while (*p!=L'\0') {
		if (*p==L'\r' || *p==L'\n') {
			p++;
			if (*p==L'\n')
				p++;
			if (*p==L'\0')
				break;
			Lines++;
			continue;
		}

		int Length=GetLineLength(p);
		if (Length==0)
			break;

		int Fit=GetFitCharCount(p,Length,Width);
		Fit=AdjustLineLength(p,Fit);
		Length-=Fit;
		p+=Fit;
		Lines++;

		if (*p==L'\r')
			p++;
		if (*p==L'\n')
			p++;
	}

	return Lines;
}


bool CTextDraw::Draw(LPCWSTR pszText,const RECT &Rect,int LineHeight)
{
	if (m_hdc==nullptr || pszText==nullptr
			|| Rect.left>=Rect.right || Rect.top>=Rect.bottom)
		return false;

	const int Width=Rect.right-Rect.left;

	LPCWSTR p=pszText;
	int y=Rect.top;

	while (*p!=L'\0' && y<Rect.bottom) {
		if (*p==L'\r' || *p==L'\n') {
			p++;
			if (*p==L'\n')
				p++;
			y+=LineHeight;
			continue;
		}

		int Length=GetLineLength(p);
		if (Length==0)
			break;

		int Fit=GetFitCharCount(p,Length,Width);

		if ((m_Flags & FLAG_END_ELLIPSIS)!=0 && Fit<Length && y+LineHeight>=Rect.bottom) {
			static const WCHAR szEllipses[]=L"...";
			const size_t BufferLength=Fit+lengthof(szEllipses);
			m_StringBuffer.clear();
			m_StringBuffer.resize(max(BufferLength,256));
			LPWSTR pszBuffer=&m_StringBuffer[0];
			::CopyMemory(pszBuffer,p,Fit*sizeof(WCHAR));
			LPWSTR pszCur=pszBuffer+Fit;
			for (;;) {
				::lstrcpyW(pszCur,szEllipses);
				Length=(int)(pszCur-pszBuffer)+(lengthof(szEllipses)-1);
				Fit=GetFitCharCount(pszBuffer,Length,Width);
				if (Fit>=Length || pszCur==pszBuffer)
					break;
				pszCur=StringPrevChar(pszBuffer,pszCur);
			}
			::TextOutW(m_hdc,Rect.left,y,pszBuffer,Fit);
			return true;
		}

		Fit=AdjustLineLength(p,Fit);
		::TextOutW(m_hdc,Rect.left,y,p,Fit);
		Length-=Fit;
		p+=Fit;
		y+=LineHeight;

		if (*p==L'\r')
			p++;
		if (*p==L'\n')
			p++;
	}

	return true;
}


int CTextDraw::GetLineLength(LPCWSTR pszText)
{
	LPCWSTR p=pszText;
	while (*p!=L'\0' && *p!=L'\r' && *p!='\n')
		p++;
	return (int)(p-pszText);
}


int CTextDraw::AdjustLineLength(LPCWSTR pszText,int Length)
{
	if (Length<1) {
		Length=StringCharLength(pszText);
		if (Length<1)
			Length=1;
	} else if ((m_Flags & FLAG_JAPANESE_HYPHNATION)!=0 && Length>1) {
		LPCWSTR p=pszText+Length-1;
		if (IsEndProhibitChar(*p)) {
			p--;
			while (p>=pszText) {
				if (!IsEndProhibitChar(*p)) {
					Length=(int)(p-pszText)+1;
					break;
				}
				p--;
			}
		} else if (IsStartProhibitChar(*(p+1))) {
			while (p>pszText) {
				if (!IsStartProhibitChar(*p)) {
					if (IsEndProhibitChar(*(p-1))) {
						p--;
						while (p>=pszText) {
							if (!IsEndProhibitChar(*p)) {
								Length=(int)(p-pszText)+1;
								break;
							}
							p--;
						}
					} else {
						Length=(int)(p-pszText);
					}
					break;
				}
				p--;
			}
		}
	}

	return Length;
}


int CTextDraw::GetFitCharCount(LPCWSTR pText,int Length,int Width)
{
	GCP_RESULTSW Results={sizeof(GCP_RESULTSW)};

	Results.nGlyphs=Length;

	if (::GetCharacterPlacementW(m_hdc,pText,Length,Width,&Results,GCP_MAXEXTENT)==0)
		return 0;

	return Results.nMaxFit;
}


bool CTextDraw::IsStartProhibitChar(WCHAR Char)
{
	return ::StrChrW(m_pszStartProhibitChars,Char)!=nullptr;
}


bool CTextDraw::IsEndProhibitChar(WCHAR Char)
{
	return ::StrChrW(m_pszEndProhibitChars,Char)!=nullptr;
}


}	// namespace TVTest
