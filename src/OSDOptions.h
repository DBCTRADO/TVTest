#ifndef OSD_OPTIONS_H
#define OSD_OPTIONS_H


#include "Options.h"


class COSDOptions : public COptions
{
public:
	enum ChannelChangeType {
		CHANNELCHANGE_LOGOANDTEXT,
		CHANNELCHANGE_TEXTONLY,
		CHANNELCHANGE_LOGOONLY,
		CHANNELCHANGE_FIRST	=CHANNELCHANGE_LOGOANDTEXT,
		CHANNELCHANGE_LAST	=CHANNELCHANGE_LOGOONLY
	};

	enum OSDType {
		OSD_CHANNEL,
		OSD_VOLUME,
		OSD_AUDIO,
		OSD_RECORDING,
		OSD_CHANNELNOINPUT,
		OSD_TRAILER_
	};

	enum {
		NOTIFY_EVENTNAME	=0x00000001,
		NOTIFY_ECMERROR		=0x00000002
	};

	COSDOptions();
	~COSDOptions();
// CSettingsBase
	bool ReadSettings(CSettings &Settings) override;
	bool WriteSettings(CSettings &Settings) override;
// CBasicDialog
	bool Create(HWND hwndOwner) override;
// COSDOptions
	bool GetShowOSD() const { return m_fShowOSD; }
	bool GetPseudoOSD() const { return m_fPseudoOSD; }
	COLORREF GetTextColor() const { return m_TextColor; }
	int GetOpacity() const { return m_Opacity; }
	const LOGFONT *GetOSDFont() const { return &m_OSDFont; }
	int GetFadeTime() const { return m_FadeTime; }
	ChannelChangeType GetChannelChangeType() const { return m_ChannelChangeType; }
	bool GetLayeredWindow() const;
	void OnDwmCompositionChanged();
	bool IsOSDEnabled(OSDType Type) const;
	bool IsNotificationBarEnabled() const { return m_fEnableNotificationBar; }
	int GetNotificationBarDuration() const { return m_NotificationBarDuration; }
	const LOGFONT *GetNotificationBarFont() const { return &m_NotificationBarFont; }
	bool IsNotifyEnabled(unsigned int Type) const;
	const LOGFONT *GetDisplayFont() const { return &m_DisplayFont; }
	bool IsDisplayFontAutoSize() const { return m_fDisplayFontAutoSize; }

private:
// CBasicDialog
	INT_PTR DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam) override;

	static UINT_PTR CALLBACK ChooseFontHookProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);

	void EnableNotify(unsigned int Type,bool fEnabled);

	bool m_fShowOSD;
	bool m_fPseudoOSD;
	COLORREF m_TextColor;
	COLORREF m_CurTextColor;
	int m_Opacity;
	LOGFONT m_OSDFont;
	LOGFONT m_CurOSDFont;
	int m_FadeTime;
	ChannelChangeType m_ChannelChangeType;
	unsigned int m_EnabledOSD;

	bool m_fLayeredWindow;
	bool m_fCompositionEnabled;

	bool m_fEnableNotificationBar;
	int m_NotificationBarDuration;
	unsigned int m_NotificationBarFlags;
	LOGFONT m_NotificationBarFont;
	LOGFONT m_CurNotificationBarFont;

	LOGFONT m_DisplayFont;
	LOGFONT m_DisplayFontCur;
	bool m_fDisplayFontAutoSize;
};


#endif
