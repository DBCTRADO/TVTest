/*
  TVTest
  Copyright(c) 2008-2019 DBCTRADO

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
		CCommandLineOptions();
		void Parse(LPCWSTR pszCmdLine);
		bool IsChannelSpecified() const;

		String m_IniFileName;
		String m_DriverName;
		bool m_fNoDriver;
		bool m_fNoTSProcessor;
		bool m_fSingleTask;
		bool m_fStandby;
		bool m_fNoView;
		bool m_fNoDirectShow;
		bool m_fMpeg2;
		bool m_fH264;
		bool m_fH265;
		bool m_fSilent;
		bool m_fInitialSettings;
		bool m_fSaveLog;
		bool m_fNoEpg;
		bool m_f1Seg;
		bool m_fJumpList;
		int m_TvRockDID;

		int m_Channel;
		int m_ControllerChannel;
		int m_ChannelIndex;
		int m_TuningSpace;
		int m_ServiceID;
		int m_NetworkID;
		int m_TransportStreamID;

		bool m_fUseNetworkRemocon;
		DWORD m_UDPPort;

		bool m_fRecord;
		bool m_fRecordStop;
		SYSTEMTIME m_RecordStartTime;
		int m_RecordDelay;
		int m_RecordDuration;
		String m_RecordFileName;
		bool m_fRecordCurServiceOnly;
		bool m_fExitOnRecordEnd;
		bool m_fRecordOnly;

		bool m_fFullscreen;
		bool m_fMinimize;
		bool m_fMaximize;
		bool m_fTray;
		int m_WindowLeft;
		int m_WindowTop;
		int m_WindowWidth;
		int m_WindowHeight;
		static const int INVALID_WINDOW_POS = INT_MIN;

		int m_Volume;
		bool m_fMute;

		bool m_fNoPlugin;
		std::vector<String> m_NoLoadPlugins;
		String m_PluginsDirectory;

		bool m_fShowProgramGuide;
		bool m_fProgramGuideOnly;
		String m_ProgramGuideTuner;
		String m_ProgramGuideSpace;

		bool m_fHomeDisplay;
		bool m_fChannelDisplay;

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

}	// namespace TVTest


#endif
