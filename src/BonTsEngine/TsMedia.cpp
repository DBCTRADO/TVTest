// TsMedia.cpp: TSメディアラッパークラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "TsMedia.h"
#include "Bitstream.h"
#include "../Common/DebugDef.h"


/////////////////////////////////////////////////////////////////////////////
// CPesPacketクラスの構築/消滅
/////////////////////////////////////////////////////////////////////////////

CPesPacket::CPesPacket()
	: CMediaData()
{
	Reset();
}

CPesPacket::CPesPacket(const DWORD dwBuffSize)
	: CMediaData(dwBuffSize)
{
	Reset();
}

CPesPacket::CPesPacket(const CPesPacket &Operand)
	: CMediaData()
{
	Reset();

	*this = Operand;
}

CPesPacket & CPesPacket::operator = (const CPesPacket &Operand)
{
	if (&Operand != this) {
		// インスタンスのコピー
		CMediaData::operator = (Operand);
		m_Header = Operand.m_Header;
	}

	return *this;
}

const bool CPesPacket::ParseHeader(void)
{
	if(m_dwDataSize < 9UL)return false;														// PES_header_data_lengthまでは6バイト
	else if(m_pData[0] != 0x00U || m_pData[1] != 0x00U || m_pData[2] != 0x01U)return false;	// packet_start_code_prefix異常
	else if((m_pData[6] & 0xC0U) != 0x80U)return false;										// 固定ビット異常

	// ヘッダ解析
	m_Header.byStreamID					= m_pData[3];										// +3 bit7-0
	m_Header.wPacketLength				= ((WORD)m_pData[4] << 8) | (WORD)m_pData[5];		// +4, +5
	m_Header.byScramblingCtrl			= (m_pData[6] & 0x30U) >> 4;						// +6 bit5-4
	m_Header.bPriority					= (m_pData[6] & 0x08U)? true : false;				// +6 bit3
	m_Header.bDataAlignmentIndicator	= (m_pData[6] & 0x04U)? true : false;				// +6 bit2
	m_Header.bCopyright					= (m_pData[6] & 0x02U)? true : false;				// +6 bit1
	m_Header.bOriginalOrCopy			= (m_pData[6] & 0x01U)? true : false;				// +6 bit0
	m_Header.byPtsDtsFlags				= (m_pData[7] & 0xC0U) >> 6;						// +7 bit7-6
	m_Header.bEscrFlag					= (m_pData[7] & 0x20U)? true : false;				// +7 bit5
	m_Header.bEsRateFlag				= (m_pData[7] & 0x10U)? true : false;				// +7 bit4
	m_Header.bDsmTrickModeFlag			= (m_pData[7] & 0x08U)? true : false;				// +7 bit3
	m_Header.bAdditionalCopyInfoFlag	= (m_pData[7] & 0x04U)? true : false;				// +7 bit2
	m_Header.bCrcFlag					= (m_pData[7] & 0x02U)? true : false;				// +7 bit1
	m_Header.bExtensionFlag				= (m_pData[7] & 0x01U)? true : false;				// +7 bit0
	m_Header.byHeaderDataLength			= m_pData[8];										// +8 bit7-0

	// ヘッダのフォーマット適合性をチェックする
	if(m_Header.byScramblingCtrl != 0U)return false;	// Not scrambled のみ対応
	else if(m_Header.byPtsDtsFlags == 1U)return false;	// 未定義のフラグ

	return true;
}

void CPesPacket::Reset(void)
{
	// データをクリアする
	ClearSize();
	::ZeroMemory(&m_Header, sizeof(m_Header));
}

const BYTE CPesPacket::GetStreamID(void) const
{
	// Stream IDを返す
	return m_Header.byStreamID;
}

const WORD CPesPacket::GetPacketLength(void) const
{
	// PES Packet Lengthを返す
	return m_Header.wPacketLength;
}

const BYTE CPesPacket::GetScramblingCtrl(void) const
{	// PES Scrambling Controlを返す
	return m_Header.byScramblingCtrl;
}

const bool CPesPacket::IsPriority(void) const
{	// PES Priorityを返す
	return m_Header.bPriority;
}

const bool CPesPacket::IsDataAlignmentIndicator(void) const
{
	// Data Alignment Indicatorを返す
	return m_Header.bDataAlignmentIndicator;
}

const bool CPesPacket::IsCopyright(void) const
{
	// Copyrightを返す
	return m_Header.bCopyright;
}

const bool CPesPacket::IsOriginalOrCopy(void) const
{
	// Original or Copyを返す
	return m_Header.bOriginalOrCopy;
}

const BYTE CPesPacket::GetPtsDtsFlags(void) const
{
	// PTS DTS Flagsを返す
	return m_Header.byPtsDtsFlags;
}

const bool CPesPacket::IsEscrFlag(void) const
{
	// ESCR Flagを返す
	return m_Header.bEscrFlag;
}

const bool CPesPacket::IsEsRateFlag(void) const
{
	// ES Rate Flagを返す
	return m_Header.bEsRateFlag;
}

const bool CPesPacket::IsDsmTrickModeFlag(void) const
{
	// DSM Trick Mode Flagを返す
	return m_Header.bDsmTrickModeFlag;
}

const bool CPesPacket::IsAdditionalCopyInfoFlag(void) const
{
	// Additional Copy Info Flagを返す
	return m_Header.bAdditionalCopyInfoFlag;
}

const bool CPesPacket::IsCrcFlag(void) const
{
	// PES CRC Flagを返す
	return m_Header.bCrcFlag;
}

const bool CPesPacket::IsExtensionFlag(void) const
{
	// PES Extension Flagを返す
	return m_Header.bExtensionFlag;
}

const BYTE CPesPacket::GetHeaderDataLength(void) const
{
	// PES Header Data Lengthを返す
	return m_Header.byHeaderDataLength;
}

const LONGLONG CPesPacket::GetPtsCount(void)const
{
	// PTS(Presentation Time Stamp)を返す
	if (m_Header.byPtsDtsFlags) {
		return HexToTimeStamp(&m_pData[9]);
	}

	// エラー(PTSがない)
	return -1LL;
}

const WORD CPesPacket::GetPacketCrc(void) const
{
	// PES Packet CRCを返す
	DWORD dwCrcPos = 9UL;

	// 位置を計算
	if(m_Header.byPtsDtsFlags == 2U)dwCrcPos += 5UL;
	if(m_Header.byPtsDtsFlags == 3U)dwCrcPos += 10UL;
	if(m_Header.bEscrFlag)dwCrcPos += 6UL;
	if(m_Header.bEsRateFlag)dwCrcPos += 3UL;
	if(m_Header.bDsmTrickModeFlag)dwCrcPos += 1UL;
	if(m_Header.bAdditionalCopyInfoFlag)dwCrcPos += 1UL;

	if(m_dwDataSize < (dwCrcPos + 2UL))return 0x0000U;

	return ((WORD)m_pData[dwCrcPos] << 8) | (WORD)m_pData[dwCrcPos + 1];
}

BYTE * CPesPacket::GetPayloadData(void) const
{
	// ペイロードポインタを返す
	const DWORD dwPayloadPos = m_Header.byHeaderDataLength + 9UL;

	return (m_dwDataSize >= (dwPayloadPos + 1UL))? &m_pData[dwPayloadPos] : NULL;
}

const DWORD CPesPacket::GetPayloadSize(void) const
{
	// ペイロードサイズを返す(実際の保持してる　※パケット長より少なくなることもある)
	const DWORD dwHeaderSize = m_Header.byHeaderDataLength + 9UL;

	return (m_dwDataSize > dwHeaderSize)? (m_dwDataSize - dwHeaderSize) : 0UL;
}

inline const LONGLONG CPesPacket::HexToTimeStamp(const BYTE *pHexData)
{
	// 33bit 90KHz タイムスタンプを解析する
	LONGLONG llCurPtsCount = 0LL;
	llCurPtsCount |= (LONGLONG)(pHexData[0] & 0x0EU) << 29;
	llCurPtsCount |= (LONGLONG)pHexData[1] << 22;
	llCurPtsCount |= (LONGLONG)(pHexData[2] & 0xFEU) << 14;
	llCurPtsCount |= (LONGLONG)pHexData[3] << 7;
	llCurPtsCount |= (LONGLONG)pHexData[4] >> 1;

	return llCurPtsCount;
}


//////////////////////////////////////////////////////////////////////
// CPesParserクラスの構築/消滅
//////////////////////////////////////////////////////////////////////

CPesParser::CPesParser(IPacketHandler *pPacketHandler)
	: m_pPacketHandler(pPacketHandler)
	, m_PesPacket(0x10005UL)
	, m_bIsStoring(false)
	, m_wStoreCrc(0x0000U)
	, m_dwStoreSize(0UL)
{

}

CPesParser::CPesParser(const CPesParser &Operand)
{
	*this = Operand;
}

CPesParser & CPesParser::operator = (const CPesParser &Operand)
{
	if (&Operand != this) {
		// インスタンスのコピー
		m_pPacketHandler = Operand.m_pPacketHandler;
		m_PesPacket = Operand.m_PesPacket;
		m_bIsStoring = Operand.m_bIsStoring;
		m_wStoreCrc = Operand.m_wStoreCrc;
	}

	return *this;
}

const bool CPesParser::StorePacket(const CTsPacket *pPacket)
{
	const BYTE *pData = pPacket->GetPayloadData();
	const BYTE bySize = pPacket->GetPayloadSize();
	if(!bySize || !pData)return false;

	bool bTrigger = false;
	BYTE byPos = 0U;

	if(pPacket->m_Header.bPayloadUnitStartIndicator){
		// ヘッダ先頭 + [ペイロード断片]

		// PESパケット境界なしのストアを完了する
		if(m_bIsStoring && !m_PesPacket.GetPacketLength()){
			OnPesPacket(&m_PesPacket);
			}

		m_bIsStoring = false;
		bTrigger = true;
		m_PesPacket.ClearSize();

		byPos += StoreHeader(&pData[byPos], bySize - byPos);
		byPos += StorePayload(&pData[byPos], bySize - byPos);
		}
	else{
		// [ヘッダ断片] + ペイロード + [スタッフィングバイト]
		byPos += StoreHeader(&pData[byPos], bySize - byPos);
		byPos += StorePayload(&pData[byPos], bySize - byPos);
		}

	return bTrigger;
}

void CPesParser::Reset(void)
{
	// 状態を初期化する
	m_PesPacket.Reset();
	m_bIsStoring = false;
	m_dwStoreSize = 0UL;
}

