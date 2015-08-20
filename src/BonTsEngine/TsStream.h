// TsStream.h: TSストリームラッパークラスのインターフェイス
//
//////////////////////////////////////////////////////////////////////

#pragma once


#include "Common.h"
#include "MediaData.h"


/////////////////////////////////////////////////////////////////////////////
// TSパケット抽象化クラス
/////////////////////////////////////////////////////////////////////////////

#define TS_PACKETSIZE	(188U)	// TSパケットサイズ

class CTsPacket : public CMediaData
{
	struct TAG_TSPACKETHEADER {
		BYTE bySyncByte;					// Sync Byte
		bool bTransportErrorIndicator;		// Transport Error Indicator
		bool bPayloadUnitStartIndicator;	// Payload Unit Start Indicator
		bool bTransportPriority;			// Transport Priority
		WORD wPID;							// PID
		BYTE byTransportScramblingCtrl;		// Transport Scrambling Control
		BYTE byAdaptationFieldCtrl;			// Adaptation Field Control
		BYTE byContinuityCounter;			// Continuity Counter
	};

	struct TAG_ADAPTFIELDHEADER {
		BYTE byAdaptationFieldLength;		// Adaptation Field Length
		BYTE Flags;							// フラグ
		bool bDiscontinuityIndicator;		// Discontinuity Indicator
		BYTE byOptionSize;					// オプションフィールド長
		const BYTE *pOptionData;			// オプションフィールドデータ
	};

	enum {
		ADAPTFIELD_DISCONTINUITY_INDICATOR		= 0x80U,	// Discontinuity Indicator
		ADAPTFIELD_RANDOM_ACCESS_INDICATOR		= 0x40U,	// Random Access Indicator
		ADAPTFIELD_ES_PRIORITY_INDICATOR		= 0x20U,	// Elementary Stream Priority Indicator
		ADAPTFIELD_PCR_FLAG						= 0x10U,	// PCR Flag
		ADAPTFIELD_OPCR_FLAG					= 0x08U,	// OPCR Flag
		ADAPTFIELD_SPLICING_POINT_FLAG			= 0x04U,	// Splicing Point Flag
		ADAPTFIELD_TRANSPORT_PRIVATE_DATA_FLAG	= 0x02U,	// Transport Private Data Flag
		ADAPTFIELD_ADAPTATION_FIELD_EXT_FLAG	= 0x01U		// Adaptation Field Extension Flag
	};

public:
	CTsPacket();
	CTsPacket(const CTsPacket &Operand);
	CTsPacket & operator = (const CTsPacket &Operand);
	~CTsPacket();

	enum	// ParsePacket() エラーコード
	{
		EC_VALID		= 0x00000000UL,		// 正常パケット
		EC_FORMAT		= 0x00000001UL,		// フォーマットエラー
		EC_TRANSPORT	= 0x00000002UL,		// トランスポートエラー(ビットエラー)
		EC_CONTINUITY	= 0x00000003UL		// 連続性カウンタエラー(ドロップ)
	};
	DWORD ParsePacket(BYTE *pContinuityCounter = NULL);

	BYTE * GetPayloadData(void);
	const BYTE * GetPayloadData(void) const;
	const BYTE GetPayloadSize(void) const;

	const WORD GetPID(void) const { return m_Header.wPID; }
	void SetPID(WORD PID);
	const bool HaveAdaptationField(void) const { return (m_Header.byAdaptationFieldCtrl & 0x02U) != 0; }
	const bool HavePayload(void) const { return (m_Header.byAdaptationFieldCtrl & 0x01U) != 0; }
	const bool IsScrambled(void) const { return (m_Header.byTransportScramblingCtrl & 0x02U) != 0; }

