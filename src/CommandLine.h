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
