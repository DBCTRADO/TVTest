// TsStream.cpp: TSストリームラッパークラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "TsStream.h"
#include "TsUtilClass.h"

#include <mmsystem.h>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


#pragma comment(lib, "winmm.lib")

#pragma intrinsic(memset, memcmp)


//////////////////////////////////////////////////////////////////////
// CTsPacketクラスの構築/消滅
//////////////////////////////////////////////////////////////////////

CTsPacket::CTsPacket()
	 //: CMediaData(TS_PACKETSIZE)
{
	_ASSERT(!m_pData);

	GetBuffer(4 + 192);

	// 空のパケットを生成する
	::ZeroMemory(&m_Header, sizeof(m_Header));
	::ZeroMemory(&m_AdaptationField, sizeof(m_AdaptationField));
}

CTsPacket::CTsPacket(const BYTE *pHexData)
	: CMediaData(pHexData, TS_PACKETSIZE)
{
	// バイナリデータからパケットを生成する
	ParsePacket();
}

CTsPacket::CTsPacket(const CTsPacket &Operand)
{
	*this = Operand;
}

CTsPacket::~CTsPacket()
{
	ClearBuffer();
}

CTsPacket & CTsPacket::operator = (const CTsPacket &Operand)
{
	if (&Operand != this) {
		// インスタンスのコピー
		CMediaData::operator = (Operand);

		m_Header = Operand.m_Header;
		m_AdaptationField = Operand.m_AdaptationField;
		if (Operand.m_AdaptationField.pOptionData)
			m_AdaptationField.pOptionData = &m_pData[6];
	}

	return *this;
}

DWORD CTsPacket::ParsePacket(BYTE *pContinuityCounter)
{
	// TSパケットヘッダ解析
	m_Header.bySyncByte					= m_pData[0];							// +0
	m_Header.bTransportErrorIndicator	= (m_pData[1] & 0x80U) != 0;	// +1 bit7
	m_Header.bPayloadUnitStartIndicator	= (m_pData[1] & 0x40U) != 0;	// +1 bit6
	m_Header.TransportPriority			= (m_pData[1] & 0x20U) != 0;	// +1 bit5
	m_Header.wPID = ((WORD)(m_pData[1] & 0x1F) << 8) | (WORD)m_pData[2];		// +1 bit4-0, +2
	m_Header.byTransportScramblingCtrl	= (m_pData[3] & 0xC0U) >> 6;			// +3 bit7-6
	m_Header.byAdaptationFieldCtrl		= (m_pData[3] & 0x30U) >> 4;			// +3 bit5-4
	m_Header.byContinuityCounter		= m_pData[3] & 0x0FU;					// +3 bit3-0

	// アダプテーションフィールド解析
	::ZeroMemory(&m_AdaptationField, sizeof(m_AdaptationField));

	if (m_Header.byAdaptationFieldCtrl & 0x02) {
		// アダプテーションフィールドあり
		m_AdaptationField.byAdaptationFieldLength = m_pData[4];							// +4
		if (m_AdaptationField.byAdaptationFieldLength > 0) {
			// フィールド長以降あり
			m_AdaptationField.bDiscontinuityIndicator	= (m_pData[5] & 0x80U) != 0;	// +5 bit7
			m_AdaptationField.bRamdomAccessIndicator	= (m_pData[5] & 0x40U) != 0;	// +5 bit6
			m_AdaptationField.bEsPriorityIndicator		= (m_pData[5] & 0x20U) != 0;	// +5 bit5
			m_AdaptationField.bPcrFlag					= (m_pData[5] & 0x10U) != 0;	// +5 bit4
			m_AdaptationField.bOpcrFlag					= (m_pData[5] & 0x08U) != 0;	// +5 bit3
			m_AdaptationField.bSplicingPointFlag		= (m_pData[5] & 0x04U) != 0;	// +5 bit2
			m_AdaptationField.bTransportPrivateDataFlag	= (m_pData[5] & 0x02U) != 0;	// +5 bit1
			m_AdaptationField.bAdaptationFieldExtFlag	= (m_pData[5] & 0x01U) != 0;	// +5 bit0

			if (m_AdaptationField.byAdaptationFieldLength > 1U) {
				m_AdaptationField.pOptionData			= &m_pData[6];
				m_AdaptationField.byOptionSize			= m_AdaptationField.byAdaptationFieldLength - 1U;
			}
		}
	}

	// パケットのフォーマット適合性をチェックする
	if(m_Header.bySyncByte != 0x47U)return EC_FORMAT;								// 同期バイト不正
	if(m_Header.bTransportErrorIndicator)return EC_TRANSPORT;						// ビット誤りあり
	if((m_Header.wPID >= 0x0002U) && (m_Header.wPID <= 0x000FU))return EC_FORMAT;	// 未定義PID範囲
	if(m_Header.byTransportScramblingCtrl == 0x01U)return EC_FORMAT;				// 未定義スクランブル制御値
	if(m_Header.byAdaptationFieldCtrl == 0x00U)return EC_FORMAT;					// 未定義アダプテーションフィールド制御値
	if((m_Header.byAdaptationFieldCtrl == 0x02U) && (m_AdaptationField.byAdaptationFieldLength > 183U))return EC_FORMAT;	// アダプテーションフィールド長異常
	if((m_Header.byAdaptationFieldCtrl == 0x03U) && (m_AdaptationField.byAdaptationFieldLength > 182U))return EC_FORMAT;	// アダプテーションフィールド長異常

	if(!pContinuityCounter || m_Header.wPID == 0x1FFFU)return EC_VALID;

	// 連続性チェック
	const BYTE byOldCounter = pContinuityCounter[m_Header.wPID];
	const BYTE byNewCounter = (m_Header.byAdaptationFieldCtrl & 0x01U)? m_Header.byContinuityCounter : 0x10U;
	pContinuityCounter[m_Header.wPID] = byNewCounter;

	if(!m_AdaptationField.bDiscontinuityIndicator){
		if((byOldCounter < 0x10U) && (byNewCounter < 0x10U)){
			if(((byOldCounter + 1U) & 0x0FU) != byNewCounter){
				return EC_CONTINUITY;
				}
			}
		}

	return EC_VALID;
}