void CPesParser::OnPesPacket(const CPesPacket *pPacket) const
{
	// ハンドラ呼び出し
	if(m_pPacketHandler)m_pPacketHandler->OnPesPacket(this, pPacket);
}

const BYTE CPesParser::StoreHeader(const BYTE *pPayload, const BYTE byRemain)
{
	// ヘッダを解析してセクションのストアを開始する
	if(m_bIsStoring)return 0U;

	const BYTE byHeaderRemain = 9U - (BYTE)m_PesPacket.GetSize();

	if(byRemain >= byHeaderRemain){
		// ヘッダストア完了、ヘッダを解析してペイロードのストアを開始する
		m_PesPacket.AddData(pPayload, byHeaderRemain);
		if(m_PesPacket.ParseHeader()){
			// ヘッダフォーマットOK
			m_dwStoreSize = m_PesPacket.GetPacketLength();
			if(m_dwStoreSize)m_dwStoreSize += 6UL;
			m_bIsStoring = true;
			return byHeaderRemain;
			}
		else{
			// ヘッダエラー
			m_PesPacket.Reset();
			return byRemain;
			}
		}
	else{
		// ヘッダストア未完了、次のデータを待つ
		m_PesPacket.AddData(pPayload, byRemain);
		return byRemain;
		}
}

const BYTE CPesParser::StorePayload(const BYTE *pPayload, const BYTE byRemain)
{
	// セクションのストアを完了する
	if(!m_bIsStoring)return 0U;
	
	const DWORD dwStoreRemain = m_dwStoreSize - m_PesPacket.GetSize();

	if(m_dwStoreSize && (dwStoreRemain <= (DWORD)byRemain)){
		// ストア完了
		m_PesPacket.AddData(pPayload, dwStoreRemain);

		// CRC正常、コールバックにセクションを渡す
		OnPesPacket(&m_PesPacket);

		// 状態を初期化し、次のセクション受信に備える
		m_PesPacket.Reset();
		m_bIsStoring = false;

		return (BYTE)dwStoreRemain;
		}
	else{
		// ストア未完了、次のペイロードを待つ
		m_PesPacket.AddData(pPayload, byRemain);
		return byRemain;
		}
}


//////////////////////////////////////////////////////////////////////
// CAdtsFrameクラスの構築/消滅
//////////////////////////////////////////////////////////////////////

CAdtsFrame::CAdtsFrame()
	: CMediaData()
{
	Reset();
}

CAdtsFrame::CAdtsFrame(const CAdtsFrame &Operand)
{
	Reset();

	*this = Operand;
}

CAdtsFrame & CAdtsFrame::operator = (const CAdtsFrame &Operand)
{
	if (&Operand != this) {
		// インスタンスのコピー
		CMediaData::operator = (Operand);
		m_Header = Operand.m_Header;
	}

	return *this;
}

const bool CAdtsFrame::ParseHeader(void)
{
	// adts_fixed_header()
	if(m_dwDataSize < 7UL)return false;									// ADTSヘッダは7バイト
	else if(m_pData[0] != 0xFFU || m_pData[1] != 0xF8U)return false;	// Syncword、ID、layer、protection_absent異常　※CRCなしは非対応
	
	m_Header.byProfile				= (m_pData[2] & 0xC0U) >> 6;									// +2 bit7-6
	m_Header.bySamplingFreqIndex	= (m_pData[2] & 0x3CU) >> 2;									// +2 bit5-2
	m_Header.bPrivateBit			= (m_pData[2] & 0x02U)? true : false;							// +2 bit1
	m_Header.byChannelConfig		= ((m_pData[2] & 0x01U) << 2) | ((m_pData[3] & 0xC0U) >> 6);	// +3 bit0, +4 bit7-6
	m_Header.bOriginalCopy			= (m_pData[3] & 0x20U)? true : false;							// +3 bit5
	m_Header.bHome					= (m_pData[3] & 0x10U)? true : false;							// +3 bit4

	// adts_variable_header()
	m_Header.bCopyrightIdBit		= (m_pData[3] & 0x08U)? true : false;							// +3 bit3
	m_Header.bCopyrightIdStart		= (m_pData[3] & 0x04U)? true : false;							// +3 bit2
	m_Header.wFrameLength			= ((WORD)(m_pData[3] & 0x03U) << 11) | ((WORD)m_pData[4] << 3) | ((WORD)(m_pData[5] & 0xE0U) >> 5);
	m_Header.wBufferFullness		= ((WORD)(m_pData[5] & 0x1FU) << 6) | ((WORD)(m_pData[6] & 0xFCU) >> 2);
	m_Header.byRawDataBlockNum		= m_pData[6] & 0x03U;

	// フォーマット適合性チェック
	if(m_Header.byProfile == 3U)
		return false;		// 未定義のプロファイル
	else if(m_Header.bySamplingFreqIndex > 0x0BU)
		return false;		// 未定義のサンプリング周波数
	else if(m_Header.byChannelConfig >= 3 && m_Header.byChannelConfig != 6)
		return false;		// チャンネル数異常
	else if(m_Header.wFrameLength < 2U || m_Header.wFrameLength > 0x2000 - 7)
		return false;		// データなしの場合も最低CRCのサイズが必要
	else if(m_Header.byRawDataBlockNum)
		return false;		// 本クラスは単一のRaw Data Blockにしか対応しない

	return true;
}

void CAdtsFrame::Reset(void)
{
	// データをクリアする
	ClearSize();	
	::ZeroMemory(&m_Header, sizeof(m_Header));
}

const BYTE CAdtsFrame::GetProfile(void) const
{
	// Profile を返す
	return m_Header.byProfile;
}

const BYTE CAdtsFrame::GetSamplingFreqIndex(void) const
{
	// Sampling Frequency Index を返す
	return m_Header.bySamplingFreqIndex;
}

const DWORD CAdtsFrame::GetSamplingFreq(void) const
{
	static const DWORD FreqTable[] = {
		96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000
	};
	return FreqTable[m_Header.bySamplingFreqIndex];
}

const bool CAdtsFrame::IsPrivateBit(void) const
{
	// Private Bit を返す
	return m_Header.bPrivateBit;
}

const BYTE CAdtsFrame::GetChannelConfig(void) const
{
	// Channel Configuration を返す
	return m_Header.byChannelConfig;
}

const bool CAdtsFrame::IsOriginalCopy(void) const
{
	// Original/Copy を返す
	return m_Header.bOriginalCopy;
}

const bool CAdtsFrame::IsHome(void) const
{
	// Home を返す
	return m_Header.bHome;
}

const bool CAdtsFrame::IsCopyrightIdBit(void) const
{
	// Copyright Identification Bit を返す
	return m_Header.bCopyrightIdBit;
}

const bool CAdtsFrame::IsCopyrightIdStart(void) const
{
	// Copyright Identification Start を返す
	return m_Header.bCopyrightIdStart;
}

const WORD CAdtsFrame::GetFrameLength(void) const
{
	// Frame Length を返す
	return m_Header.wFrameLength;
}

const WORD CAdtsFrame::GetBufferFullness(void) const
{
	// ADTS Buffer Fullness を返す
	return m_Header.wBufferFullness;
}

const BYTE CAdtsFrame::GetRawDataBlockNum(void) const
{
	// Number of Raw Data Blocks in Frame を返す
	return m_Header.byRawDataBlockNum;
}


//////////////////////////////////////////////////////////////////////
// CAdtsParserクラスの構築/消滅
//////////////////////////////////////////////////////////////////////

CAdtsParser::CAdtsParser(IFrameHandler *pFrameHandler)
	: m_pFrameHandler(pFrameHandler)
{
	// ADTSフレーム最大長のバッファ確保
	m_AdtsFrame.GetBuffer(0x2000UL);

	Reset();
}

CAdtsParser::CAdtsParser(const CAdtsParser &Operand)
{
	*this = Operand;
}

CAdtsParser & CAdtsParser::operator = (const CAdtsParser &Operand)
{
	if (&Operand != this) {
		// インスタンスのコピー
		m_pFrameHandler = Operand.m_pFrameHandler;
		m_AdtsFrame = Operand.m_AdtsFrame;
		m_bIsStoring = Operand.m_bIsStoring;
		m_wStoreCrc = Operand.m_wStoreCrc;
	}

	return *this;
}

const bool CAdtsParser::StorePacket(const CPesPacket *pPacket)
{
	return StoreEs(pPacket->GetPayloadData(), pPacket->GetPayloadSize());
}

const bool CAdtsParser::StoreEs(const BYTE *pData, const DWORD dwSize)
{
	if (pData == NULL || dwSize == 0)
		return false;

	bool bTrigger = false;
	DWORD dwPos = 0UL;

	do {
		if (!m_bIsStoring) {
			// ヘッダを検索する
			m_bIsStoring = SyncFrame(pData[dwPos++]);
			if(m_bIsStoring)bTrigger = true;
		} else {
			// データをストアする
			const DWORD dwStoreRemain = m_AdtsFrame.GetFrameLength() - (WORD)m_AdtsFrame.GetSize();
			const DWORD dwDataRemain = dwSize - dwPos;

			if (dwStoreRemain <= dwDataRemain) {
				// ストア完了
				m_AdtsFrame.AddData(&pData[dwPos], dwStoreRemain);
				dwPos += dwStoreRemain;
				m_bIsStoring = false;

				// 本来ならここでCRCチェックをすべき
				// チェック対象領域が可変で複雑なので保留、誰か実装しませんか...

				// フレーム出力
				OnAdtsFrame(&m_AdtsFrame);

				// 次のフレームを処理するためリセット
				m_AdtsFrame.ClearSize();
			} else {
				// ストア未完了、次のペイロードを待つ
				m_AdtsFrame.AddData(&pData[dwPos], dwDataRemain);
				break;
			}
		}
	} while (dwPos < dwSize);

	return bTrigger;
}

