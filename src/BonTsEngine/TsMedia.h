// TsMedia.h: TSメディアラッパークラスのインターフェイス
//
//////////////////////////////////////////////////////////////////////

#pragma once


#include "MediaData.h"
#include "TsStream.h"


/////////////////////////////////////////////////////////////////////////////
// ISO/IEC 13818-1 PESパケット抽象化クラス
/////////////////////////////////////////////////////////////////////////////

class CPesPacket : public CMediaData
{
public:
	CPesPacket();
	CPesPacket(const DWORD dwBuffSize);
	CPesPacket(const CPesPacket &Operand);

	CPesPacket & operator = (const CPesPacket &Operand);

	const bool ParseHeader(void);
	void Reset(void);

	const BYTE GetStreamID(void) const;
	const WORD GetPacketLength(void) const;
	const BYTE GetScramblingCtrl(void) const;
	const bool IsPriority(void) const;
	const bool IsDataAlignmentIndicator(void) const;
	const bool IsCopyright(void) const;
	const bool IsOriginalOrCopy(void) const;
	const BYTE GetPtsDtsFlags(void) const;
	const bool IsEscrFlag(void) const;
	const bool IsEsRateFlag(void) const;
	const bool IsDsmTrickModeFlag(void) const;
	const bool IsAdditionalCopyInfoFlag(void) const;
	const bool IsCrcFlag(void) const;
	const bool IsExtensionFlag(void) const;
	const BYTE GetHeaderDataLength(void) const;
	
	const LONGLONG GetPtsCount(void)const;

	const WORD GetPacketCrc(void) const;
	
	BYTE * GetPayloadData(void) const;
	const DWORD GetPayloadSize(void) const;

protected:
	static inline const LONGLONG HexToTimeStamp(const BYTE *pHexData);

	struct TAG_PESHEADER{
		// PES_packet()　※現状、パケット抽出に必要な要素+αのみ
		BYTE byStreamID;				// Stream ID
		WORD wPacketLength;				// PES Packet Length
		BYTE byScramblingCtrl;			// PES Scrambling Control
		bool bPriority;					// PES Priority
		bool bDataAlignmentIndicator;	// Data Alignment Indicator
		bool bCopyright;				// Copyright
		bool bOriginalOrCopy;			// Original or Copy
		BYTE byPtsDtsFlags;				// PTS DTS Flags
		bool bEscrFlag;					// ESCR Flag
		bool bEsRateFlag;				// ES Rate Flag
		bool bDsmTrickModeFlag;			// DSM Trick Mode Flag
		bool bAdditionalCopyInfoFlag;	// Additional Copy Info Flag
		bool bCrcFlag;					// PES CRC Flag
		bool bExtensionFlag;			// PES Extension Flag
		BYTE byHeaderDataLength;		// PES Header Data Length
	} m_Header;
};


/////////////////////////////////////////////////////////////////////////////
// ISO/IEC 13818-1 PESパケット抽出クラス
/////////////////////////////////////////////////////////////////////////////

class CPesParser
{
public:
	class __declspec(novtable) IPacketHandler
	{
	public:
		virtual void OnPesPacket(const CPesParser *pPesParser, const CPesPacket *pPacket) = 0;
	};

	CPesParser(IPacketHandler *pPacketHandler);
	CPesParser(const CPesParser &Operand);
	CPesParser & operator = (const CPesParser &Operand);

	const bool StorePacket(const CTsPacket *pPacket);
	void Reset(void);

protected:
	virtual void OnPesPacket(const CPesPacket *pPacket) const;

	IPacketHandler *m_pPacketHandler;
	CPesPacket m_PesPacket;

private:
	const BYTE StoreHeader(const BYTE *pPayload, const BYTE byRemain);
	const BYTE StorePayload(const BYTE *pPayload, const BYTE byRemain);

	bool m_bIsStoring;
	WORD m_wStoreCrc;
	DWORD m_dwStoreSize;
};


/////////////////////////////////////////////////////////////////////////////
// ISO/IEC 13818-7 ADTSフレーム抽象化クラス
/////////////////////////////////////////////////////////////////////////////

