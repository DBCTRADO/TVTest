#include "stdafx.h"
#include <map>
#include "LogoDownloader.h"
#include "TsDownload.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


class CLogoDataModule : public CDataModule
{
public:
	struct ServiceInfo {
		WORD OriginalNetworkID;
		WORD TransportStreamID;
		WORD ServiceID;
	};

	struct LogoInfo {
		BYTE LogoType;
		WORD LogoID;
		std::vector<ServiceInfo> ServiceList;
		WORD DataSize;
		const BYTE *pData;
	};

	class ABSTRACT_CLASS_DECL IEventHandler {
	public:
		virtual ~IEventHandler() {}
		virtual void OnLogoData(const CLogoDataModule *pModule, const LogoInfo *pInfo) = 0;
	};

	CLogoDataModule(DWORD DownloadID, WORD BlockSize, WORD ModuleID, DWORD ModuleSize, BYTE ModuleVersion,
					IEventHandler *pHandler)
		: CDataModule(DownloadID, BlockSize, ModuleID, ModuleSize, ModuleVersion)
		, m_pEventHandler(pHandler)
	{
	}

	bool EnumLogoData()
	{
		if (!IsComplete())
			return false;
		OnComplete(m_pData, m_ModuleSize);
		return true;
	}

private:
// CDataModule
	virtual void OnComplete(const BYTE *pData, const DWORD ModuleSize) override;

	IEventHandler *m_pEventHandler;
};

void CLogoDataModule::OnComplete(const BYTE *pData, const DWORD ModuleSize)
{
	if (ModuleSize < 3)
		return;

	LogoInfo Info;

	Info.LogoType = pData[0];
	if (Info.LogoType > 0x05)
		return;

	const WORD NumberOfLoop = ((WORD)pData[1] << 8) | (WORD)pData[2];

	DWORD Pos = 3;
	for (WORD i = 0; i < NumberOfLoop; i++) {
		if (Pos + 3 >= ModuleSize)
			return;

		Info.LogoID = ((WORD)(pData[Pos + 0] & 0x01) << 8) | (WORD)pData[Pos + 1];
		const BYTE NumberOfServices = pData[Pos + 2];
		Pos += 3;
		if (Pos + 6 * NumberOfServices + 2 >= ModuleSize)
			return;

		Info.ServiceList.resize(NumberOfServices);

		TRACE(TEXT("[%d/%d] Logo ID %04X / %d Services\n"),i+1,NumberOfLoop,Info.LogoID,NumberOfServices);

		for (BYTE j = 0; j < NumberOfServices; j++) {
			Info.ServiceList[j].OriginalNetworkID = ((WORD)pData[Pos + 0] << 8) | (WORD)pData[Pos + 1];
			Info.ServiceList[j].TransportStreamID = ((WORD)pData[Pos + 2] << 8) | (WORD)pData[Pos + 3];
			Info.ServiceList[j].ServiceID = ((WORD)pData[Pos + 4] << 8) | (WORD)pData[Pos + 5];
			Pos += 6;

			TRACE(TEXT("[%d:%2d/%2d] Network ID %04X / TSID %04X / Service ID %04X\n"),
				  i+1,j+1,NumberOfServices,
				  Info.ServiceList[j].OriginalNetworkID,
				  Info.ServiceList[j].TransportStreamID,
				  Info.ServiceList[j].ServiceID);
		}

		const WORD DataSize = ((WORD)pData[Pos + 0] << 8) | (WORD)pData[Pos + 1];
		Pos += 2;
		if (Pos + DataSize > ModuleSize)
			return;

		if (NumberOfServices > 0 && DataSize > 0) {
			Info.DataSize = DataSize;
			Info.pData = &pData[Pos];

			m_pEventHandler->OnLogoData(this, &Info);
		}

		Pos += DataSize;
	}
}


