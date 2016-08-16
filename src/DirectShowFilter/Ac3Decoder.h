#pragma once


#include "AudioDecoder.h"
#include "TsMedia.h"

extern "C" {
#include "../Libs/a52dec/vc++/config.h"
#include "../Libs/a52dec/vc++/inttypes.h"
#include "../Libs/a52dec/include/a52.h"
}


class CAc3Decoder
	: public CAudioDecoder
{
public:
	CAc3Decoder();
	~CAc3Decoder();

// CAudioDecoder
	bool Open() override;
	void Close() override;
	bool IsOpened() const override;
	bool Reset() override;
	bool Decode(const BYTE *pData, DWORD *pDataSize, DecodeFrameInfo *pInfo) override;

	bool IsSpdifSupported() const override { return true; }
	bool GetSpdifFrameInfo(SpdifFrameInfo *pInfo) const override;
	int GetSpdifBurstPayload(BYTE *pBuffer, DWORD BufferSize) const override;

	bool GetChannelMap(int Channels, int *pMap) const override;
	bool GetDownmixInfo(DownmixInfo *pInfo) const override;

private:
	bool OpenDecoder();
	void CloseDecoder();
	bool ResetDecoder();
	bool DecodeFrame(const BYTE *pData, DWORD *pDataSize, DecodeFrameInfo *pInfo);

	struct A52Info {
		int Flags;
		int SampleRate;
		int BitRate;
	};

	static const size_t MAX_FRAME_SIZE = 3840;
	static const size_t PCM_BUFFER_LENGTH = 256 * 6 * 6;

	a52_state_t *m_pA52State;
	A52Info m_A52Info;
	bool m_bDecodeError;
	WORD m_SyncWord;
	int m_FrameLength;
	int m_FramePos;
	BYTE m_FrameBuffer[MAX_FRAME_SIZE];
	short m_PcmBuffer[PCM_BUFFER_LENGTH];
};
