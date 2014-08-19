#include "stdafx.h"
#include "Common.h"
#include "TsAnalyzer.h"
#include "TsTable.h"
#ifdef TS_ANALYZER_EIT_SUPPORT
#include "TsEncode.h"
#endif
#include "TsUtil.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

#ifdef _DEBUG
#define TABLE_DEBUG	true
#else
#define TABLE_DEBUG
#endif


const CTsAnalyzer::StreamTypeMap CTsAnalyzer::m_VideoStreamTypeMap[] =
{
	{VIDEO_STREAM_MPEG1,	STREAM_TYPE_MPEG1_VIDEO},
	{VIDEO_STREAM_MPEG2,	STREAM_TYPE_MPEG2_VIDEO},
	{VIDEO_STREAM_MPEG4,	STREAM_TYPE_MPEG4_VISUAL},
	{VIDEO_STREAM_H264,		STREAM_TYPE_H264},
	{VIDEO_STREAM_H265,		STREAM_TYPE_H265},
};


CTsAnalyzer::CTsAnalyzer(IEventHandler *pEventHandler)
	: CMediaDecoder(pEventHandler, 1, 1)

	, m_ViewableVideoStreamTypes(
		0
#ifdef BONTSENGINE_MPEG2_SUPPORT
		| VIDEO_STREAM_MPEG2
#endif
#ifdef BONTSENGINE_H264_SUPPORT
		| VIDEO_STREAM_H264
#endif
#ifdef BONTSENGINE_H265_SUPPORT
		| VIDEO_STREAM_H265
#endif
		)
	, m_bRadioSupport(true)
{
	Reset();
}


CTsAnalyzer::~CTsAnalyzer()
{
}


const bool CTsAnalyzer::InputMedia(CMediaData *pMediaData, const DWORD dwInputIndex)
{
	{
		CBlockLock Lock(&m_DecoderLock);

		/*
		if (dwInputIndex >= GetInputNum())
			return false;

		CTsPacket *pTsPacket = dynamic_cast<CTsPacket *>(pMediaData);

		// 入力メディアデータは互換性がない
		if (!pTsPacket)
			return false;
		*/

		CTsPacket *pTsPacket = static_cast<CTsPacket *>(pMediaData);

		m_DecoderEvent = EVENT_INVALID;

		// PIDルーティング
		m_PidMapManager.StorePacket(pTsPacket);

		// 次のフィルタにデータを渡す
		OutputMedia(pMediaData);
	}

	// イベントが発生していたら通知する
	if (m_DecoderEvent != EVENT_INVALID) {
		SendDecoderEvent((DWORD)m_DecoderEvent);
	}

	return true;
}


void CTsAnalyzer::Reset()
{
	CBlockLock Lock(&m_DecoderLock);

	// イベントハンドラにリセットを通知する
	NotifyResetEvent();

	// 全テーブルアンマップ
	m_PidMapManager.UnmapAllTarget();

	// トランスポートストリームID初期化
	m_TransportStreamID = 0x0000;

	// ネットワークID初期化
	m_NetworkID = 0x0000;

	// SDT更新フラグ初期化
	m_bSdtUpdated = false;

	// NIT更新フラグ初期化
	m_bNitUpdated = false;

	// サービスリストクリア
	m_ServiceList.clear();

	// SDTサービスリストクリア
	m_SdtServiceList.clear();

	// SDT TSリストクリア
	m_SdtTsMap.clear();

	// ネットワークTSリストクリア
	m_NetworkTsList.clear();

	// ネットワーク情報クリア
	m_NitInfo.Clear();

	// PATテーブルPIDマップ追加
	m_PidMapManager.MapTarget(PID_PAT, new CPatTable(TABLE_DEBUG), OnPatUpdated, this);

	// NITテーブルPIDマップ追加
	m_PidMapManager.MapTarget(PID_NIT, new CNitMultiTable, OnNitUpdated, this);

	// SDTテーブルPIDマップ追加
	m_PidMapManager.MapTarget(PID_SDT, new CSdtTableSet, OnSdtUpdated, this);

#ifdef TS_ANALYZER_EIT_SUPPORT
	// H-EITテーブルPIDマップ追加
	m_PidMapManager.MapTarget(PID_HEIT, new CEitPfTable);

#ifdef TS_ANALYZER_L_EIT_SUPPORT
	// L-EITテーブルPIDマップ追加
	m_PidMapManager.MapTarget(PID_LEIT, new CEitPfTable);
#endif
#endif

	// TOTテーブルPIDマップ追加
	m_PidMapManager.MapTarget(PID_TOT, new CTotTable);
}


WORD CTsAnalyzer::GetServiceNum() const
{
	CBlockLock Lock(&m_DecoderLock);

	// サービス数を返す
	return (WORD)m_ServiceList.size();
}


bool CTsAnalyzer::GetServiceID(const int Index, WORD *pServiceID) const
{
	if (pServiceID == NULL)
		return false;

	CBlockLock Lock(&m_DecoderLock);

	// サービスIDを取得する
	if (Index < 0) {
		if (Is1SegStream()) {
			WORD MinPID = 0xFFFF;
			size_t MinIndex;
			for (size_t i = 0; i < m_ServiceList.size(); i++) {
				if (Is1SegPmtPid(m_ServiceList[i].PmtPID)
						&& m_ServiceList[i].PmtPID < MinPID) {
					MinPID = m_ServiceList[i].PmtPID;
					MinIndex = i;
				}
			}
			if (MinPID == 0xFFFF || !m_ServiceList[MinIndex].bIsUpdated)
				return false;
			*pServiceID = m_ServiceList[MinIndex].ServiceID;
		} else {
			if (m_ServiceList.empty() || !m_ServiceList[0].bIsUpdated)
				return false;
			*pServiceID = m_ServiceList[0].ServiceID;
		}
	} else if (Index >= 0 && (size_t)Index < m_ServiceList.size()) {
		*pServiceID = m_ServiceList[Index].ServiceID;
	} else {
		return false;
	}
	return true;
}


int CTsAnalyzer::GetServiceIndexByID(const WORD ServiceID) const
{
	CBlockLock Lock(&m_DecoderLock);

	// プログラムIDからサービスインデックスを検索する
	for (size_t Index = 0 ; Index < m_ServiceList.size() ; Index++) {
		if (m_ServiceList[Index].ServiceID == ServiceID)
			return (int)Index;
	}

	// プログラムIDが見つからない
	return -1;
}


bool CTsAnalyzer::IsViewableService(const int Index) const
{
	CBlockLock Lock(&m_DecoderLock);

	if (Index < 0 || (size_t)Index >= m_ServiceList.size())
		return false;

	const ServiceInfo &Service=m_ServiceList[Index];

	if (!Service.VideoEsList.empty()) {
		for (int i = 0; i < _countof(m_VideoStreamTypeMap); i++) {
			if (Service.VideoEsList[0].StreamType == m_VideoStreamTypeMap[i].StreamType
					&& (m_ViewableVideoStreamTypes & m_VideoStreamTypeMap[i].Flag) != 0) {
				return true;
			}
		}
	} else {
		if (m_bRadioSupport
				&& !Service.AudioEsList.empty())
			return true;
	}

	return false;
}


WORD CTsAnalyzer::GetViewableServiceNum() const
{
	CBlockLock Lock(&m_DecoderLock);
	WORD Count = 0;

	for (size_t i = 0; i < m_ServiceList.size(); i++) {
		if (IsViewableService((int)i))
			Count++;
	}
	return Count;
}


bool CTsAnalyzer::GetViewableServiceID(const int Index, WORD *pServiceID) const
{
	if (pServiceID == NULL)
		return false;

	CBlockLock Lock(&m_DecoderLock);

	int j = 0;
	for (size_t i = 0; i < m_ServiceList.size(); i++) {
		if (IsViewableService((int)i)) {
			if (j == Index) {
				*pServiceID = m_ServiceList[i].ServiceID;
				return true;
			}
			j++;
		}
	}
	return false;
}


bool CTsAnalyzer::GetFirstViewableServiceID(WORD *pServiceID) const
{
	if (pServiceID == NULL)
		return false;

	CBlockLock Lock(&m_DecoderLock);

	if (Is1SegStream())
		return GetServiceID(-1, pServiceID);

	for (size_t i = 0; i < m_ServiceList.size(); i++) {
		if (!m_ServiceList[i].bIsUpdated)
			return false;
		if (IsViewableService((int)i)) {
			*pServiceID = m_ServiceList[i].ServiceID;
			return true;
		}
	}

	return false;
}


int CTsAnalyzer::GetViewableServiceIndexByID(const WORD ServiceID) const
{
	CBlockLock Lock(&m_DecoderLock);

	int j = 0;
	for (size_t i = 0; i < m_ServiceList.size(); i++) {
		if (IsViewableService((int)i)) {
			if (m_ServiceList[i].ServiceID == ServiceID)
				return j;
			j++;
		}
	}
	return -1;
}


bool CTsAnalyzer::GetServiceInfo(const int Index, ServiceInfo *pInfo) const
{
	CBlockLock Lock(&m_DecoderLock);

	if (pInfo != NULL && Index >= 0 && (size_t)Index < m_ServiceList.size()) {
		*pInfo = m_ServiceList[Index];
		return true;
	}
	return false;
}


bool CTsAnalyzer::IsServiceUpdated(const int Index) const
{
	CBlockLock Lock(&m_DecoderLock);

	if (Index >= 0 && (size_t)Index < m_ServiceList.size()) {
		return m_ServiceList[Index].bIsUpdated;
	}
	return false;
}