class CDsmccSection : public CPsiStreamTable
					, public CDownloadInfoIndicationParser::IEventHandler
					, public CDownloadDataBlockParser::IEventHandler
					, public CLogoDataModule::IEventHandler
{
public:
	typedef void (CALLBACK *LogoDataCallback)(CLogoDownloader::LogoData *pData, DWORD DownloadID, void *pParam);

	CDsmccSection(LogoDataCallback pCallback, void *pCallbackParam
#ifdef _DEBUG
				  , WORD PID
#endif
				  );
	~CDsmccSection();
	bool EnumLogoData(DWORD DownloadID);

private:
// CPsiStreamTable
	virtual const bool OnTableUpdate(const CPsiSection *pCurSection) override;

// CDownloadInfoIndicationParser::IEventHandler
	virtual void OnDataModule(const CDownloadInfoIndicationParser::MessageInfo *pMessageInfo,
							  const CDownloadInfoIndicationParser::ModuleInfo *pModuleInfo) override;

// CDownloadDataBlockParser::IEventHandler
	virtual void OnDataBlock(const CDownloadDataBlockParser::DataBlockInfo *pDataBlock) override;

// CLogoDataModule::IEventHandler
	virtual void OnLogoData(const CLogoDataModule *pModule, const CLogoDataModule::LogoInfo *pInfo) override;

	CDownloadInfoIndicationParser m_DII;
	CDownloadDataBlockParser m_DDB;

	typedef std::map<WORD, CLogoDataModule*> LogoDataMap;
	LogoDataMap m_LogoDataMap;

	LogoDataCallback m_pLogoDataCallback;
	void *m_pLogoDataCallbackParam;
#ifdef _DEBUG
	const WORD m_PID;
#endif
};

CDsmccSection::CDsmccSection(LogoDataCallback pCallback, void *pCallbackParam
#ifdef _DEBUG
							 , WORD PID
#endif
							 )
	: CPsiStreamTable(NULL, true, true)
	, m_DII(this)
	, m_DDB(this)
	, m_pLogoDataCallback(pCallback)
	, m_pLogoDataCallbackParam(pCallbackParam)
#ifdef _DEBUG
	, m_PID(PID)
#endif
{
}

CDsmccSection::~CDsmccSection()
{
	for (LogoDataMap::iterator itr = m_LogoDataMap.begin(); itr != m_LogoDataMap.end(); ++itr) {
		delete itr->second;
	}
}

bool CDsmccSection::EnumLogoData(DWORD DownloadID)
{
	for (LogoDataMap::iterator itr = m_LogoDataMap.begin(); itr != m_LogoDataMap.end(); ++itr) {
		if (itr->second->GetDownloadID() == DownloadID) {
			if (itr->second->IsComplete()) {
				return itr->second->EnumLogoData();
			}
		}
	}

	return false;
}

const bool CDsmccSection::OnTableUpdate(const CPsiSection *pCurSection)
{
	const WORD DataSize = pCurSection->GetPayloadSize();
	const BYTE *pData = pCurSection->GetPayloadData();

	if (pCurSection->GetTableID() == 0x3B) {
		// DII
		return m_DII.ParseData(pData, DataSize);
	} else if (pCurSection->GetTableID() == 0x3C) {
		// DDB
		return m_DDB.ParseData(pData, DataSize);
	}

	return false;
}

