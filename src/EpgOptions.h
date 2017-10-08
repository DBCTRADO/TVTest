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

	enum class EpgTimeMode {
		Raw,
		JST,
		Local,
		UTC,
		TVTEST_ENUM_CLASS_TRAILER
	};

	CEpgOptions();
	~CEpgOptions();

// CSettingsBase
	bool ReadSettings(CSettings &Settings) override;
	bool WriteSettings(CSettings &Settings) override;

// CBasicDialog
	bool Create(HWND hwndOwner) override;

// CEpgOptions
	void Finalize();

	LPCTSTR GetEpgFileName() const { return m_EpgFileName.c_str(); }
	bool GetUpdateWhenStandby() const { return m_fUpdateWhenStandby; }
	bool GetUpdateBSExtended() const { return m_fUpdateBSExtended; }
	bool GetUpdateCSExtended() const { return m_fUpdateCSExtended; }

	const TVTest::Style::Font &GetEventInfoFont() const { return m_EventInfoFont; }

	bool LoadEpgFile(LibISDB::EPGDatabase *pEPGDatabase);
	bool AsyncLoadEpgFile(
		LibISDB::EPGDatabase *pEPGDatabase,
		TVTest::CEpgDataStore::CEventHandler *pEventHandler = nullptr);
	bool IsEpgFileLoading() const;
	bool WaitEpgFileLoad(DWORD Timeout = INFINITE);
	bool SaveEpgFile(LibISDB::EPGDatabase *pEPGDatabase);

	bool LoadEDCBData();
	bool AsyncLoadEDCBData(CEDCBDataLoadEventHandler *pEventHandler = nullptr);
	bool IsEDCBDataLoading() const;
	bool WaitEDCBDataLoad(DWORD Timeout = INFINITE);

	EpgTimeMode GetEpgTimeMode() const { return m_EpgTimeMode; }

	bool LoadLogoFile();
	bool SaveLogoFile();

private:
	bool m_fSaveEpgFile;
	TVTest::CFilePath m_EpgFileName;
	bool m_fUpdateWhenStandby;
	bool m_fUpdateBSExtended;
	bool m_fUpdateCSExtended;
	bool m_fUseEDCBData;
	TVTest::CFilePath m_EDCBDataFolder;
	EpgTimeMode m_EpgTimeMode;
	bool m_fSaveLogoFile;
	TVTest::CFilePath m_LogoFileName;

	TVTest::CEpgDataStore m_EpgDataStore;
	std::unique_ptr<CEpgDataLoader> m_EpgDataLoader;

	TVTest::Style::Font m_EventInfoFont;
	TVTest::Style::Font m_CurEventInfoFont;

// CBasicDialog
	INT_PTR DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

	bool GetEpgFileFullPath(LPTSTR pszFileName);
	static unsigned int __stdcall EpgFileLoadThread(void *pParameter);
};


#endif
