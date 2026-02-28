/*
  TVTest
  Copyright(c) 2008-2020 DBCTRADO

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


#ifndef TVTEST_PLAYBACK_OPTIONS_H
#define TVTEST_PLAYBACK_OPTIONS_H


#include "Options.h"


namespace TVTest
{

	class CPlaybackOptions
		: public COptions
	{
	public:
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

		static constexpr DWORD MAX_PACKET_BUFFER_LENGTH = 0x00100000UL;

		bool m_fRestoreMute = false;
		bool m_fMute = false;
		bool m_fRestorePlayStatus = false;
		bool m_fRestore1SegMode = false;
		bool m_f1SegMode = false;

		bool m_fUseAudioRendererClock = true;
		bool m_fEnablePTSSync = true;
		bool m_fAdjustAudioStreamTime = true;
		bool m_fMinTimerResolution = true;

		bool m_fPacketBuffering = false;
		DWORD m_PacketBufferLength = 40000;
		int m_PacketBufferPoolPercentage = 50;
		int m_StreamThreadPriority = THREAD_PRIORITY_NORMAL;

		bool m_fAdjust1SegFrameRate = true;

	// CBasicDialog
		INT_PTR DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) override;
	};

} // namespace TVTest


#endif