bool CTsAnalyzer::Is1SegService(const int Index) const
{
	CBlockLock Lock(&m_DecoderLock);

	if (Index >= 0 && (size_t)Index < m_ServiceList.size()) {
		return Is1SegPmtPid(m_ServiceList[Index].PmtPID);
	}
	return false;
}


bool CTsAnalyzer::GetPmtPID(const int Index, WORD *pPmtPID) const
{
	CBlockLock Lock(&m_DecoderLock);

	if (pPmtPID != NULL && Index >= 0 && (size_t)Index < m_ServiceList.size()) {
		*pPmtPID = m_ServiceList[Index].PmtPID;
		return true;
	}
	return false;
}


WORD CTsAnalyzer::GetVideoEsNum(const int Index) const
{
	CBlockLock Lock(&m_DecoderLock);

	if (Index >= 0 && (size_t)Index < m_ServiceList.size())
		return (WORD)m_ServiceList[Index].VideoEsList.size();
	return 0;
}


bool CTsAnalyzer::GetVideoEsInfo(const int Index, const int VideoIndex, EsInfo *pEsInfo) const
{
	if (Index < 0 || VideoIndex < 0 || pEsInfo == NULL)
		return false;

	CBlockLock Lock(&m_DecoderLock);

	if ((size_t)Index < m_ServiceList.size()
			&& (size_t)VideoIndex < m_ServiceList[Index].VideoEsList.size()) {
		*pEsInfo = m_ServiceList[Index].VideoEsList[VideoIndex];
		return true;
	}
	return false;
}


bool CTsAnalyzer::GetVideoEsPID(const int Index, const int VideoIndex, WORD *pVideoPID) const
{
	CBlockLock Lock(&m_DecoderLock);

	if (pVideoPID != NULL
			&& Index >= 0 && (size_t)Index < m_ServiceList.size()
			&& VideoIndex >= 0 && (size_t)VideoIndex < m_ServiceList[Index].VideoEsList.size()) {
		*pVideoPID = m_ServiceList[Index].VideoEsList[VideoIndex].PID;
		return true;
	}
	return false;
}


bool CTsAnalyzer::GetVideoStreamType(const int Index, const int VideoIndex, BYTE *pStreamType) const
{
	CBlockLock Lock(&m_DecoderLock);

	if (pStreamType != NULL
			&& Index >= 0 && (size_t)Index < m_ServiceList.size()
			&& VideoIndex >= 0 && (size_t)VideoIndex < m_ServiceList[Index].VideoEsList.size()) {
		*pStreamType = m_ServiceList[Index].VideoEsList[VideoIndex].StreamType;
		return true;
	}
	return false;
}


BYTE CTsAnalyzer::GetVideoComponentTag(const int Index, const int VideoIndex) const
{
	CBlockLock Lock(&m_DecoderLock);

	if (Index >= 0 && (size_t)Index < m_ServiceList.size()
			&& VideoIndex >= 0 && (size_t)VideoIndex < m_ServiceList[Index].VideoEsList.size())
		return m_ServiceList[Index].VideoEsList[VideoIndex].ComponentTag;
	return COMPONENTTAG_INVALID;
}


int CTsAnalyzer::GetVideoIndexByComponentTag(const int Index, const BYTE ComponentTag) const
{
	CBlockLock Lock(&m_DecoderLock);

	if (Index >= 0 && (size_t)Index < m_ServiceList.size()) {
		for (size_t i = 0; i < m_ServiceList[Index].VideoEsList.size(); i++) {
			if (m_ServiceList[Index].VideoEsList[i].ComponentTag == ComponentTag)
				return (int)i;
		}
	}

	return -1;
}


WORD CTsAnalyzer::GetAudioEsNum(const int Index) const
{
	CBlockLock Lock(&m_DecoderLock);

	if (Index >= 0 && (size_t)Index < m_ServiceList.size())
		return (WORD)m_ServiceList[Index].AudioEsList.size();
	return 0;
}


bool CTsAnalyzer::GetAudioEsInfo(const int Index, const int AudioIndex, EsInfo *pEsInfo) const
{
	if (Index < 0 || AudioIndex < 0 || pEsInfo == NULL)
		return false;

	CBlockLock Lock(&m_DecoderLock);

	if ((size_t)Index < m_ServiceList.size()
			&& (size_t)AudioIndex < m_ServiceList[Index].AudioEsList.size()) {
		*pEsInfo = m_ServiceList[Index].AudioEsList[AudioIndex];
		return true;
	}
	return false;
}


bool CTsAnalyzer::GetAudioEsPID(const int Index, const int AudioIndex, WORD *pAudioPID) const
{
	if (pAudioPID == NULL)
		return false;

	CBlockLock Lock(&m_DecoderLock);

	if (Index >= 0 && (size_t)Index < m_ServiceList.size()
			&& AudioIndex >= 0 && (size_t)AudioIndex < m_ServiceList[Index].AudioEsList.size()) {
		*pAudioPID = m_ServiceList[Index].AudioEsList[AudioIndex].PID;
		return true;
	}
	return false;
}


bool CTsAnalyzer::GetAudioStreamType(const int Index, const int AudioIndex, BYTE *pStreamType) const
{
	if (pStreamType == NULL)
		return false;

	CBlockLock Lock(&m_DecoderLock);

	if (Index >= 0 && (size_t)Index < m_ServiceList.size()
			&& AudioIndex >= 0 && (size_t)AudioIndex < m_ServiceList[Index].AudioEsList.size()) {
		*pStreamType = m_ServiceList[Index].AudioEsList[AudioIndex].StreamType;
		return true;
	}
	return false;
}


BYTE CTsAnalyzer::GetAudioComponentTag(const int Index, const int AudioIndex) const
{
	CBlockLock Lock(&m_DecoderLock);

	if (Index >= 0 && (size_t)Index < m_ServiceList.size()
			&& AudioIndex >= 0 && (size_t)AudioIndex < m_ServiceList[Index].AudioEsList.size()) {
		return m_ServiceList[Index].AudioEsList[AudioIndex].ComponentTag;
	}
	return COMPONENTTAG_INVALID;
}


int CTsAnalyzer::GetAudioIndexByComponentTag(const int Index, const BYTE ComponentTag) const
{
	CBlockLock Lock(&m_DecoderLock);

	if (Index >= 0 && (size_t)Index < m_ServiceList.size()) {
		for (size_t i = 0; i < m_ServiceList[Index].AudioEsList.size(); i++) {
			if (m_ServiceList[Index].AudioEsList[i].ComponentTag == ComponentTag)
				return (int)i;
		}
	}

	return -1;
}


#ifdef TS_ANALYZER_EIT_SUPPORT

BYTE CTsAnalyzer::GetVideoComponentType(const int Index) const
{
	CBlockLock Lock(&m_DecoderLock);

	if (Index >= 0 && (size_t)Index < m_ServiceList.size()) {
		const CDescBlock *pDescBlock = GetHEitItemDesc(Index);

		if (pDescBlock) {
			const CComponentDesc *pComponentDesc = pDescBlock->GetDesc<CComponentDesc>();

			if (pComponentDesc != NULL)
				return pComponentDesc->GetComponentType();
		}
	}
	return 0;
}


BYTE CTsAnalyzer::GetAudioComponentType(const int Index, const int AudioIndex) const
{
	CBlockLock Lock(&m_DecoderLock);

	if (Index >= 0 && (size_t)Index < m_ServiceList.size()
			&& AudioIndex >= 0 && (size_t)AudioIndex < m_ServiceList[Index].AudioEsList.size()) {
		const CDescBlock *pDescBlock = GetHEitItemDesc(Index);

		if (pDescBlock) {
			const CAudioComponentDesc *pAudioDesc =
				GetAudioComponentDescByComponentTag(pDescBlock, m_ServiceList[Index].AudioEsList[AudioIndex].ComponentTag);
			if (pAudioDesc != NULL)
				return pAudioDesc->GetComponentType();
		}
	}
	return 0;
}


int CTsAnalyzer::GetAudioComponentText(const int Index, const int AudioIndex, LPTSTR pszText, int MaxLength) const
{
	if (pszText == NULL || MaxLength < 1)
		return 0;

	pszText[0] = '\0';

	CBlockLock Lock(&m_DecoderLock);

	if (Index >= 0 && (size_t)Index < m_ServiceList.size()
			&& AudioIndex >= 0 && (size_t)AudioIndex < m_ServiceList[Index].AudioEsList.size()) {
		const CDescBlock *pDescBlock = GetHEitItemDesc(Index);

		if (pDescBlock) {
			const CAudioComponentDesc *pAudioDesc =
				GetAudioComponentDescByComponentTag(pDescBlock, m_ServiceList[Index].AudioEsList[AudioIndex].ComponentTag);
			if (pAudioDesc != NULL)
				return pAudioDesc->GetText(pszText, MaxLength);
		}
	}
	return 0;
}

#endif	// TS_ANALYZER_EIT_SUPPORT


WORD CTsAnalyzer::GetCaptionEsNum(const int Index) const
{
	CBlockLock Lock(&m_DecoderLock);

	if (Index >= 0 && (size_t)Index < m_ServiceList.size())
		return (WORD)m_ServiceList[Index].CaptionEsList.size();
	return 0;
}


bool CTsAnalyzer::GetCaptionEsPID(const int Index, const WORD CaptionIndex, WORD *pCaptionPID) const
{
	if (pCaptionPID == NULL)
		return false;

	CBlockLock Lock(&m_DecoderLock);

	if (Index >= 0 && (size_t)Index < m_ServiceList.size()
			&& (size_t)CaptionIndex < m_ServiceList[Index].CaptionEsList.size()) {
		*pCaptionPID = m_ServiceList[Index].CaptionEsList[CaptionIndex].PID;
		return true;
	}
	return false;
}