	bool GetTransportErrorIndicator() const { return m_Header.bTransportErrorIndicator; }
	bool GetPayloadUnitStartIndicator() const { return m_Header.bPayloadUnitStartIndicator; }
	bool GetTransportPriority() const { return m_Header.bTransportPriority; }
	BYTE GetTransportScramblingCtrl() const { return m_Header.byTransportScramblingCtrl; }
	bool GetDiscontinuityIndicator() const { return m_AdaptationField.bDiscontinuityIndicator; }
	bool GetRandomAccessIndicator() const { return (m_AdaptationField.Flags & ADAPTFIELD_RANDOM_ACCESS_INDICATOR) != 0; }
	bool GetEsPriorityIndicator() const { return (m_AdaptationField.Flags & ADAPTFIELD_ES_PRIORITY_INDICATOR) != 0; }
	bool GetPcrFlag() const { return (m_AdaptationField.Flags & ADAPTFIELD_PCR_FLAG) != 0; }
	bool GetOpcrFlag() const { return (m_AdaptationField.Flags & ADAPTFIELD_OPCR_FLAG) != 0; }
	bool GetSplicingPointFlag() const { return (m_AdaptationField.Flags & ADAPTFIELD_SPLICING_POINT_FLAG) != 0; }
	bool GetTransportPrivateDataFlag() const { return (m_AdaptationField.Flags & ADAPTFIELD_TRANSPORT_PRIVATE_DATA_FLAG) != 0; }
	bool GetAdaptationFieldExtFlag() const { return (m_AdaptationField.Flags & ADAPTFIELD_ADAPTATION_FIELD_EXT_FLAG) != 0; }
	const BYTE *GetOptionData() const { return m_AdaptationField.pOptionData; }
	const BYTE GetOptionSize() const { return m_AdaptationField.byOptionSize; }

	enum { BUFFER_SIZE=TS_PACKETSIZE+sizeof(TAG_TSPACKETHEADER)+sizeof(TAG_ADAPTFIELDHEADER) };
	void StoreToBuffer(void *pBuffer);
	void RestoreFromBuffer(const void *pBuffer);

private:
#ifdef TSPACKET_NEED_ALIGNED_PAYLOAD
// CMediaData
	void *Allocate(size_t Size) override;
	void Free(void *pBuffer) override;
	void *ReAllocate(void *pBuffer, size_t Size) override;
#endif

	TAG_TSPACKETHEADER m_Header;
	TAG_ADAPTFIELDHEADER m_AdaptationField;
};


/////////////////////////////////////////////////////////////////////////////
// PSIセクション抽象化クラス
/////////////////////////////////////////////////////////////////////////////

class CPsiSection : public CMediaData
{
public:
	CPsiSection();
	CPsiSection(const DWORD dwBuffSize);
	CPsiSection(const CPsiSection &Operand);

	CPsiSection & operator = (const CPsiSection &Operand);
	const bool operator == (const CPsiSection &Operand) const;
	const bool operator != (const CPsiSection &Operand) const { return !(*this == Operand); }

	const bool ParseHeader(const bool bIsExtended = true, const bool bIgnoreSectionNumber = false);
	void Reset(void);

	BYTE * GetPayloadData(void) const;
	const WORD GetPayloadSize(void) const;

	const BYTE GetTableID(void) const;
	const bool IsExtendedSection(void) const;
	const bool IsPrivate(void) const;
	const WORD GetSectionLength(void) const;
	const WORD GetTableIdExtension(void) const;
	const BYTE GetVersionNo(void) const;
	const bool IsCurrentNext(void) const;
	const BYTE GetSectionNumber(void) const;
	const BYTE GetLastSectionNumber(void) const;

protected:
	struct TAG_PSIHEADER {
		BYTE byTableID;						// テーブルID
		bool bSectionSyntaxIndicator;		// セクションシンタックスインジケータ
		bool bPrivateIndicator;				// プライベートインジケータ
		WORD wSectionLength;				// セクション長

		WORD wTableIdExtension;				// テーブルID拡張
		BYTE byVersionNo;					// バージョン番号
		bool bCurrentNextIndicator;			// カレントネクストインジケータ
		BYTE bySectionNumber;				// セクション番号
		BYTE byLastSectionNumber;			// ラストセクション番号
	} m_Header;
};


/////////////////////////////////////////////////////////////////////////////
// TS PIDマップ対象クラス
/////////////////////////////////////////////////////////////////////////////