void CDsmccSection::OnDataModule(const CDownloadInfoIndicationParser::MessageInfo *pMessageInfo,
								 const CDownloadInfoIndicationParser::ModuleInfo *pModuleInfo)
{
	if (!pModuleInfo->ModuleDesc.Name.pText
			|| (pModuleInfo->ModuleDesc.Name.Length != 7
				&& pModuleInfo->ModuleDesc.Name.Length != 10)
			|| (pModuleInfo->ModuleDesc.Name.Length == 7
				&& ::StrCmpNA(pModuleInfo->ModuleDesc.Name.pText, "LOGO-0", 6) != 0)
			|| (pModuleInfo->ModuleDesc.Name.Length == 10
				&& ::StrCmpNA(pModuleInfo->ModuleDesc.Name.pText, "CS_LOGO-0", 9) != 0))
		return;

#ifdef _DEBUG
	TRACE(TEXT("DII Logo Data [PID %04x] : Download ID %08lX / Module ID %04X / Module size %lu\n"),
		  m_PID, pMessageInfo->DownloadID, pModuleInfo->ModuleID, pModuleInfo->ModuleSize);
#endif

	LogoDataMap::iterator itr = m_LogoDataMap.find(pModuleInfo->ModuleID);
	if (itr == m_LogoDataMap.end()) {
		m_LogoDataMap.insert(std::pair<WORD, CLogoDataModule*>(pModuleInfo->ModuleID,
			new CLogoDataModule(pMessageInfo->DownloadID, pMessageInfo->BlockSize,
								pModuleInfo->ModuleID, pModuleInfo->ModuleSize, pModuleInfo->ModuleVersion, this)));
	} else if (itr->second->GetDownloadID() != pMessageInfo->DownloadID
			|| itr->second->GetBlockSize() != pMessageInfo->BlockSize
			|| itr->second->GetModuleSize() != pModuleInfo->ModuleSize
			|| itr->second->GetModuleVersion() != pModuleInfo->ModuleVersion) {
		delete itr->second;
		m_LogoDataMap[pModuleInfo->ModuleID] = new CLogoDataModule(
			pMessageInfo->DownloadID, pMessageInfo->BlockSize,
			pModuleInfo->ModuleID, pModuleInfo->ModuleSize, pModuleInfo->ModuleVersion,
			this);
	}
}

void CDsmccSection::OnDataBlock(const CDownloadDataBlockParser::DataBlockInfo *pDataBlock)
{
	LogoDataMap::iterator itr = m_LogoDataMap.find(pDataBlock->ModuleID);
	if (itr != m_LogoDataMap.end()) {
		if (itr->second->GetDownloadID() == pDataBlock->DownloadID
				&& itr->second->GetModuleVersion() == pDataBlock->ModuleVersion) {
			itr->second->StoreBlock(pDataBlock->BlockNumber, pDataBlock->pData, pDataBlock->DataSize);
		}
	}
}

void CDsmccSection::OnLogoData(const CLogoDataModule *pModule, const CLogoDataModule::LogoInfo *pInfo)
{
	CLogoDownloader::LogoData LogoData;

	LogoData.OriginalNetworkID = pInfo->ServiceList[0].OriginalNetworkID;
	LogoData.ServiceList.resize(pInfo->ServiceList.size());
	for (size_t i = 0; i < pInfo->ServiceList.size(); i++) {
		LogoData.ServiceList[i].OriginalNetworkID = pInfo->ServiceList[i].OriginalNetworkID;
		LogoData.ServiceList[i].TransportStreamID = pInfo->ServiceList[i].TransportStreamID;
		LogoData.ServiceList[i].ServiceID = pInfo->ServiceList[i].ServiceID;
	}
	LogoData.LogoID = pInfo->LogoID;
	LogoData.LogoVersion = 0;
	LogoData.LogoType = pInfo->LogoType;
	LogoData.DataSize = pInfo->DataSize;
	LogoData.pData = pInfo->pData;

	m_pLogoDataCallback(&LogoData, pModule->GetDownloadID(), m_pLogoDataCallbackParam);
}




CLogoDownloader::CLogoDownloader(IEventHandler *pEventHandler)
	: CMediaDecoder(pEventHandler, 1, 1)
	, m_pLogoHandler(NULL)
{
	Reset();
}


CLogoDownloader::~CLogoDownloader()
{
}


void CLogoDownloader::Reset()
{
	CBlockLock Lock(&m_DecoderLock);

	m_PidMapManager.UnmapAllTarget();
	m_PidMapManager.MapTarget(PID_PAT, new CPatTable, OnPatUpdated, this);
	m_PidMapManager.MapTarget(PID_CDT, new CCdtTable(this));
	m_PidMapManager.MapTarget(PID_SDTT, new CSdttTable(this));
	m_PidMapManager.MapTarget(PID_TOT, new CTotTable);

	m_ServiceList.clear();

	m_VersionMap.clear();
}