WORD CTsAnalyzer::GetDataCarrouselEsNum(const int Index) const
{
	CBlockLock Lock(&m_DecoderLock);

	if (Index >= 0 && (size_t)Index < m_ServiceList.size())
		return (WORD)m_ServiceList[Index].DataCarrouselEsList.size();
	return 0;
}


bool CTsAnalyzer::GetDataCarrouselEsPID(const int Index, const WORD DataCarrouselIndex, WORD *pDataCarrouselPID) const
{
	if (pDataCarrouselPID == NULL)
		return false;

	CBlockLock Lock(&m_DecoderLock);

	if (Index >= 0 && (size_t)Index < m_ServiceList.size()
			&& (size_t)DataCarrouselIndex < m_ServiceList[Index].DataCarrouselEsList.size()) {
		*pDataCarrouselPID = m_ServiceList[Index].DataCarrouselEsList[DataCarrouselIndex].PID;
		return true;
	}
	return false;
}


bool CTsAnalyzer::GetPcrPID(const int Index, WORD *pPcrPID) const
{
	if (pPcrPID == NULL)
		return false;

	CBlockLock Lock(&m_DecoderLock);

	if (Index >= 0 && (size_t)Index < m_ServiceList.size()
			&& m_ServiceList[Index].PcrPID != PID_INVALID) {
		*pPcrPID = m_ServiceList[Index].PcrPID;
		return true;
	}
	return false;
}


bool CTsAnalyzer::GetPcrTimeStamp(const int Index, ULONGLONG *pTimeStamp) const
{
	if (pTimeStamp == NULL)
		return false;

	CBlockLock Lock(&m_DecoderLock);

	// PCRを取得する
	if (Index >= 0 && (size_t)Index < m_ServiceList.size()) {
		*pTimeStamp = m_ServiceList[Index].PcrTimeStamp;
		return true;
	}
	return false;
}


int CTsAnalyzer::GetServiceName(const int Index, LPTSTR pszName, const int MaxLength) const
{
	CBlockLock Lock(&m_DecoderLock);

	// サービス名を取得する
	if (Index >= 0 && (size_t)Index < m_ServiceList.size()) {
		if (pszName != NULL && MaxLength > 0)
			::lstrcpyn(pszName, m_ServiceList[Index].szServiceName, MaxLength);
		return ::lstrlen(m_ServiceList[Index].szServiceName);
	}
	return 0;
}


BYTE CTsAnalyzer::GetServiceType(const int Index) const
{
	CBlockLock Lock(&m_DecoderLock);

	if (Index >= 0 && (size_t)Index < m_ServiceList.size())
		return m_ServiceList[Index].ServiceType;
	return SERVICE_TYPE_INVALID;
}


WORD CTsAnalyzer::GetLogoID(const int Index) const
{
	CBlockLock Lock(&m_DecoderLock);

	if (Index >= 0 && (size_t)Index < m_ServiceList.size())
		return m_ServiceList[Index].LogoID;
	return LOGOID_INVALID;
}


bool CTsAnalyzer::GetServiceList(ServiceList *pList) const
{
	if (!pList)
		return false;

	CBlockLock Lock(&m_DecoderLock);

	*pList = m_ServiceList;
	return true;
}


bool CTsAnalyzer::GetViewableServiceList(ServiceList *pList) const
{
	if (!pList)
		return false;

	CBlockLock Lock(&m_DecoderLock);

	pList->clear();
	for (size_t i = 0 ; i < m_ServiceList.size() ; i++) {
		if (IsViewableService((int)i))
			pList->push_back(m_ServiceList[i]);
	}
	return true;
}


WORD CTsAnalyzer::GetTransportStreamID() const
{
	return m_TransportStreamID;
}


WORD CTsAnalyzer::GetNetworkID() const
{
	return m_NetworkID;
}


BYTE CTsAnalyzer::GetBroadcastingID() const
{
	return m_NitInfo.BroadcastingID;
}


int CTsAnalyzer::GetNetworkName(LPTSTR pszName, int MaxLength) const
{
	CBlockLock Lock(&m_DecoderLock);

	if (pszName != NULL && MaxLength > 0)
		::lstrcpyn(pszName, m_NitInfo.szNetworkName, MaxLength);
	return ::lstrlen(m_NitInfo.szNetworkName);
}


BYTE CTsAnalyzer::GetRemoteControlKeyID() const
{
	return m_NitInfo.RemoteControlKeyID;
}


int CTsAnalyzer::GetTsName(LPTSTR pszName,int MaxLength) const
{
	CBlockLock Lock(&m_DecoderLock);

	if (pszName != NULL && MaxLength > 0)
		::lstrcpyn(pszName, m_NitInfo.szTSName, MaxLength);
	return ::lstrlen(m_NitInfo.szTSName);
}


bool CTsAnalyzer::GetSdtServiceList(SdtServiceList *pList) const
{
	if (!pList)
		return false;

	CBlockLock Lock(&m_DecoderLock);

	*pList = m_SdtServiceList;
	return true;
}


bool CTsAnalyzer::GetSdtTsList(SdtTsList *pList) const
{
	if (!pList)
		return false;

	pList->clear();

	CBlockLock Lock(&m_DecoderLock);

	for (auto i = m_SdtTsMap.begin(); i != m_SdtTsMap.end(); i++)
		pList->push_back(i->second);

	return true;
}


bool CTsAnalyzer::GetNetworkTsList(NetworkTsList *pList) const
{
	if (!pList)
		return false;

	CBlockLock Lock(&m_DecoderLock);

	*pList = m_NetworkTsList;
	return true;
}


bool CTsAnalyzer::IsSdtUpdated() const
{
	return m_bSdtUpdated;
}


bool CTsAnalyzer::IsNitUpdated() const
{
	return m_bNitUpdated;
}


bool CTsAnalyzer::IsSdtComplete() const
{
	CBlockLock Lock(&m_DecoderLock);

	if (!m_bSdtUpdated || !m_bNitUpdated)
		return false;

	for (size_t i = 0; i < m_NetworkTsList.size(); i++) {
		const NetworkTsInfo &TsInfo = m_NetworkTsList[i];
		if (m_SdtTsMap.find(GetSdtTsMapKey(TsInfo.OriginalNetworkID, TsInfo.TransportStreamID)) == m_SdtTsMap.end())
			return false;
	}

	return true;
}


bool CTsAnalyzer::Is1SegStream() const
{
	CBlockLock Lock(&m_DecoderLock);

	if (m_ServiceList.empty())
		return false;

	for (size_t i = 0; i < m_ServiceList.size(); i++) {
		if (!Is1SegPmtPid(m_ServiceList[i].PmtPID))
			return false;
	}

	return true;
}


bool CTsAnalyzer::SetVideoStreamTypeViewable(const BYTE StreamType, const bool bViewable)
{
	CBlockLock Lock(&m_DecoderLock);

	for (int i = 0; i < _countof(m_VideoStreamTypeMap); i++) {
		if (m_VideoStreamTypeMap[i].StreamType == StreamType) {
			if (bViewable)
				m_ViewableVideoStreamTypes |= m_VideoStreamTypeMap[i].Flag;
			else
				m_ViewableVideoStreamTypes &= ~m_VideoStreamTypeMap[i].Flag;
			return true;
		}
	}

	return false;
}


bool CTsAnalyzer::IsVideoStreamTypeViewable(const BYTE StreamType) const
{
	CBlockLock Lock(&m_DecoderLock);

	for (int i = 0; i < _countof(m_VideoStreamTypeMap); i++) {
		if (m_VideoStreamTypeMap[i].StreamType == StreamType) {
			return (m_ViewableVideoStreamTypes & m_VideoStreamTypeMap[i].Flag) != 0;
		}
	}

	return false;
}


void CTsAnalyzer::SetRadioSupport(const bool bRadioSupport)
{
	CBlockLock Lock(&m_DecoderLock);

	m_bRadioSupport = bRadioSupport;
}


bool CTsAnalyzer::IsRadioSupport() const
{
	return m_bRadioSupport;
}


bool CTsAnalyzer::Has1SegService() const
{
	CBlockLock Lock(&m_DecoderLock);

	for (size_t i = 0; i < m_ServiceList.size(); i++) {
		if (Is1SegPmtPid(m_ServiceList[i].PmtPID))
			return true;
	}

	return false;
}


bool CTsAnalyzer::GetFirst1SegServiceID(WORD *pServiceID) const
{
	if (pServiceID == NULL)
		return false;

	CBlockLock Lock(&m_DecoderLock);

	WORD MinPID = 0xFFFF;
	size_t MinIndex;
	for (size_t i = 0; i < m_ServiceList.size(); i++) {
		if (Is1SegPmtPid(m_ServiceList[i].PmtPID)
				&& m_ServiceList[i].PmtPID < MinPID) {
			MinPID = m_ServiceList[i].PmtPID;
			MinIndex = i;
		}
	}
	if (MinPID == 0xFFFF)
		return false;
	*pServiceID = m_ServiceList[MinIndex].ServiceID;

	return true;
}


bool CTsAnalyzer::Get1SegServiceIDByIndex(const int Index, WORD *pServiceID) const
{
	if (Index < 0 || Index >= ONESEG_PMT_PID_NUM || pServiceID == NULL)
		return false;

	CBlockLock Lock(&m_DecoderLock);

	WORD ServiceList[ONESEG_PMT_PID_NUM] = {0};

	for (size_t i = 0; i < m_ServiceList.size(); i++) {
		if (Is1SegPmtPid(m_ServiceList[i].PmtPID))
			ServiceList[m_ServiceList[i].PmtPID - ONESEG_PMT_PID_FIRST] = m_ServiceList[i].ServiceID;
	}

	int ServiceCount = 0;
	for (int i = 0; i < ONESEG_PMT_PID_NUM; i++) {
		if (ServiceList[i] != 0) {
			if (ServiceCount == Index) {
				*pServiceID = ServiceList[i];
				return true;
			}
			ServiceCount++;
		}
	}

	return false;
}


