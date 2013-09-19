#ifndef TVTEST_OSD_MANAGER_H
#define TVTEST_OSD_MANAGER_H


#include "OSDOptions.h"
#include "PseudoOSD.h"
#include "ChannelList.h"


class COSDManager
{
public:
	class ABSTRACT_CLASS(CEventHandler)
	{
	public:
		virtual ~CEventHandler() {}
		virtual bool GetOSDWindow(HWND *phwndParent,RECT *pRect,bool *pfForcePseudoOSD)=0;
		virtual bool SetOSDHideTimer(DWORD Delay)=0;
	};

	enum {
		SHOW_NO_FADE = 0x0001U,
		SHOW_PSEUDO  = 0x0002U
	};

	COSDManager(const COSDOptions *pOptions);
	~COSDManager();
	void SetEventHandler(CEventHandler *pEventHandler);
	void Reset();
	void ClearOSD();
	void OnParentMove();
	bool ShowOSD(LPCTSTR pszText,unsigned int Flags=0);
	void HideOSD();
	bool ShowChannelOSD(const CChannelInfo *pInfo,bool fChanging=false);
	void HideChannelOSD();
	bool ShowVolumeOSD(int Volume);
	void HideVolumeOSD();

private:
	const COSDOptions *m_pOptions;
	CEventHandler *m_pEventHandler;
	CPseudoOSD m_OSD;
	CPseudoOSD m_VolumeOSD;
};


#endif
