/*
  TVTest
  Copyright(c) 2008-2019 DBCTRADO

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


#include "stdafx.h"
#include "TVTest.h"
#include "VariableManager.h"
#include "StringUtility.h"
#include "Common/DebugDef.h"


namespace TVTest
{


bool CVariableManager::RegisterVariable(
	LPCWSTR pszKeyword, LPCWSTR pszValue, IGetVariable *pGetVariable,
	LPCWSTR pszDescription, VariableFlag Flags)
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
	if (it == m_VariableList.end() || !(it->Flags & VariableFlag::Override))
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

	for (const auto &e : m_VariableList) {
		VariableInfo Info;

		Info.pszKeyword = e.Keyword.c_str();
		Info.pszDescription = e.Description.c_str();
		Info.Flags = e.Flags;

		pList->push_back(Info);
	}

	return true;
}


}