class CAdtsFrame : public CMediaData
{
public:
	CAdtsFrame();
	CAdtsFrame(const CAdtsFrame &Operand);

	CAdtsFrame & operator = (const CAdtsFrame &Operand);

	const bool ParseHeader(void);
	void Reset(void);

	const BYTE GetProfile(void) const;
	const BYTE GetSamplingFreqIndex(void) const;
	const DWORD GetSamplingFreq(void) const;
	const bool IsPrivateBit(void) const;
	const BYTE GetChannelConfig(void) const;
	const bool IsOriginalCopy(void) const;
	const bool IsHome(void) const;
	const bool IsCopyrightIdBit(void) const;
	const bool IsCopyrightIdStart(void) const;
	const WORD GetFrameLength(void) const;
	const WORD GetBufferFullness(void) const;
	const BYTE GetRawDataBlockNum(void) const;

protected:
	struct TAG_ADTSHEADER{
		// adts_fixed_header()
		BYTE byProfile;					// Profile
		BYTE bySamplingFreqIndex;		// Sampling Frequency Index
		bool bPrivateBit;				// Private Bit
		BYTE byChannelConfig;			// Channel Configuration
		bool bOriginalCopy;				// Original/Copy
		bool bHome;						// Home

		// adts_variable_header()
		bool bCopyrightIdBit;			// Copyright Identification Bit
		bool bCopyrightIdStart;			// Copyright Identification Start
		WORD wFrameLength;				// Frame Length
		WORD wBufferFullness;			// ADTS Buffer Fullness
		BYTE byRawDataBlockNum;			// Number of Raw Data Blocks in Frame
	} m_Header;
};


/////////////////////////////////////////////////////////////////////////////
// ISO/IEC 13818-7 ADTSフレーム抽出クラス
/////////////////////////////////////////////////////////////////////////////

class CAdtsParser : public CPesParser::IPacketHandler
{
public:
	class __declspec(novtable) IFrameHandler
	{
	public:
		virtual void OnAdtsFrame(const CAdtsParser *pAdtsParser, const CAdtsFrame *pFrame) = 0;
	};

	CAdtsParser(IFrameHandler *pFrameHandler);
	CAdtsParser(const CAdtsParser &Operand);
	CAdtsParser & operator = (const CAdtsParser &Operand);

	const bool StorePacket(const CPesPacket *pPacket);
	const bool StoreEs(const BYTE *pData, const DWORD dwSize);
	bool StoreEs(const BYTE *pData, DWORD *pSize, CAdtsFrame **ppFrame);
	void Reset(void);

protected:
	virtual void OnPesPacket(const CPesParser *pPesParser, const CPesPacket *pPacket);
	virtual void OnAdtsFrame(const CAdtsFrame *pFrame) const;

	IFrameHandler *m_pFrameHandler;
	CAdtsFrame m_AdtsFrame;

private:
	inline const bool SyncFrame(const BYTE byData);

	bool m_bIsStoring;
	WORD m_wStoreCrc;
};


/////////////////////////////////////////////////////////////////////////////
// ISO/IEC 13818-2 H.262(MPEG2-ES)シーケンス抽象化クラス
/////////////////////////////////////////////////////////////////////////////

class CMpeg2Sequence : public CMediaData
{
public:
	CMpeg2Sequence();
	CMpeg2Sequence(const CMpeg2Sequence &Operand);

	CMpeg2Sequence & operator = (const CMpeg2Sequence &Operand);

	const bool ParseHeader(void);
	void Reset(void);

	const WORD GetHorizontalSize(void) const;
	const WORD GetVerticalSize(void) const;
	const BYTE GetAspectRatioInfo(void) const;
	const bool GetAspectRatio(BYTE *pAspectX, BYTE *pAspectY) const;
	const BYTE GetFrameRateCode(void) const;
	const bool GetFrameRate(DWORD *pNum, DWORD *pDenom) const;
	const DWORD GetBitRate(void) const;
	const bool IsMarkerBit(void) const;
	const DWORD GetVbvBufferSize(void) const;
	const bool IsConstrainedParamFlag(void) const;
	const bool IsLoadIntraQuantiserMatrix(void) const;