bool CAdtsParser::StoreEs(const BYTE *pData, DWORD *pSize, CAdtsFrame **ppFrame)
{
	if (pData == NULL || pSize == NULL || *pSize == 0)
		return false;

	if (m_bIsStoring) {
		if (m_AdtsFrame.GetSize() >= m_AdtsFrame.GetFrameLength()) {
			m_AdtsFrame.ClearSize();
			m_bIsStoring = false;
		}
	}

	bool bFrame = false;
	const DWORD Size = *pSize;
	DWORD Pos = 0;

	do {
		if (!m_bIsStoring) {
			// ヘッダを検索する
			m_bIsStoring = SyncFrame(pData[Pos++]);
		} else {
			// データをストアする
			const DWORD StoreRemain = m_AdtsFrame.GetFrameLength() - (WORD)m_AdtsFrame.GetSize();
			const DWORD DataRemain = Size - Pos;

			if (StoreRemain <= DataRemain) {
				// ストア完了
				m_AdtsFrame.AddData(&pData[Pos], StoreRemain);
				Pos += StoreRemain;
				if (ppFrame)
					*ppFrame = &m_AdtsFrame;
				bFrame = true;
			} else {
				// ストア未完了、次のペイロードを待つ
				m_AdtsFrame.AddData(&pData[Pos], DataRemain);
				Pos += DataRemain;
			}
			break;
		}
	} while (Pos < Size);

	*pSize = Pos;

	return bFrame;
}

void CAdtsParser::Reset(void)
{
	// 状態を初期化する
	m_bIsStoring = false;
	m_AdtsFrame.Reset();
}

void CAdtsParser::OnPesPacket(const CPesParser *pPesParser, const CPesPacket *pPacket)
{
	// CPesParser::IPacketHandlerインタフェースの実装
	StorePacket(pPacket);
}

void CAdtsParser::OnAdtsFrame(const CAdtsFrame *pFrame) const
{
	// ハンドラ呼び出し
	if(m_pFrameHandler)m_pFrameHandler->OnAdtsFrame(this, pFrame);
}

inline const bool CAdtsParser::SyncFrame(const BYTE byData)
{
	switch (m_AdtsFrame.GetSize()) {
	case 0UL :
		// syncword(8bit)
		if (byData == 0xFFU)
			m_AdtsFrame.AddByte(byData);
		break;

	case 1UL :
		// syncword(4bit), ID, layer, protection_absent	※CRC付きのフレームのみ対応
		if (byData == 0xF8U)
			m_AdtsFrame.AddByte(byData);
		else
			m_AdtsFrame.ClearSize();
		break;

	case 2UL :
	case 3UL :
	case 4UL :
	case 5UL :
		// adts_fixed_header() - adts_variable_header()
		m_AdtsFrame.AddByte(byData);
		break;

	case 6UL :
		// ヘッダが全てそろった
		m_AdtsFrame.AddByte(byData);

		// ヘッダを解析する
		if (m_AdtsFrame.ParseHeader())
			return true;
		m_AdtsFrame.ClearSize();
		break;

	default:
		// 例外
		m_AdtsFrame.ClearSize();
		break;
	}

	return false;
}


/////////////////////////////////////////////////////////////////////////////
// 映像ストリーム解析クラス
/////////////////////////////////////////////////////////////////////////////

bool CVideoStreamParser::StorePacket(const CPesPacket *pPacket)
{
	return StoreEs(pPacket->GetPayloadData(), pPacket->GetPayloadSize());
}


//////////////////////////////////////////////////////////////////////
// CMpeg2Sequenceクラスの構築/消滅
//////////////////////////////////////////////////////////////////////

CMpeg2Sequence::CMpeg2Sequence()
	: CMediaData()
	, m_bFixSquareDisplay(false)
{
	Reset();
}

bool CMpeg2Sequence::ParseHeader()
{
	// ここではStart Code PrifixとStart Codeしかチェックしない。(シーケンスの同期のみを目的とする)

	// next_start_code()
	DWORD dwHeaderSize = 12UL;
	if(m_dwDataSize < dwHeaderSize)return false;
	else if(m_pData[0] || m_pData[1] || m_pData[2] != 0x01U || m_pData[3] != 0xB3U)return false;					// +0,+1,+2,+3	

	ZeroMemory(&m_Header,sizeof(m_Header));
	m_Header.wHorizontalSize			= ((WORD)m_pData[4] << 4) | ((WORD)(m_pData[5] & 0xF0U) >> 4);				// +4,+5 bit7-4
	m_Header.wVerticalSize				= ((WORD)(m_pData[5] & 0x0FU) << 8) | (WORD)m_pData[6];						// +5 bit3-0, +6
	m_Header.byAspectRatioInfo			= (m_pData[7] & 0xF0U) >> 4;												// +7 bit7-4
	BYTE * const pAspectRatioInfo = &m_pData[7];
	m_Header.byFrameRateCode			= m_pData[7] & 0x0FU;														// +7 bit3-0
	m_Header.dwBitRate					= ((DWORD)m_pData[8] << 10) | ((DWORD)m_pData[9] << 2) | ((DWORD)(m_pData[10] & 0xC0U) >> 6);	// +8, +9, +10 bit7-6
	m_Header.bMarkerBit					= (m_pData[10] & 0x20U)? true : false;										// +10 bit5
	m_Header.dwVbvBufferSize				= ((WORD)(m_pData[10] & 0x1FU) << 5) | ((WORD)(m_pData[11] & 0xF8U) >> 3);	// +10 bit4-0, +11 bit7-3
	m_Header.bConstrainedParamFlag		= (m_pData[11] & 0x04U)? true : false;										// +11 bit2
	m_Header.bLoadIntraQuantiserMatrix	= (m_pData[11] & 0x02U)? true : false;										// +11 bit1
	m_Header.bLoadNonIntraQuantiserMatrix=(m_pData[11] & 0x01U)? true : false;										// +11 bit0
	if (m_Header.bLoadIntraQuantiserMatrix) {
		dwHeaderSize += 64;
		if (m_dwDataSize < dwHeaderSize)
			return false;
	}
	if (m_Header.bLoadNonIntraQuantiserMatrix) {
		dwHeaderSize += 64;
		if (m_dwDataSize < dwHeaderSize)
			return false;
	}

	// フォーマット適合性チェック
	if (m_Header.wHorizontalSize == 0 || m_Header.wVerticalSize == 0)
		return false;
	if(!m_Header.byAspectRatioInfo || m_Header.byAspectRatioInfo > 4U)return false;		// アスペクト比が異常
	else if(!m_Header.byFrameRateCode || m_Header.byFrameRateCode > 8U)return false;	// フレームレートが異常
	else if(!m_Header.bMarkerBit)return false;											// マーカービットが異常
	else if(m_Header.bConstrainedParamFlag)return false;								// Constrained Parameters Flag が異常

	// 拡張ヘッダ検索する
	DWORD SyncState = 0xFFFFFFFF;
	for (DWORD i = dwHeaderSize; i < min(m_dwDataSize - 1, 1024UL);) {
		SyncState = (SyncState << 8) | m_pData[i++];
		if (SyncState == 0x000001B5UL) {
			// 拡張ヘッダ発見
			switch (m_pData[i] >> 4) {
			case 1:
				// シーケンス拡張(48bits)
				if (i + 6 > m_dwDataSize)
					break;
				m_Header.Extention.Sequence.byProfileAndLevel = (m_pData[i + 0] & 0x0FU) << 4 | (m_pData[i + 1] >> 4);
				m_Header.Extention.Sequence.bProgressive = (m_pData[i + 1] & 0x08) != 0;
				m_Header.Extention.Sequence.byChromaFormat = (m_pData[i + 1] & 0x06U) >> 1;
				m_Header.wHorizontalSize |= (((m_pData[i + 1] & 0x01U) << 1) | ((m_pData[i + 2] & 0x80U) >> 7)) << 12;	// horizontal size extension
				m_Header.wVerticalSize   |= ((m_pData[i + 2] & 0x60U) >> 5) << 12;	// vertical size extension
				m_Header.dwBitRate |= (((DWORD)m_pData[i + 2] & 0x1FU) << 7) | (m_pData[i + 3] >> 1) << 18;	// bit rate extension
				if ((m_pData[i + 3] & 0x01U) == 0)	// marker bit
					break;
				m_Header.dwVbvBufferSize |= m_pData[i + 4] << 10;	// vbv buffer size extension
				m_Header.Extention.Sequence.bLowDelay = (m_pData[i + 5] & 0x80) != 0;
				m_Header.Extention.Sequence.byFrameRateExtN = (m_pData[i + 5] & 0x60U) >> 5;
				m_Header.Extention.Sequence.byFrameRateExtD = (m_pData[i + 5] & 0x18U) >> 3;
				m_Header.Extention.Sequence.bHave = true;
				i += 6;
				break;

			case 2:
				// ディスプレイ拡張(40bits(+24bits))
				if (i + 5 > m_dwDataSize)
					break;
				m_Header.Extention.Display.byVideoFormat = (m_pData[i + 0] & 0x0EU) >> 1;
				m_Header.Extention.Display.bColorDescrption = (m_pData[i + 0] & 0x01U) != 0;
				if (m_Header.Extention.Display.bColorDescrption) {
					if (i + 5 + 3 > m_dwDataSize)
						break;
					m_Header.Extention.Display.Color.byColorPrimaries = m_pData[i + 1];
					m_Header.Extention.Display.Color.byTransferCharacteristics = m_pData[i + 2];
					m_Header.Extention.Display.Color.byMatrixCoefficients = m_pData[i + 3];
					i += 3;
				}
				if ((m_pData[i + 2] & 0x02) == 0)	// marker bit
					break;
				if ((m_pData[i + 4] & 0x07) != 0)	// marker bit
					break;
				m_Header.Extention.Display.wDisplayHorizontalSize = ((WORD)m_pData[i + 1] << 6) | ((WORD)(m_pData[i + 2] & 0xFCU) >> 2);
				m_Header.Extention.Display.wDisplayVerticalSize   = ((WORD)(m_pData[i + 2] & 0x01U) << 13) | ((WORD)m_pData[i + 3] << 5) | ((WORD)(m_pData[i + 4] & 0xF8U) >> 3);
				// For CyberLink decoder bug.
				if (m_bFixSquareDisplay
						&& m_Header.Extention.Display.wDisplayHorizontalSize == 1080
						&& m_Header.Extention.Display.wDisplayVerticalSize == 1080
						&& m_Header.byAspectRatioInfo == 2) {
					m_pData[i + 1] = m_Header.wHorizontalSize >> 6;
					m_pData[i + 2] = ((m_Header.wHorizontalSize & 0x3F) << 2) | (m_pData[i + 2] & 0x03);
					*pAspectRatioInfo = (3 << 4) | (*pAspectRatioInfo & 0x0F);
				}
				m_Header.Extention.Display.bHave = true;
				i += 5;
				break;
			}
		}
	}

	return true;
}

void CMpeg2Sequence::Reset()
{
	// データをクリアする
	ClearSize();
	::ZeroMemory(&m_Header, sizeof(m_Header));
}

