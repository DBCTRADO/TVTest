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


#ifndef TVTEST_INI_FILE_H
#define TVTEST_INI_FILE_H


#include <list>
#include <vector>


namespace TVTest
{

	class CIniFile
	{
	public:
		enum class OpenFlag : unsigned int {
			None          = 0x0000U,
			Read          = 0x0001U,
			Write         = 0x0002U,
			WriteVolatile = 0x0004U,
			TVTEST_ENUM_FLAGS_TRAILER
		};

		static constexpr UINT MAX_FILE_SIZE = 0x100000;

		class CEntry
		{
		public:
			String Name;
			String Value;

			CEntry() = default;
			CEntry(const String &Text);
			CEntry(LPCWSTR pszName, LPCWSTR pszValue);
		};

		typedef std::vector<CEntry> EntryArray;

		CIniFile() = default;
		~CIniFile();

		CIniFile(const CIniFile &) = delete;
		CIniFile &operator=(const CIniFile &) = delete;

		bool Open(LPCWSTR pszFileName, OpenFlag Flags);
		bool Close();
		bool SelectSection(LPCWSTR pszSection);
		bool IsSectionExists(LPCWSTR pszSection);
		bool DeleteSection(LPCWSTR pszSection);
		bool ClearSection(LPCWSTR pszSection);
		bool ClearSection();
		bool GetValue(LPCWSTR pszName, String *pValue);
		bool SetValue(LPCWSTR pszName, LPCWSTR pszValue);
		bool IsValueExists(LPCWSTR pszName);
		bool DeleteValue(LPCWSTR pszName);
		bool GetSectionEntries(LPCWSTR pszSection, EntryArray *pEntries);
		bool GetEntries(EntryArray *pEntries);

	private:
		typedef std::list<CEntry> EntryList;

		class CSectionData
		{
		public:
			String SectionName;
			EntryList Entries;

			CSectionData(LPCWSTR pszName) : SectionName(pszName) {}
		};
		typedef std::list<CSectionData> SectionList;

		bool Parse(LPCWSTR pszBuffer);
		SectionList::iterator FindSection(LPCWSTR pszName);
		SectionList::const_iterator FindSection(LPCWSTR pszName) const;
		EntryList::iterator FindValue(EntryList &Entries, LPCWSTR pszName);
		EntryList::const_iterator FindValue(const EntryList &Entries, LPCWSTR pszName) const;
		bool CreateSection(LPCWSTR pszName);

		String m_FileName;
		String m_Section;
		OpenFlag m_OpenFlags = OpenFlag::None;
		SectionList m_SectionList;
		CSectionData *m_pCurSection = nullptr;
		CGlobalLock m_FileLock;
		HANDLE m_hFile = INVALID_HANDLE_VALUE;
	};

}


#endif
