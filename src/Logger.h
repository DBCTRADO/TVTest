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


#ifndef TVTEST_LOGGER_H
#define TVTEST_LOGGER_H


#include <vector>
#include <memory>
#include "Options.h"
#include "StringFormat.h"


namespace TVTest
{

	class CLogItem
	{
	public:
		enum class LogType {
			Information,
			Warning,
			Error,
		};

		CLogItem() = default;
		CLogItem(LogType Type, StringView Text, DWORD SerialNumber);

		LPCTSTR GetText() const { return m_Text.c_str(); }
		void GetTime(SYSTEMTIME *pTime) const;
		LogType GetType() const { return m_Type; }
		DWORD GetSerialNumber() const { return m_SerialNumber; }
		int Format(char *pszText, int MaxLength) const;
		int Format(WCHAR *pszText, int MaxLength) const;
		int FormatTime(char *pszText, int MaxLength) const;
		int FormatTime(WCHAR *pszText, int MaxLength) const;

	private:
		FILETIME m_Time;
		String m_Text;
		LogType m_Type;
		DWORD m_SerialNumber;
	};

	class CLogger
		: public COptions
	{
	public:
		CLogger();
		~CLogger();

	// CSettingsBase
		bool ReadSettings(CSettings &Settings) override;
		bool WriteSettings(CSettings &Settings) override;

	// CBasicDialog
		bool Create(HWND hwndOwner) override;

	// CLogger
		template<typename... TArgs> bool AddLog(CLogItem::LogType Type, StringView Format, TArgs&&... Args)
		{
			return AddLogV(Type, Format, MakeFormatArgs(Args...));
		}
		bool AddLogV(CLogItem::LogType Type, StringView Format, FormatArgs Args);
		bool AddLogRaw(CLogItem::LogType Type, StringView Text);
		void Clear();
		std::size_t GetLogCount() const;
		bool GetLog(std::size_t Index, CLogItem *pItem) const;
		bool GetLogBySerialNumber(DWORD SerialNumber, CLogItem *pItem) const;
		bool SetOutputToFile(bool fOutput);
		bool GetOutputToFile() const { return m_fOutputToFile; }
		bool SaveToFile(LPCTSTR pszFileName, bool fAppend);
		void GetLogText(String *pText) const;
		void GetLogText(AnsiString *pText) const;
		bool CopyToClipboard(HWND hwnd);

	private:
		enum {
			COLUMN_DUMMY,
			COLUMN_TIME,
			COLUMN_TEXT,
			NUM_COLUMNS
		};

	// CBasicDialog
		INT_PTR DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

	// CUIBase
		void RealizeStyle() override;

	// CLogger
		bool CreateFileLock(CGlobalLock *pLock) const;

		std::vector<std::unique_ptr<CLogItem>> m_LogList;
		DWORD m_SerialNumber = 0;
		bool m_fOutputToFile = false;
		mutable MutexLock m_Lock;
		String m_DefaultLogFileName;
		HANDLE m_hFile = INVALID_HANDLE_VALUE;
	};

} // namespace TVTest


#endif