#ifdef TS_ANALYZER_EIT_SUPPORT


WORD CTsAnalyzer::GetEventID(const int ServiceIndex, const bool bNext) const
{
	CBlockLock Lock(&m_DecoderLock);

	if (ServiceIndex >= 0 && (size_t)ServiceIndex < m_ServiceList.size()) {
		int Index;
		const CEitPfTable *pEitTable = GetEitPfTableByServiceID(m_ServiceList[ServiceIndex].ServiceID, &Index);
		if (pEitTable)
			return pEitTable->GetEventID(Index, bNext ? 1 : 0);
	}

	return 0;
}


bool CTsAnalyzer::GetEventStartTime(const int ServiceIndex, SYSTEMTIME *pSystemTime, const bool bNext) const
{
	if (pSystemTime == NULL)
		return false;

	CBlockLock Lock(&m_DecoderLock);

	if (ServiceIndex >= 0 && (size_t)ServiceIndex < m_ServiceList.size()) {
		int Index;
		const CEitPfTable *pEitTable = GetEitPfTableByServiceID(m_ServiceList[ServiceIndex].ServiceID, &Index);
		if (pEitTable) {
			const SYSTEMTIME *pStartTime = pEitTable->GetStartTime(Index, bNext ? 1 : 0);
			if (pStartTime) {
				*pSystemTime = *pStartTime;
				return true;
			}
		}
	}

	return false;
}


DWORD CTsAnalyzer::GetEventDuration(const int ServiceIndex, const bool bNext) const
{
	CBlockLock Lock(&m_DecoderLock);

	if (ServiceIndex >= 0 && (size_t)ServiceIndex < m_ServiceList.size()) {
		int Index;
		const CEitPfTable *pEitTable = GetEitPfTableByServiceID(m_ServiceList[ServiceIndex].ServiceID, &Index);
		if (pEitTable) {
			return pEitTable->GetDuration(Index, bNext ? 1 : 0);
		}
	}

	return 0;
}


bool CTsAnalyzer::GetEventTime(const int ServiceIndex, SYSTEMTIME *pTime, DWORD *pDuration, const bool bNext) const
{
	CBlockLock Lock(&m_DecoderLock);

	if (ServiceIndex >= 0 && (size_t)ServiceIndex < m_ServiceList.size()) {
		int Index;
		const CEitPfTable *pEitTable = GetEitPfTableByServiceID(m_ServiceList[ServiceIndex].ServiceID, &Index);
		if (pEitTable) {
			const SYSTEMTIME *pStartTime = pEitTable->GetStartTime(Index, bNext ? 1 : 0);
			if (!pStartTime)
				return false;
			if (pTime)
				*pTime = *pStartTime;
			if (pDuration)
				*pDuration = pEitTable->GetDuration(Index, bNext ? 1 : 0);
			return true;
		}
	}

	return false;
}


int CTsAnalyzer::GetEventName(const int ServiceIndex, LPTSTR pszName, int MaxLength, const bool bNext) const
{
	CBlockLock Lock(&m_DecoderLock);

	const CDescBlock *pDescBlock = GetHEitItemDesc(ServiceIndex, bNext);
	if (pDescBlock) {
		const CShortEventDesc *pShortEvent = pDescBlock->GetDesc<CShortEventDesc>();

		if (pShortEvent)
			return pShortEvent->GetEventName(pszName, MaxLength);
	}

#ifdef TS_ANALYZER_L_EIT_SUPPORT
	pDescBlock = GetLEitItemDesc(ServiceIndex, bNext);
	if (pDescBlock) {
		const CShortEventDesc *pShortEvent = pDescBlock->GetDesc<CShortEventDesc>();

		if (pShortEvent)
			return pShortEvent->GetEventName(pszName, MaxLength);
	}
#endif
	return 0;
}


int CTsAnalyzer::GetEventText(const int ServiceIndex, LPTSTR pszText, int MaxLength, const bool bNext) const
{
	CBlockLock Lock(&m_DecoderLock);

	const CDescBlock *pDescBlock = GetHEitItemDesc(ServiceIndex, bNext);
	if (pDescBlock) {
		const CShortEventDesc *pShortEvent = pDescBlock->GetDesc<CShortEventDesc>();

		if (pShortEvent)
			return pShortEvent->GetEventDesc(pszText, MaxLength);
	}

#ifdef TS_ANALYZER_L_EIT_SUPPORT
	pDescBlock = GetLEitItemDesc(ServiceIndex, bNext);
	if (pDescBlock) {
		const CShortEventDesc *pShortEvent = pDescBlock->GetDesc<CShortEventDesc>();

		if (pShortEvent)
			return pShortEvent->GetEventDesc(pszText, MaxLength);
	}
#endif
	return 0;
}


bool CTsAnalyzer::GetEventVideoInfo(const int ServiceIndex, EventVideoInfo *pInfo, const bool bNext) const
{
	if (pInfo == NULL)
		return false;

	const CDescBlock *pDescBlock = GetHEitItemDesc(ServiceIndex, bNext);
	if (pDescBlock) {
		const CComponentDesc *pComponentDesc = pDescBlock->GetDesc<CComponentDesc>();

		if (pComponentDesc) {
			pInfo->StreamContent = pComponentDesc->GetStreamContent();
			pInfo->ComponentType = pComponentDesc->GetComponentType();
			pInfo->ComponentTag = pComponentDesc->GetComponentTag();
			pInfo->LanguageCode = pComponentDesc->GetLanguageCode();
			pComponentDesc->GetText(pInfo->szText, EventVideoInfo::MAX_TEXT);
			return true;
		}
	}
	return false;
}


static void AudioComponentDescToAudioInfo(const CAudioComponentDesc *pAudioDesc, CTsAnalyzer::EventAudioInfo *pInfo)
{
	pInfo->StreamContent = pAudioDesc->GetStreamContent();
	pInfo->ComponentType = pAudioDesc->GetComponentType();
	pInfo->ComponentTag = pAudioDesc->GetComponentTag();
	pInfo->SimulcastGroupTag = pAudioDesc->GetSimulcastGroupTag();
	pInfo->bESMultiLingualFlag = pAudioDesc->GetESMultiLingualFlag();
	pInfo->bMainComponentFlag = pAudioDesc->GetMainComponentFlag();
	pInfo->QualityIndicator = pAudioDesc->GetQualityIndicator();
	pInfo->SamplingRate = pAudioDesc->GetSamplingRate();
	pInfo->LanguageCode = pAudioDesc->GetLanguageCode();
	pInfo->LanguageCode2 = pAudioDesc->GetLanguageCode2();
	pAudioDesc->GetText(pInfo->szText, CTsAnalyzer::EventAudioInfo::MAX_TEXT);
}

bool CTsAnalyzer::GetEventAudioInfo(const int ServiceIndex, const int AudioIndex, EventAudioInfo *pInfo, bool bNext) const
{
	if (pInfo == NULL)
		return false;

	CBlockLock Lock(&m_DecoderLock);

	if (ServiceIndex >= 0 && (size_t)ServiceIndex < m_ServiceList.size()
			&& AudioIndex >= 0 && (size_t)AudioIndex < m_ServiceList[ServiceIndex].AudioEsList.size()) {
		const CDescBlock *pDescBlock = GetHEitItemDesc(ServiceIndex, bNext);

		if (pDescBlock) {
			const CAudioComponentDesc *pAudioDesc =
				GetAudioComponentDescByComponentTag(pDescBlock, m_ServiceList[ServiceIndex].AudioEsList[AudioIndex].ComponentTag);
			if (pAudioDesc != NULL) {
				AudioComponentDescToAudioInfo(pAudioDesc, pInfo);
				return true;
			}
		}
	}
	return false;
}


bool CTsAnalyzer::GetEventAudioList(const int ServiceIndex, EventAudioList *pList, const bool bNext) const
{
	if (pList == NULL)
		return false;

	const CDescBlock *pDescBlock = GetHEitItemDesc(ServiceIndex, bNext);
	if (pDescBlock) {
		pList->clear();
		for (WORD i = 0; i < pDescBlock->GetDescNum(); i++) {
			const CBaseDesc *pDesc = pDescBlock->GetDescByIndex(i);

			if (pDesc->GetTag() == CAudioComponentDesc::DESC_TAG) {
				const CAudioComponentDesc *pAudioComponent = dynamic_cast<const CAudioComponentDesc*>(pDesc);

				if (pAudioComponent) {
					EventAudioInfo AudioInfo;

					AudioComponentDescToAudioInfo(pAudioComponent, &AudioInfo);
					pList->push_back(AudioInfo);
				}
			}
		}
		return true;
	}
	return false;
}


bool CTsAnalyzer::GetEventContentNibble(const int ServiceIndex, EventContentNibble *pInfo, const bool bNext) const
{
	if (pInfo == NULL)
		return false;

	CBlockLock Lock(&m_DecoderLock);

	const CDescBlock *pDescBlock = GetHEitItemDesc(ServiceIndex, bNext);
	if (pDescBlock != NULL) {
		const CContentDesc *pContentDesc = pDescBlock->GetDesc<CContentDesc>();

		if (pContentDesc) {
			pInfo->NibbleCount = pContentDesc->GetNibbleCount();
			for (int i = 0; i < pInfo->NibbleCount; i++)
				pContentDesc->GetNibble(i, &pInfo->NibbleList[i]);
			return true;
		}
	}
	return false;
}


