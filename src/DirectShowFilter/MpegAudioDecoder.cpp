#include "stdafx.h"
#include "MpegAudioDecoder.h"
#include "../Common/DebugDef.h"


#pragma comment(lib, "libmad.lib")
#if _MSC_VER >= 1900
#pragma comment(lib, "legacy_stdio_definitions.lib")
#endif


static inline short FixedToInt16(mad_fixed_t Value)
{
	Value += 1L << (MAD_F_FRACBITS - 16);
	if (Value >= MAD_F_ONE)
		Value = MAD_F_ONE - 1;
	else if (Value < -MAD_F_ONE)
		Value = -MAD_F_ONE;
	return (short)(Value >> (MAD_F_FRACBITS + 1 - 16));
}




CMpegAudioDecoder::CMpegAudioDecoder()
	: m_bInitialized(false)
	, m_bDecodeError(false)
{
}


CMpegAudioDecoder::~CMpegAudioDecoder()
{
	Close();
}


bool CMpegAudioDecoder::Open()
{
	if (!OpenDecoder())
		return false;

	ClearAudioInfo();

	return true;
}


void CMpegAudioDecoder::Close()
{
	CloseDecoder();
}


bool CMpegAudioDecoder::IsOpened() const
{
	return m_bInitialized;
}


bool CMpegAudioDecoder::Reset()
{
	if (!ResetDecoder())
		return false;

	ClearAudioInfo();
	m_bDecodeError = false;

	return true;
}


bool CMpegAudioDecoder::Decode(const BYTE *pData, DWORD *pDataSize, DecodeFrameInfo *pInfo)
{
	if (!m_bInitialized)
		return false;

	if (!DecodeFrame(pData, pDataSize, pInfo)) {
		m_bDecodeError = true;
		return false;
	}

	m_bDecodeError = false;

	return true;
}


bool CMpegAudioDecoder::OpenDecoder()
{
	CloseDecoder();

	mad_stream_init(&m_MadStream);
	mad_frame_init(&m_MadFrame);
	mad_synth_init(&m_MadSynth);

	m_bInitialized = true;
	m_bDecodeError = false;

	return true;
}


void CMpegAudioDecoder::CloseDecoder()
{
	if (m_bInitialized) {
		mad_synth_finish(&m_MadSynth);
		mad_frame_finish(&m_MadFrame);
		mad_stream_finish(&m_MadStream);

		m_bInitialized = false;
	}
}


bool CMpegAudioDecoder::ResetDecoder()
{
	if (!m_bInitialized)
		return false;

	return OpenDecoder();
}


bool CMpegAudioDecoder::DecodeFrame(const BYTE *pData, DWORD *pDataSize, DecodeFrameInfo *pInfo)
{
	if (!m_bInitialized) {
		return false;
	}

	if (m_MadStream.buffer == nullptr || m_MadStream.error == MAD_ERROR_BUFLEN) {
		size_t ReadSize, Remain;

		if (m_MadStream.next_frame != nullptr) {
			Remain = m_MadStream.bufend - m_MadStream.next_frame;
			::MoveMemory(m_InputBuffer, m_MadStream.next_frame, Remain);
			ReadSize = INPUT_BUFFER_SIZE - Remain;
		} else {
			ReadSize = INPUT_BUFFER_SIZE;
			Remain = 0;
		}

		if (ReadSize > *pDataSize)
			ReadSize = *pDataSize;
		*pDataSize = (DWORD)ReadSize;

		::CopyMemory(&m_InputBuffer[Remain], pData, ReadSize);

		mad_stream_buffer(&m_MadStream, m_InputBuffer, (unsigned long)(ReadSize + Remain));
		m_MadStream.error = MAD_ERROR_NONE;
	} else {
		*pDataSize = 0;
	}

	if (mad_frame_decode(&m_MadFrame, &m_MadStream)) {
		pInfo->Samples = 0;

		if (m_MadStream.error == MAD_ERROR_BUFLEN)
			return true;
		if (MAD_RECOVERABLE(m_MadStream.error))
			return true;
#ifdef _DEBUG
		::OutputDebugStringA("libmad error : ");
		::OutputDebugStringA(mad_stream_errorstr(&m_MadStream));
		::OutputDebugStringA("\n");
#endif
		ResetDecoder();
		return false;
	}

	mad_synth_frame(&m_MadSynth, &m_MadFrame);

	const BYTE Channels = MAD_NCHANNELS(&m_MadFrame.header);
	short *p = m_PcmBuffer;

	if (Channels == 1) {
		for (int i = 0; i < m_MadSynth.pcm.length; i++) {
			*p++ = FixedToInt16(m_MadSynth.pcm.samples[0][i]);
		}
	} else {
		for (int i = 0; i < m_MadSynth.pcm.length; i++) {
			*p++ = FixedToInt16(m_MadSynth.pcm.samples[0][i]);
			*p++ = FixedToInt16(m_MadSynth.pcm.samples[1][i]);
		}
	}

	m_AudioInfo.Frequency = m_MadSynth.pcm.samplerate;
	m_AudioInfo.Channels = Channels;
	m_AudioInfo.OrigChannels = Channels;
	m_AudioInfo.bDualMono = false;

	pInfo->pData = (const BYTE*)m_PcmBuffer;
	pInfo->Samples = m_MadSynth.pcm.length;
	pInfo->Info = m_AudioInfo;
	pInfo->bDiscontinuity = m_bDecodeError;

	return true;
}
