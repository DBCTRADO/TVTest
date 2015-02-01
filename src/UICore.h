#ifndef UI_CORE_H
#define UI_CORE_H


#include <vector>
#include "UISkin.h"
#include "AudioManager.h"


class CAppMain;
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
		PANANDSCAN_DEFAULT,
		PANANDSCAN_16x9,
		PANANDSCAN_LETTERBOX,
		PANANDSCAN_SUPERFRAME,
		PANANDSCAN_SIDECUT,
		PANANDSCAN_4x3,
		PANANDSCAN_32x9,
		PANANDSCAN_16x9_LEFT,
		PANANDSCAN_16x9_RIGHT,
		PANANDSCAN_CUSTOM
	};

	CUICore(CAppMain &App);
	~CUICore();
	bool SetSkin(CUISkin *pSkin);
	CUISkin *GetSkin() const { return m_pSkin; }
	HWND GetMainWindow() const;
	HWND GetDialogOwner() const;

	bool InitializeViewer(BYTE VideoStreamType=0);
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
	bool SetDualMonoMode(CAudioDecFilter::DualMonoMode Mode);
	CAudioDecFilter::DualMonoMode GetDualMonoMode() const;
	CAudioDecFilter::DualMonoMode GetActualDualMonoMode() const;
	bool SetStereoMode(CAudioDecFilter::StereoMode Mode);
	CAudioDecFilter::StereoMode GetStereoMode() const;
	int GetNumAudioStreams() const;
	int GetAudioStream() const;
	bool SetAudioStream(int Stream);
	bool SelectAudio(int Index);
	bool AutoSelectAudio();
	bool SwitchAudio();
	bool SwitchDualMonoMode();
	int FormatCurrentAudioText(LPTSTR pszText,int MaxLength) const;
	bool GetSelectedAudioText(LPTSTR pszText,int MaxLength) const;

	bool GetStandby() const { return m_fStandby; }
	bool SetStandby(bool fStandby,bool fTransient=false);
	bool GetTransientStandby() const { return m_fTransientStandby; }
	bool GetResident() const;
	bool SetResident(bool fResident);
	bool GetFullscreen() const { return m_fFullscreen; }
	bool SetFullscreen(bool fFullscreen);
	bool ToggleFullscreen();
	bool GetAlwaysOnTop() const { return m_fAlwaysOnTop; }
	bool SetAlwaysOnTop(bool fTop);
	bool PreventDisplaySave(bool fPrevent);

	void PopupMenu(const POINT *pPos=NULL,UINT Flags=0);
	void PopupSubMenu(int SubMenu,const POINT *pPos=NULL,UINT Flags=0,
					  const RECT *pExcludeRect=NULL);
	bool ShowSpecialMenu(MenuType Menu,const POINT *pPos=NULL,UINT Flags=0,
						 const RECT *pExcludeRect=NULL);
	void InitChannelMenu(HMENU hmenu);
	void InitTunerMenu(HMENU hmenu);
	bool ProcessTunerMenu(int Command);

	bool DoCommand(int Command);
	bool DoCommand(LPCTSTR pszCommand);
	bool DoCommandAsync(int Command);
	bool DoCommandAsync(LPCTSTR pszCommand);

	bool SetCommandEnabledState(int Command,bool fEnabled);
	bool GetCommandEnabledState(int Command) const;
	bool SetCommandCheckedState(int Command,bool fChecked);
	bool GetCommandCheckedState(int Command) const;

	bool ConfirmChannelChange();
	bool ConfirmStopRecording();

	bool UpdateIcon();
	bool UpdateTitle();
	bool SetLogo(LPCTSTR pszFileName);

	bool RegisterModelessDialog(CBasicDialog *pDialog);
	bool UnregisterModelessDialog(CBasicDialog *pDialog);
	bool ProcessDialogMessage(MSG *pMessage);

	COLORREF GetColor(LPCTSTR pszText) const;

	bool ShowHelpContent(int ID);

	void SetProgress(int Pos,int Max);
	void EndProgress();

private:
	CAppMain &m_App;
	CUISkin *m_pSkin;
	bool m_fStandby;
	bool m_fTransientStandby;
	bool m_fResident;
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

	bool SelectAudio(const TVTest::CAudioManager::AudioSelectInfo &Info,bool fUpdate=true);
	bool SelectAudioStream(int Stream);
	bool SelectDualMonoMode(CAudioDecFilter::DualMonoMode Mode,bool fUpdate=true);
};


#endif
