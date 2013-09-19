#pragma once


#include <vector>
#include "MediaDecoder.h"
#include "TsStream.h"
#include "TsTable.h"
#include "TsUtilClass.h"


class CTsSelector : public CMediaDecoder
{
public:
	CTsSelector(IEventHandler *pEventHandler = NULL);
	virtual ~CTsSelector();

// CMediaDecoder
	virtual void Reset(void);
	virtual const bool InputMedia(CMediaData *pMediaData, const DWORD dwInputIndex = 0UL);

// CTsSelector
	enum {
		STREAM_MPEG1VIDEO		= 0x00000001UL,
		STREAM_MPEG2VIDEO		= 0x00000002UL,
		STREAM_CAPTION			= 0x00000004UL,
		STREAM_DATACARROUSEL	= 0x00000008UL,
		STREAM_AAC				= 0x00000010UL,
		STREAM_H264				= 0x00000020UL,
		STREAM_ALL				= 0xFFFFFFFFUL
	};
	bool SetTargetServiceID(WORD ServiceID=0, DWORD Stream=STREAM_ALL);
	WORD GetTargetServiceID() const { return m_TargetServiceID; }
	DWORD GetTargetStream() const { return m_TargetStream; }
	ULONGLONG GetInputPacketCount() const;
	ULONGLONG GetOutputPacketCount() const;

protected:
	bool IsTargetPID(WORD PID) const;
	int GetServiceIndexByID(WORD ServiceID) const;
	bool MakePat(const CTsPacket *pSrcPacket, CTsPacket *pDstPacket);

	static void CALLBACK OnPatUpdated(const WORD wPID, CTsPidMapTarget *pMapTarget, CTsPidMapManager *pMapManager, const PVOID pParam);
	static void CALLBACK OnCatUpdated(const WORD wPID, CTsPidMapTarget *pMapTarget, CTsPidMapManager *pMapManager, const PVOID pParam);
	static void CALLBACK OnPmtUpdated(const WORD wPID, CTsPidMapTarget *pMapTarget, CTsPidMapManager *pMapManager, const PVOID pParam);

	CTsPidMapManager m_PidMapManager;

	WORD m_TargetServiceID;
	WORD m_TargetPmtPID;
	DWORD m_TargetStream;

	struct TAG_PMTPIDINFO {
		WORD ServiceID;
		WORD PmtPID;
		WORD PcrPID;
		std::vector<WORD> EcmPIDs;
		std::vector<WORD> EsPIDs;
	};
	std::vector<TAG_PMTPIDINFO> m_PmtPIDList;
	std::vector<WORD> m_EmmPIDList;

	ULONGLONG m_InputPacketCount;
	ULONGLONG m_OutputPacketCount;

	CTsPacket m_PatPacket;
	WORD m_LastTSID;
	WORD m_LastPmtPID;
	BYTE m_LastVersion;
	BYTE m_Version;
};
