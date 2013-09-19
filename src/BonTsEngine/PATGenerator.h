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
	static void CALLBACK OnPmtUpdated(const WORD wPID, CTsPidMapTarget *pMapTarget, CTsPidMapManager *pMapManager, const PVOID pParam);

	WORD m_TransportStreamID;
	bool m_bHasPAT;
	CTsPidMapManager m_PidMapManager;
	BYTE m_ContinuityCounter;
	bool m_bUpdated;
};


#endif
