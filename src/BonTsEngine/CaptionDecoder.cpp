#include "stdafx.h"
#include "Common.h"
#include "CaptionDecoder.h"
#include "TsTable.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


class CCaptionStream : public CTsPidMapTarget
{
// CTsPidMapTarget
	virtual const bool StorePacket(const CTsPacket *pPacket) {
		m_CaptionParser.StorePacket(pPacket);
		return false;
	}
	virtual void OnPidUnmapped(const WORD wPID) {
		delete this;
	}

	CCaptionParser m_CaptionParser;

public:
	void SetCaptionHandler(CCaptionParser::ICaptionHandler *pHandler) {
		m_CaptionParser.SetCaptionHandler(pHandler);
	}
	void SetDRCSMap(CCaptionParser::IDRCSMap *pDRCSMap) {
		m_CaptionParser.SetDRCSMap(pDRCSMap);
	}
	CCaptionParser *GetParser() { return &m_CaptionParser; }
};




CCaptionDecoder::CCaptionDecoder(IEventHandler *pEventHandler)
	: CMediaDecoder(pEventHandler, 1, 1)
	, m_pCaptionHandler(NULL)
	, m_pDRCSMap(NULL)
	, m_TargetServiceID(0)
	, m_TargetComponentTag(0xFF)
	, m_TargetEsPID(0)
{
	Reset();
}


CCaptionDecoder::~CCaptionDecoder()
{
}


void CCaptionDecoder::Reset()
{
	CBlockLock Lock(&m_DecoderLock);

	m_ServiceList.clear();

	m_TargetEsPID = 0;

	// 全テーブルアンマップ
	m_PidMapManager.UnmapAllTarget();

	// PATテーブルPIDマップ追加
	m_PidMapManager.MapTarget(PID_PAT, new CPatTable, OnPatUpdated, this);
}


const bool CCaptionDecoder::InputMedia(CMediaData *pMediaData, const DWORD dwInputIndex)
{
	CBlockLock Lock(&m_DecoderLock);

	CTsPacket *pTsPacket = static_cast<CTsPacket *>(pMediaData);

	// PIDルーティング
	m_PidMapManager.StorePacket(pTsPacket);

	// 次のフィルタにデータを渡す
	OutputMedia(pMediaData);

	return true;
}


bool CCaptionDecoder::SetTargetStream(WORD ServiceID, BYTE ComponentTag)
{
	CBlockLock Lock(&m_DecoderLock);

	if (m_TargetEsPID != 0) {
		CCaptionStream *pStream = dynamic_cast<CCaptionStream*>(m_PidMapManager.GetMapTarget(m_TargetEsPID));

		if (pStream != NULL) {
			pStream->SetCaptionHandler(NULL);
			pStream->SetDRCSMap(NULL);
		}
		m_TargetEsPID = 0;
	}
	const int Index = GetServiceIndexByID(ServiceID);
	if (Index >= 0) {
		if (ComponentTag == 0xFF) {
			if (m_ServiceList[Index].CaptionEsList.size() > 0)
				m_TargetEsPID = m_ServiceList[Index].CaptionEsList[0].PID;
		} else {
			for (size_t i = 0; i < m_ServiceList[Index].CaptionEsList.size(); i++) {
				if (m_ServiceList[Index].CaptionEsList[i].ComponentTag == ComponentTag) {
					m_TargetEsPID = m_ServiceList[Index].CaptionEsList[i].PID;
					break;
				}
			}
		}
		if (m_TargetEsPID != 0) {
			CCaptionStream *pStream = dynamic_cast<CCaptionStream*>(m_PidMapManager.GetMapTarget(m_TargetEsPID));

			if (pStream != NULL) {
				pStream->SetCaptionHandler(this);
				pStream->SetDRCSMap(m_pDRCSMap);
			}
		}
	}
	m_TargetServiceID = ServiceID;
	m_TargetComponentTag = ComponentTag;

	return true;
}


void CCaptionDecoder::SetCaptionHandler(IHandler *pHandler)
{
	CBlockLock Lock(&m_DecoderLock);

	m_pCaptionHandler = pHandler;
}


void CCaptionDecoder::SetDRCSMap(IDRCSMap *pDRCSMap)
{
	CBlockLock Lock(&m_DecoderLock);

	m_pDRCSMap = pDRCSMap;

	CCaptionParser *pParser = GetCurrentCaptionParser();
	if (pParser != NULL)
		pParser->SetDRCSMap(pDRCSMap);
}


int CCaptionDecoder::GetLanguageNum()
{
	CBlockLock Lock(&m_DecoderLock);

	const CCaptionParser *pParser = GetCurrentCaptionParser();
	if (pParser == NULL)
		return 0;

	return pParser->GetLanguageNum();
}


bool CCaptionDecoder::GetLanguageCode(int LanguageTag, char *pCode)
{
	CBlockLock Lock(&m_DecoderLock);

	const CCaptionParser *pParser = GetCurrentCaptionParser();
	if (pParser == NULL)
		return 0;

	return pParser->GetLanguageCode(LanguageTag, pCode);
}


