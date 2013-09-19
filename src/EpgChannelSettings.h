#ifndef EPG_CHANNEL_SETTINGS_H
#define EPG_CHANNEL_SETTINGS_H


#include "Dialog.h"
#include "ChannelList.h"


class CProgramGuide;

class CEpgChannelSettings : public CResizableDialog
{
public:
	CEpgChannelSettings(CProgramGuide *pProgramGuide);
	~CEpgChannelSettings();

	bool Show(HWND hwndOwner) override;

private:
	INT_PTR DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam) override;

	CProgramGuide *m_pProgramGuide;
	CChannelList m_ChannelList;
};


#endif
