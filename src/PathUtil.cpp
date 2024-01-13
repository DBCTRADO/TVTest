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
#include "PathUtil.h"
#include "Common/DebugDef.h"


namespace TVTest
{

namespace PathUtil
{


bool RemoveExtension(String *pPath)
{
	if (pPath == nullptr)
		return false;

	const String::size_type Pos = pPath->find_last_of(L'.');
	if ((Pos != String::npos) && (pPath->find_first_of(L"\\/" ,Pos + 1) == String::npos))
		pPath->resize(Pos);

	return true;
}


bool RenameExtension(String *pPath, LPCWSTR pszExtension)
{
	if (pPath == nullptr)
		return false;

	RemoveExtension(pPath);
	if (!IsStringEmpty(pszExtension))
		pPath->append(pszExtension);

	return true;
}


bool GetExtension(const String &Path, String *pExtension)
{
	if (pExtension == nullptr)
		return false;

	const String::size_type Pos = Path.find_last_of(L'.');
	if ((Pos != String::npos) && (Path.find_first_of(L"\\/", Pos + 1) == String::npos))
		*pExtension = Path.substr(Pos);
	else
		pExtension->clear();

	return true;
}


bool RemoveFileName(String *pPath)
{
	if (pPath == nullptr)
		return false;

	String::size_type Pos = pPath->find_last_of(L"\\/");
	if (Pos == String::npos)
		return false;

	if ((Pos > 0) && (pPath->at(Pos - 1) == L':'))
		Pos++;

	if (Pos == pPath->length())
		return false;

	pPath->resize(Pos);

	return true;
}


bool Append(String *pPath, LPCWSTR pszMore)
{
	if (pPath == nullptr)
		return false;

	if (!pPath->empty() && !IsDelimiter(pPath->back()))
		pPath->append(L"\\");
	if (!IsStringEmpty(pszMore))
		pPath->append(IsDelimiter(pszMore[0]) ? pszMore + 1 : pszMore);

	return true;
}


bool Append(String *pPath, const String &More)
{
	if (pPath == nullptr)
		return false;

	if (!pPath->empty() && !IsDelimiter(pPath->back()))
		pPath->append(L"\\");

	if (!More.empty()) {
		if (IsDelimiter(More.front()))
			pPath->append(More, 1, String::npos);
		else
			pPath->append(More);
	}

	return true;
}


bool GetFileName(const String &Path, String *pFileName)
{
	if (pFileName == nullptr)
		return false;

	const String::size_type Pos = Path.find_last_of(L"\\/");
	if (Pos == String::npos)
		*pFileName = Path;
	else
		*pFileName = Path.substr(Pos+1);

	return true;
}


bool Split(const String &Path, String *pDirectory, String *pFileName)
{
	if ((pDirectory == nullptr) && (pFileName == nullptr))
		return false;

	String::size_type Pos = Path.find_last_of(L"\\/");
	if (Pos == String::npos) {
		if (pDirectory != nullptr)
			pDirectory->clear();
		if (pFileName != nullptr)
			*pFileName = Path;
	} else {
		if (pFileName != nullptr)
			*pFileName = Path.substr(Pos + 1);
		if (pDirectory != nullptr) {
			if ((Pos > 0) && (Path.at(Pos - 1) == L':'))
				Pos++;
			*pDirectory = Path.substr(0, Pos);
		}
	}

	return true;
}


bool AppendDelimiter(String *pPath)
{
	if (pPath == nullptr)
		return false;

	if ((pPath->length() == 1) && ::IsCharAlpha((*pPath)[0])) {
		pPath->append(L":\\");
	} else if (pPath->empty() || !IsDelimiter(pPath->back())) {
		pPath->append(L"\\");
	}

	return true;
}


bool RemoveDelimiter(String *pPath)
{
	if ((pPath == nullptr) || pPath->empty())
		return false;

	String::size_type Pos = pPath->length();
	while ((Pos > 0) && IsDelimiter((*pPath)[Pos - 1]))
		Pos--;

	pPath->resize(Pos);

	return true;
}


bool IsAbsolute(const String &Path)
{
	return (Path.length() >= 2)
		&& ((IsDelimiter(Path[0]) && IsDelimiter(Path[1]))
			|| (Path[1] == L':'));
}


bool RelativeToAbsolute(String *pAbsolutePath, const String &BasePath, const String &RelativePath)
{
	if (pAbsolutePath == nullptr)
		return false;

	pAbsolutePath->clear();

	if (BasePath.empty()) {
		if (IsAbsolute(RelativePath)) {
			*pAbsolutePath = RelativePath;
			return true;
		}
		return false;
	}

	if (RelativePath.empty()) {
		*pAbsolutePath = BasePath;
		return true;
	}

	String Path(BasePath);
	Append(&Path, RelativePath);
	if (!Canonicalize(&Path))
		return false;

	*pAbsolutePath = Path;

	return true;
}


bool Canonicalize(String *pPath)
{
	if (pPath == nullptr)
		return false;

	String::size_type Next = 0;

	do {
		String::size_type Pos = pPath->find_first_of(L'\\', Next);
		if (Pos == String::npos)
			Pos = pPath->length();
		if (Pos > Next) {
			const StringView Item(pPath->data() + Next, Pos - Next);
			if (Item.compare(L".") == 0) {
				pPath->erase(Next, Pos - Next + 1);
				Pos = Next;
			} else if (Item.compare(L"..") == 0) {
				if (Next < 2)
					return false;
				const String::size_type Prev = pPath->rfind(L'\\', Next - 2);
				if (Prev == String::npos)
					return false;
				pPath->erase(Prev, Pos - Prev);
				Next = Prev + 1;
			} else {
				Next = Pos + 1;
			}
		} else {
			Next = Pos + 1;
		}
	} while (Next < pPath->length());

	if ((pPath->length() == 2) && ((*pPath)[1] == L':'))
		pPath->push_back(L'\\');

	return true;
}


bool IsRoot(const String &Path)
{
	if (Path.length() >= MAX_PATH)
		return false;

	if ((Path.length() > 2) && IsDelimiter(Path[0]) && IsDelimiter(Path[1])) {
		String Directory(Path);
		RemoveDelimiter(&Directory);
		return ::PathIsRoot(Directory.c_str()) != FALSE;
	}

	return ::PathIsRoot(Path.c_str()) != FALSE;
}


bool IsExists(const String &Path)
{
	if (Path.empty())
		return false;
	return ::GetFileAttributes(Path.c_str()) != INVALID_FILE_ATTRIBUTES;
}


bool IsFileExists(const String &Path)
{
	if (Path.empty())
		return false;
	const DWORD Attributes = ::GetFileAttributes(Path.c_str());
	return (Attributes != INVALID_FILE_ATTRIBUTES)
		&& (Attributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
}


} // namespace PathUtil

} // namespace TVTest
