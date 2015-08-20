#include "stdafx.h"
#include "TsSrcStream.h"
#include "../Common/DebugDef.h"


#define PID_INVALID	0xFFFF

#define PTS_CLOCK 90000LL	// 90kHz

#define ERR_PTS_DIFF	(PTS_CLOCK*5)
#define MAX_AUDIO_DELAY	(PTS_CLOCK)
#define MIN_AUDIO_DELAY	(PTS_CLOCK/5)


static inline LONGLONG GetPTS(const BYTE *p)
{
	return ((LONGLONG)(((DWORD)(p[0] & 0x0E) << 14) |
					   ((DWORD)p[1] << 7) |
					   ((DWORD)p[2] >> 1)) << 15) |
			(LONGLONG)(((DWORD)p[3] << 7) |
					   ((DWORD)p[4] >> 1));
}

static inline LONGLONG GetPacketPTS(const CTsPacket *pPacket)
{
	const DWORD Size = pPacket->GetPayloadSize();
	if (Size >= 14) {
		const BYTE *pData = pPacket->GetPayloadData();
		if (pData != NULL) {
			if (pData[0] == 0 && pData[1] == 0 && pData[2] == 1) {	// PES
				if ((pData[7] & 0x80) != 0) {	// pts_flag
					return GetPTS(&pData[9]);
				}
			}
		}
	}

	return -1;
}


CTsSrcStream::CTsSrcStream()
	: m_QueueSize(DEFAULT_QUEUE_SIZE)
	, m_PoolSize(DEFAULT_POOL_SIZE)
	, m_bEnableSync(false)
	, m_bSyncFor1Seg(false)
	, m_VideoPID(PID_INVALID)
	, m_AudioPID(PID_INVALID)
	, m_MapAudioPID(PID_INVALID)
{
	Reset();
}


CTsSrcStream::~CTsSrcStream()
{
}


bool CTsSrcStream::Initialize()
{
	Reset();

	if (!ResizeQueue(m_QueueSize, m_bEnableSync ? m_PoolSize : 0))
		return false;

	if (m_bEnableSync) {
		if (!m_PacketPool.Allocate(m_PoolSize))
			return false;
	}

	return true;
}


