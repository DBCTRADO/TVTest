// TsPacketParser.cpp: CTsPacketParser クラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "TsPacketParser.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


#define TS_HEADSYNCBYTE		(0x47U)


//////////////////////////////////////////////////////////////////////
// 構築/消滅
//////////////////////////////////////////////////////////////////////


CTsPacketParser::CTsPacketParser(IEventHandler *pEventHandler)
	: CMediaDecoder(pEventHandler, 1UL, 1UL)
	, m_bOutputNullPacket(false)
	, m_InputPacketCount(0)
	, m_OutputPacketCount(0)
	, m_ErrorPacketCount(0)
	, m_ContinuityErrorPacketCount(0)
#ifdef BONTSENGINE_1SEG_SUPPORT
	, m_bGeneratePAT(true)
#endif
{
	// パケット連続性カウンタを初期化する
	::FillMemory(m_abyContCounter, sizeof(m_abyContCounter), 0x10UL);
}

CTsPacketParser::~CTsPacketParser()
{
}

void CTsPacketParser::Reset(void)
{
	CBlockLock Lock(&m_DecoderLock);

	// パケットカウンタをクリアする
	m_InputPacketCount = 0;
	m_OutputPacketCount = 0;
	m_ErrorPacketCount = 0;
	m_ContinuityErrorPacketCount = 0;

	// パケット連続性カウンタを初期化する
	::FillMemory(m_abyContCounter, sizeof(m_abyContCounter), 0x10UL);

	// 状態をリセットする
	m_TsPacket.ClearSize();

#ifdef BONTSENGINE_1SEG_SUPPORT
	m_PATGenerator.Reset();
#endif
}

const bool CTsPacketParser::InputMedia(CMediaData *pMediaData, const DWORD dwInputIndex)
{
	CBlockLock Lock(&m_DecoderLock);

	/*
	if (dwInputIndex >= GetInputNum())
		return false;
	*/

	// TSパケットを処理する
	SyncPacket(pMediaData->GetData(), pMediaData->GetSize());

	return true;
}

void CTsPacketParser::SetOutputNullPacket(const bool bEnable)
{
	// NULLパケットの出力有無を設定する
	m_bOutputNullPacket = bEnable;
}

const DWORD CTsPacketParser::GetInputPacketCount(void) const
{
	// 入力パケット数を返す
	return (DWORD)m_InputPacketCount;
}

const DWORD CTsPacketParser::GetOutputPacketCount(void) const
{
	// 出力パケット数を返す
	return (DWORD)m_OutputPacketCount;
}

const DWORD CTsPacketParser::GetErrorPacketCount(void) const
{
	// エラーパケット数を返す
	return (DWORD)m_ErrorPacketCount;
}

const DWORD CTsPacketParser::GetContinuityErrorPacketCount(void) const
{
	// 連続性エラーパケット数を返す
	return (DWORD)m_ContinuityErrorPacketCount;
}

void CTsPacketParser::ResetErrorPacketCount(void)
{
	m_ErrorPacketCount=0;
	m_ContinuityErrorPacketCount=0;
}

void inline CTsPacketParser::SyncPacket(const BYTE *pData, const DWORD dwSize)
{
	// ※この方法は完全ではない、同期が乱れた場合に前回呼び出し時のデータまでさかのぼっては再同期はできない
	DWORD dwCurSize;
	DWORD dwCurPos = 0UL;

	while (dwCurPos < dwSize) {
		dwCurSize = m_TsPacket.GetSize();

		if (dwCurSize==0) {
			// 同期バイト待ち中
			do {
				if (pData[dwCurPos++] == TS_HEADSYNCBYTE) {
					// 同期バイト発見
					m_TsPacket.AddByte(TS_HEADSYNCBYTE);
					break;
				}
			} while (dwCurPos < dwSize);
		} else if (dwCurSize == TS_PACKETSIZE) {
			// パケットサイズ分データがそろった

			if (pData[dwCurPos] == TS_HEADSYNCBYTE) {
				// 次のデータは同期バイト
				ParsePacket();
			} else {
				// 同期エラー
				m_TsPacket.ClearSize();

				// 位置を元に戻す
				if (dwCurPos >= (TS_PACKETSIZE - 1UL))
					dwCurPos -= (TS_PACKETSIZE - 1UL);
				else
					dwCurPos = 0UL;
			}
		} else {
			// データ待ち
			DWORD dwRemain = (TS_PACKETSIZE - dwCurSize);
			if ((dwSize - dwCurPos) >= dwRemain) {
				m_TsPacket.AddData(&pData[dwCurPos], dwRemain);
				dwCurPos += dwRemain;
			} else {
				m_TsPacket.AddData(&pData[dwCurPos], dwSize - dwCurPos);
				break;
			}
		}
	}
}

bool inline CTsPacketParser::ParsePacket(void)
{
	bool bOK;

	// 入力カウントインクリメント
	m_InputPacketCount++;

	// パケットを解析/チェックする
	switch (m_TsPacket.ParsePacket(m_abyContCounter)) {
	case CTsPacket::EC_CONTINUITY:
		m_ContinuityErrorPacketCount++;
	case CTsPacket::EC_VALID:
		{
#ifdef BONTSENGINE_1SEG_SUPPORT
			/*
			// PAT の無い状態をシミュレート
			if (m_TsPacket.GetPID() == 0) {
				bOK = true;
				break;
			}
			*/
			if (m_PATGenerator.StorePacket(&m_TsPacket) && m_bGeneratePAT) {
				if (m_PATGenerator.GetPAT(&m_PATPacket)) {
					OutputMedia(&m_PATPacket);
				}
			}
#endif

			// 次のデコーダにデータを渡す
			if (m_bOutputNullPacket || m_TsPacket.GetPID() != 0x1FFFU) {
				// 出力カウントインクリメント
				OutputMedia(&m_TsPacket);

				m_OutputPacketCount++;
			}
		}
		bOK=true;
		break;
	case CTsPacket::EC_FORMAT:
	case CTsPacket::EC_TRANSPORT:
		// エラーカウントインクリメント
		m_ErrorPacketCount++;
		bOK=false;
		break;
	}

	// サイズをクリアし次のストアに備える
	m_TsPacket.ClearSize();

	return bOK;
}


#ifdef BONTSENGINE_1SEG_SUPPORT

bool CTsPacketParser::EnablePATGeneration(bool bEnable)
{
	m_bGeneratePAT = bEnable;
	return true;
}


bool CTsPacketParser::SetTransportStreamID(WORD TransportStreamID)
{
	return m_PATGenerator.SetTransportStreamID(TransportStreamID);
}

#endif
