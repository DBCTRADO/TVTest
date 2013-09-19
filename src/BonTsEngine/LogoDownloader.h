#pragma once


#include <vector>
#include <map>
#include "MediaDecoder.h"
#include "TsTable.h"


/*
	ロゴデータ取得クラス
*/
class CLogoDownloader : public CMediaDecoder
					  , public CPsiStreamTable::ISectionHandler
{
public:
	CLogoDownloader(IEventHandler *pEventHandler = NULL);
	virtual ~CLogoDownloader();

// IMediaDecoder
	virtual void Reset(void) override;
	virtual const bool InputMedia(CMediaData *pMediaData, const DWORD dwInputIndex = 0UL) override;

// CLogoDownloader
	struct LogoService {
		WORD OriginalNetworkID;
		WORD TransportStreamID;
		WORD ServiceID;
	};

	struct LogoData {
		WORD OriginalNetworkID;
		std::vector<LogoService> ServiceList;
		WORD LogoID;
		WORD LogoVersion;
		BYTE LogoType;
		WORD DataSize;
		const BYTE *pData;
		SYSTEMTIME Time;
	};

	class ABSTRACT_CLASS_DECL ILogoHandler
	{
	public:
		virtual ~ILogoHandler() {}
		virtual void OnLogoDownloaded(const LogoData *pData) = 0;
	};

	void SetLogoHandler(ILogoHandler *pHandler);

private:
// CPsiStreamTable::ISectionHandler
	virtual void OnSection(CPsiStreamTable *pTable, const CPsiSection *pSection) override;

	static void CALLBACK OnLogoDataModule(LogoData *pData, DWORD DownloadID, void *pParam);

	static void CALLBACK OnPatUpdated(const WORD wPID, CTsPidMapTarget *pMapTarget, CTsPidMapManager *pMapManager, const PVOID pParam);
	static void CALLBACK OnPmtUpdated(const WORD wPID, CTsPidMapTarget *pMapTarget, CTsPidMapManager *pMapManager, const PVOID pParam);
	static void CALLBACK OnNitUpdated(const WORD wPID, CTsPidMapTarget *pMapTarget, CTsPidMapManager *pMapManager, const PVOID pParam);

	int GetServiceIndexByID(const WORD ServiceID) const;
	bool MapDataEs(const int Index);
	bool UnmapDataEs(const int Index);
	bool GetTotTime(SYSTEMTIME *pTime);

	CTsPidMapManager m_PidMapManager;

	ILogoHandler *m_pLogoHandler;

	struct ServiceInfo {
		WORD ServiceID;
		WORD PmtPID;
		BYTE ServiceType;
		std::vector<WORD> EsList;
	};
	std::vector<ServiceInfo> m_ServiceList;

	std::map<DWORD, WORD> m_VersionMap;
};
