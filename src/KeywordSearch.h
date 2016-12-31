#ifndef TVTEST_KEYWORD_SEARCH_H
#define TVTEST_KEYWORD_SEARCH_H


#include <vector>


namespace TVTest
{

	class CKeywordSearch
	{
	public:
		struct SearchEngineInfo
		{
			String Name;
			String URL;
		};

		bool Load(LPCTSTR pszFileName);
		int GetSearchEngineCount() const;
		const SearchEngineInfo *GetSearchEngineInfo(int Index) const;
		bool Search(int Index,LPCTSTR pszKeyword) const;
		int InitializeMenu(HMENU hmenu,int Command,int MaxItems=-1) const;

	private:
		bool EncodeURL(UINT CodePage,LPCWSTR pszSrc,String *pDst) const;

		std::vector<SearchEngineInfo> m_SearchEngineList;
	};

}	// namespace TVTest


#endif