BYTE * CTsPacket::GetPayloadData(void)
{
	// ペイロードポインタを返す
	switch (m_Header.byAdaptationFieldCtrl) {
	case 1U :	// ペイロードのみ
		return &m_pData[4];

	case 3U :	// アダプテーションフィールド、ペイロードあり
		return &m_pData[m_AdaptationField.byAdaptationFieldLength + 5U];

	default :	// アダプテーションフィールドのみ or 例外
		return NULL;
	}
}

const BYTE * CTsPacket::GetPayloadData(void) const
{
	// ペイロードポインタを返す
	switch (m_Header.byAdaptationFieldCtrl) {
	case 1U :	// ペイロードのみ
		return &m_pData[4];

	case 3U :	// アダプテーションフィールド、ペイロードあり
		return &m_pData[m_AdaptationField.byAdaptationFieldLength + 5U];

	default :	// アダプテーションフィールドのみ or 例外
		return NULL;
	}
}

const BYTE CTsPacket::GetPayloadSize(void) const
{
	// ペイロードサイズを返す
	switch(m_Header.byAdaptationFieldCtrl){
	case 1U :	// ペイロードのみ
		return (TS_PACKETSIZE - 4U);

	case 3U :	// アダプテーションフィールド、ペイロードあり
		return (TS_PACKETSIZE - m_AdaptationField.byAdaptationFieldLength - 5U);

	default :	// アダプテーションフィールドのみ or 例外
		return 0U;
	}
}

// バッファに書き込み
void CTsPacket::StoreToBuffer(void *pBuffer)
{
	BYTE *p=static_cast<BYTE*>(pBuffer);

	::CopyMemory(p,m_pData,TS_PACKETSIZE);
	p+=TS_PACKETSIZE;
	::CopyMemory(p,&m_Header,sizeof(m_Header));
	p+=sizeof(m_Header);
	::CopyMemory(p,&m_AdaptationField,sizeof(m_AdaptationField));
}

// バッファから読み込み
void CTsPacket::RestoreFromBuffer(const void *pBuffer)
{
	const BYTE *p=static_cast<const BYTE*>(pBuffer);

	::CopyMemory(m_pData,p,TS_PACKETSIZE);
	p+=TS_PACKETSIZE;
	::CopyMemory(&m_Header,p,sizeof(m_Header));
	p+=sizeof(m_Header);
	::CopyMemory(&m_AdaptationField,p,sizeof(m_AdaptationField));
	if (m_AdaptationField.pOptionData)
		m_AdaptationField.pOptionData=&m_pData[6];
}

