#ifndef UI_CORE_H
#define UI_CORE_H


#include <vector>
#include "UISkin.h"
#include "ColorScheme.h"
#include "CoreEngine.h"
#include "AudioManager.h"
#include "Menu.h"
#include "GUIUtil.h"
#include "BonTsEngine/TsUtilClass.h"


class CAppMain;
class CBasicDialog;

class CUICore
	: public CColorSchemeOptions::CEventHandler
	, public CTracer
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
	bool GetPanAndScan(CCoreEngine::PanAndScanInfo *pInfo) const;
	bool SetPanAndScan(const CCoreEngine::PanAndScanInfo &Info);

	int GetVolume() const;
	bool SetVolume(int Volume,bool fOSD=true);
	bool GetMute() const;
	bool SetMute(bool fMute);
	bool SetDualMonoMode(CAudioDecFilter::DualMonoMode Mode,bool fApplyStereo=true);
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
	bool HandleInitMenuPopup(HMENU hmenu);

	bool DoCommand(int Command);
	bool DoCommand(LPCTSTR pszCommand);
	bool DoCommandAsync(int Command);
	bool DoCommandAsync(LPCTSTR pszCommand);

	bool SetCommandEnabledState(int Command,bool fEnabled);
	bool GetCommandEnabledState(int Command) const;
	bool SetCommandCheckedState(int Command,bool fChecked);
	bool GetCommandCheckedState(int Command) const;
	bool SetCommandRadioCheckedState(int FirstCommand,int LastCommand,int CheckedCommand);

	bool ConfirmChannelChange();
	bool ConfirmStopRecording();

	bool UpdateIcon();
	bool UpdateTitle();
	bool SetLogo(LPCTSTR pszFileName);

	bool RegisterModelessDialog(CBasicDialog *pDialog);
	bool UnregisterModelessDialog(CBasicDialog *pDialog);
	bool ProcessDialogMessage(MSG *pMessage);

	COLORREF GetColor(LPCTSTR pszText) const;
	const CColorScheme *GetCurrentColorScheme() const;

	HCURSOR GetActionCursor() const;
	HCURSOR GetLinkCursor() const;

	bool ShowHelpContent(int ID);

	void SetProgress(int Pos,int Max);
	void EndProgress();

	void SetStatusBarTrace(bool fStatusBarTrace);

private:
	class CTunerSelectMenu
	{
	public:
		CTunerSelectMenu(CUICore &UICore);
		~CTunerSelectMenu();
		bool Create(HWND hwnd);
		void Destroy();
		int Show(UINT Flags,int x,int y,const RECT *pExcludeRect=NULL);
		bool OnInitMenuPopup(HMENU hmenu);

	private:
		struct PopupInfo {
			const CChannelList *pChannelList;
			int Command;
			PopupInfo(const CChannelList *pList,int Cmd)
				: pChannelList(pList)
				, Command(Cmd)
			{
			}
		};

		CUICore &m_UICore;
		CPopupMenu m_Menu;
		HWND m_hwnd;
		std::vector<PopupInfo> m_PopupList;
	};

	CAppMain &m_App;
	CUISkin *m_pSkin;
	bool m_fStandby;
	bool m_fTransientStandby;
	bool m_fResident;
	bool m_fFullscreen;
	bool m_fAlwaysOnTop;

	TVTest::CIcon m_LogoIconBig;
	TVTest::CIcon m_LogoIconSmall;

	bool m_fViewerInitializeError;

	BOOL m_fScreenSaverActiveOriginal;
	/*
	BOOL m_fLowPowerActiveOriginal;
	BOOL m_fPowerOffActiveOriginal;
	*/

	CTunerSelectMenu m_TunerSelectMenu;

	std::vector<CBasicDialog*> m_ModelessDialogList;

	const CColorScheme *m_pColorScheme;

	bool m_fStatusBarTrace;

	bool SelectAudio(const TVTest::CAudioManager::AudioSelectInfo &Info,bool fUpdate=true);
	bool SelectAudioStream(int Stream);
	bool SelectDualMonoMode(CAudioDecFilter::DualMonoMode Mode,bool fUpdate=true);

	bool CreateChannelMenu(
		const CChannelList *pChannelList,int CurChannel,
		UINT Command,HMENU hmenu,HWND hwnd,unsigned int Flags=0);
	bool InitChannelMenuPopup(HMENU hmenuParent,HMENU hmenu);

// CColorSchemeOptions::CEventHandler
	bool ApplyColorScheme(const CColorScheme *pColorScheme) override;

// CTracer
	void OnTrace(CTracer::TraceType Type,LPCTSTR pszOutput) override;
};


#endif