WORD CMpeg2Sequence::GetHorizontalSize() const
{
	// Horizontal Size Value を返す
	return m_Header.wHorizontalSize;
}

WORD CMpeg2Sequence::GetVerticalSize() const
{
	// Vertical Size Value を返す
	return m_Header.wVerticalSize;
}

BYTE CMpeg2Sequence::GetAspectRatioInfo() const
{
	// Aspect Ratio Information を返す
	return m_Header.byAspectRatioInfo;
}

bool CMpeg2Sequence::GetAspectRatio(BYTE *pAspectX, BYTE *pAspectY) const
{
	BYTE AspectX, AspectY;

	switch (m_Header.byAspectRatioInfo) {
	case 1:
		AspectX = 1;
		AspectY = 1;
		break;
	case 2:
		AspectX = 4;
		AspectY = 3;
		break;
	case 3:
		AspectX = 16;
		AspectY = 9;
		break;
	case 4:
		AspectX = 221;
		AspectY = 100;
		break;
	default:
		return false;
	}
	if (pAspectX)
		*pAspectX = AspectX;
	if (pAspectY)
		*pAspectY = AspectY;
	return true;
}

BYTE CMpeg2Sequence::GetFrameRateCode(void) const
{
	// Frame Rate Code を返す
	return m_Header.byFrameRateCode;
}

bool CMpeg2Sequence::GetFrameRate(DWORD *pNum, DWORD *pDenom) const
{
	static const struct {
		WORD Num, Denom;
	} FrameRateList[] = {
		{24000, 1001},	// 23.976
		{   24,    1},	// 24
		{   25,    1},	// 25
		{30000, 1001},	// 29.97
		{   30,    1},	// 30
		{   50,    1},	// 50
		{60000, 1001},	// 59.94
		{   60,    1},	// 60
	};

	if (m_Header.byFrameRateCode == 0 || m_Header.byFrameRateCode > 8)
		return false;
	if (pNum)
		*pNum = FrameRateList[m_Header.byFrameRateCode - 1].Num;
	if (pDenom)
		*pDenom = FrameRateList[m_Header.byFrameRateCode - 1].Denom;
	return true;
}

DWORD CMpeg2Sequence::GetBitRate() const
{
	// Bit Rate Value を返す
	return m_Header.dwBitRate;
}

bool CMpeg2Sequence::IsMarkerBit() const
{
	// Marker Bit を返す
	return m_Header.bMarkerBit;
}

DWORD CMpeg2Sequence::GetVbvBufferSize() const
{
	// VBV Buffer Size Value を返す
	return m_Header.dwVbvBufferSize;
}

bool CMpeg2Sequence::IsConstrainedParamFlag() const
{
	// Constrained Parameters Flag を返す
	return m_Header.bConstrainedParamFlag;
}

bool CMpeg2Sequence::IsLoadIntraQuantiserMatrix() const
{
	// Load Intra Quantiser Matrix を返す
	return m_Header.bLoadIntraQuantiserMatrix;
}

bool CMpeg2Sequence::GetExtendDisplayInfo() const
{
	// Extention Sequence Display があるかを返す
	return m_Header.Extention.Display.bHave;
}

WORD CMpeg2Sequence::GetExtendDisplayHorizontalSize() const
{
	// Horizontal Size Value を返す
	return m_Header.Extention.Display.wDisplayHorizontalSize;
}

WORD CMpeg2Sequence::GetExtendDisplayVerticalSize() const
{
	// Vertical Size Value を返す
	return m_Header.Extention.Display.wDisplayVerticalSize;
}

void CMpeg2Sequence::SetFixSquareDisplay(bool bFix)
{
	m_bFixSquareDisplay = bFix;
}


//////////////////////////////////////////////////////////////////////
// CMpeg2Parserクラスの構築/消滅
//////////////////////////////////////////////////////////////////////

CMpeg2Parser::CMpeg2Parser(ISequenceHandler *pSequenceHandler)
	: m_pSequenceHandler(pSequenceHandler)
{
	Reset();
}

bool CMpeg2Parser::StoreEs(const BYTE *pData, const DWORD dwSize)
{
	static const BYTE StartCode[] = {0x00U, 0x00U, 0x01U, 0xB3U};
	bool bTrigger = false;
	DWORD dwPos,dwStart;

	for (dwPos = 0UL; dwPos < dwSize; dwPos += dwStart) {
		// スタートコードを検索する
		//dwStart = FindStartCode(&pData[dwPos], dwSize - dwPos);
		DWORD Remain = dwSize - dwPos;
		DWORD SyncState = m_dwSyncState;
		for (dwStart = 0UL; dwStart < Remain; dwStart++) {
			SyncState=(SyncState<<8) | (DWORD)pData[dwPos + dwStart];
			if (SyncState == 0x000001B3UL) {
				// スタートコード発見、シフトレジスタを初期化する
				SyncState = 0xFFFFFFFFUL;
				break;
			}
		}
		m_dwSyncState = SyncState;

		if (dwStart < Remain) {
			dwStart++;
			if (m_Mpeg2Sequence.GetSize() >= 4UL) {
				if (dwStart < 4UL) {
					// スタートコードの断片を取り除く
					m_Mpeg2Sequence.TrimTail(4UL - dwStart);
				} else if (dwStart > 4UL) {
					m_Mpeg2Sequence.AddData(&pData[dwPos], dwStart - 4);
				}

				// シーケンスを出力する
				if (m_Mpeg2Sequence.ParseHeader())
					OnMpeg2Sequence(&m_Mpeg2Sequence);
			}

			// スタートコードをセットする
			m_Mpeg2Sequence.SetData(StartCode, 4UL);
			bTrigger = true;
		} else {
			if (m_Mpeg2Sequence.GetSize() >= 4UL) {
				// シーケンスストア
				if (m_Mpeg2Sequence.AddData(&pData[dwPos], Remain) >= 0x1000000UL) {
					// 例外(シーケンスが16MBを超える)
					m_Mpeg2Sequence.ClearSize();
				}
			}
			break;
		}
	}

	return bTrigger;
}

void CMpeg2Parser::Reset()
{
	// 状態を初期化する
	//m_bIsStoring = false;
	m_dwSyncState = 0xFFFFFFFFUL;

	m_Mpeg2Sequence.Reset();
}

void CMpeg2Parser::SetFixSquareDisplay(bool bFix)
{
	m_Mpeg2Sequence.SetFixSquareDisplay(bFix);
}

void CMpeg2Parser::OnPesPacket(const CPesParser *pPesParser, const CPesPacket *pPacket)
{
	// CPesParser::IPacketHandlerインタフェースの実装
	StorePacket(pPacket);
}

void CMpeg2Parser::OnMpeg2Sequence(const CMpeg2Sequence *pSequence) const
{
	// ハンドラ呼び出し
	if(m_pSequenceHandler)m_pSequenceHandler->OnMpeg2Sequence(this, pSequence);
}

/*
inline const DWORD CMpeg2Parser::FindStartCode(const BYTE *pData, const DWORD dwDataSize)
{
	// Sequence Header Code (0x000001B3) を検索する
	DWORD dwPos;

	for(dwPos = 0UL ; dwPos < dwDataSize ; dwPos++){
		m_dwSyncState <<= 8;
		m_dwSyncState |= (DWORD)pData[dwPos];

		if(m_dwSyncState == 0x000001B3UL){
			// スタートコード発見、シフトレジスタを初期化する
			m_dwSyncState = 0xFFFFFFFFUL;
			break;
			}
		}

	return dwPos;
}
*/




static DWORD EBSPToRBSP(BYTE *pData, DWORD DataSize)
{
	DWORD j = 0;
	int Count = 0;
	for (DWORD i = 0; i < DataSize; i++) {
		if (Count == 2) {
			if (pData[i] < 0x03)
				return (DWORD)-1;
			if (pData[i] == 0x03) {
				if (i < DataSize - 1 && pData[i + 1] > 0x03)
					return (DWORD)-1;
				if (i == DataSize - 1)
					break;
				i++;
				Count = 0;
			}
		}
		pData[j++] = pData[i];
		if (pData[i] == 0x00)
			Count++;
		else
			Count = 0;
	}
	return j;
}


//////////////////////////////////////////////////////////////////////
// CH264AccessUnitクラスの構築/消滅
//////////////////////////////////////////////////////////////////////

//#define STRICT_1SEG	// ワンセグ規格準拠

CH264AccessUnit::CH264AccessUnit()
{
	Reset();
}

