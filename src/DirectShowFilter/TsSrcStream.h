#ifndef TS_SRC_STREAM_H
#define TS_SRC_STREAM_H


#include "TsStream.h"
#include "TsUtilClass.h"
#include "RingBuffer.h"


class CTsSrcStream
{
public:
	static const size_t DEFAULT_QUEUE_SIZE = 0x1000;
	static const size_t DEFAULT_POOL_SIZE  = 0x0800;

	CTsSrcStream();
	~CTsSrcStream();
	bool Initialize();
	bool InputMedia(const CMediaData *pMediaData);
	size_t GetData(BYTE *pData, size_t Size);
	void Reset();
	bool IsDataAvailable();
	bool IsBufferFull();
	int GetFillPercentage();
	bool SetQueueSize(size_t Size);
	size_t GetQueueSize() const { return m_QueueSize; }
	bool SetPoolSize(size_t Size);
	size_t GetPoolSize() const { return m_PoolSize; }
	bool EnableSync(bool bEnable, bool b1Seg = false);
	bool IsSyncEnabled() const { return m_bEnableSync; }
	bool IsSyncFor1Seg() const { return m_bSyncFor1Seg; }
	void SetVideoPID(WORD PID);
	void SetAudioPID(WORD PID);
	LONGLONG GetPTSDuration() const { return m_PTSDuration; }

private:
	enum { PACKET_SIZE = 188 };

	struct PacketPtsData {
		BYTE Data[PACKET_SIZE];
		LONGLONG PTS;
	};

	void ResetSync();
	void AddData(const CMediaData *pMediaData);
	void AddPacket(const PacketPtsData *pPacket);
	void AddPoolPackets();

	CCriticalLock m_Lock;
	CChunkedRingBuffer<BYTE,PACKET_SIZE,1024> m_PacketQueue;
	CRingBuffer<PacketPtsData> m_PacketPool;

	size_t m_QueueSize;
	size_t m_PoolSize;
	bool m_bEnableSync;
	bool m_bSyncFor1Seg;
	LONGLONG m_VideoPTS;
	LONGLONG m_VideoPTSPrev;
	LONGLONG m_AudioPTS;
	LONGLONG m_AudioPTSPrev;
	LONGLONG m_PTSDuration;
	WORD m_VideoPID;
	WORD m_AudioPID;
};


#endif
