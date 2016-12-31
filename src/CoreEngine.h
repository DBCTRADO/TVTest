#ifndef CORE_ENGINE_H
#define CORE_ENGINE_H


#include "DtvEngine.h"
#include "TSProcessor.h"
#include "EpgProgramList.h"


class CCoreEngine : public CBonErrorHandler
{
public:
	enum TSProcessorConnectPosition {
		TSPROCESSOR_CONNECTPOSITION_SOURCE,
		TSPROCESSOR_CONNECTPOSITION_PREPROCESSING,
		TSPROCESSOR_CONNECTPOSITION_POSTPROCESSING,
		TSPROCESSOR_CONNECTPOSITION_VIEWER,
		TSPROCESSOR_CONNECTPOSITION_RECORDER
	};

	enum DriverType {
		DRIVER_UNKNOWN,
		DRIVER_UDP,
		DRIVER_TCP
	};

	enum { MAX_VOLUME=100 };

	struct PanAndScanInfo
	{
		int XPos,YPos;
		int Width,Height;
		int XFactor,YFactor;
		int XAspect,YAspect;

		bool operator==(const PanAndScanInfo &Op) const {
			return XPos==Op.XPos && YPos==Op.YPos
				&& Width==Op.Width && Height==Op.Height
				&& XFactor==Op.XFactor && YFactor==Op.YFactor
				&& XAspect==Op.XAspect && YAspect==Op.YAspect;
		}
		bool operator!=(const PanAndScanInfo &Op) const { return !(*this==Op); }
	};

	CCoreEngine();
	~CCoreEngine();
	void Close();
	bool BuildDtvEngine(CDtvEngine::CEventHandler *pEventHandler);
	bool RegisterTSProcessor(TVTest::CTSProcessor *pTSProcessor,
							 TSProcessorConnectPosition ConnectPosition);
	size_t GetTSProcessorCount() const { return m_TSProcessorList.size(); }
	TVTest::CTSProcessor *GetTSProcessorByIndex(size_t Index);
	void EnableTSProcessor(bool fEnable);
	bool IsTSProcessorEnabled() const { return m_fEnableTSProcessor; }
	bool BuildMediaViewer(HWND hwndHost,HWND hwndMessage,
		CVideoRenderer::RendererType VideoRenderer,
		BYTE VideoStreamType,LPCWSTR pszVideoDecoder=NULL,
		LPCWSTR pszAudioDevice=NULL);
	bool CloseMediaViewer();
	bool EnableMediaViewer(bool fEnable);

	LPCTSTR GetDriverDirectory() const { return m_szDriverDirectory; }
	bool GetDriverDirectory(LPTSTR pszDirectory,int MaxLength) const;
	bool SetDriverDirectory(LPCTSTR pszDirectory);
	bool SetDriverFileName(LPCTSTR pszFileName);
	LPCTSTR GetDriverFileName() const { return m_szDriverFileName; }
	bool GetDriverPath(LPTSTR pszPath,int MaxLength) const;
	bool IsDriverSpecified() const { return m_szDriverFileName[0]!='\0'; }
	bool OpenTuner();
	bool CloseTuner();
	bool IsTunerOpen() const;
	DriverType GetDriverType() const { return m_DriverType; }
	bool IsUDPDriver() const { return m_DriverType==DRIVER_UDP; }
	bool IsTCPDriver() const { return m_DriverType==DRIVER_TCP; }
	bool IsNetworkDriver() const { return IsUDPDriver() || IsTCPDriver(); }

	bool SetPacketBufferLength(DWORD BufferLength);
	bool GetPacketBuffering() const { return m_fPacketBuffering; }
	bool SetPacketBufferPool(bool fBuffering,int Percentage);
	void ResetPacketBuffer();

	bool GetVideoViewSize(int *pWidth,int *pHeight);
	int GetOriginalVideoWidth() const { return m_OriginalVideoWidth; }
	int GetOriginalVideoHeight() const { return m_OriginalVideoHeight; }
	int GetDisplayVideoWidth() const { return m_DisplayVideoWidth; }
	int GetDisplayVideoHeight() const { return m_DisplayVideoHeight; }
	bool SetPanAndScan(const PanAndScanInfo &Info);
	bool GetPanAndScan(PanAndScanInfo *pInfo) const;

	bool SetVolume(int Volume);
	int GetVolume() const { return m_Volume; }
	bool SetMute(bool fMute);
	bool GetMute() const { return m_fMute; }
	bool SetAudioGainControl(int Gain,int SurroundGain);
	bool GetAudioGainControl(int *pGain,int *pSurroundGain) const;
	bool SetDualMonoMode(CAudioDecFilter::DualMonoMode Mode);
	CAudioDecFilter::DualMonoMode GetDualMonoMode() const { return m_DualMonoMode; }
	bool SetStereoMode(CAudioDecFilter::StereoMode Mode);
	CAudioDecFilter::StereoMode GetStereoMode() const { return m_StereoMode; }
	bool SetSpdifOptions(const CAudioDecFilter::SpdifOptions &Options);
	bool GetSpdifOptions(CAudioDecFilter::SpdifOptions *pOptions) const;
	bool IsSpdifPassthroughEnabled() const;