void *CTsPacket::Allocate(size_t Size)
{
	// スクランブル解除時に都合がいいように、ペイロードを16バイト境界に合わせる
	return _aligned_offset_malloc(Size, 16, 4);
}

void CTsPacket::Free(void *pBuffer)
{
	_aligned_free(pBuffer);
}

void *CTsPacket::ReAllocate(void *pBuffer, size_t Size)
{
	return _aligned_offset_realloc(pBuffer, Size, 16, 4);
}




//////////////////////////////////////////////////////////////////////
// CPsiSectionクラスの構築/消滅
//////////////////////////////////////////////////////////////////////

CPsiSection::CPsiSection()
	: CMediaData()
{
	Reset();
}

CPsiSection::CPsiSection(const DWORD dwBuffSize)
	: CMediaData(dwBuffSize)
{
	Reset();
}

CPsiSection::CPsiSection(const CPsiSection &Operand)
{
	Reset();

	*this = Operand;
}

CPsiSection & CPsiSection::operator = (const CPsiSection &Operand)
{
	if (&Operand != this) {
		// インスタンスのコピー
		CMediaData::operator = (Operand);
		m_Header = Operand.m_Header;
	}

	return *this;
}

const bool CPsiSection::operator == (const CPsiSection &Operand) const
{
	// セクションの内容を比較する
	if (GetPayloadSize() != Operand.GetPayloadSize()) {
		// サイズが異なる
		return false;
	}

	const BYTE *pSrcData = GetPayloadData();
	const BYTE *pDstData = Operand.GetPayloadData();

	if (!pSrcData && !pDstData) {
		// いずれもNULL
		return true;
	}

	if (!pSrcData || !pDstData) {
		// 一方だけNULL
		return false;
	}

#if 0
	// バイナリ比較
	for (DWORD dwPos = 0 ; dwPos < GetPayloadSize() ; dwPos++) {
		if (pSrcData[dwPos] != pDstData[dwPos])
			return false;
	}

	// 一致する
	return true;
#else
	return memcmp(pSrcData, pDstData, GetPayloadSize()) == 0;
#endif
}

const bool CPsiSection::ParseHeader(const bool bIsExtended, const bool bIgnoreSectionNumber)
{
	const DWORD dwHeaderSize = (bIsExtended)? 8UL : 3UL;

	// ヘッダサイズチェック
	if (m_dwDataSize < dwHeaderSize)
		return false;

	// 標準形式ヘッダ解析
	m_Header.byTableID					= m_pData[0];							// +0
	m_Header.bSectionSyntaxIndicator	= (m_pData[1] & 0x80U) != 0;			// +1 bit7
	m_Header.bPrivateIndicator			= (m_pData[1] & 0x40U) != 0;			// +1 bit6
	m_Header.wSectionLength = ((WORD)(m_pData[1] & 0x0FU) << 8) | (WORD)m_pData[2];		// +1 bit5-0, +2

	if(m_Header.bSectionSyntaxIndicator && bIsExtended){
		// 拡張形式のヘッダ解析
		m_Header.wTableIdExtension		= (WORD)m_pData[3] << 8 | (WORD)m_pData[4];		// +3, +4
		m_Header.byVersionNo			= (m_pData[5] & 0x3EU) >> 1;			// +5 bit5-1
		m_Header.bCurrentNextIndicator	= (m_pData[5] & 0x01U) != 0;			// +5 bit0
		m_Header.bySectionNumber		= m_pData[6];							// +6
		m_Header.byLastSectionNumber	= m_pData[7];							// +7
		}

	// ヘッダのフォーマット適合性をチェックする
	if (m_Header.byTableID == 0xFF)
		return false;
	if((m_pData[1] & 0x30U) != 0x30U)
		return false;									// 固定ビット異常
	else if(m_Header.wSectionLength > 4093U)
		return false;									// セクション長異常
	else if(m_Header.bSectionSyntaxIndicator){
		// 拡張形式のエラーチェック
		if(!bIsExtended)
			return false;								// 目的のヘッダではない
		else if((m_pData[5] & 0xC0U) != 0xC0U)
			return false;								// 固定ビット異常
		else if(!bIgnoreSectionNumber
				&& m_Header.bySectionNumber > m_Header.byLastSectionNumber)
			return false;	// セクション番号異常
		else if(m_Header.wSectionLength < 9U)
			return false;								// セクション長異常
		}
	else{
		// 標準形式のエラーチェック	
		if(bIsExtended)
			return false;								// 目的のヘッダではない
		else if(m_Header.wSectionLength < 4U)
			return false;								// セクション長異常
		}

	return true;
}

