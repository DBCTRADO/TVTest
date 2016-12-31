#pragma once


#include "AudioDecoder.h"
#include "TsMedia.h"

#if defined(_WIN64)
#define FPM_64BIT
#elif defined(_M_IX86)
#define PFM_INTEL
#endif
#include "../Libs/libmad/mad.h"


class CMpegAudioDecoder
	: public CAudioDecoder
{
public:
	CMpegAudioDecoder();
	~CMpegAudioDecoder();

// CAudioDecoder
	bool Open() override;
	void Close() override;
	bool IsOpened() const override;
	bool Reset() override;
	bool Decode(const BYTE *pData, DWORD *pDataSize, DecodeFrameInfo *pInfo) override;

	bool IsSpdifSupported() const override { return false; }

private:
	bool OpenDecoder();
	void CloseDecoder();
	bool ResetDecoder();
	bool DecodeFrame(const BYTE *pData, DWORD *pDataSize, DecodeFrameInfo *pInfo);

	static const size_t INPUT_BUFFER_SIZE = 4096;
	static const size_t PCM_BUFFER_LENGTH = 1152 * 2;

	struct mad_stream m_MadStream;
	struct mad_frame m_MadFrame;
	struct mad_synth m_MadSynth;
	bool m_bInitialized;
	bool m_bDecodeError;
	BYTE m_InputBuffer[INPUT_BUFFER_SIZE];
	short m_PcmBuffer[PCM_BUFFER_LENGTH];
};