int CTsAnalyzer::GetEventExtendedText(const int ServiceIndex, LPTSTR pszText, int MaxLength, const bool bUseEventGroup, const bool bNext) const
{
	if (pszText == NULL || MaxLength < 1)
		return 0;

	pszText[0] = '\0';

	CBlockLock Lock(&m_DecoderLock);

	const CDescBlock *pDescBlock = GetHEitItemDesc(ServiceIndex, bNext);
	if (pDescBlock == NULL)
		return 0;
	if (pDescBlock->GetDescByTag(CExtendedEventDesc::DESC_TAG) == NULL) {
		if (!bUseEventGroup)
			return 0;

		// イベント共有の参照先から情報を取得する
		const CEventGroupDesc *pEventGroup = pDescBlock->GetDesc<CEventGroupDesc>();
		if (pEventGroup == NULL
				|| pEventGroup->GetGroupType() != CEventGroupDesc::GROUPTYPE_COMMON
				|| pEventGroup->GetEventNum() < 1)
			return 0;
		const WORD EventID = GetEventID(ServiceIndex, bNext);
		int i;
		// 自己の記述がない場合は参照元
		for (i = 0; i < pEventGroup->GetEventNum(); i++) {
			CEventGroupDesc::EventInfo EventInfo;
			if (pEventGroup->GetEventInfo(i, &EventInfo)
					&& EventInfo.ServiceID == m_ServiceList[ServiceIndex].ServiceID
					&& EventInfo.EventID == EventID)
				return 0;
		}
		const CEitPfTable *pEitTable = dynamic_cast<const CEitPfTable*>(m_PidMapManager.GetMapTarget(PID_HEIT));
		for (i = 0; i < pEventGroup->GetEventNum(); i++) {
			CEventGroupDesc::EventInfo EventInfo;
			if (pEventGroup->GetEventInfo(i, &EventInfo)) {
				int Index = GetServiceIndexByID(EventInfo.ServiceID);
				if (Index >= 0) {
					if (pEitTable->GetEventID(pEitTable->GetServiceIndexByID(EventInfo.ServiceID), bNext ? 1 : 0) != EventInfo.EventID
							|| (pDescBlock = GetHEitItemDesc(Index, bNext)) == NULL
							|| pDescBlock->GetDescByTag(CExtendedEventDesc::DESC_TAG) == NULL)
						return 0;
					break;
				}
			}
		}
		if (i == pEventGroup->GetEventNum())
			return 0;
	}

	std::vector<const CExtendedEventDesc *> DescList;
	for (int i = 0; i < pDescBlock->GetDescNum(); i++) {
		const CBaseDesc *pDesc = pDescBlock->GetDescByIndex(i);
		if (pDesc != NULL && pDesc->GetTag() == CExtendedEventDesc::DESC_TAG) {
			const CExtendedEventDesc *pExtendedEvent = dynamic_cast<const CExtendedEventDesc *>(pDesc);
			if (pExtendedEvent != NULL) {
				DescList.push_back(pExtendedEvent);
			}
		}
	}
	if (DescList.size() == 0)
		return 0;

	// descriptor_number 順にソートする
	TsEngine::InsertionSort(DescList,
		[](const CExtendedEventDesc *pDesc1, const CExtendedEventDesc *pDesc2) {
			return pDesc1->GetDescriptorNumber() < pDesc2->GetDescriptorNumber();
		});

	struct ItemInfo {
		BYTE DescriptorNumber;
		LPCTSTR pszDescription;
		int Data1Length;
		const BYTE *pData1;
		int Data2Length;
		const BYTE *pData2;
	};
	std::vector<ItemInfo> ItemList;
	for (int i = 0; i < (int)DescList.size(); i++) {
		const CExtendedEventDesc *pExtendedEvent = DescList[i];
		for (int j = 0; j < pExtendedEvent->GetItemCount(); j++) {
			const CExtendedEventDesc::ItemInfo *pItem = pExtendedEvent->GetItem(j);
			if (pItem == NULL)
				continue;
			if (pItem->szDescription[0] != '\0') {
				// 新規項目
				ItemInfo Item;
				Item.DescriptorNumber = pExtendedEvent->GetDescriptorNumber();
				Item.pszDescription = pItem->szDescription;
				Item.Data1Length = pItem->ItemLength;
				Item.pData1 = pItem->ItemChar;
				Item.Data2Length = 0;
				Item.pData2 = NULL;
				ItemList.push_back(Item);
			} else if (ItemList.size() > 0) {
				// 前の項目の続き
				ItemInfo &Item = ItemList[ItemList.size() - 1];
				if (Item.DescriptorNumber == pExtendedEvent->GetDescriptorNumber() - 1
						&& Item.pData2 == NULL) {
					Item.Data2Length = pItem->ItemLength;
					Item.pData2 = pItem->ItemChar;
				}
			}
		}
	}

	TCHAR szText[1024];
	int Length;
	int Pos = 0;
	for (int i = 0; i < (int)ItemList.size(); i++) {
		ItemInfo &Item = ItemList[i];
		Length = ::lstrlen(Item.pszDescription);
		if (Length + 2 >= MaxLength - Pos)
			break;
		::lstrcpy(&pszText[Pos], Item.pszDescription);
		Pos += Length;
		pszText[Pos++] = '\r';
		pszText[Pos++] = '\n';
		if (Item.pData2 == NULL) {
			CAribString::AribToString(szText, 1024, Item.pData1, Item.Data1Length);
		} else {
			BYTE Buffer[220 * 2];
			::CopyMemory(Buffer, Item.pData1, Item.Data1Length);
			::CopyMemory(Buffer + Item.Data1Length, Item.pData2, Item.Data2Length);
			CAribString::AribToString(szText, 1024, Buffer, Item.Data1Length + Item.Data2Length);
		}
		LPTSTR p = szText;
		while (*p != '\0') {
			if (Pos >= MaxLength - 1)
				break;
			pszText[Pos++] = *p;
			if (*p == '\r') {
				if (*(p + 1) != '\n') {
					if (Pos == MaxLength - 1)
						break;
					pszText[Pos++] = '\n';
				}
			}
			p++;
		}
		if (Pos + 2 >= MaxLength)
			break;
		pszText[Pos++] = '\r';
		pszText[Pos++] = '\n';
	}
	pszText[Pos] = '\0';

	return Pos;
}


bool CTsAnalyzer::GetEventSeriesInfo(const int ServiceIndex, EventSeriesInfo *pInfo, const bool bNext) const
{
	if (pInfo == NULL)
		return false;

	CBlockLock Lock(&m_DecoderLock);

	const CDescBlock *pDescBlock = GetHEitItemDesc(ServiceIndex, bNext);
	if (pDescBlock != NULL) {
		const CSeriesDesc *pSeriesDesc = pDescBlock->GetDesc<CSeriesDesc>();

		if (pSeriesDesc) {
			pInfo->SeriesID = pSeriesDesc->GetSeriesID();
			pInfo->RepeatLabel = pSeriesDesc->GetRepeatLabel();
			pInfo->ProgramPattern = pSeriesDesc->GetProgramPattern();
			pInfo->bIsExpireDateValid = pSeriesDesc->IsExpireDateValid()
				&& pSeriesDesc->GetExpireDate(&pInfo->ExpireDate);
			pInfo->EpisodeNumber = pSeriesDesc->GetEpisodeNumber();
			pInfo->LastEpisodeNumber = pSeriesDesc->GetLastEpisodeNumber();
			pSeriesDesc->GetSeriesName(pInfo->szSeriesName, CSeriesDesc::MAX_SERIES_NAME);
			return true;
		}
	}
	return false;
}


bool CTsAnalyzer::GetEventInfo(const int ServiceIndex, EventInfo *pInfo, const bool bUseEventGroup, const bool bNext) const
{
	if (pInfo == NULL)
		return false;

	CBlockLock Lock(&m_DecoderLock);

	if (ServiceIndex < 0 || (size_t)ServiceIndex >= m_ServiceList.size())
		return false;

	int Service;
	const CEitPfTable *pEitTable =
		GetEitPfTableByServiceID(m_ServiceList[ServiceIndex].ServiceID, &Service);
	if (pEitTable == NULL)
		return false;

	const DWORD EventIndex = bNext ? 1 : 0;

	pInfo->EventID = pEitTable->GetEventID(Service, EventIndex);
	const SYSTEMTIME *pStartTime = pEitTable->GetStartTime(Service, EventIndex);
	if (pStartTime != NULL) {
		pInfo->bValidStartTime = true;
		pInfo->StartTime = *pStartTime;
	} else {
		pInfo->bValidStartTime = false;
		::ZeroMemory(&pInfo->StartTime, sizeof(SYSTEMTIME));
	}
	pInfo->Duration = pEitTable->GetDuration(Service, EventIndex);
	pInfo->RunningStatus = pEitTable->GetRunningStatus(Service, EventIndex);
	pInfo->bFreeCaMode = pEitTable->GetFreeCaMode(Service, EventIndex);
	if (pInfo->pszEventName != NULL && pInfo->MaxEventName > 0) {
		pInfo->pszEventName[0] = '\0';
		GetEventName(ServiceIndex, pInfo->pszEventName, pInfo->MaxEventName, bNext);
	}
	if (pInfo->pszEventText != NULL && pInfo->MaxEventText > 0) {
		pInfo->pszEventText[0] = '\0';
		GetEventText(ServiceIndex, pInfo->pszEventText, pInfo->MaxEventText, bNext);
	}
	if (pInfo->pszEventExtendedText != NULL && pInfo->MaxEventExtendedText > 0) {
		pInfo->pszEventExtendedText[0] = '\0';
		GetEventExtendedText(ServiceIndex, pInfo->pszEventExtendedText, pInfo->MaxEventExtendedText, bUseEventGroup, bNext);
	}

	::ZeroMemory(&pInfo->Video, sizeof(EventVideoInfo));
	GetEventVideoInfo(ServiceIndex, &pInfo->Video, bNext);

	pInfo->Audio.clear();
	GetEventAudioList(ServiceIndex, &pInfo->Audio, bNext);

	if (!GetEventContentNibble(ServiceIndex, &pInfo->ContentNibble, bNext))
		pInfo->ContentNibble.NibbleCount = 0;

	return true;
}


