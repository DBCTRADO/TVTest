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


#ifndef TVTEST_VARIABLE_MANAGER_H
#define TVTEST_VARIABLE_MANAGER_H


#include <set>
#include <vector>


namespace TVTest
{

	class CVariableManager
	{
	public:
		enum class VariableFlag : unsigned int {
			None     = 0x0000U,
			Override = 0x0001U,
			TVTEST_ENUM_FLAGS_TRAILER
		};

		struct VariableInfo
		{
			LPCWSTR pszKeyword;
			VariableFlag Flags;
			LPCWSTR pszDescription;
		};

		class IGetVariable
		{
		public:
			virtual bool GetVariable(LPCWSTR pszKeyword, String *pValue) = 0;
		};

		bool RegisterVariable(
			LPCWSTR pszKeyword,
			LPCWSTR pszValue, IGetVariable *pGetVariable,
			LPCWSTR pszDescription, VariableFlag Flags = VariableFlag::None);
		bool GetVariable(LPCWSTR pszKeyword, String *pValue) const;
		bool GetPreferredVariable(LPCWSTR pszKeyword, String *pValue) const;
		bool GetVariableList(std::vector<VariableInfo> *pList) const;

	private:
		struct VariableEntry
		{
			String Keyword;
			String Value;
			IGetVariable *pGetVariable;
			String Description;
			VariableFlag Flags;

			bool operator<(const VariableEntry &rhs) const
			{
				return Keyword.compare(rhs.Keyword) < 0;
			}
		};

		std::set<VariableEntry> m_VariableList;
	};

}


#endif
