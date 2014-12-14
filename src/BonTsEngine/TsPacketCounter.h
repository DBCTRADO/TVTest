#pragma once


#include <vector>
#include "MediaDecoder.h"
#include "TsStream.h"
#include "TsUtilClass.h"


class CTsPacketCounter : public CMediaDecoder
{
public:
	CTsPacketCounter(IEventHandler *pEventHandler = NULL);
	virtual ~CTsPacketCounter();

// CMediaDecoder
	void Reset() override;
	const bool InputMedia(CMediaData *pMediaData, const DWORD dwInputIndex = 0UL) override;
	bool SetActiveServiceID(WORD ServiceID) override;
	WORD GetActiveServiceID() const override;

// CTsPacketCounter
	UINT64 GetInputPacketCount() const;
	void ResetInputPacketCount();
	UINT64 GetScrambledPacketCount() const;
	void ResetScrambledPacketCount();

protected:
	struct ServiceInfo
	{
		WORD ServiceID;
		WORD PmtPID;
		std::vector<WORD> EsPidList;
	};

	class CEsPidMapTarget : public CTsPidMapTarget
	{
	public:
		CEsPidMapTarget(CTsPacketCounter *pTsPacketCounter);
		const bool StorePacket(const CTsPacket *pPacket) override;

	private:
		CTsPacketCounter *m_pTsPacketCounter;
	};

	CTsPidMapManager m_PidMapManager;
	CEsPidMapTarget m_EsPidMapTarget;

	std::vector<ServiceInfo> m_ServiceList;
	WORD m_TargetServiceID;

	CUInt64Counter m_InputPacketCount;
	CUInt64Counter m_ScrambledPacketCount;

	int GetServiceIndexByID(WORD ServiceID) const;
	void MapServiceESs(size_t Index);
	void UnmapServiceESs(size_t Index);

	static void CALLBACK OnPatUpdated(const WORD wPID, CTsPidMapTarget *pMapTarget, CTsPidMapManager *pMapManager, const PVOID pParam);
	static void CALLBACK OnPmtUpdated(const WORD wPID, CTsPidMapTarget *pMapTarget, CTsPidMapManager *pMapManager, const PVOID pParam);
};
