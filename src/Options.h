#ifndef OPTIONS_H
#define OPTIONS_H


#include "Settings.h"
#include "Dialog.h"


class ABSTRACT_CLASS(COptionFrame)
{
public:
	virtual ~COptionFrame() {}
	virtual void OnSettingError(class COptions *pOptions) {}
};

class COptions : public CBasicDialog, public CSettingsBase
{
public:
	enum {
		UPDATE_GENERAL_BUILDMEDIAVIEWER	= 0x00000001UL,
		UPDATE_GENERAL_EVENTINFOFONT	= 0x00000002UL,
		UPDATE_ALL						= 0xFFFFFFFFUL
	};

	COptions();
	COptions(LPCTSTR pszSection);
	virtual ~COptions();
	DWORD GetUpdateFlags() const { return m_UpdateFlags; }
	DWORD SetUpdateFlag(DWORD Flag);
	void ClearUpdateFlags() { m_UpdateFlags=0; }
	virtual bool Apply(DWORD Flags) { return true; }

	static void SetFrame(COptionFrame *pFrame) { m_pFrame=pFrame; }
	static void ClearGeneralUpdateFlags() { m_GeneralUpdateFlags=0; }
	static DWORD GetGeneralUpdateFlags() { return m_GeneralUpdateFlags; }
	static DWORD SetGeneralUpdateFlag(DWORD Flag);

protected:
	void SettingError();

	DWORD m_UpdateFlags;

private:
	static COptionFrame *m_pFrame;
	static DWORD m_GeneralUpdateFlags;
};


#endif
