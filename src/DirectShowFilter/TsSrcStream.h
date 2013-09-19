#ifndef TS_SRC_STREAM_H
#define TS_SRC_STREAM_H


#include <list>
#include "TsStream.h"
#include "TsUtilClass.h"


class CTsSrcStream
{
public:
	CTsSrcStream(DWORD BufferLength);
	~CTsSrcStream();
	bool InputMedia(const CMediaData *pMediaData);
	bool GetData(BYTE *pData,DWORD *pSize);
	bool IsDataAvailable();
	bool IsBufferFull();
	void Reset();
	bool EnableSync(bool bEnable);
	bool IsSyncEnabled() const { return m_bEnableSync; }
	void SetVideoPID(WORD PID);
	void SetAudioPID(WORD PID);

private:
	void ResetSync();

	CCriticalLock m_Lock;
	BYTE *m_pBuffer;
	DWORD m_BufferLength;
	DWORD m_BufferUsed;
	DWORD m_BufferPos;

	bool m_bEnableSync;
	LONGLONG m_VideoPTS;
	LONGLONG m_VideoPTSPrev;
	LONGLONG m_AudioPTS;
	LONGLONG m_AudioPTSPrev;
	WORD m_VideoPID;
	WORD m_AudioPID;
	struct PacketData {
		BYTE m_Data[188];
		LONGLONG m_PTS;
	};
	std::list<PacketData*> m_PoolPacketList;
	class CAllocator {
		size_t m_BlockSize;
		size_t m_BufferLength;
		BYTE *m_pBuffer;
		bool *m_pBlockUsed;
		size_t m_AllocCount;
		size_t m_AllocPos;
	public:
		CAllocator(size_t BlockSize,size_t BufferLength);
		~CAllocator();
		void *Allocate();
		void Free(void *pBlock);
		void FreeAll();
	};
	CAllocator m_Allocator;

	void AddData(const CMediaData *pMediaData);
	void AddPacket(const PacketData *pPacket);
	void AddPoolPackets();
};


#endif
