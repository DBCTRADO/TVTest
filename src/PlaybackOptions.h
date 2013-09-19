#ifndef PLAYBACK_OPTIONS_H
#define PLAYBACK_OPTIONS_H


#include "Options.h"
#include "BonTsEngine/MediaViewer.h"


class CPlaybackOptions : public COptions
{
	enum {
		UPDATE_ADJUSTAUDIOSTREAMTIME	= 0x00000001UL,
		UPDATE_PTSSYNC					= 0x00000002UL,
		UPDATE_PACKETBUFFERING			= 0x00000004UL,
		UPDATE_STREAMTHREADPRIORITY		= 0x00000008UL
#ifdef TVH264
		, UPDATE_ADJUSTFRAMERATE		= 0x00000010UL
#endif
	};
	enum {
		MAX_AUDIO_DEVICE_NAME = 128,
		MAX_AUDIO_FILTER_NAME = 128
	};
	enum {
		MAX_PACKET_BUFFER_LENGTH = 0x00100000UL
	};

	CDynamicString m_AudioDeviceName;
	CDynamicString m_AudioFilterName;

	CAudioDecFilter::SpdifOptions m_SpdifOptions;
	bool m_fDownMixSurround;
	bool m_fRestoreMute;
	bool m_fMute;
	bool m_fRestorePlayStatus;

	bool m_fUseAudioRendererClock;
	bool m_fEnablePTSSync;
	bool m_fAdjustAudioStreamTime;
	bool m_fMinTimerResolution;

	bool m_fPacketBuffering;
	DWORD m_PacketBufferLength;
	int m_PacketBufferPoolPercentage;
	int m_StreamThreadPriority;

#ifdef TVH264
	bool m_fAdjustFrameRate;
#endif

// CBasicDialog
	INT_PTR DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam) override;

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
	LPCTSTR GetAudioDeviceName() const { return m_AudioDeviceName.Get(); }
	LPCTSTR GetAudioFilterName() const { return m_AudioFilterName.Get(); }
	const CAudioDecFilter::SpdifOptions &GetSpdifOptions() const { return m_SpdifOptions; }
	bool SetSpdifOptions(const CAudioDecFilter::SpdifOptions &Options);
	bool GetDownMixSurround() const { return m_fDownMixSurround; }
	bool GetRestoreMute() const { return m_fRestoreMute; }
	bool IsMuteOnStartUp() const { return m_fRestoreMute && m_fMute; }
	bool GetRestorePlayStatus() const { return m_fRestorePlayStatus; }
	bool GetUseAudioRendererClock() const { return m_fUseAudioRendererClock; }
	bool GetAdjustAudioStreamTime() const { return m_fAdjustAudioStreamTime; }
	bool GetMinTimerResolution() const { return m_fMinTimerResolution; }
	bool GetPacketBuffering() const { return m_fPacketBuffering; }
	void SetPacketBuffering(bool fBuffering);
	DWORD GetPacketBufferLength() const { return m_PacketBufferLength; }
	int GetPacketBufferPoolPercentage() const { return m_PacketBufferPoolPercentage; }
	int GetStreamThreadPriority() const { return m_StreamThreadPriority; }
};


#endif
