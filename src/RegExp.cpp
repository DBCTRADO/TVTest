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
#include "RegExp.h"

#ifdef TVTEST_STD_REGEX_SUPPORT
#include <regex>
#endif

#ifdef TVTEST_VBSCRIPT_REGEXP_SUPPORT
#import "RegExp.tlb" no_namespace named_guids
#endif

#ifdef TVTEST_BREGONIG_SUPPORT
#include "bregexp.h"
#endif

#include "Common/DebugDef.h"


namespace TVTest
{


inline bool CharToHalfWidth(WCHAR &Char)
{
	if (Char >= L'！' && Char <= L'～') {
		Char -= L'！' - L'!';
	} else if (Char == L'　') {
		Char = L' ';
	} else {
		return false;
	}
	return true;
}


void StringToHalfWidth(String &Text)
{
#if 0
	const int MapLength = ::LCMapString(LOCALE_USER_DEFAULT, LCMAP_HALFWIDTH, Text.data(), static_cast<int>(Text.length()), nullptr, 0);
	if (MapLength > 0) {
		String MapText(MapLength, L'\0');
		::LCMapString(LOCALE_USER_DEFAULT, LCMAP_HALFWIDTH, Text.data(), static_cast<int>(Text.length()), MapText.data(), MapLength);
		Text = std::move(MapText);
	}
#else
	// 変換前後で長さが変わると面倒なので、ASCIIの範囲のみにしておく
	for (size_t i = 0; i < Text.length(); i++)
		CharToHalfWidth(Text[i]);
#endif
}




void CRegExpEngine::ClearPattern()
{
	m_Pattern.clear();
	m_Flags = CRegExp::PatternFlag::None;
}


bool CRegExpEngine::NeedMap() const
{
	return !!(m_Flags & CRegExp::PatternFlag::IgnoreWidth);
}


void CRegExpEngine::MapPatternString()
{
	if (!!(m_Flags & CRegExp::PatternFlag::IgnoreWidth)) {
		for (size_t i = 0; i < m_Pattern.length(); i++) {
			if (m_Pattern[i] == L'\\') {
				i++;
			} else if (CharToHalfWidth(m_Pattern[i])) {
				// 記号をエスケープ
				const WCHAR c = m_Pattern[i];
				if ((c >= L'!' && c <= L'/')
						|| (c >= L':' && c <= L'?')
						|| (c >= L'[' && c <= L'^')
						|| (c >= L'{' && c <= L'}')) {
					m_Pattern.insert(i, 1, L'\\');
					i++;
				}
			}
		}
	}
}


void CRegExpEngine::MapTargetString(String &Text) const
{
	if (Text.empty())
		return;

	if (!!(m_Flags & CRegExp::PatternFlag::IgnoreWidth))
		StringToHalfWidth(Text);
}




#ifdef TVTEST_STD_REGEX_SUPPORT

/*
	std::regex を使った正規表現検索
*/

class CRegExpEngine_ECMAScript
	: public CRegExpEngine
{
public:
	~CRegExpEngine_ECMAScript();
	bool GetName(LPTSTR pszName, size_t MaxLength) const override;
	bool Initialize() override;
	void Finalize() override;
	bool IsInitialized() const override;
	bool SetPattern(LPCTSTR pszPattern, CRegExp::PatternFlag Flags) override;
	bool Match(LPCTSTR pText, size_t Length, CRegExp::TextRange *pRange) override;

private:
	typedef std::basic_regex<TCHAR> RegEx;
	RegEx m_RegEx;
	bool m_fInitialized = false;
};


CRegExpEngine_ECMAScript::~CRegExpEngine_ECMAScript()
{
	Finalize();
}


bool CRegExpEngine_ECMAScript::GetName(LPTSTR pszName, size_t MaxLength) const
{
	if (pszName == nullptr)
		return false;

	StringCopy(pszName, TEXT("ECMAScript"), MaxLength);

	return true;
}


bool CRegExpEngine_ECMAScript::Initialize()
{
	m_fInitialized = true;

	return true;
}


void CRegExpEngine_ECMAScript::Finalize()
{
	m_fInitialized = false;
}


bool CRegExpEngine_ECMAScript::IsInitialized() const
{
	return m_fInitialized;
}


bool CRegExpEngine_ECMAScript::SetPattern(LPCTSTR pszPattern, CRegExp::PatternFlag Flags)
{
	if (IsStringEmpty(pszPattern))
		return false;

	m_Pattern = pszPattern;
	m_Flags = Flags;
	MapPatternString();

	RegEx::flag_type RegExFlags = std::regex_constants::ECMAScript;
	if (!!(Flags & CRegExp::PatternFlag::IgnoreCase))
		RegExFlags |= std::regex_constants::icase;
	if (!!(Flags & CRegExp::PatternFlag::Optimize))
		RegExFlags |= std::regex_constants::optimize;

	try {
		m_RegEx.assign(m_Pattern, RegExFlags);
#ifdef _DEBUG
	} catch (const std::regex_error &e) {
		TRACE(
			TEXT("std::regex::assign() error {}\n"),
			static_cast<std::underlying_type_t<std::regex_constants::error_type>>(e.code()));
#else
	} catch (...) {
#endif
		return false;
	}

	return true;
}


bool CRegExpEngine_ECMAScript::Match(LPCTSTR pText, size_t Length, CRegExp::TextRange *pRange)
{
	if (m_Pattern.empty())
		return false;
	if (pText == nullptr)
		return false;

	String Text(pText, Length);

	MapTargetString(Text);

	std::match_results<String::const_iterator> Results;
	if (!std::regex_search(Text, Results, m_RegEx))
		return false;

	if (pRange != nullptr) {
		pRange->Start = Results.position();
		pRange->Length = Results.length();
	}

	return true;
}

#endif	// TVTEST_STD_REGEX_SUPPORT




#ifdef TVTEST_VBSCRIPT_REGEXP_SUPPORT

/*
	VBScript の RegExp オブジェクトを使った正規表現検索
*/

class CRegExpEngine_VBScript
	: public CRegExpEngine
{
public:
	CRegExpEngine_VBScript();
	~CRegExpEngine_VBScript();
	bool GetName(LPTSTR pszName, size_t MaxLength) const override;
	bool Initialize() override;
	void Finalize() override;
	bool IsInitialized() const override;
	bool SetPattern(LPCTSTR pszPattern, CRegExp::PatternFlag Flags) override;
	bool Match(LPCTSTR pText, size_t Length, CRegExp::TextRange *pRange) override;

private:
	IRegExpPtr m_pRegExp;
};


CRegExpEngine_VBScript::CRegExpEngine_VBScript()
{
}


CRegExpEngine_VBScript::~CRegExpEngine_VBScript()
{
	Finalize();
}


bool CRegExpEngine_VBScript::GetName(LPTSTR pszName, size_t MaxLength) const
{
	if (pszName == nullptr)
		return false;

	StringCopy(pszName, TEXT("VBScript"), MaxLength);

	return true;
}


bool CRegExpEngine_VBScript::Initialize()
{
	if (m_pRegExp == nullptr) {
		const HRESULT hr = m_pRegExp.CreateInstance(CLSID_RegExp);
		if (FAILED(hr))
			return false;
	}

	return true;
}


void CRegExpEngine_VBScript::Finalize()
{
	if (m_pRegExp != nullptr)
		m_pRegExp.Release();
}


bool CRegExpEngine_VBScript::IsInitialized() const
{
	return m_pRegExp != nullptr;
}


bool CRegExpEngine_VBScript::SetPattern(LPCTSTR pszPattern, CRegExp::PatternFlag Flags)
{
	if (m_pRegExp == nullptr)
		return false;
	if (IsStringEmpty(pszPattern))
		return false;

	m_Pattern = pszPattern;
	m_Flags = Flags;
	MapPatternString();

	try {
		_bstr_t Pattern(m_Pattern.c_str());
		m_pRegExp->put_Pattern(Pattern);
		m_pRegExp->put_Global(VARIANT_TRUE);
		m_pRegExp->put_IgnoreCase(!!(Flags & CRegExp::PatternFlag::IgnoreCase) ? VARIANT_TRUE : VARIANT_FALSE);
	} catch (...) {
		return false;
	}

	return true;
}


bool CRegExpEngine_VBScript::Match(LPCTSTR pText, size_t Length, CRegExp::TextRange *pRange)
{
	if (m_pRegExp == nullptr || m_Pattern.empty())
		return false;
	if (pText == nullptr)
		return false;

	String Text(pText, Length);

	MapTargetString(Text);

	try {
		_bstr_t Target(Text.c_str());
		IMatchCollectionPtr pMatchCollection(m_pRegExp->Execute(Target));

		if (pMatchCollection->Count < 1)
			return false;

		if (pRange != nullptr) {
			IMatchPtr pMatch = pMatchCollection->Item[0];
			pRange->Start = pMatch->FirstIndex;
			pRange->Length = pMatch->Length;
		}
	} catch (...) {
		return false;
	}

	return true;
}

#endif	// TVTEST_VBSCRIPT_REGEXP_SUPPORT




#ifdef TVTEST_BREGONIG_SUPPORT

/*
	bregonig.dll を使った正規表現検索

	http://homepage3.nifty.com/k-takata/mysoft/bregonig.html
*/

class CRegExpEngine_Bregonig
	: public CRegExpEngine
{
public:
	~CRegExpEngine_Bregonig();
	bool GetName(LPTSTR pszName, size_t MaxLength) const override;
	bool Initialize() override;
	void Finalize() override;
	bool IsInitialized() const override;
	bool SetPattern(LPCTSTR pszPattern, CRegExp::PatternFlag Flags) override;
	void ClearPattern() override;
	bool Match(LPCTSTR pText, size_t Length, CRegExp::TextRange *pRange) override;

	static bool IsAvailable();

private:
	void FreeBRegExp();

	static void GetLibraryPath(LPTSTR pszPath);

	typedef int (*BoMatchFunc)(
		const TCHAR *patternp, const TCHAR *optionp,
		const TCHAR *strstartp,
		const TCHAR *targetstartp, const TCHAR *targetendp,
		BOOL one_shot,
		BREGEXP **rxp, TCHAR *msg);;
	typedef void (*BRegFreeFunc)(BREGEXP *rx);

	HMODULE m_hLib = nullptr;
	BREGEXP *m_pBRegExp = nullptr;
	BoMatchFunc m_pBoMatch = nullptr;
	BRegFreeFunc m_pBRegFree = nullptr;
};


CRegExpEngine_Bregonig::~CRegExpEngine_Bregonig()
{
	Finalize();
}


bool CRegExpEngine_Bregonig::GetName(LPTSTR pszName, size_t MaxLength) const
{
	if (pszName == nullptr)
		return false;

	StringCopy(pszName, TEXT("bregonig.dll"), MaxLength);

	return true;
}


bool CRegExpEngine_Bregonig::Initialize()
{
	if (m_hLib == nullptr) {
		TCHAR szPath[MAX_PATH];

		GetLibraryPath(szPath);
		m_hLib = ::LoadLibrary(szPath);
		if (m_hLib == nullptr)
			return false;

		m_pBoMatch = reinterpret_cast<BoMatchFunc>(::GetProcAddress(m_hLib, "BoMatchW"));
		m_pBRegFree = reinterpret_cast<BRegFreeFunc>(::GetProcAddress(m_hLib, "BRegfreeW"));

		if (m_pBoMatch == nullptr || m_pBRegFree == nullptr) {
			Finalize();
			return false;
		}
	}

	return true;
}


void CRegExpEngine_Bregonig::Finalize()
{
	m_Pattern.clear();
	m_Flags = CRegExp::PatternFlag::None;

	if (m_hLib != nullptr) {
		FreeBRegExp();

		::FreeLibrary(m_hLib);
		m_hLib = nullptr;
		m_pBoMatch = nullptr;
		m_pBRegFree = nullptr;
	}
}


bool CRegExpEngine_Bregonig::IsInitialized() const
{
	return m_hLib != nullptr;
}


bool CRegExpEngine_Bregonig::SetPattern(LPCTSTR pszPattern, CRegExp::PatternFlag Flags)
{
	if (m_hLib == nullptr)
		return false;
	if (IsStringEmpty(pszPattern))
		return false;

	FreeBRegExp();

	m_Pattern = pszPattern;
	m_Flags = Flags;

	MapPatternString();

	return true;
}


void CRegExpEngine_Bregonig::ClearPattern()
{
	FreeBRegExp();
	CRegExpEngine::ClearPattern();
}


bool CRegExpEngine_Bregonig::Match(LPCTSTR pText, size_t Length, CRegExp::TextRange *pRange)
{
	if (m_hLib == nullptr || m_Pattern.empty())
		return false;
	if (pText == nullptr)
		return false;

	LPCTSTR pSrcText = pText;
	const size_t SrcLength = Length;

	String MapText;

	if (NeedMap()) {
		MapText = pText;
		MapTargetString(MapText);
		pSrcText = MapText.c_str();
	}

	TCHAR szMessage[BREGEXP_MAX_ERROR_MESSAGE_LEN];

	const int Result = m_pBoMatch(
		m_Pattern.c_str(),
		!!(m_Flags & CRegExp::PatternFlag::IgnoreCase) ? TEXT("i") : nullptr,
		pSrcText, pSrcText, pSrcText + SrcLength,
		FALSE, &m_pBRegExp, szMessage);
	if (Result <= 0)
		return false;

	if (pRange != nullptr) {
		pRange->Start = m_pBRegExp->startp[0] - pSrcText;
		pRange->Length = m_pBRegExp->endp[0] - m_pBRegExp->startp[0];
	}

	return true;
}


void CRegExpEngine_Bregonig::FreeBRegExp()
{
	if (m_pBRegExp != nullptr) {
		m_pBRegFree(m_pBRegExp);
		m_pBRegExp = nullptr;
	}
}


bool CRegExpEngine_Bregonig::IsAvailable()
{
	TCHAR szPath[MAX_PATH];

	GetLibraryPath(szPath);
	return ::PathFileExists(szPath) != FALSE;
}


void CRegExpEngine_Bregonig::GetLibraryPath(LPTSTR pszPath)
{
	::GetModuleFileName(nullptr, pszPath, MAX_PATH);
	StringCopy(::PathFindFileName(pszPath), TEXT("bregonig.dll"));
}

#endif	// TVTEST_BREGONIG_SUPPORT




CRegExp::CRegExp() = default;
CRegExp::~CRegExp() = default;


bool CRegExp::Initialize()
{
	if (m_Engine)
		return true;

#ifdef TVTEST_BREGONIG_SUPPORT
	if (CRegExpEngine_Bregonig::IsAvailable()) {
		m_Engine = std::make_unique<CRegExpEngine_Bregonig>();
		if (m_Engine->Initialize())
			return true;
		Finalize();
	}
#endif

#ifdef TVTEST_VBSCRIPT_REGEXP_SUPPORT
	m_Engine = std::make_unique<CRegExpEngine_VBScript>();
	if (m_Engine->Initialize())
		return true;
	Finalize();
#endif

#ifdef TVTEST_STD_REGEX_SUPPORT
	m_Engine = std::make_unique<CRegExpEngine_ECMAScript>();
	if (m_Engine->Initialize())
		return true;
	Finalize();
#endif

	return false;
}


void CRegExp::Finalize()
{
	m_Engine.reset();
}


bool CRegExp::IsInitialized() const
{
	return static_cast<bool>(m_Engine);
}


bool CRegExp::SetPattern(LPCTSTR pszPattern, CRegExp::PatternFlag Flags)
{
	if (!m_Engine)
		return false;

	return m_Engine->SetPattern(pszPattern, Flags);
}


void CRegExp::ClearPattern()
{
	if (m_Engine)
		m_Engine->ClearPattern();
}


bool CRegExp::Match(LPCTSTR pText, size_t Length, TextRange *pRange)
{
	if (!m_Engine)
		return false;

	return m_Engine->Match(pText, Length, pRange);
}


bool CRegExp::Match(LPCTSTR pszText, TextRange *pRange)
{
	if (pszText == nullptr)
		return false;

	return Match(pszText, ::lstrlen(pszText), pRange);
}


bool CRegExp::Match(const String &Text, TextRange *pRange)
{
	return Match(Text.data(), Text.length(), pRange);
}


bool CRegExp::GetEngineName(LPTSTR pszName, size_t MaxLength) const
{
	if (!m_Engine)
		return false;

	return m_Engine->GetName(pszName, MaxLength);
}


}	// namespace TVTest
