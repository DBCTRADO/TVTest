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


#include "stdafx.h"
#include <utility>
#include <mlang.h>
#include "TVTest.h"
#include "KeywordSearch.h"
#include "Settings.h"
#include "Common/DebugDef.h"


namespace TVTest
{


bool CKeywordSearch::Load(LPCTSTR pszFileName)
{
	TRACE(TEXT("CKeywordSearch::Load() : \"{}\"\n"), pszFileName);

	if (IsStringEmpty(pszFileName))
		return false;

	CSettings Settings;

	if (!Settings.Open(pszFileName, CSettings::OpenFlag::Read))
		return false;

	if (!Settings.SetSection(TEXT("SearchEngineList")))
		return false;

	m_SearchEngineList.clear();

	CSettings::EntryList Entries;
	if (Settings.GetEntries(&Entries)) {
		m_SearchEngineList.reserve(Entries.size() / 2);

		for (const auto &Entry : Entries) {
			const String::size_type Pos = Entry.Name.find(_T('.'));

			if (Pos != String::npos && Pos > 0
					&& ::lstrcmpi(Entry.Name.c_str() + Pos + 1, TEXT("Name")) == 0) {
				SearchEngineInfo Info;
				String Key(Entry.Name.substr(0, Pos + 1));

				Key += TEXT("URL");
				if (Settings.Read(Key.c_str(), &Info.URL)) {
					Info.Name = Entry.Value;
					m_SearchEngineList.push_back(std::move(Info));
				}
			}
		}
	}

	return true;
}


int CKeywordSearch::GetSearchEngineCount() const
{
	return static_cast<int>(m_SearchEngineList.size());
}


const CKeywordSearch::SearchEngineInfo *CKeywordSearch::GetSearchEngineInfo(int Index) const
{
	if (Index < 0 || static_cast<size_t>(Index) >= m_SearchEngineList.size())
		return nullptr;

	return &m_SearchEngineList[Index];
}


bool CKeywordSearch::Search(int Index, LPCTSTR pszKeyword) const
{
	if (pszKeyword == nullptr)
		return false;

	const SearchEngineInfo *pEngine = GetSearchEngineInfo(Index);
	if (pEngine == nullptr)
		return false;

	String Buffer;
	String::size_type Pos = 0;

	while (Pos < pEngine->URL.length()) {
		const String::size_type Begin = pEngine->URL.find(_T('{'), Pos);
		if (Begin == String::npos)
			break;
		const String::size_type End = pEngine->URL.find(_T('}'), Begin + 1);
		if (End == String::npos)
			break;

		if (Begin > Pos)
			Buffer += pEngine->URL.substr(Pos, Begin - Pos);

		const String Param = pEngine->URL.substr(Begin + 1, End - Begin - 1);
		static const struct {
			LPCTSTR pszParam;
			UINT CodePage;
		} ParameterList[] = {
			{TEXT("keyword:utf-8"),       CP_UTF8},
			{TEXT("keyword:shift_jis"),   932},
			{TEXT("keyword:euc-jp"),      51932},
			{TEXT("keyword:iso-2022-jp"), 50220},
		};
		bool fFound = false;
		for (const auto &e : ParameterList) {
			if (StringUtility::CompareNoCase(Param, e.pszParam) == 0) {
				EncodeURL(e.CodePage, pszKeyword, &Buffer);
				fFound = true;
				break;
			}
		}
		if (!fFound) {
			TRACE(TEXT("Unknown parameter : {{{}}}\n"), Param);
			return false;
		}

		Pos = End + 1;
	}

	if (Pos < pEngine->URL.length())
		Buffer += pEngine->URL.substr(Pos);

	if (Buffer.empty())
		return false;

	TRACE(TEXT("Search by {} : \"{}\"\n"), pEngine->Name, Buffer);

	::ShellExecute(nullptr, TEXT("open"), Buffer.c_str(), nullptr, nullptr, SW_SHOWNORMAL);

	return true;
}


int CKeywordSearch::InitializeMenu(HMENU hmenu, int Command, int MaxItems) const
{
	if (hmenu == nullptr)
		return 0;

	int i;
	for (i = 0; i < static_cast<int>(m_SearchEngineList.size()) && (MaxItems < 0 || i < MaxItems); i++) {
		TCHAR szText[256], szMenu[256];

		StringFormat(
			szText, TEXT("{} で検索"),
			m_SearchEngineList[i].Name);
		CopyToMenuText(szText, szMenu, lengthof(szMenu));
		::AppendMenu(hmenu, MF_STRING | MF_ENABLED, Command + i, szMenu);
	}

	return i;
}


bool CKeywordSearch::EncodeURL(UINT CodePage, LPCWSTR pszSrc, String *pDst) const
{
	int SrcLength = ::lstrlenW(pszSrc);
	if (SrcLength < 1)
		return true;

	String SrcString;

	if (!StringUtility::ToHalfWidthNoKatakana(pszSrc, SrcLength, &SrcString))
		SrcString.assign(pszSrc, SrcLength);

	StringUtility::Trim(SrcString, L" \t\r\n");
	if (SrcString.empty())
		return true;

	int DstLength;
	std::vector<char> Buffer;

	if (CodePage == CP_UTF8 || CodePage == 932) {
		DstLength = ::WideCharToMultiByte(
			CodePage, 0,
			SrcString.data(), static_cast<int>(SrcString.length()),
			nullptr, 0, nullptr, nullptr);
		if (DstLength < 1)
			return false;
		Buffer.resize(DstLength);
		::WideCharToMultiByte(
			CodePage, 0,
			SrcString.data(), static_cast<int>(SrcString.length()),
			Buffer.data(), DstLength, nullptr, nullptr);
	} else {
		bool fOK = false;
		const HMODULE hMLang = Util::LoadSystemLibrary(TEXT("mlang.dll"));
		if (hMLang != nullptr) {
			const auto pConvertINetUnicodeToMultiByte =
				GET_LIBRARY_FUNCTION(hMLang, ConvertINetUnicodeToMultiByte);
			if (pConvertINetUnicodeToMultiByte != nullptr) {
				DWORD Mode = 0;
				SrcLength = static_cast<int>(SrcString.length());
				DstLength = 0;
				if (pConvertINetUnicodeToMultiByte(
							&Mode, CodePage,
							SrcString.data(), &SrcLength,
							nullptr, &DstLength) == S_OK
						&& DstLength > 0) {
					Buffer.resize(DstLength);
					if (pConvertINetUnicodeToMultiByte(
								&Mode, CodePage,
								SrcString.data(), &SrcLength,
								Buffer.data(), &DstLength) == S_OK)
						fOK = true;
				}
			}

			::FreeLibrary(hMLang);
		}

		if (!fOK)
			return false;
	}

	for (int i = 0; i < DstLength; i++) {
		if (Buffer[i] == 0x20) {
			pDst->push_back('+');
		} else if ((Buffer[i] >= 0x41 && Buffer[i] <= 0x5A)
				|| (Buffer[i] >= 0x61 && Buffer[i] <= 0x7A)) {
			pDst->push_back(Buffer[i]);
		} else {
			TCHAR szHex[4];
			StringFormat(szHex, TEXT("%{:02X}"), static_cast<BYTE>(Buffer[i]));
			pDst->append(szHex);
		}
	}

	return true;
}


}	// namespace TVTest