bool CH264AccessUnit::ParseHeader()
{
	if (m_dwDataSize < 5
			|| m_pData[0] != 0 || m_pData[1] != 0 || m_pData[2] != 0x01)
		return false;

	DWORD Pos = 3;
	while (true) {
		DWORD SyncState = 0xFFFFFFFFUL;
		DWORD NextPos = Pos + 1;
		bool bFoundStartCode = false;
		for (; NextPos < m_dwDataSize - 2; NextPos++) {
			SyncState = (SyncState << 8) | (DWORD)m_pData[NextPos];
			if ((SyncState & 0x00FFFFFF) == 0x00000001UL) {
				bFoundStartCode = true;
				NextPos++;
				break;
			}
		}
		if (!bFoundStartCode)
			break;

		const BYTE NALUnitType = m_pData[Pos++] & 0x1F;
		DWORD NALUnitSize = NextPos - 3 - Pos;

		NALUnitSize = EBSPToRBSP(&m_pData[Pos], NALUnitSize);
		if (NALUnitSize == (DWORD)-1)
			break;

		if (NALUnitType == 0x07) {
			// Sequence parameter set
			CBitstream Bitstream(&m_pData[Pos], NALUnitSize);

			m_Header.SPS.ProfileIdc = (BYTE)Bitstream.GetBits(8);
			m_Header.SPS.bConstraintSet0Flag = Bitstream.GetFlag();
			m_Header.SPS.bConstraintSet1Flag = Bitstream.GetFlag();
			m_Header.SPS.bConstraintSet2Flag = Bitstream.GetFlag();
			m_Header.SPS.bConstraintSet3Flag = Bitstream.GetFlag();
			if (Bitstream.GetBits(4) != 0)	// reserved_zero_4bits
				return false;
			m_Header.SPS.LevelIdc = (BYTE)Bitstream.GetBits(8);
			m_Header.SPS.SeqParameterSetId = Bitstream.GetUE_V();
			m_Header.SPS.ChromaFormatIdc = 1;
			m_Header.SPS.bSeparateColourPlaneFlag = false;
			m_Header.SPS.BitDepthLumaMinus8 = 0;
			m_Header.SPS.BitDepthChromaMinus8 = 0;
			m_Header.SPS.bQpprimeYZeroTransformBypassFlag = false;
			m_Header.SPS.bSeqScalingMatrixPresentFlag = false;
			if (m_Header.SPS.ProfileIdc == 100
					|| m_Header.SPS.ProfileIdc == 110
					|| m_Header.SPS.ProfileIdc == 122
					|| m_Header.SPS.ProfileIdc == 244
					|| m_Header.SPS.ProfileIdc == 44
					|| m_Header.SPS.ProfileIdc == 83
					|| m_Header.SPS.ProfileIdc == 86) {
				// High profile
				m_Header.SPS.ChromaFormatIdc = Bitstream.GetUE_V();
				if (m_Header.SPS.ChromaFormatIdc == 3)	// YUY444
					m_Header.SPS.bSeparateColourPlaneFlag = Bitstream.GetFlag();
				m_Header.SPS.BitDepthLumaMinus8 = Bitstream.GetUE_V();
				m_Header.SPS.BitDepthChromaMinus8 = Bitstream.GetUE_V();
				m_Header.SPS.bQpprimeYZeroTransformBypassFlag = Bitstream.GetFlag();
				m_Header.SPS.bSeqScalingMatrixPresentFlag = Bitstream.GetFlag();
				if (m_Header.SPS.bSeqScalingMatrixPresentFlag) {
					const int Length = m_Header.SPS.ChromaFormatIdc != 3 ? 8 : 12;
					for (int i = 0; i < Length; i++) {
						if (Bitstream.GetFlag()) {	// seq_scaling_list_present_flag
							int LastScale = 8, NextScale = 8;
							for (int j = 0; j < (i < 6 ? 16 : 64); j++) {
								if (NextScale != 0) {
									int DeltaScale = Bitstream.GetSE_V();
									NextScale = (LastScale + DeltaScale + 256) % 256;
									LastScale = NextScale;
								}
							}
						}
					}
				}
			}
			m_Header.SPS.Log2MaxFrameNumMinus4 = Bitstream.GetUE_V();
			m_Header.SPS.PicOrderCntType = Bitstream.GetUE_V();
			if (m_Header.SPS.PicOrderCntType == 0) {
				m_Header.SPS.Log2MaxPicOrderCntLsbMinus4 = Bitstream.GetUE_V();
			} else if (m_Header.SPS.PicOrderCntType == 1) {
				m_Header.SPS.bDeltaPicOrderAlwaysZeroFlag = Bitstream.GetFlag();
				m_Header.SPS.OffsetForNonRefPic = Bitstream.GetSE_V();
				m_Header.SPS.OffsetForTopToBottomField = Bitstream.GetSE_V();
				m_Header.SPS.NumRefFramesInPicOrderCntCycle = Bitstream.GetUE_V();
				for (int i = 0; i < m_Header.SPS.NumRefFramesInPicOrderCntCycle; i++)
					Bitstream.GetSE_V();	// offset_for_ref_frame
			}
			m_Header.SPS.NumRefFrames = Bitstream.GetUE_V();
			m_Header.SPS.bGapsInFrameNumValueAllowedFlag = Bitstream.GetFlag();
			m_Header.SPS.PicWidthInMbsMinus1 = Bitstream.GetUE_V();
			m_Header.SPS.PicHeightInMapUnitsMinus1 = Bitstream.GetUE_V();
			m_Header.SPS.bFrameMbsOnlyFlag = Bitstream.GetFlag();
			if (!m_Header.SPS.bFrameMbsOnlyFlag)
				m_Header.SPS.bMbAdaptiveFrameFieldFlag = Bitstream.GetFlag();
			m_Header.SPS.bDirect8x8InferenceFlag = Bitstream.GetFlag();
			m_Header.SPS.bFrameCroppingFlag = Bitstream.GetFlag();
			if (m_Header.SPS.bFrameCroppingFlag) {
				m_Header.SPS.FrameCropLeftOffset = Bitstream.GetUE_V();
				m_Header.SPS.FrameCropRightOffset = Bitstream.GetUE_V();
				m_Header.SPS.FrameCropTopOffset = Bitstream.GetUE_V();
				m_Header.SPS.FrameCropBottomOffset = Bitstream.GetUE_V();
			}
			m_Header.SPS.bVuiParametersPresentFlag = Bitstream.GetFlag();
			if (m_Header.SPS.bVuiParametersPresentFlag) {
				m_Header.SPS.VUI.bAspectRatioInfoPresentFlag = Bitstream.GetFlag();
				if (m_Header.SPS.VUI.bAspectRatioInfoPresentFlag) {
					m_Header.SPS.VUI.AspectRatioIdc = (BYTE)Bitstream.GetBits(8);
					if (m_Header.SPS.VUI.AspectRatioIdc == 255) {
						m_Header.SPS.VUI.SarWidth = (WORD)Bitstream.GetBits(16);
						m_Header.SPS.VUI.SarHeight = (WORD)Bitstream.GetBits(16);
					}
				}
				m_Header.SPS.VUI.bOverscanInfoPresentFlag = Bitstream.GetFlag();
				if (m_Header.SPS.VUI.bOverscanInfoPresentFlag)
					m_Header.SPS.VUI.bOverscanAppropriateFlag = Bitstream.GetFlag();
				m_Header.SPS.VUI.bVideoSignalTypePresentFlag = Bitstream.GetFlag();
				if (m_Header.SPS.VUI.bVideoSignalTypePresentFlag) {
					m_Header.SPS.VUI.VideoFormat = (BYTE)Bitstream.GetBits(3);
					m_Header.SPS.VUI.bVideoFullRangeFlag = Bitstream.GetFlag();
					m_Header.SPS.VUI.bColourDescriptionPresentFlag = Bitstream.GetFlag();
					if (m_Header.SPS.VUI.bColourDescriptionPresentFlag) {
						m_Header.SPS.VUI.ColourPrimaries = (BYTE)Bitstream.GetBits(8);
						m_Header.SPS.VUI.TransferCharacteristics = (BYTE)Bitstream.GetBits(8);
						m_Header.SPS.VUI.MatrixCoefficients = (BYTE)Bitstream.GetBits(8);
					}
				}
				m_Header.SPS.VUI.bChromaLocInfoPresentFlag = Bitstream.GetFlag();
				if (m_Header.SPS.VUI.bChromaLocInfoPresentFlag) {
					m_Header.SPS.VUI.ChromaSampleLocTypeTopField = Bitstream.GetUE_V();
					m_Header.SPS.VUI.ChromaSampleLocTypeBottomField = Bitstream.GetUE_V();
				}
				m_Header.SPS.VUI.bTimingInfoPresentFlag = Bitstream.GetFlag();
				if (m_Header.SPS.VUI.bTimingInfoPresentFlag) {
					m_Header.SPS.VUI.NumUnitsInTick = Bitstream.GetBits(32);
					m_Header.SPS.VUI.TimeScale = Bitstream.GetBits(32);
					m_Header.SPS.VUI.bFixedFrameRateFlag = Bitstream.GetFlag();
				}
				// 以下まだまだあるが省略
			}
			if (m_Header.SPS.bSeparateColourPlaneFlag)
				m_Header.SPS.ChromaArrayType = 0;
			else
				m_Header.SPS.ChromaArrayType = m_Header.SPS.ChromaFormatIdc;
#ifdef STRICT_1SEG
			// ワンセグ規格に合致している?
			if (m_Header.SPS.ProfileIdc != 66
					|| !m_Header.SPS.bConstraintSet0Flag
					|| !m_Header.SPS.bConstraintSet1Flag
					|| !m_Header.SPS.bConstraintSet2Flag
					|| m_Header.SPS.LevelIdc != 12
					|| m_Header.SPS.SeqParameterSetID > 31
					|| m_Header.SPS.Log2MaxFrameNumMinus4 > 12
					|| m_Header.SPS.PicOrderCntType != 2
					|| m_Header.SPS.NumRefFrames == 0
						|| m_Header.SPS.NumRefFrames > 3
					|| m_Header.SPS.bGapsInFrameNumValueAllowedFlag
					|| m_Header.SPS.PicWidthInMbsMinus1 != 19
					|| (m_Header.SPS.PicHeightInMapUnitsMinus1 != 11
						&& m_Header.SPS.PicHeightInMapUnitsMinus1 != 14)
					|| !m_Header.SPS.bFrameMbsOnlyFlag
					|| !m_Header.SPS.bDirect8x8InferenceFlag
					|| (m_Header.SPS.bFrameCroppingFlag !=
						(m_Header.SPS.PicHeightInMapUnitsMinus1 == 11))
					|| (m_Header.SPS.bFrameCroppingFlag
						&& (m_Header.SPS.FrameCropLeftOffset != 0
							|| m_Header.SPS.FrameCropRightOffset != 0
							|| m_Header.SPS.FrameCropTopOffset != 0
							|| m_Header.SPS.FrameCropBottomOffset != 6))
					|| !m_Header.SPS.bVuiParametersPresentFlag
					|| m_Header.SPS.VUI.bAspectRatioInfoPresentFlag
					|| m_Header.SPS.VUI.bOverscanInfoPresentFlag
					|| m_Header.SPS.VUI.bVideoSignalTypePresentFlag
					|| m_Header.SPS.VUI.bChromaLocInfoPresentFlag
					|| !m_Header.SPS.VUI.bTimingInfoPresentFlag
					|| m_Header.SPS.VUI.NumUnitsInTick == 0
					|| m_Header.SPS.VUI.NumUnitsInTick % 1001 != 0
					|| (m_Header.SPS.VUI.TimeScale != 24000
						&& m_Header.SPS.VUI.TimeScale != 30000)) {
				return false;
			}
#endif
			m_bFoundSPS = true;
		} else if (NALUnitType == 0x09) {
			// Access unit delimiter
			m_Header.AUD.PrimaryPicType = m_pData[Pos] >> 5;
		} else if (NALUnitType == 0x0A) {
			// End of sequence
			break;
		}

		Pos = NextPos;
	}

	return m_bFoundSPS;
}

void CH264AccessUnit::Reset()
{
	m_bFoundSPS = false;
	::ZeroMemory(&m_Header, sizeof(m_Header));
}

