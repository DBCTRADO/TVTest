#include "stdafx.h"
#include "TVTest.h"
#include "VariableManager.h"
#include "StringUtility.h"
#include "Common/DebugDef.h"


namespace TVTest
{


bool CVariableManager::RegisterVariable(
	LPCWSTR pszKeyword, LPCWSTR pszValue, IGetVariable *pGetVariable,
	LPCWSTR pszDescription, unsigned int Flags)
{
	if (IsStringEmpty(pszKeyword))
		return false;

	VariableEntry Entry;

	Entry.Keyword = pszKeyword;
	StringUtility::ToLower(Entry.Keyword);
	if (pszValue != nullptr)
		Entry.Value = pszValue;
	Entry.pGetVariable = pGetVariable;
	if (pszDescription != nullptr)
		Entry.Description = pszDescription;
	Entry.Flags = Flags;

	auto it = m_VariableList.find(Entry);
	if (it != m_VariableList.end())
		m_VariableList.erase(it);

	m_VariableList.insert(Entry);

	return true;
}


bool CVariableManager::GetVariable(LPCWSTR pszKeyword, String *pValue) const
{
	if (IsStringEmpty(pszKeyword) || pValue == nullptr)
		return false;

	VariableEntry Entry;

	Entry.Keyword = pszKeyword;
	StringUtility::ToLower(Entry.Keyword);

	auto it = m_VariableList.find(Entry);
	if (it == m_VariableList.end())
		return false;

	if (it->pGetVariable != nullptr) {
		if (!it->pGetVariable->GetVariable(pszKeyword, pValue))
			return false;
	} else {
		*pValue = it->Value;
	}

	return true;
}


bool CVariableManager::GetPreferredVariable(LPCWSTR pszKeyword, String *pValue) const
{
	if (IsStringEmpty(pszKeyword) || pValue == nullptr)
		return false;

	VariableEntry Entry;

	Entry.Keyword = pszKeyword;
	StringUtility::ToLower(Entry.Keyword);

	auto it = m_VariableList.find(Entry);
	if (it == m_VariableList.end() || (it->Flags & FLAG_OVERRIDE) == 0)
		return false;

	if (it->pGetVariable != nullptr) {
		if (!it->pGetVariable->GetVariable(pszKeyword, pValue))
			return false;
	} else {
		*pValue = it->Value;
	}

	return true;
}


bool CVariableManager::GetVariableList(std::vector<VariableInfo> *pList) const
{
	if (pList == nullptr)
		return false;

	pList->clear();
	pList->reserve(m_VariableList.size());

	for (auto it = m_VariableList.begin(); it != m_VariableList.end(); ++it) {
		VariableInfo Info;

		Info.pszKeyword = it->Keyword.c_str();
		Info.pszDescription = it->Description.c_str();
		Info.Flags = it->Flags;

		pList->push_back(Info);
	}

	return true;
}


}
