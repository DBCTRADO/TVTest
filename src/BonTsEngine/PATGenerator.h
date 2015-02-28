#ifndef PAT_GENERATOR_H
#define PAT_GENERATOR_H


#include "TsStream.h"


class CPATGenerator
{
public:
	CPATGenerator();
	~CPATGenerator();
	void Reset();
	bool StorePacket(const CTsPacket *pPacket);
	bool GetPAT(CTsPacket *pPacket);
	bool SetTransportStreamID(WORD TransportStreamID);

protected:
	static void CALLBACK OnNitUpdated(const WORD wPID, CTsPidMapTarget *pMapTarget, CTsPidMapManager *pMapManager, const PVOID pParam);

	WORD m_TransportStreamID;
	bool m_bHasPAT;
	bool m_bGeneratePAT;
	CTsPidMapManager m_PidMapManager;
	BYTE m_ContinuityCounter;
	DWORD m_PmtCount[ONESEG_PMT_PID_NUM];
};


#endif
