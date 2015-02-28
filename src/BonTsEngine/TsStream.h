// TsStream.h: TS�X�g���[�����b�p�[�N���X�̃C���^�[�t�F�C�X
//
//////////////////////////////////////////////////////////////////////

#pragma once


#include "Common.h"
#include "MediaData.h"


/////////////////////////////////////////////////////////////////////////////
// TS�p�P�b�g���ۉ��N���X
/////////////////////////////////////////////////////////////////////////////

#define TS_PACKETSIZE	(188U)	// TS�p�P�b�g�T�C�Y

class CTsPacket : public CMediaData
{
public:
	CTsPacket();
	CTsPacket(const CTsPacket &Operand);
	CTsPacket & operator = (const CTsPacket &Operand);
	~CTsPacket();

	enum	// ParsePacket() �G���[�R�[�h
	{
		EC_VALID		= 0x00000000UL,		// ����p�P�b�g
		EC_FORMAT		= 0x00000001UL,		// �t�H�[�}�b�g�G���[
		EC_TRANSPORT	= 0x00000002UL,		// �g�����X�|�[�g�G���[(�r�b�g�G���[)
		EC_CONTINUITY	= 0x00000003UL		// �A�����J�E���^�G���[(�h���b�v)
	};
	DWORD ParsePacket(BYTE *pContinuityCounter = NULL);

	BYTE * GetPayloadData(void);
	const BYTE * GetPayloadData(void) const;
	const BYTE GetPayloadSize(void) const;

	const WORD GetPID(void) const { return m_Header.wPID; }
	const bool HaveAdaptationField(void) const { return (m_Header.byAdaptationFieldCtrl & 0x02U) != 0; }
	const bool HavePayload(void) const { return (m_Header.byAdaptationFieldCtrl & 0x01U) != 0; }
	const bool IsScrambled(void) const { return (m_Header.byTransportScramblingCtrl & 0x02U) != 0; }

	struct TAG_TSPACKETHEADER {
		BYTE bySyncByte;					// Sync Byte
		bool bTransportErrorIndicator;		// Transport Error Indicator
		bool bPayloadUnitStartIndicator;	// Payload Unit Start Indicator
		bool TransportPriority;				// Transport Priority
		WORD wPID;							// PID
		BYTE byTransportScramblingCtrl;		// Transport Scrambling Control
		BYTE byAdaptationFieldCtrl;			// Adaptation Field Control
		BYTE byContinuityCounter;			// Continuity Counter
	} m_Header;

	struct TAG_ADAPTFIELDHEADER {
		BYTE byAdaptationFieldLength;		// Adaptation Field Length
		bool bDiscontinuityIndicator;		// Discontinuity Indicator
		bool bRamdomAccessIndicator;		// Random Access Indicator
		bool bEsPriorityIndicator;			// Elementary Stream Priority Indicator
		bool bPcrFlag;						// PCR Flag
		bool bOpcrFlag;						// OPCR Flag
		bool bSplicingPointFlag;			// Splicing Point Flag
		bool bTransportPrivateDataFlag;		// Transport Private Data Flag
		bool bAdaptationFieldExtFlag;		// Adaptation Field Extension Flag
		const BYTE *pOptionData;			// �I�v�V�����t�B�[���h�f�[�^
		BYTE byOptionSize;					// �I�v�V�����t�B�[���h��
	} m_AdaptationField;

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
};


/////////////////////////////////////////////////////////////////////////////
// PSI�Z�N�V�������ۉ��N���X
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
		BYTE byTableID;						// �e�[�u��ID
		bool bSectionSyntaxIndicator;		// �Z�N�V�����V���^�b�N�X�C���W�P�[�^
		bool bPrivateIndicator;				// �v���C�x�[�g�C���W�P�[�^
		WORD wSectionLength;				// �Z�N�V������

		WORD wTableIdExtension;				// �e�[�u��ID�g��
		BYTE byVersionNo;					// �o�[�W�����ԍ�
		bool bCurrentNextIndicator;			// �J�����g�l�N�X�g�C���W�P�[�^
		BYTE bySectionNumber;				// �Z�N�V�����ԍ�
		BYTE byLastSectionNumber;			// ���X�g�Z�N�V�����ԍ�
	} m_Header;
};


/////////////////////////////////////////////////////////////////////////////
// TS PID�}�b�v�ΏۃN���X
/////////////////////////////////////////////////////////////////////////////

class ABSTRACT_CLASS_DECL CTsPidMapTarget
{
public:
	virtual const bool StorePacket(const CTsPacket *pPacket) = 0;

	virtual void OnPidMapped(const WORD wPID, const PVOID pParam);
	virtual void OnPidUnmapped(const WORD wPID);
};


/////////////////////////////////////////////////////////////////////////////
// TS PID�}�b�v�Ǘ��N���X
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
	} m_PidMap[0x2000];
	
	WORD m_wMapCount;
};


/////////////////////////////////////////////////////////////////////////////
// PSI�Z�N�V�������o�N���X
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
// PCR���ۉ��N���X
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