bool CTsSrcStream::InputMedia(CMediaData *pMediaData)
{
	CBlockLock Lock(&m_Lock);

	/*
		PTSを同期する処理は、TBS問題対策案(up0357)を元にしています。
	*/

	CTsPacket *pPacket = static_cast<CTsPacket*>(pMediaData);
	const WORD PID = pPacket->GetPID();
	if (PID != m_VideoPID && PID != m_AudioPID) {
		if (PID != m_MapAudioPID)
			AddData(pMediaData);
		return true;
	}

	const bool bVideoPacket = (PID == m_VideoPID);

	if (!bVideoPacket && m_MapAudioPID != PID_INVALID) {
		pPacket->SetPID(m_MapAudioPID);
	}

	if (pPacket->GetPayloadUnitStartIndicator()) {
		const LONGLONG PTS = GetPacketPTS(pPacket);
		if (PTS >= 0) {
			if (bVideoPacket) {
				m_VideoPTSPrev = m_VideoPTS;
				m_VideoPTS = PTS;
			} else {
				if (m_AudioPTSPrev >= 0) {
					if (m_AudioPTSPrev < PTS)
						m_PTSDuration += PTS - m_AudioPTSPrev;
				}
				m_AudioPTSPrev = m_AudioPTS;
				m_AudioPTS = PTS;
			}
		}
	}

	if (!m_bEnableSync || m_PacketPool.GetCapacity() == 0) {
		AddData(pMediaData);
		return true;
	}

	if (m_bSyncFor1Seg) {
		LONGLONG AudioPTS = m_AudioPTS;
		if (m_AudioPTSPrev >= 0) {
			if (AudioPTS < m_AudioPTSPrev) {
				TRACE(TEXT("Audio PTS wrap-around\n"));
				AudioPTS += 0x200000000LL;
			}
			if (AudioPTS >= m_AudioPTSPrev + ERR_PTS_DIFF) {
				TRACE(TEXT("Reset Audio PTS : Adj=%llX Cur=%llX Prev=%llX\n"),
					  AudioPTS, m_AudioPTS, m_AudioPTSPrev);
				AddPoolPackets();
				ResetSync();
				AudioPTS = -1;
			}
		}

		if (bVideoPacket && m_VideoPTS >= 0) {
			if (m_PacketPool.IsFull())
				AddPoolPacket();
			PacketPtsData *pData = m_PacketPool.Push();
			::CopyMemory(pData->Data, pMediaData->GetData(), PACKET_SIZE);
			pData->PTS = m_VideoPTS;
		} else {
			AddData(pMediaData);
		}

		while (!m_PacketPool.IsEmpty() && !m_PacketQueue.IsFull()) {
			PacketPtsData *pData = m_PacketPool.Front();
			if (AudioPTS < 0
					|| pData->PTS <= AudioPTS + MAX_AUDIO_DELAY
					|| pData->PTS >= AudioPTS + ERR_PTS_DIFF) {
				AddPacket(pData);
				m_PacketPool.Pop();
			} else {
				break;
			}
		}
	} else {
		LONGLONG VideoPTS = m_VideoPTS;
		if (m_VideoPTSPrev >= 0 && _abs64(VideoPTS - m_VideoPTSPrev) >= ERR_PTS_DIFF) {
			if (VideoPTS < m_VideoPTSPrev) {
				TRACE(TEXT("Video PTS wrap-around\n"));
				VideoPTS += 0x200000000LL;
			}
			if (VideoPTS >= m_VideoPTSPrev + ERR_PTS_DIFF) {
				TRACE(TEXT("Reset Video PTS : Adj=%llX Cur=%llX Prev=%llX\n"),
					  VideoPTS, m_VideoPTS, m_VideoPTSPrev);
				AddPoolPackets();
				ResetSync();
				VideoPTS = -1;
			}
		}

		if (!bVideoPacket && m_AudioPTS >= 0) {
			if (m_PacketPool.IsFull())
				AddPoolPacket();
			PacketPtsData *pData = m_PacketPool.Push();
			::CopyMemory(pData->Data, pMediaData->GetData(), PACKET_SIZE);
			pData->PTS = m_AudioPTS;
		} else {
			AddData(pMediaData);
		}

		while (!m_PacketPool.IsEmpty() && !m_PacketQueue.IsFull()) {
			PacketPtsData *pData = m_PacketPool.Front();
			if (VideoPTS < 0
					|| pData->PTS + MIN_AUDIO_DELAY <= VideoPTS
					|| pData->PTS >= VideoPTS + ERR_PTS_DIFF) {
				AddPacket(pData);
				m_PacketPool.Pop();
			} else {
				break;
			}
		}
	}

//#ifdef _DEBUG
#if 0
	static DWORD Time;
	DWORD CurTime=::GetTickCount();
	if (CurTime-Time>=10000) {
		DWORD VideoMs=(DWORD)(m_VideoPTS/90);
		DWORD AudioMs=(DWORD)(m_AudioPTS/90);
		TRACE(TEXT("PTS Video %lu.%03lu / Audio %lu.%03lu / Diff %+lld / Pool %lu/%lu / Buffer %lu/%lu/%lu\n"),
			  VideoMs/1000,VideoMs%1000,AudioMs/1000,AudioMs%1000,
			  m_AudioPTS-m_VideoPTS,
			  (DWORD)m_PacketPool.GetUsed(),(DWORD)m_PacketPool.GetCapacity(),
			  (DWORD)m_PacketQueue.GetUsed(),(DWORD)m_PacketQueue.GetAllocatedSize(),(DWORD)m_PacketQueue.GetCapacity());
		Time=CurTime;
	}
#endif

	return true;
}


void CTsSrcStream::AddData(const BYTE *pData)
{
	if (m_PacketQueue.IsFull())
		m_PacketQueue.Pop(m_QueueSize / 2);
	m_PacketQueue.Write(pData);
}


void CTsSrcStream::AddPoolPacket()
{
	if (!m_PacketPool.IsEmpty()) {
		AddPacket(m_PacketPool.Front());
		m_PacketPool.Pop();
	}
}


void CTsSrcStream::AddPoolPackets()
{
	while (!m_PacketPool.IsEmpty()) {
		AddPacket(m_PacketPool.Front());
		m_PacketPool.Pop();
	}
}


