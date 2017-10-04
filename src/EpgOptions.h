#ifndef EPG_OPTIONS_H
#define EPG_OPTIONS_H


#include "CoreEngine.h"
#include "Options.h"
#include "LogoManager.h"
#include "EpgDataStore.h"
#include "EpgDataLoader.h"
#include "Style.h"


class CEpgOptions
	: public COptions
{
public:
	class ABSTRACT_CLASS(CEpgFileLoadEventHandler)
	{
	public:
		virtual ~CEpgFileLoadEventHandler() = default;
		virtual void OnBeginLoad() {}
		virtual void OnEndLoad(bool fSuccess) {}
	};

	typedef CEpgDataLoader::CEventHandler CEDCBDataLoadEventHandler;

	enum EpgTimeMode {
		EPGTIME_RAW,
		EPGTIME_JST,
		EPGTIME_LOCAL,
		EPGTIME_UTC
	};
	static const EpgTimeMode EPGTIME_FIRST = EPGTIME_RAW;
	static const EpgTimeMode EPGTIME_LAST  = EPGTIME_UTC;

	CEpgOptions();
	~CEpgOptions();

// CSettingsBase
	bool ReadSettings(CSettings &Settings) override;
	bool WriteSettings(CSettings &Settings) override;

// CBasicDialog
	bool Create(HWND hwndOwner) override;

// CEpgOptions
	void Finalize();

	LPCTSTR GetEpgFileName() const { return m_szEpgFileName; }
	bool GetUpdateWhenStandby() const { return m_fUpdateWhenStandby; }
	bool GetUpdateBSExtended() const { return m_fUpdateBSExtended; }
	bool GetUpdateCSExtended() const { return m_fUpdateCSExtended; }

	const TVTest::Style::Font &GetEventInfoFont() const { return m_EventInfoFont; }

	bool LoadEpgFile(LibISDB::EPGDatabase *pEPGDatabase);
	bool AsyncLoadEpgFile(
		LibISDB::EPGDatabase *pEPGDatabase,
		TVTest::CEpgDataStore::CEventHandler *pEventHandler = NULL);
	bool IsEpgFileLoading() const;
	bool WaitEpgFileLoad(DWORD Timeout = INFINITE);
	bool SaveEpgFile(LibISDB::EPGDatabase *pEPGDatabase);

	bool LoadEDCBData();
	bool AsyncLoadEDCBData(CEDCBDataLoadEventHandler *pEventHandler = NULL);
	bool IsEDCBDataLoading() const;
	bool WaitEDCBDataLoad(DWORD Timeout = INFINITE);

	EpgTimeMode GetEpgTimeMode() const { return m_EpgTimeMode; }

	bool LoadLogoFile();
	bool SaveLogoFile();

private:
	bool m_fSaveEpgFile;
	TCHAR m_szEpgFileName[MAX_PATH];
	bool m_fUpdateWhenStandby;
	bool m_fUpdateBSExtended;
	bool m_fUpdateCSExtended;
	bool m_fUseEDCBData;
	TCHAR m_szEDCBDataFolder[MAX_PATH];
	EpgTimeMode m_EpgTimeMode;
	bool m_fSaveLogoFile;
	TCHAR m_szLogoFileName[MAX_PATH];

	TVTest::CEpgDataStore m_EpgDataStore;
	CEpgDataLoader *m_pEpgDataLoader;

	TVTest::Style::Font m_EventInfoFont;
	TVTest::Style::Font m_CurEventInfoFont;

// CBasicDialog
	INT_PTR DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

	bool GetEpgFileFullPath(LPTSTR pszFileName);
	static unsigned int __stdcall EpgFileLoadThread(void *pParameter);
};


#endif