const bool CLogoDownloader::InputMedia(CMediaData *pMediaData, const DWORD dwInputIndex)
{
	CBlockLock Lock(&m_DecoderLock);

	CTsPacket *pTsPacket = static_cast<CTsPacket *>(pMediaData);

	m_PidMapManager.StorePacket(pTsPacket);

	// 次のフィルタにデータを渡す
	OutputMedia(pMediaData);

	return true;
}


void CLogoDownloader::SetLogoHandler(ILogoHandler *pHandler)
{
	CBlockLock Lock(&m_DecoderLock);

	m_pLogoHandler = pHandler;
}


void CLogoDownloader::OnSection(CPsiStreamTable *pTable, const CPsiSection *pSection)
{
	const BYTE TableID = pSection->GetTableID();

	if (TableID == CCdtTable::TABLE_ID) {
		// CDTからロゴ取得
		const CCdtTable *pCdtTable = dynamic_cast<const CCdtTable*>(pTable);

		if (pCdtTable
				&& pCdtTable->GetDataType() == CCdtTable::DATATYPE_LOGO
				&& m_pLogoHandler) {
			const WORD DataSize = pCdtTable->GetDataModuleSize();
			const BYTE *pData = pCdtTable->GetDataModuleByte();

			if (DataSize > 7 && pData) {
				LogoData Data;

				Data.OriginalNetworkID = pCdtTable->GetOriginalNetworkId();
				Data.LogoID = ((WORD)(pData[1] & 0x01) << 8) | (WORD)pData[2];
				Data.LogoVersion = ((WORD)(pData[3] & 0x0F) << 8) | (WORD)pData[4];
				Data.LogoType = pData[0];
				Data.DataSize = ((WORD)pData[5] << 8) | (WORD)pData[6];
				Data.pData = &pData[7];
				if (Data.LogoType <= 0x05
						&& Data.DataSize <= DataSize - 7) {
					GetTotTime(&Data.Time);
					m_pLogoHandler->OnLogoDownloaded(&Data);
				}
			}
		}
	} else if (TableID == CSdttTable::TABLE_ID) {
		// SDTTからバージョンを取得
		// (SDTTを元にダウンロードするのが本来だと思うが、SDTTがあまり流れて来ないのでこのような形になっている)
		const CSdttTable *pSdttTable = dynamic_cast<const CSdttTable*>(pTable);

		if (pSdttTable && pSdttTable->IsCommon()) {
			std::vector<DWORD> UpdatedDownloadIDList;

			const CSdttTable::ContentInfo *pInfo;
			for (BYTE i = 0; (pInfo = pSdttTable->GetContentInfo(i)) != NULL; i++) {
				const CBaseDesc *pDesc;
				for (WORD j = 0; (pDesc = pInfo->DescBlock.GetDescByIndex(j)) != NULL; j++) {
					if (pDesc->GetTag() == CDownloadContentDesc::DESC_TAG) {
						const CDownloadContentDesc *pDownloadContentDesc =
							dynamic_cast<const CDownloadContentDesc *>(pDesc);
						if (pDownloadContentDesc) {
							const DWORD DownloadID = pDownloadContentDesc->GetDownloadID();
							TRACE(TEXT("Download version 0x%x = 0x%03x\n"), DownloadID, pInfo->NewVersion);
							std::map<DWORD, WORD>::iterator itrVersion = m_VersionMap.find(DownloadID);
							if (itrVersion == m_VersionMap.end()
									|| itrVersion->second != pInfo->NewVersion) {
								m_VersionMap[DownloadID] = pInfo->NewVersion;
								UpdatedDownloadIDList.push_back(DownloadID);
							}
						}
					}
				}
			}

			if (!UpdatedDownloadIDList.empty()) {
				for (std::vector<ServiceInfo>::iterator itrService = m_ServiceList.begin();
						itrService != m_ServiceList.end(); ++itrService) {
					for (size_t i = 0; i < itrService->EsList.size(); i++) {
						CDsmccSection *pDsmccSection =
							dynamic_cast<CDsmccSection*>(m_PidMapManager.GetMapTarget(itrService->EsList[i]));
						if (pDsmccSection) {
							for (size_t j = 0; j < UpdatedDownloadIDList.size(); j++)
								pDsmccSection->EnumLogoData(UpdatedDownloadIDList[j]);
						}
					}
				}
			}
		}
	}
}


