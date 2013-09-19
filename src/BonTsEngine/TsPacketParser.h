// TsPacketParser.h: CTsPacketParser クラスのインターフェイス
//
//////////////////////////////////////////////////////////////////////

#pragma once


#include "MediaDecoder.h"
#include "TsStream.h"
#ifdef BONTSENGINE_1SEG_SUPPORT
#include "PATGenerator.h"
#endif


/////////////////////////////////////////////////////////////////////////////
// TSパケット抽出デコーダ(バイナリデータからTSパケットを抽出する)
/////////////////////////////////////////////////////////////////////////////
// Input	#0	: CMediaData	TSパケットを含むバイナリデータ
// Output	#0	: CTsPacket		TSパケット
/////////////////////////////////////////////////////////////////////////////

class CTsPacketParser : public CMediaDecoder
{
public:
	CTsPacketParser(IEventHandler *pEventHandler = NULL);
	virtual ~CTsPacketParser();

// IMediaDecoder
	virtual void Reset(void);
	virtual const bool InputMedia(CMediaData *pMediaData, const DWORD dwInputIndex = 0UL);

// CTsPacketParser
	void SetOutputNullPacket(const bool bEnable = true);
	const DWORD GetInputPacketCount(void) const;
	const DWORD GetOutputPacketCount(void) const;
	const DWORD GetErrorPacketCount(void) const;
	const DWORD GetContinuityErrorPacketCount(void) const;
	void ResetErrorPacketCount(void);

	// Append by HDUSTestの中の人
#ifdef BONTSENGINE_1SEG_SUPPORT
	bool EnablePATGeneration(bool bEnable);
	bool IsPATGenerationEnabled() const { return m_bGeneratePAT; }
	bool SetTransportStreamID(WORD TransportStreamID);
#endif

private:
	void inline SyncPacket(const BYTE *pData, const DWORD dwSize);
	bool inline ParsePacket(void);

	CTsPacket m_TsPacket;

	bool m_bOutputNullPacket;

	ULONGLONG m_InputPacketCount;
	ULONGLONG m_OutputPacketCount;
	ULONGLONG m_ErrorPacketCount;
	ULONGLONG m_ContinuityErrorPacketCount;
	BYTE m_abyContCounter[0x1FFF];

#ifdef BONTSENGINE_1SEG_SUPPORT
	CPATGenerator m_PATGenerator;
	bool m_bGeneratePAT;
	CTsPacket m_PATPacket;
#endif
};
