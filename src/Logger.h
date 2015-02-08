#ifndef LOGGER_H
#define LOGGER_H


#include <vector>
#include "Options.h"


class CLogItem
{
public:
	enum LogType {
		TYPE_INFORMATION,
		TYPE_WARNING,
		TYPE_ERROR
	};

	CLogItem();
	CLogItem(LogType Type,LPCTSTR pszText,DWORD SerialNumber);
	~CLogItem();
	LPCTSTR GetText() const { return m_Text.c_str(); }
	void GetTime(SYSTEMTIME *pTime) const;
	LogType GetType() const { return m_Type; }
	DWORD GetSerialNumber() const { return m_SerialNumber; }
	int Format(char *pszText,int MaxLength) const;
	int Format(WCHAR *pszText,int MaxLength) const;
	int FormatTime(char *pszText,int MaxLength) const;
	int FormatTime(WCHAR *pszText,int MaxLength) const;

private:
	FILETIME m_Time;
	TVTest::String m_Text;
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
	bool AddLog(CLogItem::LogType Type,LPCTSTR pszText, ...);
	bool AddLogV(CLogItem::LogType Type,LPCTSTR pszText,va_list Args);
	bool AddLogRaw(CLogItem::LogType Type,LPCTSTR pszText);
	void Clear();
	std::size_t GetLogCount() const;
	bool GetLog(std::size_t Index,CLogItem *pItem) const;
	bool GetLogBySerialNumber(DWORD SerialNumber,CLogItem *pItem) const;
	bool SetOutputToFile(bool fOutput);
	bool GetOutputToFile() const { return m_fOutputToFile; }
	bool SaveToFile(LPCTSTR pszFileName,bool fAppend);
	void GetDefaultLogFileName(LPTSTR pszFileName) const;
	bool CopyToClipboard(HWND hwnd);

private:
// CBasicDialog
	INT_PTR DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam) override;

	std::vector<CLogItem*> m_LogList;
	DWORD m_SerialNumber;
	bool m_fOutputToFile;
	mutable CCriticalLock m_Lock;
};


#endif