	const bool GetExtendDisplayInfo() const;
	const WORD GetExtendDisplayHorizontalSize(void) const;
	const WORD GetExtendDisplayVerticalSize(void) const;

	void SetFixSquareDisplay(bool bFix);

protected:
	struct TAG_MPEG2SEQHEADER {
		// sequence_header()
		WORD wHorizontalSize;				// Horizontal Size Value
		WORD wVerticalSize;					// Vertical Size Value
		BYTE byAspectRatioInfo;				// Aspect Ratio Information
		BYTE byFrameRateCode;				// Frame Rate Code
		DWORD dwBitRate;					// Bit Rate Value
		bool bMarkerBit;					// Marker Bit
		DWORD dwVbvBufferSize;				// VBV Buffer Size Value
		bool bConstrainedParamFlag;			// Constrained Parameters Flag
		bool bLoadIntraQuantiserMatrix;		// Load Intra Quantiser Matrix
		bool bLoadNonIntraQuantiserMatrix;	// Load NonIntra Quantiser Matrix
		// Sequence Extention
		struct {
			struct {
				bool bHave;
				BYTE byProfileAndLevel;
				bool bProgressive;
				BYTE byChromaFormat;
				bool bLowDelay;
				BYTE byFrameRateExtN;
				BYTE byFrameRateExtD;
			} Sequence;
			struct {
				bool bHave;
				BYTE byVideoFormat;
				bool bColorDescrption;
				struct {
					BYTE byColorPrimaries;
					BYTE byTransferCharacteristics;
					BYTE byMatrixCoefficients;
				} Color;
				WORD wDisplayHorizontalSize;
				WORD wDisplayVerticalSize;
			} Display;
		} Extention;
	} m_Header;
	bool m_bFixSquareDisplay;
};


/////////////////////////////////////////////////////////////////////////////
// ISO/IEC 13818-2 H.262フレーム抽出クラス
/////////////////////////////////////////////////////////////////////////////

class CMpeg2Parser : public CPesParser::IPacketHandler
{
public:
	class __declspec(novtable) ISequenceHandler
	{
	public:
		virtual void OnMpeg2Sequence(const CMpeg2Parser *pMpeg2Parser, const CMpeg2Sequence *pSequence) = 0;
	};

	CMpeg2Parser(ISequenceHandler *pSequenceHandler);
	CMpeg2Parser(const CMpeg2Parser &Operand);
	CMpeg2Parser & operator = (const CMpeg2Parser &Operand);

	const bool StorePacket(const CPesPacket *pPacket);
	const bool StoreEs(const BYTE *pData, const DWORD dwSize);
	void Reset(void);
	void SetFixSquareDisplay(bool bFix);

protected:
	virtual void OnPesPacket(const CPesParser *pPesParser, const CPesPacket *pPacket);
	virtual void OnMpeg2Sequence(const CMpeg2Sequence *pSequence) const;

	ISequenceHandler *m_pSequenceHandler;
	CMpeg2Sequence m_Mpeg2Sequence;

private:
	//inline const DWORD FindStartCode(const BYTE *pData, const DWORD dwDataSize);

	//bool m_bIsStoring;
	DWORD m_dwSyncState;
};


/////////////////////////////////////////////////////////////////////////////
// ISO/IEC 14496-10Video H.264 アクセスユニット抽象化クラス
/////////////////////////////////////////////////////////////////////////////

class CH264AccessUnit : public CMediaData
{
public:
	CH264AccessUnit();
	CH264AccessUnit(const CH264AccessUnit &Operand);

	CH264AccessUnit & operator = (const CH264AccessUnit &Operand);

	const bool ParseHeader(void);
	void Reset(void);