class ABSTRACT_CLASS_DECL CTsPidMapTarget
{
public:
	virtual const bool StorePacket(const CTsPacket *pPacket) = 0;

	virtual void OnPidMapped(const WORD wPID, const PVOID pParam);
	virtual void OnPidUnmapped(const WORD wPID);
};


/////////////////////////////////////////////////////////////////////////////
// TS PIDマップ管理クラス
/////////////////////////////////////////////////////////////////////////////

class CTsPidMapManager
{
public:
	typedef void (CALLBACK * PIDMAPHANDLERFUNC)(const WORD wPID, CTsPidMapTarget *pMapTarget, CTsPidMapManager *pMapManager, const PVOID pParam);

	CTsPidMapManager();
	virtual ~CTsPidMapManager();

	const bool StorePacket(const CTsPacket *pPacket);

	const bool MapTarget(const WORD wPID, CTsPidMapTarget *pMapTarget, const PIDMAPHANDLERFUNC pMapCallback = NULL, const PVOID pMapParam = NULL);
	const bool UnmapTarget(const WORD wPID);
	void UnmapAllTarget(void);

	CTsPidMapTarget * GetMapTarget(const WORD wPID) const;

	const WORD GetMapCount(void) const;

protected:
	struct TAG_MAPTARGETITEM
	{
		CTsPidMapTarget *pMapTarget;
		PIDMAPHANDLERFUNC pMapCallback;
		PVOID pMapParam;
	};

	TAG_MAPTARGETITEM *m_PidMap[0x2000];
	WORD m_wMapCount;
};


/////////////////////////////////////////////////////////////////////////////
// PSIセクション抽出クラス
/////////////////////////////////////////////////////////////////////////////

class CPsiSectionParser
{
public:
	class ABSTRACT_CLASS_DECL IPsiSectionHandler
	{
	public:
		virtual void OnPsiSection(const CPsiSectionParser *pPsiSectionParser, const CPsiSection *pSection) = 0;
	};

	CPsiSectionParser(IPsiSectionHandler *pSectionHandler, const bool bTargetExt = true, const bool bIgnoreSectionNumber = false);
	CPsiSectionParser(const CPsiSectionParser &Operand);
	CPsiSectionParser & operator = (const CPsiSectionParser &Operand);

	void StorePacket(const CTsPacket *pPacket);
	void Reset(void);

	const DWORD GetCrcErrorCount(void) const;

private:
	const bool StoreHeader(const BYTE *pPayload, BYTE *pbyRemain);
	const bool StorePayload(const BYTE *pPayload, BYTE *pbyRemain);

	IPsiSectionHandler *m_pPsiSectionHandler;
	CPsiSection m_PsiSection;
	bool m_bTargetExt;
	bool m_bIgnoreSectionNumber;

	bool m_bIsStoring;
	DWORD m_dwStoreCrc;
	WORD m_wStoreSize;
	DWORD m_dwCrcErrorCount;
};


/////////////////////////////////////////////////////////////////////////////
// PCR抽象化クラス
/////////////////////////////////////////////////////////////////////////////

class CTsClockRef
{
public:
	CTsClockRef();
	CTsClockRef(const CTsClockRef &Operand);

	CTsClockRef & operator = (const CTsClockRef &Operand);

	const bool StorePacket(const CTsPacket *pPacket, const WORD wPcrPID);
	void Reset(void);

	const LONGLONG GetGlobalPcr(void) const;
	const LONGLONG GetCurrentPcr(void) const;
	const LONGLONG PtsToGlobalPcr(const LONGLONG llPts) const;

protected:
	void InitPcrPll(const LONGLONG llCurPcr);
	void ProcPcrPll(const LONGLONG llCurPcr);
	void SyncPcrPll(const LONGLONG llCurPcr);

	static inline const LONGLONG GetPcrFromHex(const BYTE *pPcrData);

	LONGLONG m_llHrcUnitFreq;
	LONGLONG m_llHrcLastTime;
	
	LONGLONG m_llCurPcrCount;
	double m_lfPllFeedBack;

	LONGLONG m_llGlobalPcrCount;
	LONGLONG m_llBasePcrCount;
};