void CPsiSection::Reset(void)
{
	// データをクリアする
	ClearSize();
	::ZeroMemory(&m_Header, sizeof(m_Header));
}

BYTE * CPsiSection::GetPayloadData(void) const
{
	// ペイロードポインタを返す
	const DWORD dwHeaderSize = (m_Header.bSectionSyntaxIndicator)? 8UL : 3UL;

	return m_dwDataSize > dwHeaderSize ? &m_pData[dwHeaderSize] : NULL;
}

const WORD CPsiSection::GetPayloadSize(void) const
{
	// ペイロードサイズを返す(実際に保持してる　※セクション長より少なくなることもある)
	const DWORD dwHeaderSize = (m_Header.bSectionSyntaxIndicator)? 8UL : 3UL;

	if(m_dwDataSize <= dwHeaderSize)return 0U;
	else if(m_Header.bSectionSyntaxIndicator){
		// 拡張セクション
		return (m_dwDataSize >= (m_Header.wSectionLength + 3UL))? (m_Header.wSectionLength - 9U) : ((WORD)m_dwDataSize - 8U);
		}
	else{
		// 標準セクション
		return (m_dwDataSize >= (m_Header.wSectionLength + 3UL))? m_Header.wSectionLength : ((WORD)m_dwDataSize - 3U);
		}
}

const BYTE CPsiSection::GetTableID(void) const
{
	// テーブルIDを返す
	return m_Header.byTableID;
}

const bool CPsiSection::IsExtendedSection(void) const
{
	// セクションシンタックスインジケータを返す
	return m_Header.bSectionSyntaxIndicator;
}

const bool CPsiSection::IsPrivate(void) const
{
	// プライベートインジケータを返す
	return m_Header.bPrivateIndicator;
}

const WORD CPsiSection::GetSectionLength(void) const
{
	// セクション長を返す
	return m_Header.wSectionLength;
}

const WORD CPsiSection::GetTableIdExtension(void) const
{
	// テーブルID拡張を返す
	return m_Header.wTableIdExtension;
}

const BYTE CPsiSection::GetVersionNo(void) const
{
	// バージョン番号を返す
	return m_Header.byVersionNo;
}

const bool CPsiSection::IsCurrentNext(void) const
{
	// カレントネクストインジケータを返す
	return m_Header.bCurrentNextIndicator;
}

const BYTE CPsiSection::GetSectionNumber(void) const
{
	// セクション番号を返す
	return m_Header.bySectionNumber;
}

const BYTE CPsiSection::GetLastSectionNumber(void) const
{
	// ラストセクション番号を返す
	return m_Header.byLastSectionNumber;
}


/////////////////////////////////////////////////////////////////////////////
// TS PIDマップ対象クラス
/////////////////////////////////////////////////////////////////////////////

void CTsPidMapTarget::OnPidMapped(const WORD wPID, const PVOID pParam)
{
	// マッピングされた
}

void CTsPidMapTarget::OnPidUnmapped(const WORD wPID)
{
	// マップ解除された
}


/////////////////////////////////////////////////////////////////////////////
// TS PIDマップ管理クラス
/////////////////////////////////////////////////////////////////////////////

CTsPidMapManager::CTsPidMapManager()
	: m_wMapCount(0U)
{
	::ZeroMemory(m_PidMap, sizeof(m_PidMap));
}

CTsPidMapManager::~CTsPidMapManager()
{
	UnmapAllTarget();
}

