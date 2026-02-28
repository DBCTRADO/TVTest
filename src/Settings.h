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


#ifndef TVTEST_SETTINGS_H
#define TVTEST_SETTINGS_H


#include <vector>
#include "IniFile.h"


namespace TVTest
{

	class CSettings
	{
	public:
		enum class OpenFlag : unsigned int {
			None          = 0x0000U,
			Read          = 0x0001U,
			Write         = 0x0002U,
			WriteVolatile = 0x0004U,
			TVTEST_ENUM_FLAGS_TRAILER
		};

		typedef CIniFile::CEntry CEntry;
		typedef CIniFile::EntryArray EntryList;

		~CSettings();

		bool Open(LPCTSTR pszFileName, OpenFlag Flags);
		void Close();
		bool IsOpened() const;
		bool Clear();
		bool SetSection(LPCTSTR pszSection);
		bool IsSectionExists(LPCTSTR pszSection);
		bool GetEntries(EntryList *pEntries);
		bool IsValueExists(LPCTSTR pszValueName);
		bool DeleteValue(LPCTSTR pszValueName);
		bool Read(LPCTSTR pszValueName, int *pData);
		bool Write(LPCTSTR pszValueName, int Data);
		bool Read(LPCTSTR pszValueName, unsigned int *pData);
		bool Write(LPCTSTR pszValueName, unsigned int Data);
		bool Read(LPCTSTR pszValueName, LPTSTR pszData, unsigned int Max);
		bool Write(LPCTSTR pszValueName, LPCTSTR pszData);
		bool Read(LPCTSTR pszValueName, String *pValue);
		bool Write(LPCTSTR pszValueName, const String &Value);
		bool Read(LPCTSTR pszValueName, bool *pfData);
		bool Write(LPCTSTR pszValueName, bool fData);
		bool Read(LPCTSTR pszValueName, double *pData);
		bool Write(LPCTSTR pszValueName, double Data, int Digits);
		bool Read(LPCTSTR pszValueName, float *pData);
		bool ReadColor(LPCTSTR pszValueName, COLORREF *pcrData);
		bool WriteColor(LPCTSTR pszValueName, COLORREF crData);
		bool Read(LPCTSTR pszValueName, LOGFONT *pFont);
		bool Write(LPCTSTR pszValueName, const LOGFONT *pFont);

		template<Concept::EnumFlags T> bool Read(LPCTSTR pszValueName, T *pData)
		{
			std::underlying_type_t<T> Flags;
			if (!Read(pszValueName, &Flags))
				return false;
			*pData = static_cast<T>(Flags) & T::AllFlags_;
			return true;
		}

		template<Concept::EnumFlags T> bool Write(LPCTSTR pszValueName, T Data)
		{
			return Write(pszValueName, static_cast<std::underlying_type_t<T>>(Data));
		}

	private:
		CIniFile m_IniFile;
		OpenFlag m_OpenFlags = OpenFlag::None;
	};

	class ABSTRACT_CLASS(CSettingsBase)
	{
	public:
		CSettingsBase();
		CSettingsBase(LPCTSTR pszSection);
		virtual ~CSettingsBase() = default;

		virtual bool ReadSettings(CSettings &Settings) { return false; }
		virtual bool WriteSettings(CSettings &Settings) { return false; }
		virtual bool LoadSettings(CSettings &Settings);
		virtual bool SaveSettings(CSettings &Settings);
		bool LoadSettings(LPCTSTR pszFileName);
		bool SaveSettings(LPCTSTR pszFileName);
		bool IsChanged() const { return m_fChanged; }
		void SetChanged() { m_fChanged = true; }
		void ClearChanged() { m_fChanged = false; }

	protected:
		LPCTSTR m_pszSection;
		bool m_fChanged = false;
	};

} // namespace TVTest


#endif
