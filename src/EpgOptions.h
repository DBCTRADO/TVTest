#ifndef EPG_OPTIONS_H
#define EPG_OPTIONS_H


#include "CoreEngine.h"
#include "EpgProgramList.h"
#include "Options.h"
#include "LogoManager.h"
#include "EpgDataLoader.h"


class CEpgOptions : public COptions
{
public:
	class ABSTRACT_CLASS(CEpgFileLoadEventHandler)
	{
	public:
		virtual ~CEpgFileLoadEventHandler() {}
		virtual void OnBeginLoad() {}
		virtual void OnEndLoad(bool fSuccess) {}
	};

	typedef CEpgDataLoader::CEventHandler CEDCBDataLoadEventHandler;

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

	const LOGFONT *GetEventInfoFont() const { return &m_EventInfoFont; }

	bool LoadEpgFile(CEpgProgramList *pEpgList);
	bool AsyncLoadEpgFile(CEpgProgramList *pEpgList,CEpgFileLoadEventHandler *pEventHandler=NULL);
	bool IsEpgFileLoading() const;
	bool WaitEpgFileLoad(DWORD Timeout=INFINITE);
	bool SaveEpgFile(CEpgProgramList *pEpgList);

	bool LoadEDCBData();
	bool AsyncLoadEDCBData(CEDCBDataLoadEventHandler *pEventHandler=NULL);
	bool IsEDCBDataLoading() const;
	bool WaitEDCBDataLoad(DWORD Timeout=INFINITE);

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
	bool m_fSaveLogoFile;
	TCHAR m_szLogoFileName[MAX_PATH];
	HANDLE m_hLoadThread;
	CEpgDataLoader *m_pEpgDataLoader;

	LOGFONT m_EventInfoFont;
	LOGFONT m_CurEventInfoFont;

// CBasicDialog
	INT_PTR DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam) override;

	bool GetEpgFileFullPath(LPTSTR pszFileName);
	static unsigned int __stdcall EpgFileLoadThread(void *pParameter);
};


#endif
