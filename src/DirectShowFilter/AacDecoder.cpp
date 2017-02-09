// AacDecoder.cpp: CAacDecoder クラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "AacDecoder.h"

// ダウンミックス係数のパラメータ取得にFAAD2の内部情報が必要
#define HAVE_CONFIG_H
#pragma include_alias("neaacdec.h", "../Libs/faad2/include/neaacdec.h")
#include "../Libs/faad2/libfaad/common.h"
#include "../Libs/faad2/libfaad/structs.h"

#include "../Common/DebugDef.h"


// "Code from FAAD2 is copyright (c) Nero AG, www.nero.com"
#pragma comment(lib, "LibFaad.lib")


//////////////////////////////////////////////////////////////////////
// 構築/消滅
//////////////////////////////////////////////////////////////////////

CAacDecoder::CAacDecoder()
	: m_AdtsParser(NULL)
	, m_hDecoder(NULL)
	, m_bInitRequest(false)
	, m_LastChannelConfig(0xFF)
	, m_pAdtsFrame(NULL)
	, m_bDecodeError(false)
{
}


CAacDecoder::~CAacDecoder()
{
	Close();
}


bool CAacDecoder::Open()
{
	if (!OpenDecoder())
		return false;

	m_AdtsParser.Reset();
	ClearAudioInfo();

	return true;
}


void CAacDecoder::Close()
{
	CloseDecoder();
	m_AdtsParser.Reset();
	m_pAdtsFrame = NULL;
}


bool CAacDecoder::IsOpened() const
{
	return m_hDecoder != NULL;
}


bool CAacDecoder::Reset()
{
	if (!ResetDecoder())
		return false;

	m_AdtsParser.Reset();
	ClearAudioInfo();
	m_bDecodeError = false;

	return true;
}


bool CAacDecoder::Decode(const BYTE *pData, DWORD *pDataSize, DecodeFrameInfo *pInfo)
{
	if (m_hDecoder == NULL)
		return false;

	m_pAdtsFrame = NULL;

	CAdtsFrame *pFrame;
	if (!m_AdtsParser.StoreEs(pData, pDataSize, &pFrame))
		return false;

	if (!DecodeFrame(pFrame, pInfo)) {
		m_bDecodeError = true;
		return false;
	}

	m_pAdtsFrame = pFrame;
	m_bDecodeError = false;

	return true;
}


bool CAacDecoder::GetSpdifFrameInfo(SpdifFrameInfo *pInfo) const
{
	if (pInfo == NULL || m_pAdtsFrame == NULL)
		return false;

	pInfo->Pc = 0x0007;	// MPEG-2 AAC ADTS
	pInfo->FrameSize = m_pAdtsFrame->GetFrameLength();
	pInfo->SamplesPerFrame = 1024;

	return true;
}


int CAacDecoder::GetSpdifBurstPayload(BYTE *pBuffer, DWORD BufferSize) const
{
	if (pBuffer == NULL || m_pAdtsFrame == NULL)
		return 0;

	if (m_pAdtsFrame->GetRawDataBlockNum() != 0) {
		TRACE(TEXT("Invalid no_raw_data_blocks_in_frame (%d)\r\n"),
			  m_pAdtsFrame->GetRawDataBlockNum());
		return 0;
	}

	const int FrameSize = m_pAdtsFrame->GetFrameLength();
	const int DataBurstSize = (FrameSize + 1) & ~1;
	if (BufferSize < (DWORD)DataBurstSize)
		return 0;

	_swab(pointer_cast<char*>(const_cast<BYTE*>(m_pAdtsFrame->GetData())),
		  pointer_cast<char*>(pBuffer), FrameSize & ~1);
	if (FrameSize & 1) {
		pBuffer[FrameSize - 1] = 0;
		pBuffer[FrameSize] = m_pAdtsFrame->GetAt(FrameSize - 1);
	}

	return DataBurstSize;
}


bool CAacDecoder::GetChannelMap(int Channels, int *pMap) const
{
	switch (Channels) {
	case 2:
		pMap[CHANNEL_2_L] = 0;
		pMap[CHANNEL_2_R] = 1;
		break;

	case 6:
		pMap[CHANNEL_6_FL]  = 1;
		pMap[CHANNEL_6_FR]  = 2;
		pMap[CHANNEL_6_FC]  = 0;
		pMap[CHANNEL_6_LFE] = 5;
		pMap[CHANNEL_6_BL]  = 3;
		pMap[CHANNEL_6_BR]  = 4;
		break;

	default:
		return false;
	}

	return true;
}