const bool CTsPidMapManager::StorePacket(const CTsPacket *pPacket)
{
	const WORD wPID = pPacket->GetPID();

	if(wPID > 0x1FFFU)return false;					// PID範囲外

	if(!m_PidMap[wPID].pMapTarget)return false;		// PIDマップターゲットなし

	if(m_PidMap[wPID].pMapTarget->StorePacket(pPacket)){
		// ターゲットの更新があったときはコールバックを呼び出す

		if(m_PidMap[wPID].pMapCallback){
			m_PidMap[wPID].pMapCallback(wPID, m_PidMap[wPID].pMapTarget, this, m_PidMap[wPID].pMapParam);
			}
		}

	return true;
}

const bool CTsPidMapManager::MapTarget(const WORD wPID, CTsPidMapTarget *pMapTarget, const PIDMAPHANDLERFUNC pMapCallback, const PVOID pMapParam)
{
	if((wPID > 0x1FFFU) || (!pMapTarget))return false;

	// 現在のターゲットをアンマップ
	UnmapTarget(wPID);

	// 新しいターゲットをマップ
	m_PidMap[wPID].pMapTarget = pMapTarget;
	m_PidMap[wPID].pMapCallback = pMapCallback;
	m_PidMap[wPID].pMapParam = pMapParam;
	m_wMapCount++;

	pMapTarget->OnPidMapped(wPID, pMapParam);

	return true;
}

const bool CTsPidMapManager::UnmapTarget(const WORD wPID)
{
	if(wPID > 0x1FFFU)return false;

	if(!m_PidMap[wPID].pMapTarget)return false;

	// 現在のターゲットをアンマップ
	CTsPidMapTarget *pTarget = m_PidMap[wPID].pMapTarget;
	::ZeroMemory(&m_PidMap[wPID], sizeof(m_PidMap[wPID]));
	m_wMapCount--;

	pTarget->OnPidUnmapped(wPID);

	return true;
}

void CTsPidMapManager::UnmapAllTarget(void)
{
	// 全ターゲットをアンマップ
	for (WORD wPID = 0x0000U ; wPID <= 0x1FFFU ; wPID++) {
		UnmapTarget(wPID);
	}
}

CTsPidMapTarget * CTsPidMapManager::GetMapTarget(const WORD wPID) const
{
	// マップされているターゲットを返す
	return (wPID <= 0x1FFFU)? m_PidMap[wPID].pMapTarget : NULL;
}

const WORD CTsPidMapManager::GetMapCount(void) const
{
	// マップされているPIDの総数を返す
	return m_wMapCount;
}


/////////////////////////////////////////////////////////////////////////////
// PSIセクション抽出クラス
/////////////////////////////////////////////////////////////////////////////

CPsiSectionParser::CPsiSectionParser(IPsiSectionHandler *pPsiSectionHandler,
									 const bool bTargetExt, const bool bIgnoreSectionNumber)
	: m_pPsiSectionHandler(pPsiSectionHandler)
	, m_PsiSection(0x10002UL)		// PSIセクション最大サイズのバッファ確保
	, m_bTargetExt(bTargetExt)
	, m_bIgnoreSectionNumber(bIgnoreSectionNumber)
	, m_bIsStoring(false)
	, m_dwCrcErrorCount(0UL)
{

}

CPsiSectionParser::CPsiSectionParser(const CPsiSectionParser &Operand)
{
	*this = Operand;
}

CPsiSectionParser & CPsiSectionParser::operator = (const CPsiSectionParser &Operand)
{
	if (&Operand != this) {
		// インスタンスのコピー
		m_pPsiSectionHandler = Operand.m_pPsiSectionHandler;
		m_bTargetExt = Operand.m_bTargetExt;
		m_PsiSection = Operand.m_PsiSection;
		m_bIsStoring = Operand.m_bIsStoring;
		m_dwStoreCrc = Operand.m_dwStoreCrc;
		m_wStoreSize = Operand.m_wStoreSize;
		m_dwCrcErrorCount = Operand.m_dwCrcErrorCount;
	}

	return *this;
}

