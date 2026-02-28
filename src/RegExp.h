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
			Optimize    = 0x0004U,
			TVTEST_ENUM_FLAGS_TRAILER
		};

		struct TextRange
		{
			size_t Start;
			size_t Length;
		};

		CRegExp();
		~CRegExp();

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
		std::unique_ptr<CRegExpEngine> m_Engine;
	};

	class ABSTRACT_CLASS(CRegExpEngine)
	{
	public:
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
		CRegExp::PatternFlag m_Flags = CRegExp::PatternFlag::None;
	};

}


#endif