const CEitPfTable *CTsAnalyzer::GetEitPfTableByServiceID(const WORD ServiceID, int *pIndex) const
{
	int Index = -1;
	const CEitPfTable *pEitTable = dynamic_cast<const CEitPfTable*>(m_PidMapManager.GetMapTarget(PID_HEIT));
	if (pEitTable)
		Index = pEitTable->GetServiceIndexByID(ServiceID);

#ifdef TS_ANALYZER_L_EIT_SUPPORT
	if (Index < 0) {
		pEitTable = dynamic_cast<const CEitPfTable*>(m_PidMapManager.GetMapTarget(PID_LEIT));
		if (pEitTable)
			Index = pEitTable->GetServiceIndexByID(ServiceID);
	}
#endif

	if (Index < 0)
		return NULL;
	if (pIndex)
		*pIndex = Index;
	return pEitTable;
}


const CDescBlock *CTsAnalyzer::GetHEitItemDesc(const int ServiceIndex, const bool bNext) const
{
	if (ServiceIndex >= 0 && (size_t)ServiceIndex < m_ServiceList.size()) {
		const CEitPfTable *pEitTable = dynamic_cast<const CEitPfTable*>(m_PidMapManager.GetMapTarget(PID_HEIT));

		if (pEitTable) {
			int Index = pEitTable->GetServiceIndexByID(m_ServiceList[ServiceIndex].ServiceID);

			if (Index >= 0)
				return pEitTable->GetItemDesc(Index, bNext ? 1 : 0);
		}
	}
	return NULL;
}


#ifdef TS_ANALYZER_L_EIT_SUPPORT
const CDescBlock *CTsAnalyzer::GetLEitItemDesc(const int ServiceIndex, const bool bNext) const
{
	if (ServiceIndex >= 0 && (size_t)ServiceIndex < m_ServiceList.size()) {
		const CEitPfTable *pEitTable = dynamic_cast<const CEitPfTable*>(m_PidMapManager.GetMapTarget(PID_LEIT));

		if (pEitTable) {
			int Index = pEitTable->GetServiceIndexByID(m_ServiceList[ServiceIndex].ServiceID);

			if (Index >= 0)
				return pEitTable->GetItemDesc(Index, bNext ? 1 : 0);
		}
	}
	return NULL;
}
#endif


const CAudioComponentDesc *CTsAnalyzer::GetAudioComponentDescByComponentTag(const CDescBlock *pDescBlock, const BYTE ComponentTag) const
{
	if (pDescBlock != NULL) {
		for (WORD i = 0; i < pDescBlock->GetDescNum(); i++) {
			const CBaseDesc *pDesc = pDescBlock->GetDescByIndex(i);

			if (pDesc->GetTag() == CAudioComponentDesc::DESC_TAG) {
				const CAudioComponentDesc *pAudioDesc = dynamic_cast<const CAudioComponentDesc*>(pDesc);

				if (pAudioDesc != NULL
						&& pAudioDesc->GetComponentTag() == ComponentTag)
					return pAudioDesc;
			}
		}
	}
	return NULL;
}


#endif	// TS_ANALYZER_EIT_SUPPORT


bool CTsAnalyzer::GetTotTime(SYSTEMTIME *pTime) const
{
	if (pTime == NULL)
		return false;

	CBlockLock Lock(&m_DecoderLock);

	const CTotTable *pTotTable = dynamic_cast<const CTotTable*>(m_PidMapManager.GetMapTarget(PID_TOT));
	if (pTotTable)
		return pTotTable->GetDateTime(pTime);
	return false;
}


bool CTsAnalyzer::GetSatelliteDeliverySystemList(SatelliteDeliverySystemList *pList) const
{
	if (pList == NULL)
		return false;

	CBlockLock Lock(&m_DecoderLock);

	pList->clear();

	const CNitMultiTable *pNitMultiTable =
		dynamic_cast<const CNitMultiTable*>(m_PidMapManager.GetMapTarget(PID_NIT));
	if (pNitMultiTable == NULL || !pNitMultiTable->IsNitComplete())
		return false;

	for (WORD SectionNo = 0; SectionNo < pNitMultiTable->GetNitSectionNum(); SectionNo++) {
		const CNitTable *pNitTable = pNitMultiTable->GetNitTable(SectionNo);
		if (pNitTable == NULL)
			continue;

		for (WORD i = 0; i < pNitTable->GetTransportStreamNum(); i++) {
			const CDescBlock *pDescBlock = pNitTable->GetItemDesc(i);
			if (pDescBlock) {
				const CSatelliteDeliverySystemDesc *pSatelliteDesc =
					pDescBlock->GetDesc<CSatelliteDeliverySystemDesc>();
				if (pSatelliteDesc) {
					SatelliteDeliverySystemInfo Info;

					Info.TransportStreamID = pNitTable->GetTransportStreamID(i);
					Info.Frequency = pSatelliteDesc->GetFrequency();
					Info.OrbitalPosition = pSatelliteDesc->GetOrbitalPosition();
					Info.bWestEastFlag = pSatelliteDesc->GetWestEastFlag();
					Info.Polarization = pSatelliteDesc->GetPolarization();
					Info.Modulation = pSatelliteDesc->GetModulation();
					Info.SymbolRate = pSatelliteDesc->GetSymbolRate();
					Info.FECInner = pSatelliteDesc->GetFECInner();
					pList->push_back(Info);
				}
			}
		}
	}

	return !pList->empty();
}


bool CTsAnalyzer::GetTerrestrialDeliverySystemList(TerrestrialDeliverySystemList *pList) const
{
	if (pList == NULL)
		return false;

	CBlockLock Lock(&m_DecoderLock);

	pList->clear();

	const CNitMultiTable *pNitMultiTable =
		dynamic_cast<const CNitMultiTable*>(m_PidMapManager.GetMapTarget(PID_NIT));
	if (pNitMultiTable == NULL || !pNitMultiTable->IsNitComplete())
		return false;

	for (WORD SectionNo = 0; SectionNo < pNitMultiTable->GetNitSectionNum(); SectionNo++) {
		const CNitTable *pNitTable = pNitMultiTable->GetNitTable(SectionNo);
		if (pNitTable == NULL)
			continue;

		for (WORD i = 0; i < pNitTable->GetTransportStreamNum(); i++) {
			const CDescBlock *pDescBlock = pNitTable->GetItemDesc(i);
			if (pDescBlock) {
				const CTerrestrialDeliverySystemDesc *pTerrestrialDesc =
					pDescBlock->GetDesc<CTerrestrialDeliverySystemDesc>();
				if (pTerrestrialDesc) {
					TerrestrialDeliverySystemInfo Info;

					Info.TransportStreamID = pNitTable->GetTransportStreamID(i);
					Info.AreaCode = pTerrestrialDesc->GetAreaCode();
					Info.GuardInterval = pTerrestrialDesc->GetGuardInterval();
					Info.TransmissionMode = pTerrestrialDesc->GetTransmissionMode();
					Info.Frequency.resize(pTerrestrialDesc->GetFrequencyNum());
					for (int j = 0; j < (int)Info.Frequency.size(); j++)
						Info.Frequency[j] = pTerrestrialDesc->GetFrequency(j);
					pList->push_back(Info);
				}
			}
		}
	}

	return !pList->empty();
}


bool CTsAnalyzer::AddEventHandler(IAnalyzerEventHandler *pHandler)
{
	if (pHandler == NULL)
		return false;

	CBlockLock Lock(&m_DecoderLock);

	m_EventHandlerList.push_back(pHandler);

	return true;
}


bool CTsAnalyzer::RemoveEventHandler(IAnalyzerEventHandler *pHandler)
{
	CBlockLock Lock(&m_DecoderLock);

	for (std::vector<IAnalyzerEventHandler*>::iterator itr = m_EventHandlerList.begin(); itr != m_EventHandlerList.end(); ++itr) {
		if (*itr == pHandler) {
			m_EventHandlerList.erase(itr);
			return true;
		}
	}
	return false;
}


void CTsAnalyzer::CallEventHandler(EventType Type)
{
	for (size_t i = 0; i < m_EventHandlerList.size(); i++) {
		m_EventHandlerList[i]->OnEvent(this, Type);
	}

#if 0
	//SendDecoderEvent((DWORD)Type);
#else
#ifdef _DEBUG
	if (m_DecoderEvent != EVENT_INVALID) ::DebugBreak();
#endif
	m_DecoderEvent = Type;
#endif
}


void CTsAnalyzer::NotifyResetEvent()
{
	for (size_t i = 0; i < m_EventHandlerList.size(); i++) {
		m_EventHandlerList[i]->OnReset(this);
	}
}