size_t CTsSrcStream::GetData(BYTE *pData, size_t Size)
{
	if (pData == NULL || Size == 0)
		return 0;

	CBlockLock Lock(&m_Lock);

	if (m_PacketQueue.IsEmpty())
		return 0;

	const size_t ActualSize = m_PacketQueue.Read(pData, Size);

	// 不要そうなメモリを解放
	if (m_PacketQueue.GetAllocatedChunkCount() >= 8
			&& m_PacketQueue.GetUsed() + m_PacketQueue.GetChunkSize() < m_PacketQueue.GetAllocatedSize() / 2) {
		TRACE(TEXT("CTsSrcStream::GetData() : Shrink to fit\n"));
		m_PacketQueue.ShrinkToFit();
	}

	return ActualSize;
}


void CTsSrcStream::Reset()
{
	CBlockLock Lock(&m_Lock);

	ResetSync();
	//m_PacketQueue.Clear();
	m_PacketQueue.Free();
}


void CTsSrcStream::ResetSync()
{
	m_VideoPTS = -1;
	m_VideoPTSPrev = -1;
	m_AudioPTS = -1;
	m_AudioPTSPrev = -1;
	m_PTSDuration = 0;
	m_PacketPool.Clear();
}


bool CTsSrcStream::IsDataAvailable()
{
	CBlockLock Lock(&m_Lock);

	return !m_PacketQueue.IsEmpty();
}


bool CTsSrcStream::IsBufferFull()
{
	CBlockLock Lock(&m_Lock);

	return m_PacketQueue.GetUsed() >= m_QueueSize;
}


bool CTsSrcStream::IsBufferActuallyFull()
{
	CBlockLock Lock(&m_Lock);

	return m_PacketQueue.IsFull();
}


int CTsSrcStream::GetFillPercentage()
{
	CBlockLock Lock(&m_Lock);

	if (m_PacketQueue.GetCapacity() == 0)
		return 0;

	return (int)(m_PacketQueue.GetUsed() * 100 / m_PacketQueue.GetCapacity());
}


bool CTsSrcStream::SetQueueSize(size_t Size)
{
	if (Size == 0)
		return false;

	CBlockLock Lock(&m_Lock);

	if (m_QueueSize != Size) {
		if (!ResizeQueue(Size, m_bEnableSync ? m_PoolSize : 0))
			return false;
		m_QueueSize = Size;
	}

	return true;
}


bool CTsSrcStream::SetPoolSize(size_t Size)
{
	if (Size == 0)
		return false;

	CBlockLock Lock(&m_Lock);

	if (m_PoolSize != Size) {
		if (m_PacketPool.IsAllocated()) {
			if (!m_PacketPool.Resize(Size))
				return false;
		}
		m_PoolSize = Size;
	}

	return true;
}


bool CTsSrcStream::EnableSync(bool bEnable,bool b1Seg)
{
	CBlockLock Lock(&m_Lock);

	if (m_bEnableSync != bEnable || m_bSyncFor1Seg != b1Seg) {
		TRACE(TEXT("CTsSrcStream::EnableSync(%d,%d)\n"), bEnable, b1Seg);

		ResetSync();

		if (!m_bEnableSync && bEnable) {
			if (!m_PacketPool.Allocate(m_PoolSize))
				return false;
		}

		m_bEnableSync = bEnable;
		m_bSyncFor1Seg = b1Seg;
	}

	return true;
}


void CTsSrcStream::SetVideoPID(WORD PID)
{
	CBlockLock Lock(&m_Lock);

	m_VideoPID = PID;
}


void CTsSrcStream::SetAudioPID(WORD PID)
{
	CBlockLock Lock(&m_Lock);

	m_AudioPID = PID;
	m_MapAudioPID = PID_INVALID;
}


void CTsSrcStream::MapAudioPID(WORD AudioPID, WORD MapPID)
{
	TRACE(TEXT("CTsSrcStream::MapAudioPID() : %04x -> %04x\n"), AudioPID, MapPID);

	CBlockLock Lock(&m_Lock);

	m_AudioPID = AudioPID;
	if (AudioPID == MapPID)
		m_MapAudioPID = PID_INVALID;
	else
		m_MapAudioPID = MapPID;
}


bool CTsSrcStream::ResizeQueue(size_t QueueSize, size_t PoolSize)
{
	const size_t ChunkSize = m_PacketQueue.GetChunkSize();
	return m_PacketQueue.Resize((QueueSize + PoolSize + ChunkSize - 1) / ChunkSize);
}