void CPsiSectionParser::StorePacket(const CTsPacket *pPacket)
{
	const BYTE *pData = pPacket->GetPayloadData();
	const BYTE byPayloadSize = pPacket->GetPayloadSize();
	if (!byPayloadSize || !pData)
		return;

	BYTE byPos, bySize;

	if (pPacket->m_Header.bPayloadUnitStartIndicator) {
		// [ヘッダ断片 | ペイロード断片] + [スタッフィングバイト] + ヘッダ先頭 + [ヘッダ断片] + [ペイロード断片] + [スタッフィングバイト]
		const BYTE byUnitStartPos = pData[0] + 1U;
		if (byUnitStartPos >= byPayloadSize)
			return;

		if (byUnitStartPos > 1U) {
			// ユニット開始位置が先頭ではない場合(断片がある場合)
			byPos = 1U;
			bySize = byUnitStartPos - byPos;
			if (m_bIsStoring) {
				StorePayload(&pData[byPos], &bySize);
			} else if (m_PsiSection.GetSize() > 0
					&& StoreHeader(&pData[byPos], &bySize)) {
				byPos += bySize;
				bySize = byUnitStartPos - byPos;
				StorePayload(&pData[byPos], &bySize);
			}
		}

		// ユニット開始位置から新規セクションのストアを開始する
		m_PsiSection.Reset();
		m_bIsStoring = false;

		byPos = byUnitStartPos;
		while (byPos < byPayloadSize) {
			bySize = byPayloadSize - byPos;
			if (!m_bIsStoring) {
				if (!StoreHeader(&pData[byPos], &bySize))
					break;
				byPos += bySize;
				bySize = byPayloadSize - byPos;
			}
			if (!StorePayload(&pData[byPos], &bySize))
				break;
			byPos += bySize;
			if (byPos >= byPayloadSize || pData[byPos] == 0xFF)
				break;
		}
	} else {
		// [ヘッダ断片] + ペイロード + [スタッフィングバイト]
		byPos = 0U;
		bySize = byPayloadSize;
		if (!m_bIsStoring) {
			if (m_PsiSection.GetSize() == 0
					|| !StoreHeader(&pData[byPos], &bySize))
				return;
			byPos += bySize;
			bySize = byPayloadSize - byPos;
		}
		StorePayload(&pData[byPos], &bySize);
	}
}

void CPsiSectionParser::Reset(void)
{
	// 状態を初期化する
	m_bIsStoring = false;
	m_wStoreSize = 0U;
	m_dwCrcErrorCount = 0UL;
	m_PsiSection.Reset();
}

const DWORD CPsiSectionParser::GetCrcErrorCount(void) const
{
	// CRCエラー回数を返す
	return m_dwCrcErrorCount;
}

const bool CPsiSectionParser::StoreHeader(const BYTE *pPayload, BYTE *pbyRemain)
{
	// ヘッダを解析してセクションのストアを開始する
	if (m_bIsStoring) {
		*pbyRemain = 0;
		return false;
	}

	const BYTE byHeaderSize = (m_bTargetExt)? 8U : 3U;
	const BYTE byHeaderRemain = byHeaderSize - (BYTE)m_PsiSection.GetSize();

	if (*pbyRemain >= byHeaderRemain) {
		// ヘッダストア完了、ヘッダを解析してペイロードのストアを開始する
		m_PsiSection.AddData(pPayload, byHeaderRemain);
		if (m_PsiSection.ParseHeader(m_bTargetExt, m_bIgnoreSectionNumber)) {
			// ヘッダフォーマットOK、ヘッダのみのCRCを計算する
			m_wStoreSize = m_PsiSection.GetSectionLength() + 3U;
			m_dwStoreCrc = CCrcCalculator::CalcCrc32(m_PsiSection.GetData(), byHeaderSize);
			m_bIsStoring = true;
			*pbyRemain = byHeaderRemain;
			return true;
		} else {
			// ヘッダエラー
			m_PsiSection.Reset();
			*pbyRemain = byHeaderRemain;
			TRACE(TEXT("PSI header format error\n"));
			return false;
		}
	} else {
		// ヘッダストア未完了、次のデータを待つ
		m_PsiSection.AddData(pPayload, *pbyRemain);
		return false;
	}
}