WORD CH264AccessUnit::GetHorizontalSize() const
{
	WORD Width = (m_Header.SPS.PicWidthInMbsMinus1 + 1) * 16;
	if (m_Header.SPS.bFrameCroppingFlag) {
		WORD Crop = m_Header.SPS.FrameCropLeftOffset + m_Header.SPS.FrameCropRightOffset;
		if (m_Header.SPS.ChromaArrayType != 0)
			Crop *= GetSubWidthC();
		if (Crop < Width)
			Width -= Crop;
	}
	return Width;
}

WORD CH264AccessUnit::GetVerticalSize() const
{
	WORD Height = (m_Header.SPS.PicHeightInMapUnitsMinus1 + 1) * 16;
	if (!m_Header.SPS.bFrameMbsOnlyFlag)
		Height *= 2;
	if (m_Header.SPS.bFrameCroppingFlag) {
		WORD Crop = m_Header.SPS.FrameCropTopOffset + m_Header.SPS.FrameCropBottomOffset;
		if (m_Header.SPS.ChromaArrayType != 0)
			 Crop *= GetSubHeightC();
		if (!m_Header.SPS.bFrameMbsOnlyFlag)
			Crop *= 2;
		if (Crop < Height)
			Height -= Crop;
	}
	return Height;
}

bool CH264AccessUnit::GetSAR(WORD *pHorz, WORD *pVert) const
{
	static const struct {
		BYTE Horz, Vert;
	} SarList[] = {
		{  0,   0},
		{  1,   1},
		{ 12,  11},
		{ 10,  11},
		{ 16,  11},
		{ 40,  33},
		{ 24,  11},
		{ 20,  11},
		{ 32,  11},
		{ 80,  33},
		{ 18,  11},
		{ 15,  11},
		{ 64,  33},
		{160,  99},
		{  4,   3},
		{  3,   2},
		{  2,   1},
	};

	if (!m_Header.SPS.bVuiParametersPresentFlag
			|| !m_Header.SPS.VUI.bAspectRatioInfoPresentFlag)
		return false;

	WORD Horz, Vert;
	if (m_Header.SPS.VUI.AspectRatioIdc < _countof(SarList)) {
		Horz = SarList[m_Header.SPS.VUI.AspectRatioIdc].Horz;
		Vert = SarList[m_Header.SPS.VUI.AspectRatioIdc].Vert;
	} else if (m_Header.SPS.VUI.AspectRatioIdc == 255) {	// Extended_SAR
		Horz = m_Header.SPS.VUI.SarWidth;
		Vert = m_Header.SPS.VUI.SarHeight;
	} else {
		return false;
	}
	if (pHorz)
		*pHorz = Horz;
	if (pVert)
		*pVert = Vert;
	return true;
}

bool CH264AccessUnit::GetTimingInfo(TimingInfo *pInfo) const
{
	if (!m_Header.SPS.bVuiParametersPresentFlag
			|| !m_Header.SPS.VUI.bTimingInfoPresentFlag)
		return false;
	if (pInfo) {
		pInfo->NumUnitsInTick = m_Header.SPS.VUI.NumUnitsInTick;
		pInfo->TimeScale = m_Header.SPS.VUI.TimeScale;
		pInfo->bFixedFrameRateFlag = m_Header.SPS.VUI.bFixedFrameRateFlag;
	}
	return true;
}

int CH264AccessUnit::GetSubWidthC() const
{
	return (m_Header.SPS.ChromaFormatIdc == 1
			|| m_Header.SPS.ChromaFormatIdc == 2) ? 2 : 1;
}

int CH264AccessUnit::GetSubHeightC() const
{
	return (m_Header.SPS.ChromaFormatIdc == 1) ? 2 : 1;
}


//////////////////////////////////////////////////////////////////////
// CH264Parserクラスの構築/消滅
//////////////////////////////////////////////////////////////////////

CH264Parser::CH264Parser(IAccessUnitHandler *pAccessUnitHandler)
	: m_pAccessUnitHandler(pAccessUnitHandler)
{
	Reset();
}

bool CH264Parser::StoreEs(const BYTE *pData, const DWORD dwSize)
{
	bool bTrigger = false;
	DWORD dwPos,dwStart;

	for (dwPos = 0UL ; dwPos < dwSize ; dwPos += dwStart) {
		// Access unit delimiterを検索する
		DWORD Remain = dwSize - dwPos;
		DWORD SyncState = m_dwSyncState;
		for (dwStart = 0UL ; dwStart < Remain ; dwStart++) {
			SyncState = (SyncState << 8) | (DWORD)pData[dwStart + dwPos];
			if ((SyncState & 0xFFFFFF1F) == 0x00000109UL) {
				// Access unit delimiter発見
				break;
			}
		}

		if (dwStart < Remain) {
			dwStart++;
			if (m_AccessUnit.GetSize() >= 4) {
				if (dwStart > 4) {
					m_AccessUnit.AddData(&pData[dwPos], dwStart - 4);
				} else if (dwStart < 4) {
					// スタートコードの断片を取り除く
					m_AccessUnit.TrimTail(4 - dwStart);
				}

				// シーケンスを出力する
				if (m_AccessUnit.ParseHeader())
					OnAccessUnit(&m_AccessUnit);
			}

			// スタートコードをセットする
			BYTE StartCode[4];
			StartCode[0] = (BYTE)(SyncState >> 24);
			StartCode[1] = (BYTE)((SyncState >> 16) & 0xFF);
			StartCode[2] = (BYTE)((SyncState >> 8) & 0xFF);
			StartCode[3] = (BYTE)(SyncState & 0xFF);
			m_AccessUnit.SetData(StartCode, 4);

			// シフトレジスタを初期化する
			m_dwSyncState = 0xFFFFFFFFUL;
			bTrigger = true;
		} else if (m_AccessUnit.GetSize() >= 4) {
			// シーケンスストア
			if (m_AccessUnit.AddData(&pData[dwPos], Remain) >= 0x1000000UL) {
				// 例外(シーケンスが16MBを超える)
				m_AccessUnit.ClearSize();
			}
		}
	}

	return bTrigger;
}

void CH264Parser::Reset()
{
	// 状態を初期化する
	m_dwSyncState = 0xFFFFFFFFUL;

	m_AccessUnit.Reset();
}

void CH264Parser::OnPesPacket(const CPesParser *pPesParser, const CPesPacket *pPacket)
{
	// CPesParser::IPacketHandlerインタフェースの実装
	StorePacket(pPacket);
}

void CH264Parser::OnAccessUnit(const CH264AccessUnit *pAccessUnit) const
{
	// ハンドラ呼び出し
	if (m_pAccessUnitHandler)
		m_pAccessUnitHandler->OnAccessUnit(this, pAccessUnit);
}


//////////////////////////////////////////////////////////////////////
// CH265AccessUnitクラスの構築/消滅
//////////////////////////////////////////////////////////////////////

CH265AccessUnit::CH265AccessUnit()
{
	Reset();
}

