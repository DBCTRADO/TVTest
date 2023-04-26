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
#include "TVTest.h"
#include "IniFile.h"
#include "Common/DebugDef.h"


namespace TVTest
{

CIniFile::~CIniFile()
{
	Close();
}

bool CIniFile::Open(LPCWSTR pszFileName, OpenFlag Flags)
{
	Close();

	TRACE(
		TEXT("CIniFile::Open( \"{}\", {:#x})\n"),
		pszFileName,
		static_cast<std::underlying_type_t<OpenFlag>>(Flags));

	if (IsStringEmpty(pszFileName)
			|| !(Flags & (OpenFlag::Read | OpenFlag::Write))
			|| (Flags & (OpenFlag::Write | OpenFlag::WriteVolatile)) == (OpenFlag::Write | OpenFlag::WriteVolatile))
		return false;

	String MutexName;
	MutexName = APP_NAME L"_Ini_Mutex_";
	MutexName += pszFileName;
	StringUtility::Replace(MutexName, L'\\', L':');
	if (m_FileLock.Create(MutexName.c_str())) {
		if (!m_FileLock.Wait(5000)) {
			TRACE(TEXT("Ini file locked\n"));
			m_FileLock.Close();
			//return false;
		}
	}

	DWORD DesiredAccess = 0, ShareMode = 0;
	if (!!(Flags & OpenFlag::Write)) {
		DesiredAccess |= GENERIC_READ | GENERIC_WRITE;
	} else {
		DesiredAccess |= GENERIC_READ;
		ShareMode |= FILE_SHARE_READ;
	}

	const HANDLE hFile = ::CreateFile(
		pszFileName, DesiredAccess, ShareMode, nullptr,
		!!(Flags & OpenFlag::Write) ? OPEN_ALWAYS : OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL, nullptr);
	try {
		if (hFile != INVALID_HANDLE_VALUE) {
			LARGE_INTEGER FileSize;
			if (!::GetFileSizeEx(hFile, &FileSize) || FileSize.QuadPart > MAX_FILE_SIZE) {
				throw __LINE__;
			}

			if (FileSize.LowPart > 2) {
				std::unique_ptr<BYTE[]> Buffer(new BYTE[FileSize.LowPart + 3]);
				DWORD Read;
				if (!::ReadFile(hFile, Buffer.get(), FileSize.LowPart, &Read, nullptr)
						|| Read != FileSize.LowPart) {
					throw __LINE__;
				}
				Buffer[FileSize.LowPart + 0] = 0;
				Buffer[FileSize.LowPart + 1] = 0;
				Buffer[FileSize.LowPart + 2] = 0;

				if (Buffer[0] == 0xFF && Buffer[1] == 0xFE) {
					Parse(reinterpret_cast<WCHAR*>(Buffer.get() + 2));
				} else {
					const int Length = ::MultiByteToWideChar(CP_ACP, 0, reinterpret_cast<char*>(Buffer.get()), FileSize.LowPart, nullptr, 0);
					std::wstring Data(Length, L'\0');
					::MultiByteToWideChar(CP_ACP, 0, reinterpret_cast<char*>(Buffer.get()), FileSize.LowPart, Data.data(), Length);
					Parse(Data.c_str());
				}
			}
		} else {
			TRACE(TEXT("Ini file open failed {:#x}\n"), ::GetLastError());
			throw __LINE__;
		}
	} catch (...) {
		if (hFile != INVALID_HANDLE_VALUE)
			::CloseHandle(hFile);
		m_FileLock.Release();
		m_FileLock.Close();
		return false;
	}

	if (!(Flags & OpenFlag::Write)) {
		::CloseHandle(hFile);
		m_FileLock.Release();
		m_FileLock.Close();
	} else {
		m_hFile = hFile;
	}

	m_FileName = pszFileName;
	m_OpenFlags = Flags;

	return true;
}

bool CIniFile::Close()
{
	if (!m_OpenFlags)
		return true;

	bool fOK = true;

	if (!!(m_OpenFlags & OpenFlag::Write) && m_hFile != INVALID_HANDLE_VALUE) {
		String Buffer;

		for (const auto &Section : m_SectionList) {
			if (!Section.SectionName.empty()) {
				Buffer += L"[";
				Buffer += Section.SectionName;
				Buffer += L"]\r\n";
			}

			for (const auto &Value : Section.Entries) {
				if (!Value.Name.empty()) {
					Buffer += Value.Name;
					Buffer += L"=";
				}
				Buffer += Value.Value;
				Buffer += L"\r\n";
			}
		}

		::SetFilePointer(m_hFile, 0, nullptr, FILE_BEGIN);

		const DWORD Size = static_cast<DWORD>(Buffer.size() * sizeof(String::value_type));
		DWORD Write;
		static const WORD BOM = 0xFEFF;
		if (!::WriteFile(m_hFile, &BOM, sizeof(BOM), &Write, nullptr) || Write != sizeof(BOM)
				|| !::WriteFile(m_hFile, Buffer.data(), Size, &Write, nullptr)
				|| Write != Size) {
			fOK = false;
		} else {
			::SetEndOfFile(m_hFile);
			::FlushFileBuffers(m_hFile);
		}
	}

	if (m_hFile != INVALID_HANDLE_VALUE) {
		::CloseHandle(m_hFile);
		m_hFile = INVALID_HANDLE_VALUE;
	}

	m_FileLock.Release();
	m_FileLock.Close();

	m_FileName.clear();
	m_Section.clear();
	m_OpenFlags = OpenFlag::None;

	m_SectionList.clear();
	m_pCurSection = nullptr;

	return fOK;
}

bool CIniFile::SelectSection(LPCWSTR pszSection)
{
	if (!m_OpenFlags)
		return false;

	m_Section.clear();
	m_pCurSection = nullptr;

	if (IsStringEmpty(pszSection))
		return false;

	auto i = FindSection(pszSection);
	if (i == m_SectionList.end()) {
		if (!(m_OpenFlags & (OpenFlag::Write | OpenFlag::WriteVolatile)))
			return false;

		CreateSection(pszSection);
		m_pCurSection = &m_SectionList.back();
	} else {
		m_pCurSection = &*i;
	}

	m_Section = pszSection;

	return true;
}

bool CIniFile::IsSectionExists(LPCWSTR pszSection)
{
	if (IsStringEmpty(pszSection))
		return false;

	return FindSection(pszSection) != m_SectionList.end();
}

bool CIniFile::DeleteSection(LPCWSTR pszSection)
{
	if (!(m_OpenFlags & (OpenFlag::Write | OpenFlag::WriteVolatile))
			|| IsStringEmpty(pszSection))
		return false;

	auto i = FindSection(pszSection);
	if (i == m_SectionList.end())
		return false;

	if (m_pCurSection == &*i) {
		m_pCurSection = nullptr;
		m_Section.clear();
	}

	m_SectionList.erase(i);

	return true;
}

bool CIniFile::ClearSection(LPCWSTR pszSection)
{
	if (!(m_OpenFlags & (OpenFlag::Write | OpenFlag::WriteVolatile))
			|| IsStringEmpty(pszSection))
		return false;

	auto i = FindSection(pszSection);
	if (i == m_SectionList.end())
		return false;

	i->Entries.remove_if([](const CEntry & Entry) -> bool { return !Entry.Name.empty(); });

	return true;
}

bool CIniFile::ClearSection()
{
	if (m_Section.empty())
		return false;

	return ClearSection(m_Section.c_str());
}

bool CIniFile::GetValue(LPCWSTR pszName, String *pValue)
{
	if (pValue == nullptr)
		return false;

	pValue->clear();

	if (!(m_OpenFlags & OpenFlag::Read)
			|| m_Section.empty()
			|| IsStringEmpty(pszName))
		return false;

	CSectionData *pSection = m_pCurSection;
	if (pSection == nullptr)
		return false;

	auto itrValue = FindValue(pSection->Entries, pszName);
	if (itrValue == pSection->Entries.end())
		return false;

	*pValue = itrValue->Value;
	if (pValue->length() >= 2
			&& pValue->at(0) == L'"'
			&& pValue->at(pValue->length() - 1) == L'"') {
		if (pValue->length() > 2)
			*pValue = pValue->substr(1, pValue->length() - 2);
		else
			pValue->clear();
	}

	return true;
}

bool CIniFile::SetValue(LPCWSTR pszName, LPCWSTR pszValue)
{
	if (!(m_OpenFlags & (OpenFlag::Write | OpenFlag::WriteVolatile))
			|| m_Section.empty()
			|| IsStringEmpty(pszName))
		return false;

	CSectionData *pSection = m_pCurSection;
	if (pSection == nullptr)
		return false;

	auto itrValue = FindValue(pSection->Entries, pszName);
	if (itrValue == pSection->Entries.end()) {
		//TRACE(TEXT("新規設定 : {}={}\n"), pszName, pszValue);
		bool fInserted = false;
		for (auto i = pSection->Entries.rbegin(); i != pSection->Entries.rend(); i++) {
			if (!i->Name.empty()) {
				pSection->Entries.emplace(i.base(), pszName, pszValue);
				fInserted = true;
				break;
			}
		}
		if (!fInserted)
			pSection->Entries.emplace_front(pszName, pszValue);
	} else {
		if (IsStringEmpty(pszValue))
			itrValue->Value.clear();
		else
			itrValue->Value = pszValue;
	}

	return true;
}

bool CIniFile::IsValueExists(LPCWSTR pszName)
{
	if (m_Section.empty()
			|| IsStringEmpty(pszName))
		return false;

	CSectionData *pSection = m_pCurSection;
	if (pSection == nullptr)
		return false;

	auto itrValue = FindValue(pSection->Entries, pszName);
	return itrValue != pSection->Entries.end();
}

bool CIniFile::DeleteValue(LPCWSTR pszName)
{
	if (!(m_OpenFlags & (OpenFlag::Write | OpenFlag::WriteVolatile))
			|| m_Section.empty()
			|| IsStringEmpty(pszName))
		return false;

	CSectionData *pSection = m_pCurSection;
	if (pSection == nullptr)
		return false;

	auto itrValue = FindValue(pSection->Entries, pszName);
	if (itrValue == pSection->Entries.end())
		return false;

	pSection->Entries.erase(itrValue);

	return true;
}

bool CIniFile::GetSectionEntries(LPCWSTR pszSection, EntryArray *pEntries)
{
	if (pEntries == nullptr)
		return false;

	pEntries->clear();

	if (!(m_OpenFlags & OpenFlag::Read)
			|| IsStringEmpty(pszSection))
		return false;

	auto itrSection = FindSection(pszSection);
	if (itrSection == m_SectionList.end())
		return false;

	for (const auto &e : itrSection->Entries) {
		if (!e.Name.empty()) {
			pEntries->push_back(e);
		}
	}

	return true;
}

bool CIniFile::GetEntries(EntryArray *pEntries)
{
	if (m_Section.empty())
		return false;

	return GetSectionEntries(m_Section.c_str(), pEntries);
}

bool CIniFile::Parse(LPCWSTR pszBuffer)
{
	int LineCount = 0;
	String Line;

	LPCWSTR p = pszBuffer;
	while (*p != L'\0') {
		String::size_type Length = 0;
		for (; p[Length] != L'\0' && p[Length] != L'\r' && p[Length] != L'\n'; Length++);
		Line.assign(p, Length);

		String::size_type End;
		if (Length > 3 && Line[0] == L'[' && (End = Line.find(L']')) != String::npos) {
			String Name(Line.substr(1, End - 1));
			StringUtility::Trim(Name);
			CreateSection(Name.c_str());
		} else {
			//TRACE(L"{:4} : {}\n", LineCount + 1, Line);
			if (m_SectionList.empty())
				CreateSection(L"");
			m_SectionList.back().Entries.emplace_back(Line);
		}

		LineCount++;
		p += Length;
		if (*p == L'\r')
			p++;
		if (*p == L'\n')
			p++;
	}

	return true;
}

CIniFile::SectionList::iterator CIniFile::FindSection(LPCWSTR pszName)
{
	if (IsStringEmpty(pszName))
		return m_SectionList.end();

	for (auto i = m_SectionList.begin(); i != m_SectionList.end(); i++) {
		if (StringUtility::CompareNoCase(i->SectionName, pszName) == 0)
			return i;
	}

	return m_SectionList.end();
}

CIniFile::SectionList::const_iterator CIniFile::FindSection(LPCWSTR pszName) const
{
	if (IsStringEmpty(pszName))
		return m_SectionList.end();

	for (auto i = m_SectionList.begin(); i != m_SectionList.end(); i++) {
		if (StringUtility::CompareNoCase(i->SectionName, pszName) == 0)
			return i;
	}

	return m_SectionList.end();
}

CIniFile::EntryList::iterator CIniFile::FindValue(EntryList &Entries, LPCWSTR pszName)
{
	for (auto i = Entries.begin(); i != Entries.end(); i++) {
		if (!i->Name.empty() && StringUtility::CompareNoCase(i->Name, pszName) == 0)
			return i;
	}

	return Entries.end();
}

CIniFile::EntryList::const_iterator CIniFile::FindValue(const EntryList &Entries, LPCWSTR pszName) const
{
	for (auto i = Entries.begin(); i != Entries.end(); i++) {
		if (!i->Name.empty() && StringUtility::CompareNoCase(i->Name, pszName) == 0)
			return i;
	}

	return Entries.end();
}

bool CIniFile::CreateSection(LPCWSTR pszName)
{
	//TRACE(L"INIセクション作成 : [{}]\n", pszName);

	m_SectionList.emplace_back(pszName);

	if (!IsStringEmpty(pszName)
			&& m_SectionList.size() > 1) {
		auto i = m_SectionList.rbegin();
		i++;
		if (!i->SectionName.empty()
				&& (i->Entries.empty()
					|| !i->Entries.back().Name.empty()
					|| !i->Entries.back().Value.empty())) {
			i->Entries.emplace_back();
		}
	}

	return true;
}


CIniFile::CEntry::CEntry(const String &Text)
{
	String::size_type Pos = Text.find_first_not_of(L" \t");

	if (Pos != String::npos && Text[Pos] == L';') {
		Value = Text;
	} else {
		Pos = Text.find(L'=');
		if (Pos != String::npos) {
			Name = Text.substr(0, Pos);
			StringUtility::Trim(Name);
			Value = Text.substr(Pos + 1);
		} else {
			Value = Text;
		}
	}
}

CIniFile::CEntry::CEntry(LPCWSTR pszName, LPCWSTR pszValue)
{
	if (!IsStringEmpty(pszName))
		Name = pszName;
	if (!IsStringEmpty(pszValue))
		Value = pszValue;
}

}
