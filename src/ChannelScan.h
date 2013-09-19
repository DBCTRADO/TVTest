#ifndef CHANNEL_SCAN_H
#define CHANNEL_SCAN_H


#include "CoreEngine.h"
#include "ChannelList.h"
#include "Options.h"


class CChannelScan : public COptions
{
public:
	CChannelScan();
	~CChannelScan();

// COptions
	bool Apply(DWORD Flags) override;

// CSettingsBase
	bool ReadSettings(CSettings &Settings) override;
	bool WriteSettings(CSettings &Settings) override;

// CBasicDialog
	bool Create(HWND hwndOwner) override;

// CChannelScan
	bool SetTuningSpaceList(const CTuningSpaceList *pTuningSpaceList);
	const CTuningSpaceList *GetTuningSpaceList() const { return &m_TuningSpaceList; }
	bool IsScanning() const { return m_hScanThread!=NULL; }

	bool AutoUpdateChannelList(CTuningSpaceList *pTuningSpaceList,std::vector<TVTest::String> *pMessageList=NULL);

private:
	enum {
		UPDATE_CHANNELLIST	= 0x0000001UL,
		UPDATE_PREVIEW		= 0x0000002UL
	};

	int m_ScanSpace;
	int m_ScanChannel;
	const CTuningSpaceList *m_pOriginalTuningSpaceList;
	CTuningSpaceList m_TuningSpaceList;
	CChannelList m_ScanningChannelList;
	std::vector<TVTest::String> m_BonDriverChannelList;
	bool m_fScanService;
	bool m_fIgnoreSignalLevel;
	unsigned int m_ScanWait;
	int m_RetryCount;
	unsigned int m_RetryInterval;
	bool m_fUpdated;
	bool m_fScaned;
	bool m_fRestorePreview;
	HWND m_hScanDlg;
	HANDLE m_hScanThread;
	HANDLE m_hCancelEvent;
	bool m_fCancelled;
	int m_SortColumn;
	bool m_fSortDescending;
	bool m_fChanging;
	float m_MaxSignalLevel;
	float m_ChannelMaxSignalLevel;
	DWORD m_MaxBitRate;
	std::vector<float> m_ChannelSignalLevel;

// CBasicDialog
	INT_PTR DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam) override;

	void InsertChannelInfo(int Index,const CChannelInfo *pChInfo);
	void SetChannelList(int Space);
	CChannelInfo *GetSelectedChannelInfo() const;
	bool LoadPreset(LPCTSTR pszFileName,CChannelList *pChannelList,int Space,bool *pfCorrupted);
	bool SetPreset(bool fAuto);
	void Scan();
	float GetSignalLevel();
	INT_PTR ScanDlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);

	static bool IsScanService(const CTsAnalyzer::SdtServiceInfo &ServiceInfo,bool fData=true);
	static bool IsScanServiceType(BYTE ServiceType,bool fData=true);

	static unsigned int __stdcall ScanProc(LPVOID lpParameter);
	static INT_PTR CALLBACK ScanDialogProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
};


#endif