	enum {
		STATUS_VIDEOSIZE			=0x00000001UL,
		STATUS_AUDIOCHANNELS		=0x00000002UL,
		STATUS_AUDIOSTREAMS			=0x00000004UL,
		STATUS_AUDIOCOMPONENTTYPE	=0x00000008UL,
		STATUS_SPDIFPASSTHROUGH		=0x00000010UL,
		STATUS_EVENTINFO			=0x00000020UL,
		STATUS_EVENTID				=0x00000040UL,
		STATUS_TOT					=0x00000080UL
	};
	DWORD UpdateAsyncStatus();
	void SetAsyncStatusUpdatedFlag(DWORD Status);
	enum {
		STATISTIC_ERRORPACKETCOUNT				=0x00000001UL,
		STATISTIC_CONTINUITYERRORPACKETCOUNT	=0x00000002UL,
		STATISTIC_SCRAMBLEPACKETCOUNT			=0x00000004UL,
		STATISTIC_SIGNALLEVEL					=0x00000008UL,
		STATISTIC_BITRATE						=0x00000010UL,
		STATISTIC_STREAMREMAIN					=0x00000020UL,
		STATISTIC_PACKETBUFFERRATE				=0x00000040UL
	};
	DWORD UpdateStatistics();
	ULONGLONG GetErrorPacketCount() const { return m_ErrorPacketCount; }
	ULONGLONG GetContinuityErrorPacketCount() const { return m_ContinuityErrorPacketCount; }
	ULONGLONG GetScramblePacketCount() const { return m_ScramblePacketCount; }
	void ResetErrorCount();
	float GetSignalLevel() const { return m_SignalLevel; }
	int GetSignalLevelText(LPTSTR pszText,int MaxLength) const;
	int GetSignalLevelText(float SignalLevel,LPTSTR pszText,int MaxLength) const;
	DWORD GetBitRate() const { return m_BitRate; }
	static float BitRateToFloat(DWORD BitRate) { return (float)BitRate/(float)(1000*1000); }
	float GetBitRateFloat() const { return BitRateToFloat(m_BitRate); }
	int GetBitRateText(LPTSTR pszText,int MaxLength) const;
	int GetBitRateText(DWORD BitRate,LPTSTR pszText,int MaxLength) const;
	int GetBitRateText(float BitRate,LPTSTR pszText,int MaxLength,int Precision=2) const;
	DWORD GetStreamRemain() const { return m_StreamRemain; }
	int GetPacketBufferUsedPercentage() const;
	bool GetCurrentEventInfo(CEventInfoData *pInfo,WORD ServiceID=0xFFFF,bool fNext=false);
	void *GetCurrentImage();
	bool SetMinTimerResolution(bool fMin);
	bool SetNoEpg(bool fNoEpg) { m_fNoEpg=fNoEpg; return true; }

//private:
	CDtvEngine m_DtvEngine;

private:
	struct TSProcessorInfo
	{
		TVTest::CTSProcessor *pTSProcessor;
		TSProcessorConnectPosition ConnectPosition;
		int DecoderID;
	};

	struct TSProcessorConnectionList
	{
		std::vector<CDtvEngine::DecoderConnectionInfo> List;

		void Add(int OutputDecoder,int InputDecoder,int OutputIndex=0)
		{
			List.push_back(CDtvEngine::DecoderConnectionInfo(OutputDecoder,InputDecoder,OutputIndex));
		}
	};

	TCHAR m_szDriverDirectory[MAX_PATH];
	TCHAR m_szDriverFileName[MAX_PATH];
	DriverType m_DriverType;

	std::vector<TSProcessorInfo> m_TSProcessorList;
	bool m_fEnableTSProcessor;

	bool m_fPacketBuffering;

	int m_OriginalVideoWidth;
	int m_OriginalVideoHeight;
	int m_DisplayVideoWidth;
	int m_DisplayVideoHeight;
	int m_NumAudioChannels;
	int m_NumAudioStreams;
	BYTE m_AudioComponentType;
	bool m_fMute;
	int m_Volume;
	int m_AudioGain;
	int m_SurroundAudioGain;
	CAudioDecFilter::DualMonoMode m_DualMonoMode;
	CAudioDecFilter::StereoMode m_StereoMode;
	CAudioDecFilter::SpdifOptions m_SpdifOptions;
	bool m_fSpdifPassthrough;
	ULONGLONG m_ErrorPacketCount;
	ULONGLONG m_ContinuityErrorPacketCount;
	ULONGLONG m_ScramblePacketCount;
	float m_SignalLevel;
	DWORD m_BitRate;
	DWORD m_StreamRemain;
	int m_PacketBufferFillPercentage;
	UINT m_TimerResolution;
	bool m_fNoEpg;

	DWORD m_AsyncStatusUpdatedFlags;

	void ConnectTSProcessor(TSProcessorConnectionList *pList,
							TSProcessorConnectPosition ConnectPosition,
							int *pDecoderID,int *pOutputIndex=NULL);
};


#endif
