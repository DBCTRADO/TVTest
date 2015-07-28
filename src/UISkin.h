#ifndef UI_SKIN_H
#define UI_SKIN_H


#include "AppEvent.h"
#include "NotificationBar.h"
#include "BonTsEngine/Exception.h"


class CUICore;

class ABSTRACT_CLASS(CUISkin) : public TVTest::CAppEventHandler
{
protected:
	enum {
		TIMER_ID_UPDATE=1,
		TIMER_ID_OSD,
		TIMER_ID_DISPLAY,
		TIMER_ID_WHEELCHANNELCHANGE,
		TIMER_ID_PROGRAMLISTUPDATE,
		TIMER_ID_PROGRAMGUIDEUPDATE,
		TIMER_ID_VIDEOSIZECHANGED,
		TIMER_ID_RESETERRORCOUNT,
		TIMER_ID_HIDETOOLTIP,
		TIMER_ID_CHANNELNO,
		TIMER_ID_HIDECURSOR,
		TIMER_ID_USER=256
	};

	CUICore *m_pCore;
	bool m_fWheelChannelChanging;

	virtual bool InitializeViewer(BYTE VideoStreamType=0) = 0;
	virtual bool FinalizeViewer() = 0;
	virtual bool EnableViewer(bool fEnable) = 0;
	virtual bool IsViewerEnabled() const = 0;
	virtual HWND GetViewerWindow() const = 0;
	virtual bool SetZoomRate(int Rate,int Factor) = 0;
	virtual bool GetZoomRate(int *pRate,int *pFactor) = 0;
	virtual bool SetLogo(HBITMAP hbm) = 0;
	virtual bool SetAlwaysOnTop(bool fTop) = 0;
	virtual bool SetFullscreen(bool fFullscreen) = 0;
	virtual bool SetStandby(bool fStandby) = 0;
	virtual bool ShowVolumeOSD() = 0;

	void SetWheelChannelChanging(bool fChanging,DWORD Delay=0);

public:
	CUISkin();
	virtual ~CUISkin();
	virtual HWND GetMainWindow() const = 0;
	virtual HWND GetVideoHostWindow() const = 0;
	virtual int ShowMessage(LPCTSTR pszText,LPCTSTR pszCaption=NULL,
							UINT Type=MB_OK | MB_ICONEXCLAMATION) const;
	virtual void ShowErrorMessage(LPCTSTR pszText) const;
	virtual void ShowErrorMessage(const CBonErrorHandler *pErrorHandler,
								  LPCTSTR pszTitle=NULL) const;
	virtual void ShowNotificationBar(LPCTSTR pszText,
		CNotificationBar::MessageType Type=CNotificationBar::MESSAGE_INFO,
		DWORD Duration=0,bool fSkippable=false) = 0;
	bool IsWheelChannelChanging() const { return m_fWheelChannelChanging; }

	friend CUICore;
};


#endif
