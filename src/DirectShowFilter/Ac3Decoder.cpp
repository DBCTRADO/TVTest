#include "stdafx.h"
#include "Ac3Decoder.h"

extern "C" {
//#include "../Libs/a52dec/include/mm_accel.h"
#include "../Libs/a52dec/liba52/a52_internal.h"
}

#include "../Common/DebugDef.h"


#pragma comment(lib, "liba52.lib")


static inline short SampleToInt16(float Sample)
{
	const INT32 i = *pointer_cast<INT32*>(&Sample);
	if (i > 0x43C07FFF)
		return 32767;
	if (i < 0x43BF8000)
		return -32768;
	return (short)(i - 0x43C00000);
}




CAc3Decoder::CAc3Decoder()
	: m_pA52State(nullptr)
	, m_bDecodeError(false)
{
}


CAc3Decoder::~CAc3Decoder()
{
	Close();
}


bool CAc3Decoder::Open()
{
	if (!OpenDecoder())
		return false;

	ClearAudioInfo();

	return true;
}


void CAc3Decoder::Close()
{
	CloseDecoder();
}


bool CAc3Decoder::IsOpened() const
{
	return m_pA52State != nullptr;
}


bool CAc3Decoder::Reset()
{
	if (!ResetDecoder())
		return false;

	ClearAudioInfo();
	m_bDecodeError = false;

	return true;
}


bool CAc3Decoder::Decode(const BYTE *pData, DWORD *pDataSize, DecodeFrameInfo *pInfo)
{
	if (!m_pA52State)
		return false;

	if (!DecodeFrame(pData, pDataSize, pInfo)) {
		m_bDecodeError = true;
		return false;
	}

	m_bDecodeError = false;

	return true;
}


bool CAc3Decoder::GetSpdifFrameInfo(SpdifFrameInfo *pInfo) const
{
	if (pInfo == nullptr || m_FrameLength == 0)
		return false;

	pInfo->Pc = 0x0001;	// AC-3
	pInfo->FrameSize = m_FrameLength;
	pInfo->SamplesPerFrame = 256 * 6;

	return true;
}


int CAc3Decoder::GetSpdifBurstPayload(BYTE *pBuffer, DWORD BufferSize) const
{
	if (pBuffer == nullptr || m_FrameLength == 0 || m_FramePos != m_FrameLength)
		return 0;

	const int FrameSize = m_FrameLength;
	const int DataBurstSize = (FrameSize + 1) & ~1;
	if (BufferSize < (DWORD)DataBurstSize)
		return 0;

	_swab(pointer_cast<char*>(const_cast<BYTE*>(m_FrameBuffer)),
		  pointer_cast<char*>(pBuffer), FrameSize & ~1);
	if (FrameSize & 1) {
		pBuffer[FrameSize - 1] = 0;
		pBuffer[FrameSize] = m_FrameBuffer[FrameSize - 1];
	}

	return DataBurstSize;
}


bool CAc3Decoder::GetChannelMap(int Channels, int *pMap) const
{
	switch (Channels) {
	case 2:
		pMap[CHANNEL_2_L] = 0;
		pMap[CHANNEL_2_R] = 1;
		break;

	case 6:
		pMap[CHANNEL_6_FL ] = 1;
		pMap[CHANNEL_6_FR ] = 3;
		pMap[CHANNEL_6_FC ] = 2;
		pMap[CHANNEL_6_LFE] = 0;
		pMap[CHANNEL_6_BL ] = 4;
		pMap[CHANNEL_6_BR ] = 5;
		break;

	default:
		return false;
	}

	return true;
}


bool CAc3Decoder::GetDownmixInfo(DownmixInfo *pInfo) const
{
	if (pInfo == NULL)
		return false;

	pInfo->Front  = 1.0;
	pInfo->LFE    = 0.0;
	if (m_pA52State) {
		pInfo->Center = m_pA52State->clev;
		pInfo->Rear   = m_pA52State->slev;
	} else {
		pInfo->Center = LEVEL_3DB;
		pInfo->Rear   = LEVEL_3DB;
	}

	return true;
}


bool CAc3Decoder::OpenDecoder()
{
	CloseDecoder();

	m_pA52State = a52_init(0);
	if (!m_pA52State)
		return false;

	m_SyncWord = 0;
	m_FrameLength = 0;
	m_FramePos = 0;

	m_bDecodeError = false;

	return true;
}


void CAc3Decoder::CloseDecoder()
{
	if (m_pA52State) {
		a52_free(m_pA52State);
		m_pA52State = nullptr;
	}
}


bool CAc3Decoder::ResetDecoder()
{
	if (!m_pA52State)
		return false;

	return OpenDecoder();
}


