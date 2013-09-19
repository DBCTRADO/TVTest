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


#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


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
		int MapLength = ::LCMapString(LOCALE_USER_DEFAULT, LCMAP_HALFWIDTH, Text.data(), (int)Text.length(), NULL, 0);
		if (MapLength > 0) {
			pMapText = new TCHAR[MapLength];
			::LCMapString(LOCALE_USER_DEFAULT, LCMAP_HALFWIDTH, Text.data(), (int)Text.length(), pMapText, MapLength);
			Text.assign(pMapText, MapLength);
			delete [] pMapText;
		}
#else
		// 変換前後で長さが変わると面倒なので、ASCIIの範囲のみにしておく
		for (size_t i = 0; i < Text.length(); i++)
			CharToHalfWidth(Text[i]);
#endif
	}




	CRegExpEngine::CRegExpEngine()
		: m_Flags(0)
	{
	}


	CRegExpEngine::~CRegExpEngine()
	{
	}


	void CRegExpEngine::ClearPattern()
	{
		m_Pattern.clear();
		m_Flags = 0;
	}


	bool CRegExpEngine::NeedMap() const
	{
		return (m_Flags & CRegExp::FLAG_IGNORE_WIDTH) != 0;
	}


	void CRegExpEngine::MapPatternString()
	{
		if ((m_Flags & CRegExp::FLAG_IGNORE_WIDTH) != 0) {
			for (size_t i = 0; i < m_Pattern.length(); i++) {
				if (m_Pattern[i] == L'\\') {
					i++;
				} else if (CharToHalfWidth(m_Pattern[i])) {
					// 記号をエスケープ
					WCHAR c = m_Pattern[i];
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

		if ((m_Flags & CRegExp::FLAG_IGNORE_WIDTH) != 0)
			StringToHalfWidth(Text);
	}




#ifdef TVTEST_STD_REGEX_SUPPORT

	/*
		std::regex を使った正規表現検索
	*/

	class CRegExpEngine_ECMAScript : public CRegExpEngine
	{
	public:
		CRegExpEngine_ECMAScript();
		~CRegExpEngine_ECMAScript();
		bool GetName(LPTSTR pszName, size_t MaxLength) const override;
		bool Initialize() override;
		void Finalize() override;
		bool IsInitialized() const override;
		bool SetPattern(LPCTSTR pszPattern, UINT Flags) override;
		bool Match(LPCTSTR pText, size_t Length, CRegExp::TextRange *pRange) override;

	private:
		typedef std::basic_regex<TCHAR> RegEx;
		RegEx m_RegEx;
		bool m_fInitialized;
	};


	CRegExpEngine_ECMAScript::CRegExpEngine_ECMAScript()
		: m_fInitialized(false)
	{
	}


	CRegExpEngine_ECMAScript::~CRegExpEngine_ECMAScript()
	{
		Finalize();
	}


	bool CRegExpEngine_ECMAScript::GetName(LPTSTR pszName, size_t MaxLength) const
	{
		if (pszName == NULL)
			return false;

		StdUtil::strncpy(pszName, MaxLength, TEXT("ECMAScript"));

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


	bool CRegExpEngine_ECMAScript::SetPattern(LPCTSTR pszPattern, UINT Flags)
	{
		if (IsStringEmpty(pszPattern))
			return false;

		m_Pattern = pszPattern;
		m_Flags = Flags;
		MapPatternString();

		RegEx::flag_type RegExFlags = std::regex_constants::ECMAScript;
		if (Flags & CRegExp::FLAG_IGNORE_CASE)
			RegExFlags |= std::regex_constants::icase;

		try {
			m_RegEx.assign(m_Pattern, RegExFlags);
#ifdef _DEBUG
		} catch (std::regex_error &e) {
			TRACE(TEXT("std::regex::assign() error %d\n"), e.code());
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
		if (pText == NULL)
			return false;

		String Text(pText, Length);

		MapTargetString(Text);

		std::match_results<String::const_iterator> Results;
		if (!std::regex_search(Text, Results, m_RegEx))
			return false;

		if (pRange != NULL) {
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

	class CRegExpEngine_VBScript : public CRegExpEngine
	{
	public:
		CRegExpEngine_VBScript();
		~CRegExpEngine_VBScript();
		bool GetName(LPTSTR pszName, size_t MaxLength) const override;
		bool Initialize() override;
		void Finalize() override;
		bool IsInitialized() const override;
		bool SetPattern(LPCTSTR pszPattern, UINT Flags) override;
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
		if (pszName == NULL)
			return false;

		StdUtil::strncpy(pszName, MaxLength, TEXT("VBScript"));

		return true;
	}


	bool CRegExpEngine_VBScript::Initialize()
	{
		if (m_pRegExp == NULL) {
			HRESULT hr = m_pRegExp.CreateInstance(CLSID_RegExp);
			if (FAILED(hr))
				return false;
		}

		return true;
	}


	void CRegExpEngine_VBScript::Finalize()
	{
		if (m_pRegExp != NULL)
			m_pRegExp.Release();
	}


	bool CRegExpEngine_VBScript::IsInitialized() const
	{
		return m_pRegExp != NULL;
	}


	bool CRegExpEngine_VBScript::SetPattern(LPCTSTR pszPattern, UINT Flags)
	{
		if (m_pRegExp == NULL)
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
			m_pRegExp->put_IgnoreCase((Flags & CRegExp::FLAG_IGNORE_CASE) ? VARIANT_TRUE : VARIANT_FALSE);
		} catch (...) {
			return false;
		}

		return true;
	}


	bool CRegExpEngine_VBScript::Match(LPCTSTR pText, size_t Length, CRegExp::TextRange *pRange)
	{
		if (m_pRegExp == NULL || m_Pattern.empty())
			return false;
		if (pText == NULL)
			return false;

		String Text(pText, Length);

		MapTargetString(Text);

		try {
			_bstr_t Target(Text.c_str());
			IMatchCollectionPtr pMatchCollection(m_pRegExp->Execute(Target));

			if (pMatchCollection->Count < 1)
				return false;

			if (pRange != NULL) {
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

	class CRegExpEngine_Bregonig : public CRegExpEngine
	{
	public:
		CRegExpEngine_Bregonig();
		~CRegExpEngine_Bregonig();
		bool GetName(LPTSTR pszName, size_t MaxLength) const override;
		bool Initialize() override;
		void Finalize() override;
		bool IsInitialized() const override;
		bool SetPattern(LPCTSTR pszPattern, UINT Flags) override;
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

		HMODULE m_hLib;
		BREGEXP *m_pBRegExp;
		BoMatchFunc m_pBoMatch;
		BRegFreeFunc m_pBRegFree;
	};


	CRegExpEngine_Bregonig::CRegExpEngine_Bregonig()
		: m_hLib(NULL)
		, m_pBRegExp(NULL)
		, m_pBoMatch(NULL)
		, m_pBRegFree(NULL)
	{
	}


	CRegExpEngine_Bregonig::~CRegExpEngine_Bregonig()
	{
		Finalize();
	}


	bool CRegExpEngine_Bregonig::GetName(LPTSTR pszName, size_t MaxLength) const
	{
		if (pszName == NULL)
			return false;

		StdUtil::strncpy(pszName, MaxLength, TEXT("bregonig.dll"));

		return true;
	}


	bool CRegExpEngine_Bregonig::Initialize()
	{
		if (m_hLib == NULL) {
			TCHAR szPath[MAX_PATH];

			GetLibraryPath(szPath);
			m_hLib = ::LoadLibrary(szPath);
			if (m_hLib == NULL)
				return false;

			m_pBoMatch = reinterpret_cast<BoMatchFunc>(::GetProcAddress(m_hLib, "BoMatchW"));
			m_pBRegFree = reinterpret_cast<BRegFreeFunc>(::GetProcAddress(m_hLib, "BRegfreeW"));

			if (m_pBoMatch == NULL || m_pBRegFree == NULL) {
				Finalize();
				return false;
			}
		}

		return true;
	}


	void CRegExpEngine_Bregonig::Finalize()
	{
		m_Pattern.clear();
		m_Flags = 0;

		if (m_hLib != NULL) {
			FreeBRegExp();

			::FreeLibrary(m_hLib);
			m_hLib = NULL;
			m_pBoMatch = NULL;
			m_pBRegFree = NULL;
		}
	}


	bool CRegExpEngine_Bregonig::IsInitialized() const
	{
		return m_hLib != NULL;
	}


	bool CRegExpEngine_Bregonig::SetPattern(LPCTSTR pszPattern, UINT Flags)
	{
		if (m_hLib == NULL)
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
		if (m_hLib == NULL || m_Pattern.empty())
			return false;
		if (pText == NULL)
			return false;

		LPCTSTR pSrcText = pText;
		size_t SrcLength = Length;

		String MapText;

		if (NeedMap()) {
			MapText = pText;
			MapTargetString(MapText);
			pSrcText = MapText.c_str();
		}

		TCHAR szMessage[BREGEXP_MAX_ERROR_MESSAGE_LEN];

		int Result = m_pBoMatch(m_Pattern.c_str(),
								(m_Flags & CRegExp::FLAG_IGNORE_CASE) ? TEXT("i") : NULL,
								pSrcText, pSrcText, pSrcText + SrcLength,
								FALSE, &m_pBRegExp, szMessage);
		if (Result <= 0)
			return false;

		if (pRange != NULL) {
			pRange->Start = m_pBRegExp->startp[0] - pSrcText;
			pRange->Length = m_pBRegExp->endp[0] - m_pBRegExp->startp[0];
		}

		return true;
	}


	void CRegExpEngine_Bregonig::FreeBRegExp()
	{
		if (m_pBRegExp != NULL) {
			m_pBRegFree(m_pBRegExp);
			m_pBRegExp = NULL;
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
		::GetModuleFileName(NULL, pszPath, MAX_PATH);
		::lstrcpy(::PathFindFileName(pszPath), TEXT("bregonig.dll"));
	}

#endif	// TVTEST_BREGONIG_SUPPORT




	CRegExp::CRegExp()
		: m_pEngine(NULL)
	{
	}


	CRegExp::~CRegExp()
	{
		Finalize();
	}


	bool CRegExp::Initialize()
	{
		if (m_pEngine != NULL)
			return true;

#ifdef TVTEST_BREGONIG_SUPPORT
		if (CRegExpEngine_Bregonig::IsAvailable()) {
			m_pEngine = new CRegExpEngine_Bregonig;
			if (m_pEngine->Initialize())
				return true;
			Finalize();
		}
#endif

#ifdef TVTEST_VBSCRIPT_REGEXP_SUPPORT
		m_pEngine = new CRegExpEngine_VBScript;
		if (m_pEngine->Initialize())
			return true;
		Finalize();
#endif

#ifdef TVTEST_STD_REGEX_SUPPORT
		m_pEngine = new CRegExpEngine_ECMAScript;
		if (m_pEngine->Initialize())
			return true;
		Finalize();
#endif

		return false;
	}


	void CRegExp::Finalize()
	{
		if (m_pEngine != NULL) {
			delete m_pEngine;
			m_pEngine = NULL;
		}
	}


	bool CRegExp::IsInitialized() const
	{
		return m_pEngine != NULL;
	}


	bool CRegExp::SetPattern(LPCTSTR pszPattern, UINT Flags)
	{
		if (m_pEngine == NULL)
			return false;

		return m_pEngine->SetPattern(pszPattern, Flags);
	}


	void CRegExp::ClearPattern()
	{
		if (m_pEngine != NULL)
			m_pEngine->ClearPattern();
	}


	bool CRegExp::Match(LPCTSTR pText, size_t Length, TextRange *pRange)
	{
		if (m_pEngine == NULL)
			return false;

		return m_pEngine->Match(pText, Length, pRange);
	}


	bool CRegExp::Match(LPCTSTR pszText, TextRange *pRange)
	{
		if (pszText == NULL)
			return false;

		return Match(pszText, ::lstrlen(pszText), pRange);
	}


	bool CRegExp::Match(const String &Text, TextRange *pRange)
	{
		return Match(Text.data(), Text.length(), pRange);
	}


	bool CRegExp::GetEngineName(LPTSTR pszName, size_t MaxLength) const
	{
		if (m_pEngine == NULL)
			return false;

		return m_pEngine->GetName(pszName, MaxLength);
	}

}
