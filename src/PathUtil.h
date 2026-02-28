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


#ifndef TVTEST_PATH_UTIL_H
#define TVTEST_PATH_UTIL_H


#include "StringUtility.h"
#include <format>


namespace TVTest
{

	namespace PathUtil
	{

		inline bool IsDelimiter(TCHAR c) { return (c == _T('\\')) || (c == _T('/')); }
		bool RemoveExtension(String *pPath);
		bool RenameExtension(String *pPath, LPCWSTR pszExtension);
		bool GetExtension(const String &Path, String *pExtension);
		bool RemoveFileName(String *pPath);
		bool Append(String *pPath, LPCWSTR pszMore);
		bool Append(String *pPath, const String &More);
		bool GetFileName(const String &Path, String *pFileName);
		bool Split(const String &Path, String *pDirectory, String *pFileName);
		bool AppendDelimiter(String *pPath);
		bool RemoveDelimiter(String *pPath);
		bool IsAbsolute(const String &Path);
		inline bool IsRelative(const String &Path) { return !IsAbsolute(Path); }
		bool RelativeToAbsolute(String *pAbsolutePath, const String &BasePath, const String &RelativePath);
		bool Canonicalize(String *pPath);
		bool IsRoot(const String &Path);
		bool IsExists(const String &Path);
		bool IsFileExists(const String &Path);

	} // namespace PathUtil

	class CFilePath : public String
	{
	public:
		using String::String;
		using String::operator=;
		using String::operator+=;

		bool RemoveExtension() { return PathUtil::RemoveExtension(this); }
		bool RenameExtension(LPCWSTR pszExtension) { return PathUtil::RenameExtension(this, pszExtension); }
		bool GetExtension(String *pExtension) const { return PathUtil::GetExtension(*this, pExtension); }
		bool RemoveFileName() { return PathUtil::RemoveFileName(this); }
		bool Append(LPCWSTR pszMore) { return PathUtil::Append(this, pszMore); }
		bool Append(const String &More) { return PathUtil::Append(this, More); }
		bool GetFileName(String *pFileName) const { return PathUtil::GetFileName(*this, pFileName); }
		bool Split(String *pDirectory, String *pFileName) const { return PathUtil::Split(*this, pDirectory, pFileName); }
		bool AppendDelimiter() { return PathUtil::AppendDelimiter(this); }
		bool RemoveDelimiter() { return PathUtil::RemoveDelimiter(this); }
		bool IsAbsolute() const { return PathUtil::IsAbsolute(*this); }
		bool IsRelative() const { return PathUtil::IsRelative(*this); }
		bool GetAbsolute(String *pAbsolute, const String &Base) const { return PathUtil::RelativeToAbsolute(pAbsolute, Base, *this); }
		bool Canonicalize() { return PathUtil::Canonicalize(this); }
		bool IsRoot() const { return PathUtil::IsRoot(*this); }
		bool IsExists() const { return PathUtil::IsExists(*this); }
		bool IsFileExists() const { return PathUtil::IsFileExists(*this); }
	};

} // namespace TVTest

template<> struct std::formatter<TVTest::CFilePath, TCHAR>
	: public std::formatter<TVTest::String, TCHAR>
{
	template<typename TContext> auto format(const TVTest::CFilePath &Value, TContext &Context) const
	{
		return formatter<TVTest::String, TCHAR>::format(Value, Context);
	}
};


#endif
