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
		bool Search(int Index, LPCTSTR pszKeyword) const;
		int InitializeMenu(HMENU hmenu, int Command, int MaxItems = -1) const;

	private:
		bool EncodeURL(UINT CodePage, LPCWSTR pszSrc, String *pDst) const;

		std::vector<SearchEngineInfo> m_SearchEngineList;
	};

} // namespace TVTest


#endif