bool CH265AccessUnit::ParseHeader()
{
	if (m_dwDataSize < 6
			|| m_pData[0] != 0 || m_pData[1] != 0 || m_pData[2] != 0x01)
		return false;

	DWORD Pos = 3;
	while (true) {
		DWORD SyncState = 0xFFFFFFFFUL;
		DWORD NextPos = Pos + 1;
		bool bFoundStartCode = false;
		for (; NextPos < m_dwDataSize - 3; NextPos++) {
			SyncState = (SyncState << 8) | (DWORD)m_pData[NextPos];
			if ((SyncState & 0x00FFFFFF) == 0x00000001UL) {
				bFoundStartCode = true;
				NextPos++;
				break;
			}
		}
		if (!bFoundStartCode)
			break;

		if (m_pData[Pos] & 0x80)								// forbidden_zero_bit		f(1)
			break;
		const BYTE NALUnitType = (m_pData[Pos] & 0x7E) >> 1;	// nal_unit_type			u(6)
																// nuh_layer_id				u(6)
																// nuh_temporal_id_plus1	u(3)
		Pos += 2;
		DWORD NALUnitSize = NextPos - 3 - Pos;

		NALUnitSize = EBSPToRBSP(&m_pData[Pos], NALUnitSize);
		if (NALUnitSize == (DWORD)-1)
			break;

		if (NALUnitType == 0x21) {
			// Sequence parameter set
			CBitstream Bitstream(&m_pData[Pos], NALUnitSize);

			m_Header.SPS.SpsVideoParameterSetId = (BYTE)Bitstream.GetBits(4);
			m_Header.SPS.SpsMaxSubLayersMinus1 = (BYTE)Bitstream.GetBits(3);
			m_Header.SPS.bSpsTemporalIdNestingFlag = Bitstream.GetFlag();
			// profile_tier_level
			m_Header.SPS.PTL.GeneralProfileSpace = (BYTE)Bitstream.GetBits(2);
			m_Header.SPS.PTL.bGeneralTierFlag = Bitstream.GetFlag();
			m_Header.SPS.PTL.GeneralProfileIdc = (BYTE)Bitstream.GetBits(5);
			for (int i = 0; i < 32; i++)
				m_Header.SPS.PTL.bGeneralProfileCompatibilityFlag[i] = Bitstream.GetFlag();
			m_Header.SPS.PTL.bGeneralProgressiveSourceFlag = Bitstream.GetFlag();
			m_Header.SPS.PTL.bGeneralInterlacedSourceFlag = Bitstream.GetFlag();
			m_Header.SPS.PTL.bGeneralNonPackedConstraintFlag = Bitstream.GetFlag();
			m_Header.SPS.PTL.bGeneralFrameOnlyConstraintFlag = Bitstream.GetFlag();
			Bitstream.Skip(44);	// general_reserved_zero_44bits
			m_Header.SPS.PTL.GeneralLevelIdc = (BYTE)Bitstream.GetBits(8);
			for (int i = 0; i < m_Header.SPS.SpsMaxSubLayersMinus1; i++) {
				m_Header.SPS.PTL.SubLayer[i].bSubLayerProfilePresentFlag = Bitstream.GetFlag();
				m_Header.SPS.PTL.SubLayer[i].bSubLayerLevelPresentFlag = Bitstream.GetFlag();
			}
			if (m_Header.SPS.SpsMaxSubLayersMinus1 > 0)
				Bitstream.Skip((8 - m_Header.SPS.SpsMaxSubLayersMinus1) * 2);	// reserved_zero_2bits
			for (int i = 0; i < m_Header.SPS.SpsMaxSubLayersMinus1; i++) {
				if (m_Header.SPS.PTL.SubLayer[i].bSubLayerProfilePresentFlag) {
					m_Header.SPS.PTL.SubLayer[i].SubLayerProfileSpace = (BYTE)Bitstream.GetBits(2);
					m_Header.SPS.PTL.SubLayer[i].bSubLayerTierFlag = Bitstream.GetFlag();
					m_Header.SPS.PTL.SubLayer[i].SubLayerProfileIdc = (BYTE)Bitstream.GetBits(5);
					for (int j = 0; j < 32; j++)
						m_Header.SPS.PTL.SubLayer[i].bSubLayerProfileCompatibilityFlag[j] = Bitstream.GetFlag();
					m_Header.SPS.PTL.SubLayer[i].bSubLayerProgressiveSourceFlag = Bitstream.GetFlag();
					m_Header.SPS.PTL.SubLayer[i].bSubLayerInterlacedSourceFlag = Bitstream.GetFlag();
					m_Header.SPS.PTL.SubLayer[i].bSubLayerNonPackedConstraintFlag = Bitstream.GetFlag();
					m_Header.SPS.PTL.SubLayer[i].bSubLayerFrameOnlyConstraintFlag = Bitstream.GetFlag();
					Bitstream.Skip(44);	// sub_layer_reserved_zero_44bits
				}
				if (m_Header.SPS.PTL.SubLayer[i].bSubLayerLevelPresentFlag) {
					m_Header.SPS.PTL.SubLayer[i].SubLayerLevelIdc = (BYTE)Bitstream.GetBits(8);
				}
			}
			m_Header.SPS.SpsSeqParameterSetId = Bitstream.GetUE_V();
			m_Header.SPS.ChromaFormatIdc = Bitstream.GetUE_V();
			if (m_Header.SPS.ChromaFormatIdc == 3)
				m_Header.SPS.bSeparateColourPlaneFlag = Bitstream.GetFlag();
			m_Header.SPS.PicWidthInLumaSamples = Bitstream.GetUE_V();
			m_Header.SPS.PicHeightInLumaSamples = Bitstream.GetUE_V();
			m_Header.SPS.bConformanceWindowFlag = Bitstream.GetFlag();
			if (m_Header.SPS.bConformanceWindowFlag) {
				m_Header.SPS.ConfWinLeftOffset = Bitstream.GetUE_V();
				m_Header.SPS.ConfWinRightOffset = Bitstream.GetUE_V();
				m_Header.SPS.ConfWinTopOffset = Bitstream.GetUE_V();
				m_Header.SPS.ConfWinBottomOffset = Bitstream.GetUE_V();
			}
			m_Header.SPS.BitDepthLumaMinus8 = Bitstream.GetUE_V();
			m_Header.SPS.BitDepthChromaMinus8 = Bitstream.GetUE_V();
			m_Header.SPS.Log2MaxPicOrderCntLsbMinus4 = Bitstream.GetUE_V();
			m_Header.SPS.bSpsSubLayerOrderingInfoPresentFlag = Bitstream.GetFlag();
			for (int i = m_Header.SPS.bSpsSubLayerOrderingInfoPresentFlag ? 0 : m_Header.SPS.SpsMaxSubLayersMinus1;
					i <= m_Header.SPS.SpsMaxSubLayersMinus1; i++) {
				m_Header.SPS.SubLayerOrderingInfo[i].SpsMaxDecPicBufferingMinus1 = Bitstream.GetUE_V();
				m_Header.SPS.SubLayerOrderingInfo[i].SpsMaxNumReorderPics = Bitstream.GetUE_V();
				m_Header.SPS.SubLayerOrderingInfo[i].SpsMaxLatencyIncreasePlus1 = Bitstream.GetUE_V();
			}
			m_Header.SPS.Log2MinLumaCodingBlockSizeMinus3 = Bitstream.GetUE_V();
			m_Header.SPS.Log2DiffMaxMinLumaCodingBlockSize = Bitstream.GetUE_V();
			m_Header.SPS.Log2MinTransformBlockSizeMinus2 = Bitstream.GetUE_V();
			m_Header.SPS.Log2DiffMaxMinTransformBlockSize = Bitstream.GetUE_V();
			m_Header.SPS.MaxTransformHierarchyDepthInter = Bitstream.GetUE_V();
			m_Header.SPS.MaxTransformHierarchyDepthIntra = Bitstream.GetUE_V();
			m_Header.SPS.bScalingListEnabledFlag = Bitstream.GetFlag();
			if (m_Header.SPS.bScalingListEnabledFlag) {
				m_Header.SPS.bSpsScalingListDataPresentFlag = Bitstream.GetFlag();
				if (m_Header.SPS.bSpsScalingListDataPresentFlag) {
					// scaling_list_data
					for (int SizeId = 0; SizeId < 4; SizeId++) {
						for (int MatrixId = 0; MatrixId < ((SizeId == 3) ? 2 : 6); MatrixId++) {
							bool bScalingListPredModeFlag = Bitstream.GetFlag();
							if (!bScalingListPredModeFlag) {
								Bitstream.GetUE_V();	// scaling_list_pred_matrix_id_delta
							} else {
								int NextCoef = 8, CoefNum = min(64, 1 << (4 + (SizeId << 1)));
								if (SizeId > 1) {
									int ScalingListDcCoefMinus8 = Bitstream.GetSE_V();
									NextCoef = ScalingListDcCoefMinus8 + 8;
								}
								for (int i = 0; i < CoefNum; i++) {
									int ScalingListDeltaCoef = Bitstream.GetSE_V();
									NextCoef = (NextCoef + ScalingListDeltaCoef + 256) % 256;
								}
							}
						}
					}
				}
			}
			m_Header.SPS.bAmpEnabledFlag = Bitstream.GetFlag();
			m_Header.SPS.bSampleAdaptiveOffsetEnabledFlag = Bitstream.GetFlag();
			m_Header.SPS.bPcmEnabledFlag = Bitstream.GetFlag();
			if (m_Header.SPS.bPcmEnabledFlag) {
				m_Header.SPS.PcmSampleBitDepthLumaMinus1 = (BYTE)Bitstream.GetBits(4);
				m_Header.SPS.PcmSampleBitDepthChromaMinus1 = (BYTE)Bitstream.GetBits(4);
				m_Header.SPS.Log2MinPcmLumaCodingBlockSizeMinus3 = Bitstream.GetUE_V();
				m_Header.SPS.Log2DiffMaxMinPcmLumaCodingBlockSize = Bitstream.GetUE_V();
				m_Header.SPS.bPcmLoopFilterDisabledFlag = Bitstream.GetFlag();
			}
			m_Header.SPS.NumShortTermRefPicSets = Bitstream.GetUE_V();
			int NumPics = 0;
			for (int i = 0; i < m_Header.SPS.NumShortTermRefPicSets; i++) {
				// short_term_ref_pic_set
				bool bInterRefPicSetPredictionFlag = false;
				if (i != 0)
					bInterRefPicSetPredictionFlag = Bitstream.GetFlag();
				if (bInterRefPicSetPredictionFlag) {
					Bitstream.GetFlag();	// delta_rps_sign
					Bitstream.GetUE_V();	// abs_delta_rps_minus1
					int NumPicsNew = 0;
					for (int j = 0; j <= NumPics; j++) {
						bool bUsedByCurrPicFlag = Bitstream.GetFlag();
						if (bUsedByCurrPicFlag) {
							NumPicsNew++;
						} else {
							bool bUseDeltaFlag = Bitstream.GetFlag();
							if (bUseDeltaFlag)
								NumPicsNew++;
						}
					}
					NumPics = NumPicsNew;
				} else {
					int NumNegativePics = Bitstream.GetUE_V();
					int NumPositivePics = Bitstream.GetUE_V();
					NumPics = NumNegativePics + NumPositivePics;
					for (int j = 0; j < NumNegativePics; j++) {
						Bitstream.GetUE_V();	// delta_poc_s0_minus1
						Bitstream.GetFlag();	// used_by_curr_pic_s0_flag
					}
					for (int j = 0; j < NumPositivePics; j++) {
						Bitstream.GetUE_V();	// delta_poc_s1_minus1
						Bitstream.GetFlag();	// used_by_curr_pic_s1_flag
					}
				}
			}
			m_Header.SPS.bLongTermRefPicsPresentFlag = Bitstream.GetFlag();
			if (m_Header.SPS.bLongTermRefPicsPresentFlag) {
				m_Header.SPS.NumLongTermRefPicsSps = Bitstream.GetUE_V();
				for (int i = 0; i < m_Header.SPS.NumLongTermRefPicsSps; i++) {
					Bitstream.Skip(m_Header.SPS.Log2MaxPicOrderCntLsbMinus4 + 4);	// lt_ref_pic_poc_lsb_sps
					Bitstream.GetFlag();											// used_by_curr_pic_lt_sps_flag
				}
			}
			m_Header.SPS.bSpsTemporalMvpEnabledFlag = Bitstream.GetFlag();
			m_Header.SPS.bStrongIntraSmoothingEnabledFlag = Bitstream.GetFlag();
			m_Header.SPS.bVuiParametersPresentFlag = Bitstream.GetFlag();
			if (m_Header.SPS.bVuiParametersPresentFlag) {
				// vui_parameters
				m_Header.SPS.VUI.bAspectRatioInfoPresentFlag = Bitstream.GetFlag();
				if (m_Header.SPS.VUI.bAspectRatioInfoPresentFlag) {
					m_Header.SPS.VUI.AspectRatioIdc = (BYTE)Bitstream.GetBits(8);
					if (m_Header.SPS.VUI.AspectRatioIdc == 0xFF) {	// EXTENDED_SAR
						m_Header.SPS.VUI.SarWidth = (WORD)Bitstream.GetBits(16);
						m_Header.SPS.VUI.SarHeight = (WORD)Bitstream.GetBits(16);
					}
				}
				m_Header.SPS.VUI.bOverscanInfoPresentFlag = Bitstream.GetFlag();
				if (m_Header.SPS.VUI.bOverscanInfoPresentFlag)
					m_Header.SPS.VUI.bOverscanAppropriateFlag = Bitstream.GetFlag();
				m_Header.SPS.VUI.bVideoSignalTypePresentFlag = Bitstream.GetFlag();
				if (m_Header.SPS.VUI.bVideoSignalTypePresentFlag) {
					m_Header.SPS.VUI.VideoFormat = (BYTE)Bitstream.GetBits(3);
					m_Header.SPS.VUI.bVideoFullRangeFlag = Bitstream.GetFlag();
					m_Header.SPS.VUI.bColourDescriptionPresentFlag = Bitstream.GetFlag();
					if (m_Header.SPS.VUI.bColourDescriptionPresentFlag) {
						m_Header.SPS.VUI.ColourPrimaries = (BYTE)Bitstream.GetBits(8);
						m_Header.SPS.VUI.TransferCharacteristics = (BYTE)Bitstream.GetBits(8);
						m_Header.SPS.VUI.MatrixCoeffs = (BYTE)Bitstream.GetBits(8);
					}
				}
				m_Header.SPS.VUI.bChromaLocInfoPresentFlag = Bitstream.GetFlag();
				if (m_Header.SPS.VUI.bChromaLocInfoPresentFlag) {
					m_Header.SPS.VUI.ChromaSampleLocTypeTopField = Bitstream.GetUE_V();
					m_Header.SPS.VUI.ChromaSampleLocTypeBottomField = Bitstream.GetUE_V();
				}
				m_Header.SPS.VUI.bNeutralChromaIndicationFlag = Bitstream.GetFlag();
				m_Header.SPS.VUI.bFieldSeqFlag = Bitstream.GetFlag();
				m_Header.SPS.VUI.bFrameFieldInfoPresentFlag = Bitstream.GetFlag();
				m_Header.SPS.VUI.bDefaultDisplayWindowFlag = Bitstream.GetFlag();
				if (m_Header.SPS.VUI.bDefaultDisplayWindowFlag) {
					m_Header.SPS.VUI.DefDispWinLeftOffset = Bitstream.GetUE_V();
					m_Header.SPS.VUI.DefDispWinRightOffset = Bitstream.GetUE_V();
					m_Header.SPS.VUI.DefDispWinTopOffset = Bitstream.GetUE_V();
					m_Header.SPS.VUI.DefDispWinBottomOffset = Bitstream.GetUE_V();
				}
				m_Header.SPS.VUI.bVuiTimingInfoPresentFlag = Bitstream.GetFlag();
				if (m_Header.SPS.VUI.bVuiTimingInfoPresentFlag) {
					m_Header.SPS.VUI.VuiNumUnitsInTick = Bitstream.GetBits(32);
					m_Header.SPS.VUI.VuiTimeScale = Bitstream.GetBits(32);
					m_Header.SPS.VUI.bVuiPocProportionalToTimingFlag = Bitstream.GetFlag();
					if (m_Header.SPS.VUI.bVuiPocProportionalToTimingFlag)
						m_Header.SPS.VUI.VuiNumTicksPocDiffOneMinus1 = Bitstream.GetUE_V();
					m_Header.SPS.VUI.bVuiHrdParametersPresentFlag = Bitstream.GetFlag();
#if 0	// 割愛…
					if (m_Header.SPS.VUI.bVuiHrdParametersPresentFlag) {
						// hrd_parameters
					}
#endif
				}
#if 0
				m_Header.SPS.VUI.bBitstreamRestrictionFlag = Bitstream.GetFlag();
				if (m_Header.SPS.VUI.bBitstreamRestrictionFlag) {
					m_Header.SPS.VUI.bTilesFixedStructureFlag = Bitstream.GetFlag();
					m_Header.SPS.VUI.bMotionVectorsOverPicBoundariesFlag = Bitstream.GetFlag();
					m_Header.SPS.VUI.bRestrictedRefPicListsFlag = Bitstream.GetFlag();
					m_Header.SPS.VUI.MinSpatialSegmentationIdc = Bitstream.GetUE_V();
					m_Header.SPS.VUI.MaxBytesPerPicDenom = Bitstream.GetUE_V();
					m_Header.SPS.VUI.MaxBitsPerMinCuDenom = Bitstream.GetUE_V();
					m_Header.SPS.VUI.Log2MaxMvLengthHorizontal = Bitstream.GetUE_V();
					m_Header.SPS.VUI.Log2MaxMvLengthVertical = Bitstream.GetUE_V();
				}
#endif
			}

			m_bFoundSPS = true;
		} else if (NALUnitType == 0x23) {
			// Access unit delimiter
			m_Header.AUD.PicType = m_pData[Pos] >> 5;
		} else if (NALUnitType == 0x24) {
			// End of sequence
			break;
		}

		Pos = NextPos;
	}

	return m_bFoundSPS;
}