void CALLBACK CLogoDownloader::OnLogoDataModule(LogoData *pData, DWORD DownloadID, void *pParam)
{
	CLogoDownloader *pThis = static_cast<CLogoDownloader*>(pParam);

	if (pThis->m_pLogoHandler) {
		std::map<DWORD, WORD>::iterator itrVersion = pThis->m_VersionMap.find(DownloadID);
		if (itrVersion != pThis->m_VersionMap.end())
			pData->LogoVersion = itrVersion->second;

		pThis->GetTotTime(&pData->Time);

		pThis->m_pLogoHandler->OnLogoDownloaded(pData);
	}
}


void CALLBACK CLogoDownloader::OnPatUpdated(const WORD wPID, CTsPidMapTarget *pMapTarget, CTsPidMapManager *pMapManager, const PVOID pParam)
{
	// PATが更新された
	CLogoDownloader *pThis = static_cast<CLogoDownloader *>(pParam);
	CPatTable *pPatTable = dynamic_cast<CPatTable *>(pMapTarget);
	if (pPatTable == NULL)
		return;

	for (size_t i = 0; i < pThis->m_ServiceList.size(); i++) {
		pMapManager->UnmapTarget(pThis->m_ServiceList[i].PmtPID);
		if (pThis->m_ServiceList[i].ServiceType == SERVICE_TYPE_ENGINEERING) {
			pThis->UnmapDataEs((int)i);
		}
	}

	pThis->m_ServiceList.resize(pPatTable->GetProgramNum());

	for (size_t i = 0; i < pThis->m_ServiceList.size(); i++) {
		pThis->m_ServiceList[i].ServiceID = pPatTable->GetProgramID((WORD)i);
		pThis->m_ServiceList[i].PmtPID = pPatTable->GetPmtPID((WORD)i);
		pThis->m_ServiceList[i].ServiceType = SERVICE_TYPE_INVALID;
		pThis->m_ServiceList[i].EsList.clear();

		pMapManager->MapTarget(pPatTable->GetPmtPID((WORD)i), new CPmtTable, OnPmtUpdated, pParam);
	}

	pMapManager->MapTarget(PID_NIT, new CNitTable, OnNitUpdated, pThis);
}


void CALLBACK CLogoDownloader::OnPmtUpdated(const WORD wPID, CTsPidMapTarget *pMapTarget, CTsPidMapManager *pMapManager, const PVOID pParam)
{
	// PMTが更新された
	CLogoDownloader *pThis = static_cast<CLogoDownloader *>(pParam);
	CPmtTable *pPmtTable = dynamic_cast<CPmtTable *>(pMapTarget);
	if (pPmtTable == NULL)
		return;

	const int ServiceIndex = pThis->GetServiceIndexByID(pPmtTable->m_CurSection.GetTableIdExtension());
	if (ServiceIndex < 0)
		return;
	ServiceInfo &Info = pThis->m_ServiceList[ServiceIndex];

	if (Info.ServiceType == SERVICE_TYPE_ENGINEERING) {
		pThis->UnmapDataEs(ServiceIndex);
	}

	Info.EsList.clear();

	for (WORD EsIndex = 0; EsIndex < pPmtTable->GetEsInfoNum(); EsIndex++) {
		if (pPmtTable->GetStreamTypeID(EsIndex) == STREAM_TYPE_DATACARROUSEL) {
			const CDescBlock *pDescBlock = pPmtTable->GetItemDesc(EsIndex);
			if (pDescBlock) {
				const CStreamIdDesc *pStreamIdDesc = dynamic_cast<const CStreamIdDesc*>(pDescBlock->GetDescByTag(CStreamIdDesc::DESC_TAG));

				if (pStreamIdDesc
						&& (pStreamIdDesc->GetComponentTag() == 0x79
							|| pStreamIdDesc->GetComponentTag() == 0x7A)) {
					// 全受信機共通データ
					Info.EsList.push_back(pPmtTable->GetEsPID(EsIndex));
				}
			}
		}
	}

	if (Info.ServiceType == SERVICE_TYPE_ENGINEERING) {
		pThis->MapDataEs(ServiceIndex);
	}
}