bool CAacDecoder::GetDownmixInfo(DownmixInfo *pInfo) const
{
	if (pInfo == NULL)
		return false;

	// 5.1chダウンミックス設定
	/*
	ダウンミックス計算式 (STD-B21 6.2.1)
	┌─┬──┬─┬───┬───────────┐
	│*1│*2  │*3│kの値 │計算式(*4)            │
	├─┼──┼─┼───┼───────────┤
	│ 1│ 0/1│ 0│1/√2 │Set1                  │
	│  │    │ 1│1/2   │Lt=L+1/√2*C＋k*Ls    │
	│  │    │ 2│1/2√2│Rt=R+1/√2*C＋k*Rs    │
	│  │    │ 3│0     │                      │
	├─┼──┼─┼───┼───────────┤
	│ 0│    │  │      │Set3                  │
	│  │    │  │      │Lt=L+1/√2*C＋1/√2*Ls│
	│  │    │  │      │Rt=R+1/√2*C＋1/√2*Rs│
	└─┴──┴─┴───┴───────────┘
	*1 matrix_mixdown_idx_present
	*2 pseudo_surround_enable
	*3 matrix_mixdown_idx
	*4 L=Left, R=Right, C=Center, Ls=Rear left, Rs=Rear right

	STD-B21 5.3版より前は、最終的に Lt/Rt に 1/√2 を乗じる規定(STD-B21 付属4参照)
	ただしTVTestでは元々この規定は無視していた
	*/

	static const double PSQR = 1.0 / 1.4142135623730950488016887242097;

	pInfo->Center = PSQR;
	pInfo->Front  = 1.0;
	pInfo->Rear   = PSQR;
	pInfo->LFE    = 0.0;

	if (m_hDecoder) {
		const NeAACDecStruct *pDec = static_cast<const NeAACDecStruct*>(m_hDecoder);

		if (pDec->pce.matrix_mixdown_idx_present) {
			static const double k[4] = {PSQR, 0.5, 0.5 * PSQR, 0.0};
			pInfo->Rear = pDec->pce.matrix_mixdown_idx <= 3 ? k[pDec->pce.matrix_mixdown_idx] : PSQR;
		}
	}

	return true;
}


bool CAacDecoder::OpenDecoder()
{
	CloseDecoder();

	// FAAD2オープン
	m_hDecoder = ::NeAACDecOpen();
	if (m_hDecoder == NULL)
		return false;

	// デフォルト設定取得
	NeAACDecConfigurationPtr pDecodeConfig = ::NeAACDecGetCurrentConfiguration(m_hDecoder);

	// デコーダ設定
	pDecodeConfig->defObjectType = LC;
	pDecodeConfig->defSampleRate = 48000UL;
	pDecodeConfig->outputFormat = FAAD_FMT_16BIT;
	pDecodeConfig->downMatrix = 0;
	pDecodeConfig->useOldADTSFormat = 0;

	if (!::NeAACDecSetConfiguration(m_hDecoder, pDecodeConfig)) {
		Close();
		return false;
	}

	m_bInitRequest = true;
	m_LastChannelConfig = 0xFF;
	m_bDecodeError = false;

	return true;
}


void CAacDecoder::CloseDecoder()
{
	// FAAD2クローズ
	if (m_hDecoder) {
		::NeAACDecClose(m_hDecoder);
		m_hDecoder = NULL;
	}
}


bool CAacDecoder::ResetDecoder()
{
	if (m_hDecoder == NULL)
		return false;

	return OpenDecoder();
}


bool CAacDecoder::DecodeFrame(const CAdtsFrame *pFrame, DecodeFrameInfo *pInfo)
{
	if (m_hDecoder == NULL) {
		return false;
	}

	// 初回フレーム解析
	if (m_bInitRequest || pFrame->GetChannelConfig() != m_LastChannelConfig) {
		if (!m_bInitRequest) {
			// チャンネル設定が変化した、デコーダリセット
			if (!ResetDecoder())
				return false;
		}

		unsigned long SampleRate;
		unsigned char Channels;
		if (::NeAACDecInit(m_hDecoder,
				const_cast<BYTE*>(pFrame->GetData()), pFrame->GetSize(),
				&SampleRate, &Channels) < 0) {
			return false;
		}

		m_bInitRequest = false;
		m_LastChannelConfig = pFrame->GetChannelConfig();
	}

	// デコード
	NeAACDecFrameInfo FrameInfo;
	//::ZeroMemory(&FrameInfo, sizeof(FrameInfo));

	BYTE *pPcmBuffer = pointer_cast<BYTE*>(
		::NeAACDecDecode(m_hDecoder, &FrameInfo,
						 const_cast<BYTE*>(pFrame->GetData()), pFrame->GetSize()));

	bool bOK = false;

	if (FrameInfo.error == 0) {
		m_AudioInfo.Frequency = FrameInfo.samplerate;
		m_AudioInfo.Channels = FrameInfo.channels;
		// FAADではモノラルが2chにデコードされる
		if (FrameInfo.channels == 2 && m_LastChannelConfig == 1)
			m_AudioInfo.OrigChannels = 1;
		else
			m_AudioInfo.OrigChannels = FrameInfo.channels;
		m_AudioInfo.bDualMono = FrameInfo.channels == 2 && m_LastChannelConfig == 0;
		if (FrameInfo.samples > 0) {
			pInfo->pData = pPcmBuffer;
			pInfo->Samples = FrameInfo.samples / FrameInfo.channels;
			pInfo->Info = m_AudioInfo;
			pInfo->bDiscontinuity = m_bDecodeError;
			bOK = true;
		}
	} else {
		// エラー発生
#ifdef _DEBUG
		::OutputDebugString(TEXT("CAacDecoder::Decode error - "));
		::OutputDebugStringA(NeAACDecGetErrorMessage(FrameInfo.error));
		::OutputDebugString(TEXT("\n"));
#endif
		// リセットする
		ResetDecoder();
	}

	return bOK;
}
