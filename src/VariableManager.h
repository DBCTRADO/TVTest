#ifndef TVTEST_VARIABLE_MANAGER_H
#define TVTEST_VARIABLE_MANAGER_H


#include <set>
#include <vector>


namespace TVTest
{

	class CVariableManager
	{
	public:
		enum {
			FLAG_OVERRIDE = 0x0001
		};

		struct VariableInfo
		{
			LPCWSTR pszKeyword;
			LPCWSTR pszDescription;
			unsigned int Flags;
		};

		class IGetVariable
		{
		public:
			virtual bool GetVariable(LPCWSTR pszKeyword, String *pValue) = 0;
		};

		bool RegisterVariable(LPCWSTR pszKeyword,
							  LPCWSTR pszValue, IGetVariable *pGetVariable,
							  LPCWSTR pszDescription, unsigned int Flags = 0);
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
			unsigned int Flags;

			bool operator<(const VariableEntry &rhs) const
			{
				return Keyword.compare(rhs.Keyword) < 0;
			}
		};

		std::set<VariableEntry> m_VariableList;
	};

}


#endif