void CALLBACK CTsAnalyzer::OnPatUpdated(const WORD wPID, CTsPidMapTarget *pMapTarget, CTsPidMapManager *pMapManager, const PVOID pParam)
{
	// PATが更新された
	TRACE(TEXT("CTsAnalyzer::OnPatUpdated()\n"));

	CTsAnalyzer *pThis = static_cast<CTsAnalyzer *>(pParam);
	CPatTable *pPatTable = dynamic_cast<CPatTable *>(pMapTarget);
	if (pPatTable == NULL)
		return;

	// トランスポートストリームID更新
	pThis->m_TransportStreamID = pPatTable->GetTransportStreamID();

	// 現PMT/PCRのPIDをアンマップする
	for (size_t Index = 0; Index < pThis->m_ServiceList.size(); Index++) {
		pMapManager->UnmapTarget(pThis->m_ServiceList[Index].PmtPID);
		pMapManager->UnmapTarget(pThis->m_ServiceList[Index].PcrPID);
	}

	// 新PMTをストアする
	pThis->m_ServiceList.resize(pPatTable->GetProgramNum());

	for (size_t Index = 0; Index < pThis->m_ServiceList.size(); Index++) {
		// サービスリスト更新
		pThis->m_ServiceList[Index].bIsUpdated = false;
		pThis->m_ServiceList[Index].ServiceID = pPatTable->GetProgramID((WORD)Index);
		pThis->m_ServiceList[Index].PmtPID = pPatTable->GetPmtPID((WORD)Index);
		pThis->m_ServiceList[Index].VideoEsList.clear();
		pThis->m_ServiceList[Index].AudioEsList.clear();
		pThis->m_ServiceList[Index].CaptionEsList.clear();
		pThis->m_ServiceList[Index].DataCarrouselEsList.clear();
		pThis->m_ServiceList[Index].OtherEsList.clear();
		pThis->m_ServiceList[Index].PcrPID = PID_INVALID;
		pThis->m_ServiceList[Index].EcmList.clear();
		pThis->m_ServiceList[Index].RunningStatus = 0xFF;
		pThis->m_ServiceList[Index].bIsCaService = false;
		pThis->m_ServiceList[Index].szServiceName[0] = '\0';
		pThis->m_ServiceList[Index].ServiceType = SERVICE_TYPE_INVALID;
		pThis->m_ServiceList[Index].LogoID = LOGOID_INVALID;

		// PMTのPIDをマップ
		pMapManager->MapTarget(pPatTable->GetPmtPID((WORD)Index), new CPmtTable(TABLE_DEBUG), OnPmtUpdated, pParam);
	}

	// イベントハンドラ呼び出し
	pThis->CallEventHandler(EVENT_PAT_UPDATED);
}


static void GetSdtServiceInfo(CTsAnalyzer::ServiceInfo *pServiceInfo, const CSdtTable *pSdtTable, int SdtIndex)
{
	// SDTからサービス情報を取得

	// サービス情報更新
	pServiceInfo->RunningStatus = pSdtTable->GetRunningStatus(SdtIndex);
	pServiceInfo->bIsCaService = pSdtTable->GetFreeCaMode(SdtIndex);

	const CDescBlock *pDescBlock = pSdtTable->GetItemDesc(SdtIndex);

	// サービス名更新
	pServiceInfo->szServiceName[0] = '\0';
	pServiceInfo->ServiceType = SERVICE_TYPE_INVALID;
	const CServiceDesc *pServiceDesc = pDescBlock->GetDesc<CServiceDesc>();
	if (pServiceDesc) {
		pServiceDesc->GetServiceName(pServiceInfo->szServiceName, CTsAnalyzer::MAX_SERVICE_NAME);
		pServiceInfo->ServiceType = pServiceDesc->GetServiceType();
	}

	// ロゴID更新
	const CLogoTransmissionDesc *pLogoDesc = pDescBlock->GetDesc<CLogoTransmissionDesc>();
	if (pLogoDesc) {
		pServiceInfo->LogoID = pLogoDesc->GetLogoID();
	} else {
		pServiceInfo->LogoID = CTsAnalyzer::LOGOID_INVALID;
	}
}


void CALLBACK CTsAnalyzer::OnPmtUpdated(const WORD wPID, CTsPidMapTarget *pMapTarget, CTsPidMapManager *pMapManager, const PVOID pParam)
{
	// PMTが更新された
	TRACE(TEXT("CTsAnalyzer::OnPmtUpdated()\n"));

	CTsAnalyzer *pThis = static_cast<CTsAnalyzer *>(pParam);
	CPmtTable *pPmtTable = dynamic_cast<CPmtTable *>(pMapTarget);
	if (pPmtTable == NULL)
		return;

	// サービスインデックスを検索
	const int ServiceIndex = pThis->GetServiceIndexByID(pPmtTable->m_CurSection.GetTableIdExtension());
	if (ServiceIndex < 0)
		return;
	ServiceInfo &Info = pThis->m_ServiceList[ServiceIndex];

	// ESのPIDをストア
	Info.VideoEsList.clear();
	Info.AudioEsList.clear();
	Info.CaptionEsList.clear();
	Info.DataCarrouselEsList.clear();
	Info.OtherEsList.clear();

	for (WORD EsIndex = 0; EsIndex < pPmtTable->GetEsInfoNum(); EsIndex++) {
		const BYTE StreamType = pPmtTable->GetStreamTypeID(EsIndex);
		const WORD EsPID = pPmtTable->GetEsPID(EsIndex);

		BYTE ComponentTag = COMPONENTTAG_INVALID;
		const CDescBlock *pDescBlock = pPmtTable->GetItemDesc(EsIndex);
		if (pDescBlock) {
			const CStreamIdDesc *pStreamIdDesc = pDescBlock->GetDesc<CStreamIdDesc>();

			if (pStreamIdDesc)
				ComponentTag = pStreamIdDesc->GetComponentTag();
		}

		EsInfo Es(EsPID, StreamType, ComponentTag);

		switch (StreamType) {
		case STREAM_TYPE_MPEG1_VIDEO:
		case STREAM_TYPE_MPEG2_VIDEO:
		case STREAM_TYPE_MPEG4_VISUAL:
		case STREAM_TYPE_H264:
		case STREAM_TYPE_H265:
			Info.VideoEsList.push_back(Es);
			break;

		case STREAM_TYPE_AAC:
			Info.AudioEsList.push_back(Es);
			break;

		case STREAM_TYPE_CAPTION:
			Info.CaptionEsList.push_back(Es);
			break;

		case STREAM_TYPE_DATACARROUSEL:
			Info.DataCarrouselEsList.push_back(Es);
			break;

		default:
			Info.OtherEsList.push_back(Es);
			break;
		}
	}

	// component_tag 順にソート
	auto ComponentTagCmp =
		[](const EsInfo &Info1, const EsInfo &Info2) {
			return Info1.ComponentTag < Info2.ComponentTag;
		};
	TsEngine::InsertionSort(Info.VideoEsList, ComponentTagCmp);
	TsEngine::InsertionSort(Info.AudioEsList, ComponentTagCmp);
	TsEngine::InsertionSort(Info.CaptionEsList, ComponentTagCmp);
	TsEngine::InsertionSort(Info.DataCarrouselEsList, ComponentTagCmp);

	WORD PcrPID = pPmtTable->GetPcrPID();
	if (PcrPID < 0x1FFFU) {
		Info.PcrPID = PcrPID;
		CTsPidMapTarget *pMap = pMapManager->GetMapTarget(PcrPID);
		if (!pMap) {
			// 新規Map
			pMapManager->MapTarget(PcrPID, new CPcrTable(ServiceIndex), OnPcrUpdated, pParam);
		} else {
			// 既存Map
			CPcrTable *pPcrTable = dynamic_cast<CPcrTable*>(pMap);
			if(pPcrTable) {
				// サービス追加
				pPcrTable->AddServiceIndex(ServiceIndex);
			}
		}
	}

	// ECM
	const CDescBlock *pPmtDesc = pPmtTable->GetTableDesc();
	if (pPmtDesc) {
		for (WORD i = 0; i < pPmtDesc->GetDescNum(); i++) {
			const CBaseDesc *pDesc = pPmtDesc->GetDescByIndex(i);

			if (pDesc && pDesc->GetTag() == CCaMethodDesc::DESC_TAG) {
				const CCaMethodDesc *pCaDesc = dynamic_cast<const CCaMethodDesc*>(pDesc);

				if (pCaDesc) {
					EcmInfo Ecm;

					Ecm.CaSystemID = pCaDesc->GetCaMethodID();
					Ecm.PID = pCaDesc->GetCaPID();

					Info.EcmList.push_back(Ecm);
				}
			}
		}
	}

	// 更新済みマーク
	Info.bIsUpdated = true;

	// SDTからサービス情報を取得
	CSdtTableSet *pSdtTableSet = dynamic_cast<CSdtTableSet *>(pMapManager->GetMapTarget(PID_SDT));
	if (pSdtTableSet) {
		CSdtTable *pSdtTable = pSdtTableSet->GetActualSdtTable();
		if (pSdtTable) {
			WORD SdtIndex = pSdtTable->GetServiceIndexByID(Info.ServiceID);
			if (SdtIndex != 0xFFFF)
				GetSdtServiceInfo(&Info, pSdtTable, SdtIndex);
		}
	}

	// イベントハンドラ呼び出し
	pThis->CallEventHandler(EVENT_PMT_UPDATED);
}


