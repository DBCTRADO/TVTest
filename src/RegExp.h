#ifndef TVTEST_REGEXP_H
#define TVTEST_REGEXP_H


// std::regex ‘Î‰ž
#define TVTEST_STD_REGEX_SUPPORT
// VBScript RegExp ‘Î‰ž
//#define TVTEST_VBSCRIPT_REGEXP_SUPPORT
// bregonig.dll ‘Î‰ž
#define TVTEST_BREGONIG_SUPPORT


namespace TVTest
{

	class CRegExp
	{
	public:
		enum {
			FLAG_IGNORE_CASE  = 0x0001U,
			FLAG_IGNORE_WIDTH = 0x0002U
		};

		struct TextRange {
			size_t Start;
			size_t Length;
		};

		CRegExp();
		~CRegExp();
		bool Initialize();
		void Finalize();
		bool IsInitialized() const;
		bool SetPattern(LPCTSTR pszPattern, UINT Flags = 0);
		void ClearPattern();
		bool Match(LPCTSTR pText, size_t Length, TextRange *pRange = NULL);
		bool Match(LPCTSTR pszText, TextRange *pRange = NULL);
		bool Match(const String &Text, TextRange *pRange = NULL);
		bool GetEngineName(LPTSTR pszName, size_t MaxLength) const;

	private:
		class CRegExpEngine *m_pEngine;
	};

	class ABSTRACT_CLASS(CRegExpEngine)
	{
	public:
		CRegExpEngine();
		virtual ~CRegExpEngine();
		virtual bool GetName(LPTSTR pszName, size_t MaxLength) const = 0;
		virtual bool Initialize() = 0;
		virtual void Finalize() = 0;
		virtual bool IsInitialized() const = 0;
		virtual bool SetPattern(LPCTSTR pszPattern, UINT Flags) = 0;
		virtual void ClearPattern();
		virtual bool Match(LPCTSTR pText, size_t Length, CRegExp::TextRange *pRange) = 0;

	protected:
		bool NeedMap() const;
		void MapPatternString();
		void MapTargetString(String &Text) const;

		String m_Pattern;
		UINT m_Flags;
	};

}


#endif