const bool CPsiSectionParser::StorePayload(const BYTE *pPayload, BYTE *pbyRemain)
{
	// セクションのストアを完了する
	if (!m_bIsStoring) {
		*pbyRemain = 0;
		return false;
	}

	const BYTE byRemain = *pbyRemain;
	const WORD wStoreRemain = m_wStoreSize - (WORD)m_PsiSection.GetSize();

	if (wStoreRemain <= (WORD)byRemain) {
		// ストア完了
		m_PsiSection.AddData(pPayload, wStoreRemain);

		if (!CCrcCalculator::CalcCrc32(pPayload, wStoreRemain, m_dwStoreCrc)) {
			// CRC正常、ハンドラにセクションを渡す
			if (m_pPsiSectionHandler)
				m_pPsiSectionHandler->OnPsiSection(this, &m_PsiSection);
			//TRACE(TEXT("[%02X] PSI Stored: %lu / %lu\n"), m_PsiSection.GetTableID(), m_PsiSection.GetSize(), (DWORD)m_wStoreSize);
		} else {
			// CRC異常
			//if (m_dwCrcErrorCount < 0xFFFFFFFFUL)
				m_dwCrcErrorCount++;
			//TRACE(TEXT("[%02X] PSI CRC Error: %lu / %lu\n"), m_PsiSection.GetTableID(), m_PsiSection.GetSize(), (DWORD)m_wStoreSize);
		}

		// 状態を初期化し、次のセクション受信に備える
		m_PsiSection.Reset();
		m_bIsStoring = false;

		*pbyRemain = (BYTE)wStoreRemain;
		return true;
	} else {
		// ストア未完了、次のペイロードを待つ
		m_PsiSection.AddData(pPayload, byRemain);
		m_dwStoreCrc = CCrcCalculator::CalcCrc32(pPayload, byRemain, m_dwStoreCrc);
		return false;
	}
}


/////////////////////////////////////////////////////////////////////////////
// PCR抽象化クラス
/////////////////////////////////////////////////////////////////////////////

CTsClockRef::CTsClockRef()
{
	InitPcrPll(0LL);
}

CTsClockRef::CTsClockRef(const CTsClockRef &Operand)
{
	*this = Operand;
}

CTsClockRef & CTsClockRef::operator = (const CTsClockRef &Operand)
{
	if (&Operand != this) {
		// インスタンスのコピー
		m_llHrcUnitFreq = Operand.m_llHrcUnitFreq;
		m_llHrcLastTime = Operand.m_llHrcLastTime;
		m_llCurPcrCount = Operand.m_llCurPcrCount;
		m_lfPllFeedBack = Operand.m_lfPllFeedBack;
	}

	return *this;
}

const bool CTsClockRef::StorePacket(const CTsPacket *pPacket, const WORD wPcrPID)
{
	if(pPacket->GetPID() != wPcrPID)return false;
	if(!pPacket->m_AdaptationField.bPcrFlag)return false;

	// 33bit 90KHz PCRを計算
	const LONGLONG llCurPcrCount = GetPcrFromHex(pPacket->m_AdaptationField.pOptionData);

	if(llCurPcrCount < 0LL){
		// PCRなし(エラー)
		TRACE(TEXT("PCRなし(エラー)\n"));
		return true;
		}
	else if(!m_llCurPcrCount){
		// PCR PLL初期化
		InitPcrPll(llCurPcrCount);
		TRACE(TEXT("PLL初期化\n"));
		}
	else if(pPacket->m_AdaptationField.bDiscontinuityIndicator){
		// PCR PLL再同期
		SyncPcrPll(llCurPcrCount);
		TRACE(TEXT("PLL再同期\n"));
		}
	else{
		// PCR PLL処理
		ProcPcrPll(llCurPcrCount);
		}

	return true;
}

void CTsClockRef::Reset(void)
{
	InitPcrPll(0LL);
}

const LONGLONG CTsClockRef::GetGlobalPcr(void) const
{
	// 現在時刻からグローバルPCRを取得する
	LONGLONG llHrcCurTime;
	::QueryPerformanceCounter((LARGE_INTEGER *)&llHrcCurTime);

	// 最後に更新されたPCR + 更新からの経過時間 = 現在のPCR
	return (m_llGlobalPcrCount + (LONGLONG)(((double)(llHrcCurTime - m_llHrcLastTime) * 90000.0) / (double)m_llHrcUnitFreq));
}

