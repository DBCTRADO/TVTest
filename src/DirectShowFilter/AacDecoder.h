// AacDecoder.h: CAacDecoder クラスのインターフェイス
//
//////////////////////////////////////////////////////////////////////

#pragma once


#include "neaacdec.h"
#include "AudioDecoder.h"
#include "TsMedia.h"


// FAAD2 AACデコーダラッパークラス 
//
// "Code from FAAD2 is copyright (c) Nero AG, www.nero.com"
//


class CAacDecoder
	: public CAudioDecoder
{
public:
	CAacDecoder();
	~CAacDecoder();

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
	bool DecodeFrame(const CAdtsFrame *pFrame, DecodeFrameInfo *pInfo);

	CAdtsParser m_AdtsParser;
	NeAACDecHandle m_hDecoder;
	bool m_bInitRequest;
	BYTE m_LastChannelConfig;
	const CAdtsFrame *m_pAdtsFrame;
	bool m_bDecodeError;
};