bool CAc3Decoder::DecodeFrame(const BYTE *pData, DWORD *pDataSize, DecodeFrameInfo *pInfo)
{
	if (!m_pA52State) {
		return false;
	}

	pInfo->Samples = 0;

	const DWORD DataSize = *pDataSize;
	DWORD Pos = 0;

	if (m_FramePos == m_FrameLength)
		m_FramePos = 0;

	if (m_FramePos == 0) {
		// syncword検索
		WORD SyncWord = m_SyncWord;

		while (Pos < DataSize) {
			SyncWord = (SyncWord << 8) | pData[Pos++];
			if (SyncWord == 0x0B77) {
				m_FrameLength = 0;
				m_FramePos = 2;
				m_FrameBuffer[0] = 0x0B;
				m_FrameBuffer[1] = 0x77;
				break;
			}
		}

		if (Pos == DataSize) {
			m_SyncWord = SyncWord;
			return true;
		}
	}

	if (m_FramePos < 7) {
		// a52_syncinfo に7バイト必要
		int Remain = min((int)(DataSize - Pos), 7 - m_FramePos);
		::CopyMemory(&m_FrameBuffer[m_FramePos], &pData[Pos], Remain);
		m_FramePos += Remain;
		if (m_FramePos < 7)
			return true;
		Pos += Remain;

		m_FrameLength = a52_syncinfo(m_FrameBuffer, &m_A52Info.Flags, &m_A52Info.SampleRate, &m_A52Info.BitRate);

		if (m_FrameLength == 0) {
			TRACE(TEXT("a52_syncinfo() error\n"));

			m_FramePos = 0;

			// syncword再検索
			for (int i = 2; i < 7; i++) {
				if (m_FrameBuffer[i] == 0x0B) {
					if (i == 6) {
						m_SyncWord = 0x000B;
						break;
					}
					if (m_FrameBuffer[i + 1] == 0x77) {
						m_FramePos = 7 - i;
						::MoveMemory(&m_FrameBuffer[0], &m_FrameBuffer[i], m_FramePos);
						break;
					}
				}
			}

			*pDataSize = Pos;

			return true;
		}
	}

	int Remain = min((int)(DataSize - Pos), m_FrameLength - m_FramePos);
	::CopyMemory(&m_FrameBuffer[m_FramePos], &pData[Pos], Remain);
	m_FramePos += Remain;
	if (m_FramePos < m_FrameLength)
		return true;
	Pos += Remain;
	*pDataSize = Pos;

	const bool bLFE = (m_A52Info.Flags & A52_LFE) != 0;
	BYTE Channels, OutChannels;
	bool bDualMono = false;

	switch (m_A52Info.Flags & A52_CHANNEL_MASK) {
	case A52_CHANNEL:	// Dual mono
		Channels = 2;
		bDualMono = true;
		break;

	case A52_MONO:		// Mono
	case A52_CHANNEL1:	// 1st channel of Dual mono
	case A52_CHANNEL2:	// 2nd channel of Dual mono
		Channels = 1;
		break;

	case A52_STEREO:	// Stereo
	case A52_DOLBY:		// Dolby surround compatible stereo
		Channels = 2;
		break;

	case A52_3F:		// 3 front (L, C, R)
	case A52_2F1R:		// 2 front, 1 rear (L, R, S)
		Channels = 3;
		break;

	case A52_3F1R:		// 3 front, 1 rear (L, C, R, S)
	case A52_2F2R:		// 2 front, 2 rear (L, R, LS, RS)
		Channels = 4;
		break;

	case A52_3F2R:		// 3 front, 2 rear (L, C, R, LS, RS)
		Channels = 5;
		break;

	default:
		return false;
	}

	int FrameFlags;

	if (bLFE) {
		Channels++;
		OutChannels = 6;
		FrameFlags = A52_3F2R | A52_LFE;
	} else if (Channels <= 2) {
		OutChannels = Channels;
		FrameFlags = m_A52Info.Flags & A52_CHANNEL_MASK;
	} else {
		OutChannels = 6;
		FrameFlags = A52_3F2R;
	}

	sample_t Level = 1.0f;
	if (a52_frame(m_pA52State, m_FrameBuffer, &FrameFlags, &Level, 384.0f)) {
		TRACE(TEXT("a52_frame() error\n"));
		ResetDecoder();
		return false;
	}

	// Disable dynamic range compression
	a52_dynrng(m_pA52State, nullptr, nullptr);

	for (int i = 0; i < 6; i++) {
		if (a52_block(m_pA52State)) {
			TRACE(TEXT("a52_block() error\n"));
			ResetDecoder();
			return false;
		}

		const sample_t *pSamples = a52_samples(m_pA52State);
		const sample_t *p = pSamples;
		short *pOutBuffer = m_PcmBuffer + (i * (OutChannels * 256));
		int j = 0;

		if (Channels > 2 && !bLFE) {
			for (int k = 0; k < 256; k++) {
				pOutBuffer[k * OutChannels] = 0;
			}
			j = 1;
		}

		for (; j < OutChannels; j++) {
			for (int k = 0; k < 256; k++) {
				pOutBuffer[k * OutChannels + j] = SampleToInt16(*p++);
			}
		}
	}

	m_AudioInfo.Frequency = m_A52Info.SampleRate;
	m_AudioInfo.Channels = OutChannels;
	m_AudioInfo.OrigChannels = Channels;
	m_AudioInfo.bDualMono = bDualMono;

	pInfo->pData = (const BYTE*)m_PcmBuffer;
	pInfo->Samples = 256 * 6;
	pInfo->Info = m_AudioInfo;
	pInfo->bDiscontinuity = m_bDecodeError;

	return true;
}
