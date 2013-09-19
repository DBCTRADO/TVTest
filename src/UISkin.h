#ifndef UI_SKIN_H
#define UI_SKIN_H


#include "BonTsEngine/Exception.h"


class CUICore;

class ABSTRACT_CLASS(CUISkin)
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
		TIMER_ID_USER=256
	};

	struct PanAndScanInfo {
		int XPos,YPos;
		int Width,Height;
		int XFactor,YFactor;
		int XAspect,YAspect;

		bool operator==(const PanAndScanInfo &Op) const {
			return XPos==Op.XPos && YPos==Op.YPos
				&& Width==Op.Width && Height==Op.Height
				&& XFactor==Op.XFactor && YFactor==Op.YFactor
				&& XAspect==Op.XAspect && YAspect==Op.YAspect;
		}
		bool operator!=(const PanAndScanInfo &Op) const { return !(*this==Op); }
	};

	CUICore *m_pCore;
	bool m_fWheelChannelChanging;

	virtual bool InitializeViewer() = 0;
	virtual bool FinalizeViewer() = 0;
	virtual bool EnableViewer(bool fEnable) = 0;
	virtual bool IsViewerEnabled() const = 0;
	virtual HWND GetViewerWindow() const = 0;
	virtual bool SetZoomRate(int Rate,int Factor) = 0;
	virtual bool GetZoomRate(int *pRate,int *pFactor) = 0;
	virtual bool SetPanAndScan(const PanAndScanInfo &Info) = 0;
	virtual bool GetPanAndScan(PanAndScanInfo *pInfo) const = 0;
	virtual void OnVolumeChanged(bool fOSD) {}
	virtual void OnMuteChanged() {}
	virtual void OnStereoModeChanged() {}
	virtual void OnAudioStreamChanged() {}
	virtual bool OnStandbyChange(bool fStandby) { return true; }
	virtual bool OnFullscreenChange(bool fFullscreen) = 0;
	virtual bool SetAlwaysOnTop(bool fTop) = 0;
	virtual void OnTunerChanged() {}
	virtual void OnTunerOpened() {}
	virtual void OnTunerClosed() {}
	virtual void OnChannelListChanged() {}
	virtual void OnChannelChanged(unsigned int Status) {}
	virtual void OnServiceChanged() {}
	virtual void OnRecordingStarted() {}
	virtual void OnRecordingStopped() {}

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
	bool IsWheelChannelChanging() const { return m_fWheelChannelChanging; }

	friend CUICore;
};


#endif
