#ifndef TVTEST_REGEXP_H
#define TVTEST_REGEXP_H


// std::regex 対応
#define TVTEST_STD_REGEX_SUPPORT
// VBScript RegExp 対応
//#define TVTEST_VBSCRIPT_REGEXP_SUPPORT
// bregonig.dll 対応
#define TVTEST_BREGONIG_SUPPORT


#include <memory>


namespace TVTest
{

	class CRegExpEngine;

	class CRegExp
	{
	public:
		enum class PatternFlag : unsigned int {
			None        = 0x0000U,
			IgnoreCase  = 0x0001U,
			IgnoreWidth = 0x0002U,
		};

		struct TextRange
		{
			size_t Start;
			size_t Length;
		};

		bool Initialize();
		void Finalize();
		bool IsInitialized() const;
		bool SetPattern(LPCTSTR pszPattern, PatternFlag Flags = PatternFlag::None);
		void ClearPattern();
		bool Match(LPCTSTR pText, size_t Length, TextRange *pRange = nullptr);
		bool Match(LPCTSTR pszText, TextRange *pRange = nullptr);
		bool Match(const String &Text, TextRange *pRange = nullptr);
		bool GetEngineName(LPTSTR pszName, size_t MaxLength) const;

	private:
		struct RegExpEngineDeleter { void operator()(CRegExpEngine *p) const; };
		std::unique_ptr<CRegExpEngine, RegExpEngineDeleter> m_Engine;
	};

	TVTEST_ENUM_FLAGS(CRegExp::PatternFlag)

	class ABSTRACT_CLASS(CRegExpEngine)
	{
	public:
		CRegExpEngine();
		virtual ~CRegExpEngine() = default;
		virtual bool GetName(LPTSTR pszName, size_t MaxLength) const = 0;
		virtual bool Initialize() = 0;
		virtual void Finalize() = 0;
		virtual bool IsInitialized() const = 0;
		virtual bool SetPattern(LPCTSTR pszPattern, CRegExp::PatternFlag Flags) = 0;
		virtual void ClearPattern();
		virtual bool Match(LPCTSTR pText, size_t Length, CRegExp::TextRange *pRange) = 0;

	protected:
		bool NeedMap() const;
		void MapPatternString();
		void MapTargetString(String &Text) const;

		String m_Pattern;
		CRegExp::PatternFlag m_Flags;
	};

}


#endif