void CALLBACK CLogoDownloader::OnNitUpdated(const WORD wPID, CTsPidMapTarget *pMapTarget, CTsPidMapManager *pMapManager, const PVOID pParam)
{
	// NITが更新された
	CLogoDownloader *pThis = static_cast<CLogoDownloader*>(pParam);
	CNitTable *pNitTable = dynamic_cast<CNitTable*>(pMapTarget);
	if (pNitTable == NULL)
		return;

	for (WORD i = 0; i < pNitTable->GetTransportStreamNum(); i++) {
		const CDescBlock *pDescBlock = pNitTable->GetItemDesc(i);

		if (pDescBlock) {
			const CServiceListDesc *pServiceListDesc = dynamic_cast<const CServiceListDesc*>(pDescBlock->GetDescByTag(CServiceListDesc::DESC_TAG));
			if (pServiceListDesc) {
				for (int j = 0; j < pServiceListDesc->GetServiceNum(); j++) {
					CServiceListDesc::ServiceInfo Info;

					if (pServiceListDesc->GetServiceInfo(j, &Info)) {
						int Index = pThis->GetServiceIndexByID(Info.ServiceID);
						if (Index >= 0) {
							const BYTE ServiceType = Info.ServiceType;
							if (pThis->m_ServiceList[Index].ServiceType != ServiceType) {
								if (ServiceType == SERVICE_TYPE_ENGINEERING) {
									pThis->MapDataEs(Index);
								} else if (pThis->m_ServiceList[Index].ServiceType == SERVICE_TYPE_ENGINEERING) {
									pThis->UnmapDataEs(Index);
								}
								pThis->m_ServiceList[Index].ServiceType = ServiceType;
							}
						}
					}
				}
			}
		}
	}
}


int CLogoDownloader::GetServiceIndexByID(const WORD ServiceID) const
{
	for (size_t i = 0; i < m_ServiceList.size(); i++) {
		if (m_ServiceList[i].ServiceID == ServiceID)
			return (int)i;
	}
	return -1;
}


bool CLogoDownloader::MapDataEs(const int Index)
{
	if (Index < 0 || (size_t)Index >= m_ServiceList.size())
		return false;

	ServiceInfo &Info = m_ServiceList[Index];

	TRACE(TEXT("CLogoDownloader::MapDataEs() : SID %04X / %lu stream(s)\n"),
		  Info.ServiceID, (ULONG)Info.EsList.size());

	for (size_t i = 0; i < Info.EsList.size(); i++) {
		m_PidMapManager.MapTarget(Info.EsList[i],
								  new CDsmccSection(OnLogoDataModule, this
#ifdef _DEBUG
													, Info.EsList[i]
#endif
													));
	}

	return true;
}


bool CLogoDownloader::UnmapDataEs(const int Index)
{
	if (Index < 0 || (size_t)Index >= m_ServiceList.size())
		return false;

	ServiceInfo &Info = m_ServiceList[Index];
	for (size_t i = 0; i < Info.EsList.size(); i++) {
		m_PidMapManager.UnmapTarget(Info.EsList[i]);
	}

	return true;
}


bool CLogoDownloader::GetTotTime(SYSTEMTIME *pTime)
{
	const CTotTable *pTotTable = dynamic_cast<const CTotTable*>(m_PidMapManager.GetMapTarget(PID_TOT));
	if (pTotTable == NULL || !pTotTable->GetDateTime(pTime)) {
		::ZeroMemory(pTime,sizeof(SYSTEMTIME));
		return false;
	}
	return true;
}