	const WORD GetHorizontalSize() const;
	const WORD GetVerticalSize() const;
	const bool GetSAR(WORD *pHorz, WORD *pVert) const;
	struct TimingInfo {
		DWORD NumUnitsInTick;
		DWORD TimeScale;
		bool bFixedFrameRateFlag;
	};
	const bool GetTimingInfo(TimingInfo *pInfo) const;

protected:
	bool m_bFoundSPS;
	struct TAG_H264ACCESSUNIT {
		// AUD (Access Unit Delimiter)
		struct {
			BYTE PrimaryPicType;
		} AUD;
		// SPS (Sequence Parameter Set)
		struct {
			BYTE ProfileIdc;
			bool bConstraintSet0Flag;
			bool bConstraintSet1Flag;
			bool bConstraintSet2Flag;
			bool bConstraintSet3Flag;
			BYTE LevelIdc;
			BYTE SeqParameterSetId;
			BYTE ChromaFormatIdc;
			bool bSeparateColourPlaneFlag;
			BYTE BitDepthLumaMinus8;
			BYTE BitDepthChromaMinus8;
			bool bQpprimeYZeroTransformBypassFlag;
			bool bSeqScalingMatrixPresentFlag;
			BYTE Log2MaxFrameNumMinus4;
			BYTE PicOrderCntType;
			BYTE Log2MaxPicOrderCntLsbMinus4;
			bool bDeltaPicOrderAlwaysZeroFlag;
			BYTE OffsetForNonRefPic;
			BYTE OffsetForTopToBottomField;
			BYTE NumRefFramesInPicOrderCntCycle;
			BYTE NumRefFrames;
			bool bGapsInFrameNumValueAllowedFlag;
			BYTE PicWidthInMbsMinus1;
			BYTE PicHeightInMapUnitsMinus1;
			bool bFrameMbsOnlyFlag;
			bool bMbAdaptiveFrameFieldFlag;
			bool bDirect8x8InferenceFlag;
			bool bFrameCroppingFlag;
			WORD FrameCropLeftOffset;
			WORD FrameCropRightOffset;
			WORD FrameCropTopOffset;
			WORD FrameCropBottomOffset;
			bool bVuiParametersPresentFlag;
			// VUI (Video Usability Information)
			struct {
				bool bAspectRatioInfoPresentFlag;
				BYTE AspectRatioIdc;
				WORD SarWidth;
				WORD SarHeight;
				bool bOverscanInfoPresentFlag;
				bool bOverscanAppropriateFlag;
				bool bVideoSignalTypePresentFlag;
				BYTE VideoFormat;
				bool bVideoFullRangeFlag;
				bool bColourDescriptionPresentFlag;
				BYTE ColourPrimaries;
				BYTE TransferCharacteristics;
				BYTE MatrixCoefficients;
				bool bChromaLocInfoPresentFlag;
				WORD ChromaSampleLocTypeTopField;
				WORD ChromaSampleLocTypeBottomField;
				bool bTimingInfoPresentFlag;
				DWORD NumUnitsInTick;
				DWORD TimeScale;
				bool bFixedFrameRateFlag;
				// ...
			} VUI;
			BYTE ChromaArrayType;
		} SPS;
	} m_Header;
};


/////////////////////////////////////////////////////////////////////////////
// ISO/IEC 14496-10Video H.264 アクセスユニット抽出クラス
/////////////////////////////////////////////////////////////////////////////

class CH264Parser : public CPesParser::IPacketHandler
{
public:
	class __declspec(novtable) IAccessUnitHandler
	{
	public:
		virtual void OnAccessUnit(const CH264Parser *pParser, const CH264AccessUnit *pAccessUnit) = 0;
	};

	CH264Parser(IAccessUnitHandler *pAccessUnitHandler);
	CH264Parser(const CH264Parser &Operand);
	CH264Parser & operator = (const CH264Parser &Operand);

	const bool StorePacket(const CPesPacket *pPacket);
	const bool StoreEs(const BYTE *pData, const DWORD dwSize);
	void Reset(void);

protected:
	virtual void OnPesPacket(const CPesParser *pPesParser, const CPesPacket *pPacket);
	virtual void OnAccessUnit(const CH264AccessUnit *pAccessUnit) const;

	IAccessUnitHandler *m_pAccessUnitHandler;
	CH264AccessUnit m_AccessUnit;

private:
	DWORD m_dwSyncState;
};