const LONGLONG CTsClockRef::GetCurrentPcr(void) const
{
	// 現在時刻からPCRを取得する
	LONGLONG llHrcCurTime;
	::QueryPerformanceCounter((LARGE_INTEGER *)&llHrcCurTime);

	// 最後に更新されたPCR + 更新からの経過時間 = 現在のPCR
	return (m_llCurPcrCount + (LONGLONG)(((double)(llHrcCurTime - m_llHrcLastTime) * 90000.0) / (double)m_llHrcUnitFreq));
}

const LONGLONG CTsClockRef::PtsToGlobalPcr(const LONGLONG llPts) const
{
	// PTSをPCRに変換する
	if(llPts > m_llCurPcrCount){
		// グローバルPCRにPCRのオフセットを加算した値を返す
		return m_llGlobalPcrCount + (llPts - m_llCurPcrCount);
		}
	else{
		// 既に時刻を過ぎている(エラー or 大幅な処理遅れ)
		LONGLONG llHrcCurTime;
		::QueryPerformanceCounter((LARGE_INTEGER *)&llHrcCurTime);

		// 最後に更新されたPCR + 更新からの経過時間 = 現在のPCR
		return (m_llGlobalPcrCount + (LONGLONG)(((double)(llHrcCurTime - m_llHrcLastTime) * 90000.0) / (double)m_llHrcUnitFreq));		
		}
}

void CTsClockRef::InitPcrPll(const LONGLONG llCurPcr)
{
	// PLLを初期化する
	::QueryPerformanceFrequency((LARGE_INTEGER *)&m_llHrcUnitFreq);
	::QueryPerformanceCounter((LARGE_INTEGER *)&m_llHrcLastTime);

	m_llCurPcrCount = llCurPcr;
	m_lfPllFeedBack = 0.0;

	m_llGlobalPcrCount = 0LL;
	m_llBasePcrCount = llCurPcr;
}

void CTsClockRef::ProcPcrPll(const LONGLONG llCurPcr)
{
	// PLLを処理する
	ULONGLONG llHrcCurTime;
	::QueryPerformanceCounter((LARGE_INTEGER *)&llHrcCurTime);

	// ローカルPCRを計算する(高分解能タイマによる実周期の積分値)
	m_llCurPcrCount += (LONGLONG)(((double)(llHrcCurTime - m_llHrcLastTime) * 90000.0) / (double)m_llHrcUnitFreq);	// 50ms
	m_llHrcLastTime = llHrcCurTime;

	// ローカルPCRとストリームPCRの位相差にLPFを施す(フィードバックゲイン = -40dB、時定数 = 約5.5s)
	m_lfPllFeedBack = m_lfPllFeedBack * 0.99 + (double)(m_llCurPcrCount - llCurPcr) * 0.01;

	// ローカルPCRに位相差をフィードバックする
	m_llCurPcrCount -= (LONGLONG)m_lfPllFeedBack;

	// グローバルPCRを更新する
	m_llGlobalPcrCount = m_llCurPcrCount - m_llBasePcrCount;
}

void CTsClockRef::SyncPcrPll(const LONGLONG llCurPcr)
{
	// PCR不連続発生時にPLLを再同期する
	::QueryPerformanceCounter((LARGE_INTEGER *)&m_llHrcLastTime);

	// PLL初期値設定
	m_llCurPcrCount = llCurPcr;
	m_lfPllFeedBack = 0.0;

	// グローバルPCRの基点再設定
	m_llBasePcrCount = llCurPcr;
}

inline const LONGLONG CTsClockRef::GetPcrFromHex(const BYTE *pPcrData)
{
	// PCRを解析する(42bit 27MHz)
	LONGLONG llCurPcrCount = 0LL;
	llCurPcrCount |= (LONGLONG)pPcrData[0] << 34;
	llCurPcrCount |= (LONGLONG)pPcrData[1] << 26;
	llCurPcrCount |= (LONGLONG)pPcrData[2] << 18;
	llCurPcrCount |= (LONGLONG)pPcrData[3] << 10;
	llCurPcrCount |= (LONGLONG)(pPcrData[4] & 0x80U) << 2;
	llCurPcrCount |= (LONGLONG)(pPcrData[4] & 0x01U) << 8;
	llCurPcrCount |= (LONGLONG)pPcrData[5];

	// 33bit 90KHzにシフトする(42bitでは演算誤差が蓄積して使い物にならない)
	return llCurPcrCount >> 9;
}
