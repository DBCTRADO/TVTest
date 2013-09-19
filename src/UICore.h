#ifndef UI_CORE_H
#define UI_CORE_H


#include <vector>
#include "UISkin.h"


class CBasicDialog;

class CUICore
{
public:
	enum {
		POPUPMENU_DEFAULT = 0x0001U
	};

	enum MenuType {
		MENU_TUNERSELECT,
		MENU_RECORD,
		MENU_CAPTURE,
		MENU_BUFFERING,
		MENU_STREAMERROR,
		MENU_CLOCK,
		MENU_PROGRAMINFO
	};

	typedef CUISkin::PanAndScanInfo PanAndScanInfo;

	enum {
		CHANNEL_CHANGED_STATUS_SPACE_CHANGED = 0x0001U,
		CHANNEL_CHANGED_STATUS_DETECTED      = 0x0002U
	};

	CUICore();
	virtual ~CUICore();
	bool SetSkin(CUISkin *pSkin);
	CUISkin *GetSkin() const { return m_pSkin; }
	HWND GetMainWindow() const;

	bool InitializeViewer();
	bool IsViewerInitializeError() const { return m_fViewerInitializeError; }
	bool FinalizeViewer();
	bool IsViewerEnabled() const;
	bool EnableViewer(bool fEnable);
	HWND GetViewerWindow() const;

	bool SetZoomRate(int Rate,int Factor=100);
	bool GetZoomRate(int *pRate,int *pFactor) const;
	int GetZoomPercentage() const;
	bool GetPanAndScan(PanAndScanInfo *pInfo) const;
	bool SetPanAndScan(const PanAndScanInfo &Info);

	int GetVolume() const;
	bool SetVolume(int Volume,bool fOSD=true);
	bool GetMute() const;
	bool SetMute(bool fMute);
	int GetStereoMode() const;
	bool SetStereoMode(int StereoMode);
	int GetAudioStream() const;
	int GetNumAudioStreams() const;
	bool SetAudioStream(int Stream);
	bool SwitchStereoMode();
	bool SwitchAudio();
	int FormatCurrentAudioText(LPTSTR pszText,int MaxLength) const;

	bool GetStandby() const { return m_fStandby; }
	bool SetStandby(bool fStandby);
	bool GetResident() const;
	bool SetResident(bool fResident);
	bool GetFullscreen() const { return m_fFullscreen; }
	bool SetFullscreen(bool fFullscreen);
	bool ToggleFullscreen();
	bool GetAlwaysOnTop() const { return m_fAlwaysOnTop; }
	bool SetAlwaysOnTop(bool fTop);
	bool PreventDisplaySave(bool fPrevent);

	void PopupMenu(const POINT *pPos=NULL,UINT Flags=0);
	void PopupSubMenu(int SubMenu,const POINT *pPos=NULL,UINT Flags=0);
	bool ShowSpecialMenu(MenuType Menu,const POINT *pPos=NULL,UINT Flags=0);
	void InitChannelMenu(HMENU hmenu);
#ifdef NETWORK_REMOCON_SUPPORT
	void InitNetworkRemoconChannelMenu(HMENU hmenu);
#endif
	void InitTunerMenu(HMENU hmenu);
	bool ProcessTunerMenu(int Command);

	bool DoCommand(int Command);
	bool DoCommand(LPCTSTR pszCommand);

	bool ConfirmStopRecording();

	bool UpdateIcon();
	bool UpdateTitle();

	bool RegisterModelessDialog(CBasicDialog *pDialog);
	bool UnregisterModelessDialog(CBasicDialog *pDialog);
	bool ProcessDialogMessage(MSG *pMessage);

	void OnTunerChanged();
	void OnTunerOpened();
	void OnTunerClosed();
	void OnChannelListChanged();
	void OnChannelChanged(unsigned int Status);
	void OnServiceChanged();
	void OnRecordingStarted();
	void OnRecordingStopped();

private:
	CUISkin *m_pSkin;
	bool m_fStandby;
	bool m_fFullscreen;
	bool m_fAlwaysOnTop;

	HICON m_hicoLogoBig;
	HICON m_hicoLogoSmall;

	bool m_fViewerInitializeError;

	BOOL m_fScreenSaverActiveOriginal;
	/*
	BOOL m_fLowPowerActiveOriginal;
	BOOL m_fPowerOffActiveOriginal;
	*/

	std::vector<CBasicDialog*> m_ModelessDialogList;
};


#endif
