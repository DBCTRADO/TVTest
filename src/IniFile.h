#ifndef TVTEST_INI_FILE_H
#define TVTEST_INI_FILE_H


#include <list>


namespace TVTest
{

	class CIniFile
	{
	public:
		static const UINT OPEN_READ           = 0x0001U;
		static const UINT OPEN_WRITE          = 0x0002U;
		static const UINT OPEN_WRITE_VOLATILE = 0x0004U;

		static const UINT MAX_FILE_SIZE = 0x100000;

		class CEntry
		{
		public:
			String Name;
			String Value;

			CEntry() {}
			CEntry(const String &Text);
			CEntry(LPCWSTR pszName,LPCWSTR pszValue);
		};

		CIniFile();
		~CIniFile();
		bool Open(LPCWSTR pszFileName,UINT Flags);
		bool Close();
		bool SelectSection(LPCWSTR pszSection);
		bool IsSectionExists(LPCWSTR pszSection);
		bool DeleteSection(LPCWSTR pszSection);
		bool ClearSection(LPCWSTR pszSection);
		bool ClearSection();
		bool GetValue(LPCWSTR pszName,String *pValue);
		bool SetValue(LPCWSTR pszName,LPCWSTR pszValue);
		bool IsValueExists(LPCWSTR pszName);
		bool DeleteValue(LPCWSTR pszName);
		bool GetSectionEntries(LPCWSTR pszSection,std::vector<CEntry> *pEntries);
		bool GetEntries(std::vector<CEntry> *pEntries);

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
		EntryList::iterator FindValue(EntryList &Entries,LPCWSTR pszName);
		EntryList::const_iterator FindValue(const EntryList &Entries,LPCWSTR pszName) const;
		bool CreateSection(LPCWSTR pszName);
		CSectionData *GetCurSection();

		String m_FileName;
		String m_Section;
		UINT m_OpenFlags;
		SectionList m_SectionList;
		CSectionData *m_pCurSection;
		CGlobalLock m_FileLock;
		HANDLE m_hFile;
	};

}


#endif
