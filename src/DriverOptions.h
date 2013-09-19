#ifndef DRIVER_OPTIONS_H
#define DRIVER_OPTIONS_H


#include <vector>
#include "Options.h"
#include "DriverManager.h"
#include "ChannelManager.h"


class CDriverSettings;

class CDriverSettingList
{
	std::vector<CDriverSettings*> m_SettingList;

public:
	CDriverSettingList();
	CDriverSettingList(const CDriverSettingList &Src);
	~CDriverSettingList();
	CDriverSettingList &operator=(const CDriverSettingList &Src);
	void Clear();
	size_t NumDrivers() const { return m_SettingList.size(); }
	bool Add(CDriverSettings *pSettings);
	CDriverSettings *GetDriverSettings(size_t Index);
	const CDriverSettings *GetDriverSettings(size_t Index) const;
	int Find(LPCTSTR pszFileName) const;
};

class CDriverOptions : public COptions
{
	CDriverManager *m_pDriverManager;
	CDriverSettingList m_SettingList;
	CDriverSettingList m_CurSettingList;

// CBasicDialog
	INT_PTR DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam) override;

	void InitDlgItem(int Driver);
	void SetChannelList(int Driver);
	void AddChannelList(const CChannelList *pChannelList);
	CDriverSettings *GetCurSelDriverSettings() const;

public:
	struct ChannelInfo {
		int Space;
		int Channel;
		int ServiceID;
		int TransportStreamID;
		bool fAllChannels;
	};

	struct BonDriverOptions {
		bool fNoDescramble;
		bool fNoSignalLevel;
		bool fIgnoreInitialStream;
		bool fPurgeStreamOnChannelChange;
		bool fResetChannelChangeErrorCount;
		DWORD FirstChannelSetDelay;
		DWORD MinChannelChangeInterval;

		BonDriverOptions();
		BonDriverOptions(LPCTSTR pszBonDriverName);
	};

	CDriverOptions();
	~CDriverOptions();
// CSettingsBase
	bool ReadSettings(CSettings &Settings) override;
	bool WriteSettings(CSettings &Settings) override;
// CBasicDialog
	bool Create(HWND hwndOwner) override;
// CDriverOptions
	bool Initialize(CDriverManager *pDriverManager);
	bool GetInitialChannel(LPCTSTR pszFileName,ChannelInfo *pChannelInfo) const;
	bool SetLastChannel(LPCTSTR pszFileName,const ChannelInfo *pChannelInfo);
	bool IsNoDescramble(LPCTSTR pszFileName) const;
	bool IsNoSignalLevel(LPCTSTR pszFileName) const;
	bool IsResetChannelChangeErrorCount(LPCTSTR pszFileName) const;
	bool GetBonDriverOptions(LPCTSTR pszFileName,BonDriverOptions *pOptions) const;
};


#endif