static void UpdateSdtServiceList(const CSdtTable *pSdtTable, CTsAnalyzer::SdtServiceList *pList)
{
	pList->resize(pSdtTable->GetServiceNum());

	for (WORD SdtIndex = 0; SdtIndex < pSdtTable->GetServiceNum(); SdtIndex++) {
		CTsAnalyzer::SdtServiceInfo &Service = (*pList)[SdtIndex];

		Service.ServiceID = pSdtTable->GetServiceID(SdtIndex);
		Service.RunningStatus = pSdtTable->GetRunningStatus(SdtIndex);
		Service.bFreeCaMode = pSdtTable->GetFreeCaMode(SdtIndex);

		Service.szServiceName[0] = '\0';
		Service.ServiceType = SERVICE_TYPE_INVALID;

		const CDescBlock *pDescBlock = pSdtTable->GetItemDesc(SdtIndex);
		const CServiceDesc *pServiceDesc = pDescBlock->GetDesc<CServiceDesc>();
		if (pServiceDesc) {
			pServiceDesc->GetServiceName(Service.szServiceName, CTsAnalyzer::MAX_SERVICE_NAME);
			Service.ServiceType = pServiceDesc->GetServiceType();
		}
	}
}

static void UpdateSdtTsMap(const CSdtTable *pSdtTable, CTsAnalyzer::SdtTsMap *pSdtTsMap)
{
	const WORD TransportStreamID = pSdtTable->GetTransportStreamID();
	const WORD NetworkID = pSdtTable->GetNetworkID();

	std::pair<CTsAnalyzer::SdtTsMap::iterator, bool> Result =
		pSdtTsMap->insert(std::pair<DWORD, CTsAnalyzer::SdtTsInfo>(CTsAnalyzer::GetSdtTsMapKey(NetworkID, TransportStreamID), CTsAnalyzer::SdtTsInfo()));

	CTsAnalyzer::SdtTsInfo &TsInfo = Result.first->second;

	TsInfo.TransportStreamID = TransportStreamID;
	TsInfo.OriginalNetworkID = NetworkID;

	UpdateSdtServiceList(pSdtTable, &TsInfo.ServiceList);
}

void CALLBACK CTsAnalyzer::OnSdtUpdated(const WORD wPID, CTsPidMapTarget *pMapTarget, CTsPidMapManager *pMapManager, const PVOID pParam)
{
	// SDTが更新された
	TRACE(TEXT("CTsAnalyzer::OnSdtUpdated()\n"));

	CTsAnalyzer *pThis = static_cast<CTsAnalyzer *>(pParam);
	CSdtTableSet *pTableSet = dynamic_cast<CSdtTableSet *>(pMapTarget);
	if (pTableSet == NULL)
		return;

	const BYTE TableID = pTableSet->GetLastUpdatedTableID();

	if (TableID == CSdtTable::TABLE_ID_ACTUAL) {
		// 現在のTSのSDT
		const CSdtTable *pSdtTable = pTableSet->GetActualSdtTable();
		if (pSdtTable == NULL)
			return;

		pThis->m_TransportStreamID = pSdtTable->GetTransportStreamID();
		pThis->m_NetworkID = pSdtTable->GetNetworkID();

		UpdateSdtServiceList(pSdtTable, &pThis->m_SdtServiceList);

		for (WORD SdtIndex = 0 ; SdtIndex < pSdtTable->GetServiceNum() ; SdtIndex++) {
			// サービスIDを検索
			const int ServiceIndex = pThis->GetServiceIndexByID(pSdtTable->GetServiceID(SdtIndex));
			if (ServiceIndex >= 0)
				GetSdtServiceInfo(&pThis->m_ServiceList[ServiceIndex], pSdtTable, SdtIndex);
		}

		UpdateSdtTsMap(pSdtTable, &pThis->m_SdtTsMap);

		pThis->m_bSdtUpdated = true;

		// イベントハンドラ呼び出し
		pThis->CallEventHandler(EVENT_SDT_UPDATED);
	} else if (TableID == CSdtTable::TABLE_ID_OTHER) {
		// 他のTSのSDT
		const CSdtOtherTable *pSdtOtherTable = pTableSet->GetOtherSdtTable();
		if (pSdtOtherTable == NULL)
			return;

		for (WORD Extension = 0; Extension < pSdtOtherTable->GetExtensionNum(); Extension++) {
			if (!pSdtOtherTable->IsExtensionComplete(Extension))
				continue;

			for (BYTE Section = 0; Section < pSdtOtherTable->GetSectionNum(Extension); Section++) {
				const CSdtTable *pSdtTable = dynamic_cast<const CSdtTable *>(pSdtOtherTable->GetSection(Extension, Section));

				if (pSdtTable != NULL)
					UpdateSdtTsMap(pSdtTable, &pThis->m_SdtTsMap);
			}
		}
	}
}


void CALLBACK CTsAnalyzer::OnNitUpdated(const WORD wPID, CTsPidMapTarget *pMapTarget, CTsPidMapManager *pMapManager, const PVOID pParam)
{
	// NITが更新された
	TRACE(TEXT("CTsAnalyzer::OnNitUpdated()\n"));

	CTsAnalyzer *pThis = static_cast<CTsAnalyzer*>(pParam);
	CNitMultiTable *pNitMultiTable = dynamic_cast<CNitMultiTable*>(pMapTarget);
	if (pNitMultiTable == NULL || !pNitMultiTable->IsNitComplete())
		return;

	const CNitTable *pNitTable = pNitMultiTable->GetNitTable(0);
	if (pNitTable == NULL)
		return;

	pThis->m_NitInfo.Clear();

	pThis->m_NetworkID = pNitTable->GetNetworkID();

	const CDescBlock *pDescBlock;

	// ネットワーク情報取得
	pDescBlock = pNitTable->GetNetworkDesc();
	if (pDescBlock) {
		const CNetworkNameDesc *pNetworkDesc = pDescBlock->GetDesc<CNetworkNameDesc>();
		if(pNetworkDesc) {
			pNetworkDesc->GetNetworkName(pThis->m_NitInfo.szNetworkName,
										 sizeof(pThis->m_NitInfo.szNetworkName) / sizeof(TCHAR));
		}

		const CSystemManageDesc *pSysManageDesc = pDescBlock->GetDesc<CSystemManageDesc>();
		if (pSysManageDesc) {
			pThis->m_NitInfo.BroadcastingFlag = pSysManageDesc->GetBroadcastingFlag();
			pThis->m_NitInfo.BroadcastingID = pSysManageDesc->GetBroadcastingID();
		}
	}

	// TSリスト取得
	pThis->m_NetworkTsList.clear();

	for (WORD SectionNo = 0; SectionNo < pNitMultiTable->GetNitSectionNum(); SectionNo++) {
		if (SectionNo > 0) {
			pNitTable = pNitMultiTable->GetNitTable(SectionNo);
			if (pNitTable == NULL)
				break;
		}

		for (WORD i = 0; i < pNitTable->GetTransportStreamNum(); i++) {
			pDescBlock = pNitTable->GetItemDesc(i);
			if (pDescBlock) {
				pThis->m_NetworkTsList.push_back(NetworkTsInfo());
				NetworkTsInfo &TsInfo = pThis->m_NetworkTsList.back();
				TsInfo.TransportStreamID = pNitTable->GetTransportStreamID(i);
				TsInfo.OriginalNetworkID = pNitTable->GetOriginalNetworkID(i);

				// サービスリスト取得
				const CServiceListDesc *pServiceListDesc = pDescBlock->GetDesc<CServiceListDesc>();
				if (pServiceListDesc) {
					for (int j = 0; j < pServiceListDesc->GetServiceNum(); j++) {
						CServiceListDesc::ServiceInfo Info;
						if (pServiceListDesc->GetServiceInfo(j, &Info)) {
							TsInfo.ServiceList.push_back(Info);

							int Index = pThis->GetServiceIndexByID(Info.ServiceID);
							if (Index >= 0) {
								ServiceInfo &Service = pThis->m_ServiceList[Index];
								if (Service.ServiceType == SERVICE_TYPE_INVALID)
									Service.ServiceType = Info.ServiceType;
							}
						}
					}
				}

				if (SectionNo == 0 && i == 0) {
					const CTSInfoDesc *pTsInfoDesc = pDescBlock->GetDesc<CTSInfoDesc>();
					if (pTsInfoDesc) {
						pTsInfoDesc->GetTSName(pThis->m_NitInfo.szTSName,
											   sizeof(pThis->m_NitInfo.szTSName) / sizeof(TCHAR));
						pThis->m_NitInfo.RemoteControlKeyID = pTsInfoDesc->GetRemoteControlKeyID();
					}
				}
			}
		}
	}

	pThis->m_bNitUpdated = true;

	// イベントハンドラ呼び出し
	pThis->CallEventHandler(EVENT_NIT_UPDATED);
}


void CALLBACK CTsAnalyzer::OnPcrUpdated(const WORD wPID, CTsPidMapTarget *pMapTarget, CTsPidMapManager *pMapManager, const PVOID pParam)
{
	// PCRが更新された
	CTsAnalyzer *pThis = static_cast<CTsAnalyzer *>(pParam);
	CPcrTable *pPcrTable = dynamic_cast<CPcrTable *>(pMapTarget);
	if (pPcrTable == NULL)
		return;

	const ULONGLONG TimeStamp = pPcrTable->GetPcrTimeStamp();

	WORD ServiceIndex;
	for (WORD Index = 0; pPcrTable->GetServiceIndex(&ServiceIndex, Index); Index++) {
		if (ServiceIndex < pThis->m_ServiceList.size()) {
			pThis->m_ServiceList[ServiceIndex].PcrTimeStamp = TimeStamp;
		}
	}

	// イベントハンドラ呼び出し
	pThis->CallEventHandler(EVENT_PCR_UPDATED);
}




CTsAnalyzer::IAnalyzerEventHandler::IAnalyzerEventHandler()
{
}


CTsAnalyzer::IAnalyzerEventHandler::~IAnalyzerEventHandler()
{
}


void CTsAnalyzer::IAnalyzerEventHandler::OnReset(CTsAnalyzer *pAnalyzer)
{
}