void CH265AccessUnit::Reset(void)
{
	m_bFoundSPS = false;
	::ZeroMemory(&m_Header, sizeof(m_Header));
}

WORD CH265AccessUnit::GetHorizontalSize() const
{
	WORD Width = m_Header.SPS.PicWidthInLumaSamples;
	if (m_Header.SPS.bConformanceWindowFlag) {
		WORD Crop = (m_Header.SPS.ConfWinLeftOffset + m_Header.SPS.ConfWinRightOffset) * GetSubWidthC();
		if (Crop < Width)
			Width -= Crop;
	}
	return Width;
}

WORD CH265AccessUnit::GetVerticalSize() const
{
	WORD Height = m_Header.SPS.PicHeightInLumaSamples;
	if (m_Header.SPS.bConformanceWindowFlag) {
		WORD Crop = (m_Header.SPS.ConfWinTopOffset + m_Header.SPS.ConfWinBottomOffset) * GetSubHeightC();
		if (Crop < Height)
			Height -= Crop;
	}
	return Height;
}

bool CH265AccessUnit::GetSAR(WORD *pHorz, WORD *pVert) const
{
	static const struct {
		BYTE Horz, Vert;
	} SarList[] = {
		{  0,   0},
		{  1,   1},
		{ 12,  11},
		{ 10,  11},
		{ 16,  11},
		{ 40,  33},
		{ 24,  11},
		{ 20,  11},
		{ 32,  11},
		{ 80,  33},
		{ 18,  11},
		{ 15,  11},
		{ 64,  33},
		{160,  99},
		{  4,   3},
		{  3,   2},
		{  2,   1},
	};

	if (!m_Header.SPS.bVuiParametersPresentFlag
			|| !m_Header.SPS.VUI.bAspectRatioInfoPresentFlag)
		return false;

	WORD Horz, Vert;
	if (m_Header.SPS.VUI.AspectRatioIdc < _countof(SarList)) {
		Horz = SarList[m_Header.SPS.VUI.AspectRatioIdc].Horz;
		Vert = SarList[m_Header.SPS.VUI.AspectRatioIdc].Vert;
	} else if (m_Header.SPS.VUI.AspectRatioIdc == 255) {	// EXTENDED_SAR
		Horz = m_Header.SPS.VUI.SarWidth;
		Vert = m_Header.SPS.VUI.SarHeight;
	} else {
		return false;
	}
	if (pHorz)
		*pHorz = Horz;
	if (pVert)
		*pVert = Vert;
	return true;
}

bool CH265AccessUnit::GetTimingInfo(TimingInfo *pInfo) const
{
	if (!m_Header.SPS.bVuiParametersPresentFlag
			|| !m_Header.SPS.VUI.bVuiTimingInfoPresentFlag)
		return false;
	if (pInfo) {
		pInfo->NumUnitsInTick = m_Header.SPS.VUI.VuiNumUnitsInTick;
		pInfo->TimeScale = m_Header.SPS.VUI.VuiTimeScale;
	}
	return true;
}

int CH265AccessUnit::GetSubWidthC() const
{
	return (m_Header.SPS.ChromaFormatIdc == 1
			|| m_Header.SPS.ChromaFormatIdc == 2) ? 2 : 1;
}

int CH265AccessUnit::GetSubHeightC() const
{
	return (m_Header.SPS.ChromaFormatIdc == 1) ? 2 : 1;
}


//////////////////////////////////////////////////////////////////////
// CH265Parserクラスの構築/消滅
//////////////////////////////////////////////////////////////////////

CH265Parser::CH265Parser(IAccessUnitHandler *pAccessUnitHandler)
	: m_pAccessUnitHandler(pAccessUnitHandler)
{
	Reset();
}

bool CH265Parser::StoreEs(const BYTE *pData, const DWORD dwSize)
{
	bool bTrigger = false;
	DWORD dwPos,dwStart;

	for (dwPos = 0UL ; dwPos < dwSize ; dwPos += dwStart) {
		// Access unit delimiterを検索する
		DWORD Remain = dwSize - dwPos;
		DWORD SyncState = m_dwSyncState;
		for (dwStart = 0UL ; dwStart < Remain ; dwStart++) {
			SyncState = (SyncState << 8) | (DWORD)pData[dwStart + dwPos];
			if ((SyncState & 0xFFFFFFFE) == 0x00000146UL) {
				// Access unit delimiter発見
				break;
			}
		}

		if (dwStart < Remain) {
			dwStart++;
			if (m_AccessUnit.GetSize() >= 4) {
				if (dwStart > 4) {
					m_AccessUnit.AddData(&pData[dwPos], dwStart - 4);
				} else if (dwStart < 4) {
					// スタートコードの断片を取り除く
					m_AccessUnit.TrimTail(4 - dwStart);
				}

				// シーケンスを出力する
				if (m_AccessUnit.ParseHeader())
					OnAccessUnit(&m_AccessUnit);
			}

			// スタートコードをセットする
			BYTE StartCode[4];
			StartCode[0] = (BYTE)(SyncState >> 24);
			StartCode[1] = (BYTE)((SyncState >> 16) & 0xFF);
			StartCode[2] = (BYTE)((SyncState >> 8) & 0xFF);
			StartCode[3] = (BYTE)(SyncState & 0xFF);
			m_AccessUnit.SetData(StartCode, 4);

			// シフトレジスタを初期化する
			m_dwSyncState = 0xFFFFFFFFUL;
			bTrigger = true;
		} else if (m_AccessUnit.GetSize() >= 4) {
			// シーケンスストア
			if (m_AccessUnit.AddData(&pData[dwPos], Remain) >= 0x1000000UL) {
				// 例外(シーケンスが16MBを超える)
				m_AccessUnit.ClearSize();
			}
		}
	}

	return bTrigger;
}

void CH265Parser::Reset()
{
	// 状態を初期化する
	m_dwSyncState = 0xFFFFFFFFUL;

	m_AccessUnit.Reset();
}

void CH265Parser::OnPesPacket(const CPesParser *pPesParser, const CPesPacket *pPacket)
{
	// CPesParser::IPacketHandlerインタフェースの実装
	StorePacket(pPacket);
}

void CH265Parser::OnAccessUnit(const CH265AccessUnit *pAccessUnit) const
{
	// ハンドラ呼び出し
	if (m_pAccessUnitHandler)
		m_pAccessUnitHandler->OnAccessUnit(this, pAccessUnit);
}
