#ifndef LOGGER_H
#define LOGGER_H


#include <vector>
#include "Options.h"
#include "TsUtilClass.h"


class CLogItem
{
	FILETIME m_Time;
	TVTest::String m_Text;
	DWORD m_SerialNumber;

public:
	CLogItem();
	CLogItem(LPCTSTR pszText,DWORD SerialNumber);
	~CLogItem();
	LPCTSTR GetText() const { return m_Text.c_str(); }
	void GetTime(SYSTEMTIME *pTime) const;
	DWORD GetSerialNumber() const { return m_SerialNumber; }
	int Format(char *pszText,int MaxLength) const;
	int Format(WCHAR *pszText,int MaxLength) const;
	int FormatTime(char *pszText,int MaxLength) const;
	int FormatTime(WCHAR *pszText,int MaxLength) const;
};

class CLogger : public COptions, public CTracer
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
	bool AddLog(LPCTSTR pszText, ...);
	bool AddLogV(LPCTSTR pszText,va_list Args);
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

// CTracer
	void OnTrace(LPCTSTR pszOutput) override;

	std::vector<CLogItem*> m_LogList;
	DWORD m_SerialNumber;
	bool m_fOutputToFile;
	mutable CCriticalLock m_Lock;
};


#endif
