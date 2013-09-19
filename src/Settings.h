#ifndef TVTEST_SETTINGS_H
#define TVTEST_SETTINGS_H


#include "IniFile.h"


class CSettings
{
	TVTest::CIniFile m_IniFile;
	unsigned int m_OpenFlags;

public:
	enum {
		OPEN_READ           = 0x00000001U,
		OPEN_WRITE          = 0x00000002U,
		OPEN_WRITE_VOLATILE = 0x00000004U
	};

	CSettings();
	~CSettings();
	bool Open(LPCTSTR pszFileName,unsigned int Flags);
	void Close();
	bool IsOpened() const;
	bool Clear();
	bool SetSection(LPCTSTR pszSection);
	bool Read(LPCTSTR pszValueName,int *pData);
	bool Write(LPCTSTR pszValueName,int Data);
	bool Read(LPCTSTR pszValueName,unsigned int *pData);
	bool Write(LPCTSTR pszValueName,unsigned int Data);
	bool Read(LPCTSTR pszValueName,LPTSTR pszData,unsigned int Max);
	bool Write(LPCTSTR pszValueName,LPCTSTR pszData);
	bool Read(LPCTSTR pszValueName,TVTest::String *pValue);
	bool Write(LPCTSTR pszValueName,const TVTest::String &Value);
	bool Read(LPCTSTR pszValueName,bool *pfData);
	bool Write(LPCTSTR pszValueName,bool fData);
	bool ReadColor(LPCTSTR pszValueName,COLORREF *pcrData);
	bool WriteColor(LPCTSTR pszValueName,COLORREF crData);
	bool Read(LPCTSTR pszValueName,LOGFONT *pFont);
	bool Write(LPCTSTR pszValueName,const LOGFONT *pFont);
};

class ABSTRACT_CLASS(CSettingsBase)
{
public:
	CSettingsBase();
	CSettingsBase(LPCTSTR pszSection);
	virtual ~CSettingsBase();
	virtual bool ReadSettings(CSettings &Settings) { return false; }
	virtual bool WriteSettings(CSettings &Settings) { return false; }
	virtual bool LoadSettings(CSettings &Settings);
	virtual bool SaveSettings(CSettings &Settings);
	bool LoadSettings(LPCTSTR pszFileName);
	bool SaveSettings(LPCTSTR pszFileName);
	bool IsChanged() const { return m_fChanged; }
	void ClearChanged() { m_fChanged=false; }

protected:
	LPCTSTR m_pszSection;
	bool m_fChanged;
};


#endif
