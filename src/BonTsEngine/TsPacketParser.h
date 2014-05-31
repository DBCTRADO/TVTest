// TsPacketParser.h: CTsPacketParser クラスのインターフェイス
//
//////////////////////////////////////////////////////////////////////

#pragma once


#include "MediaDecoder.h"
#include "TsStream.h"
#include "PATGenerator.h"


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
	void Reset() override;
	const bool InputMedia(CMediaData *pMediaData, const DWORD dwInputIndex = 0UL) override;

// CTsPacketParser
	void SetOutputNullPacket(const bool bEnable = true);
	ULONGLONG GetInputPacketCount() const;
	ULONGLONG GetOutputPacketCount() const;
	ULONGLONG GetErrorPacketCount() const;
	ULONGLONG GetContinuityErrorPacketCount() const;
	void ResetErrorPacketCount();

	bool EnablePATGeneration(bool bEnable);
	bool IsPATGenerationEnabled() const { return m_bGeneratePAT; }
	bool SetTransportStreamID(WORD TransportStreamID);

private:
	void inline SyncPacket(const BYTE *pData, const DWORD dwSize);
	bool inline ParsePacket();

	CTsPacket m_TsPacket;

	bool m_bOutputNullPacket;

	ULONGLONG m_InputPacketCount;
	ULONGLONG m_OutputPacketCount;
	ULONGLONG m_ErrorPacketCount;
	ULONGLONG m_ContinuityErrorPacketCount;
	BYTE m_abyContCounter[0x1FFF];

	CPATGenerator m_PATGenerator;
	bool m_bGeneratePAT;
	CTsPacket m_PATPacket;
};