void CCaptionDecoder::OnLanguageUpdate(CCaptionParser *pParser)
{
	if (m_pCaptionHandler)
		m_pCaptionHandler->OnLanguageUpdate(this);
}


void CCaptionDecoder::OnCaption(CCaptionParser *pParser, BYTE Language, LPCTSTR pszText, const CAribString::FormatList *pFormatList)
{
	if (m_pCaptionHandler)
		m_pCaptionHandler->OnCaption(this, Language, pszText, pFormatList);
}


int CCaptionDecoder::GetServiceIndexByID(WORD ServiceID) const
{
	for (size_t i = 0; i < m_ServiceList.size(); i++) {
		if (m_ServiceList[i].ServiceID == ServiceID)
			return (int)i;
	}
	return -1;
}


CCaptionParser *CCaptionDecoder::GetCurrentCaptionParser() const
{
	if (m_TargetEsPID != 0) {
		CCaptionStream *pStream = dynamic_cast<CCaptionStream*>(m_PidMapManager.GetMapTarget(m_TargetEsPID));

		if (pStream != NULL)
			return pStream->GetParser();
	}
	return NULL;
}


void CALLBACK CCaptionDecoder::OnPatUpdated(const WORD wPID, CTsPidMapTarget *pMapTarget, CTsPidMapManager *pMapManager, const PVOID pParam)
{
	CCaptionDecoder *pThis = static_cast<CCaptionDecoder *>(pParam);
	CPatTable *pPatTable = dynamic_cast<CPatTable *>(pMapTarget);
	if (pPatTable == NULL)
		return;

	// 現PMTのPIDをアンマップする
	for (size_t i = 0; i < pThis->m_ServiceList.size(); i++) {
		pMapManager->UnmapTarget(pThis->m_ServiceList[i].PmtPID);
		for (size_t j = 0; j < pThis->m_ServiceList[i].CaptionEsList.size(); j++)
			pMapManager->UnmapTarget(pThis->m_ServiceList[i].CaptionEsList[j].PID);
	}

	pThis->m_ServiceList.resize(pPatTable->GetProgramNum());

	for (size_t i = 0; i < pThis->m_ServiceList.size(); i++) {
		pThis->m_ServiceList[i].ServiceID = pPatTable->GetProgramID((WORD)i);
		pThis->m_ServiceList[i].PmtPID = pPatTable->GetPmtPID((WORD)i);
		pThis->m_ServiceList[i].CaptionEsList.clear();
		pMapManager->MapTarget(pPatTable->GetPmtPID((WORD)i), new CPmtTable, OnPmtUpdated, pParam);
	}
}


void CALLBACK CCaptionDecoder::OnPmtUpdated(const WORD wPID, CTsPidMapTarget *pMapTarget, CTsPidMapManager *pMapManager, const PVOID pParam)
{
	CCaptionDecoder *pThis = static_cast<CCaptionDecoder *>(pParam);
	CPmtTable *pPmtTable = dynamic_cast<CPmtTable *>(pMapTarget);
	if (pPmtTable == NULL)
		return;

	// サービスインデックスを検索
	const int ServiceIndex = pThis->GetServiceIndexByID(pPmtTable->m_CurSection.GetTableIdExtension());
	if (ServiceIndex < 0)
		return;

	ServiceInfo &Info = pThis->m_ServiceList[ServiceIndex];

	Info.CaptionEsList.clear();
	for (WORD EsIndex = 0; EsIndex < pPmtTable->GetEsInfoNum(); EsIndex++) {
		const BYTE StreamType = pPmtTable->GetStreamTypeID(EsIndex);

		if (StreamType == STREAM_TYPE_CAPTION) {
			CaptionEsInfo CaptionInfo;

			CaptionInfo.PID = pPmtTable->GetEsPID(EsIndex);
			CaptionInfo.ComponentTag = 0xFF;
			const CDescBlock *pDescBlock = pPmtTable->GetItemDesc(EsIndex);
			if (pDescBlock) {
				const CStreamIdDesc *pStreamIdDesc = dynamic_cast<const CStreamIdDesc*>(pDescBlock->GetDescByTag(CStreamIdDesc::DESC_TAG));

				if (pStreamIdDesc)
					CaptionInfo.ComponentTag = pStreamIdDesc->GetComponentTag();
			}
			Info.CaptionEsList.push_back(CaptionInfo);

			CCaptionStream *pStream = new CCaptionStream;
			if (Info.ServiceID == pThis->m_TargetServiceID
					&& ((pThis->m_TargetComponentTag == 0xFF && Info.CaptionEsList.size() == 1)
						|| (pThis->m_TargetComponentTag != 0xFF 
							&& CaptionInfo.ComponentTag == pThis->m_TargetComponentTag))) {
				TRACE(TEXT("Select caption PID %d\n"), CaptionInfo.PID);
				pStream->SetCaptionHandler(pThis);
				pStream->SetDRCSMap(pThis->m_pDRCSMap);
			}
			pMapManager->MapTarget(CaptionInfo.PID, pStream);
		}
	}
}




CCaptionDecoder::IHandler::~IHandler()
{
}
