#ifndef GENERAL_OPTIONS_H
#define GENERAL_OPTIONS_H


#include "Options.h"


class CGeneralOptions
	: public COptions
{
public:
	enum DefaultDriverType {
		DEFAULT_DRIVER_NONE,
		DEFAULT_DRIVER_LAST,
		DEFAULT_DRIVER_CUSTOM
	};

	CGeneralOptions();
	~CGeneralOptions();

// COptions
	bool Apply(DWORD Flags) override;

// CSettingsBase
	bool ReadSettings(CSettings &Settings) override;
	bool WriteSettings(CSettings &Settings) override;

// CBasicDialog
	bool Create(HWND hwndOwner) override;

// CGeneralOptions
	DefaultDriverType GetDefaultDriverType() const;
	LPCTSTR GetDefaultDriverName() const;
	bool SetDefaultDriverName(LPCTSTR pszDriverName);
	bool GetFirstDriverName(TVTest::String *pDriverName) const;
	bool GetResident() const;
	bool GetKeepSingleTask() const;
	bool GetStandaloneProgramGuide() const { return m_fStandaloneProgramGuide; }

private:
// CBasicDialog
	INT_PTR DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

	enum {
		UPDATE_RESIDENT     = 0x00000001UL,
		UPDATE_1SEGFALLBACK = 0x00000002UL
	};

	TVTest::String m_BonDriverDirectory;
	DefaultDriverType m_DefaultDriverType;
	TVTest::String m_DefaultBonDriverName;
	TVTest::String m_LastBonDriverName;
	bool m_fResident;
	bool m_fKeepSingleTask;
	bool m_fStandaloneProgramGuide;
	bool m_fEnable1SegFallback;
};


#endif
