#ifndef COMMAND_LINE_H
#define COMMAND_LINE_H


#include <vector>


class CCommandLineOptions
{
public:
	CCommandLineOptions();
	void Parse(LPCWSTR pszCmdLine);
	bool IsChannelSpecified() const;

	TVTest::String m_IniFileName;
	TVTest::String m_DriverName;
	TVTest::String m_CasLibraryName;
	bool m_fNoDriver;
	bool m_fNoDescramble;
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
	int m_TvRockDID;

	int m_Channel;
	int m_ControllerChannel;
	int m_TuningSpace;
	int m_ServiceID;
	int m_NetworkID;
	int m_TransportStreamID;

	bool m_fUseNetworkRemocon;
	DWORD m_UDPPort;

	bool m_fRecord;
	bool m_fRecordStop;
	FILETIME m_RecordStartTime;
	int m_RecordDelay;
	int m_RecordDuration;
	TVTest::String m_RecordFileName;
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
	static const int INVALID_WINDOW_POS=INT_MIN;

	int m_Volume;
	bool m_fMute;

	bool m_fNoPlugin;
	std::vector<TVTest::String> m_NoLoadPlugins;
	TVTest::String m_PluginsDirectory;

	bool m_fShowProgramGuide;
	bool m_fProgramGuideOnly;
	TVTest::String m_ProgramGuideTuner;
	TVTest::String m_ProgramGuideSpace;

	bool m_fHomeDisplay;
	bool m_fChannelDisplay;

	TVTest::String m_StyleFileName;

	struct IniEntry {
		TVTest::String Section;
		TVTest::String Name;
		TVTest::String Value;
	};
	std::vector<IniEntry> m_IniValueList;
};


#endif
