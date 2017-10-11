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

	TVTEST_ENUM_FLAGS(CVariableManager::VariableFlag)

}


#endif
