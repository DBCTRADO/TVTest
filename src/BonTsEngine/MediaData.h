// MediaData.h: CMediaData クラスのインターフェイス
//
//////////////////////////////////////////////////////////////////////

#pragma once


class CMediaData
{
public:
	CMediaData();
	CMediaData(const CMediaData &Operand);
	CMediaData(const DWORD dwBuffSize);
	CMediaData(const BYTE *pData, const DWORD dwDataSize);
	CMediaData(const BYTE byFiller, const DWORD dwDataSize);

	virtual ~CMediaData();

	CMediaData & operator = (const CMediaData &Operand);
	CMediaData & operator += (const CMediaData &Operand);

	BYTE *GetData();
	const BYTE *GetData() const;
	const DWORD GetSize() const { return m_dwDataSize; }
	const DWORD GetBufferSize() const { return m_dwBuffSize; }

	void SetAt(const DWORD dwPos, const BYTE byData);
	const BYTE GetAt(const DWORD dwPos) const;

	const DWORD SetData(const void *pData, const DWORD dwDataSize);
	const DWORD AddData(const void *pData, const DWORD dwDataSize);
	const DWORD AddData(const CMediaData *pData);
	const DWORD AddByte(const BYTE byData);
	const DWORD TrimHead(const DWORD dwTrimSize = 1UL);
	const DWORD TrimTail(const DWORD dwTrimSize = 1UL);

	const DWORD GetBuffer(const DWORD dwGetSize);

	const DWORD SetSize(const DWORD dwSetSize);
	const DWORD SetSize(const DWORD dwSetSize, const BYTE byFiller);

	void ClearSize(void);
	void ClearBuffer(void);

protected:
	virtual void *Allocate(size_t Size);
	virtual void Free(void *pBuffer);
	virtual void *ReAllocate(void *pBuffer, size_t Size);

	DWORD m_dwDataSize;
	DWORD m_dwBuffSize;
	BYTE *m_pData;
};
