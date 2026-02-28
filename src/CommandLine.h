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


#ifndef TVTEST_COMMAND_LINE_H
#define TVTEST_COMMAND_LINE_H


#include <vector>


namespace TVTest
{

	class CCommandLineOptions
	{
	public:
		void Parse(LPCWSTR pszCmdLine);
		bool IsChannelSpecified() const;

		String m_IniFileName;
		String m_DriverName;
		bool m_fNoDriver = false;
		bool m_fNoTSProcessor = false;
		bool m_fSingleTask = false;
		bool m_fStandby = false;
		bool m_fNoView = false;
		bool m_fNoDirectShow = false;
		bool m_fMpeg2 = false;
		bool m_fH264 = false;
		bool m_fH265 = false;
		bool m_fSilent = false;
		bool m_fInitialSettings = false;
		bool m_fSaveLog = false;
		bool m_fNoEpg = false;
		bool m_f1Seg = false;
		bool m_fJumpList = false;
		int m_TvRockDID = -1;

		int m_Channel = 0;
		int m_ControllerChannel = 0;
		int m_ChannelIndex = -1;
		int m_TuningSpace = -1;
		int m_ServiceID = 0;
		int m_NetworkID = 0;
		int m_TransportStreamID = 0;

		DWORD m_UDPPort = 1234;

		bool m_fRecord = false;
		bool m_fRecordStop = false;
		SYSTEMTIME m_RecordStartTime{};
		int m_RecordDelay = 0;
		int m_RecordDuration = 0;
		String m_RecordFileName;
		bool m_fRecordCurServiceOnly = false;
		bool m_fExitOnRecordEnd = false;
		bool m_fRecordOnly = false;

		bool m_fFullscreen = false;
		bool m_fMinimize = false;
		bool m_fMaximize = false;
		bool m_fTray = false;
		static constexpr int INVALID_WINDOW_POS = INT_MIN;
		int m_WindowLeft = INVALID_WINDOW_POS;
		int m_WindowTop = INVALID_WINDOW_POS;
		int m_WindowWidth = 0;
		int m_WindowHeight = 0;

		int m_Volume = -1;
		bool m_fMute = false;

		bool m_fNoPlugin = false;
		std::vector<String> m_NoLoadPlugins;
		String m_PluginsDirectory;

		bool m_fShowProgramGuide = false;
		bool m_fProgramGuideOnly = false;
		String m_ProgramGuideTuner;
		String m_ProgramGuideSpace;

		bool m_fHomeDisplay = false;
		bool m_fChannelDisplay = false;

		String m_StyleFileName;

		struct IniEntry
		{
			String Section;
			String Name;
			String Value;
		};
		std::vector<IniEntry> m_IniValueList;

		String m_Command;
	};

} // namespace TVTest


#endif
