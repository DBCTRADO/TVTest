#ifndef PLAYBACK_OPTIONS_H
#define PLAYBACK_OPTIONS_H


#include "Options.h"


class CPlaybackOptions
	: public COptions
{
public:
	CPlaybackOptions();
	~CPlaybackOptions();

// COptions
	bool Apply(DWORD Flags) override;

// CSettingsBase
	bool ReadSettings(CSettings &Settings) override;
	bool WriteSettings(CSettings &Settings) override;

// CBasicDialog
	bool Create(HWND hwndOwner) override;

// CPlaybackOptions
	bool GetRestoreMute() const { return m_fRestoreMute; }
	bool IsMuteOnStartUp() const { return m_fRestoreMute && m_fMute; }
	bool GetRestorePlayStatus() const { return m_fRestorePlayStatus; }
	bool GetRestore1SegMode() const { return m_fRestore1SegMode; }
	bool Is1SegModeOnStartup() const { return m_fRestore1SegMode && m_f1SegMode; }
	bool GetUseAudioRendererClock() const { return m_fUseAudioRendererClock; }
	bool GetAdjustAudioStreamTime() const { return m_fAdjustAudioStreamTime; }
	bool GetMinTimerResolution() const { return m_fMinTimerResolution; }
	bool GetPacketBuffering() const { return m_fPacketBuffering; }
	void SetPacketBuffering(bool fBuffering);
	DWORD GetPacketBufferLength() const { return m_PacketBufferLength; }
	int GetPacketBufferPoolPercentage() const { return m_PacketBufferPoolPercentage; }
	int GetStreamThreadPriority() const { return m_StreamThreadPriority; }

private:
	enum {
		UPDATE_ADJUSTAUDIOSTREAMTIME = 0x00000001UL,
		UPDATE_PTSSYNC               = 0x00000002UL,
		UPDATE_PACKETBUFFERING       = 0x00000004UL,
		UPDATE_STREAMTHREADPRIORITY  = 0x00000008UL,
		UPDATE_ADJUSTFRAMERATE       = 0x00000010UL
	};
	enum {
		MAX_PACKET_BUFFER_LENGTH = 0x00100000UL
	};

	bool m_fRestoreMute;
	bool m_fMute;
	bool m_fRestorePlayStatus;
	bool m_fRestore1SegMode;
	bool m_f1SegMode;

	bool m_fUseAudioRendererClock;
	bool m_fEnablePTSSync;
	bool m_fAdjustAudioStreamTime;
	bool m_fMinTimerResolution;

	bool m_fPacketBuffering;
	DWORD m_PacketBufferLength;
	int m_PacketBufferPoolPercentage;
	int m_StreamThreadPriority;

	bool m_fAdjust1SegFrameRate;

// CBasicDialog
	INT_PTR DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) override;
};


#endif
