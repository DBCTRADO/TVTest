#include "stdafx.h"
#include "TsSrcStream.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


#define TS_PACKET_SIZE	188
#define PID_INVALID	0xFFFF

#define PTS_CLOCK 90000LL	// 90kHz
#define GET_PTS(p) \
	(((LONGLONG)(((DWORD)((p)[0] & 0x0E) << 14) | ((DWORD)(p)[1] << 7) | ((DWORD)(p)[2] >> 1)) << 15) | \
	(LONGLONG)(((DWORD)(p)[3] << 7) | ((DWORD)(p)[4] >> 1)))

#define ERR_PTS_DIFF	(PTS_CLOCK*5)
#define MAX_AUDIO_DELAY	(PTS_CLOCK)
#define MIN_AUDIO_DELAY	(PTS_CLOCK/5)


CTsSrcStream::CTsSrcStream(DWORD BufferLength)
	: m_pBuffer(NULL)
	, m_BufferLength(BufferLength)
	, m_bEnableSync(false)
	, m_VideoPID(PID_INVALID)
	, m_AudioPID(PID_INVALID)
	, m_Allocator(sizeof(PacketData),2048)
{
	m_pBuffer=new BYTE[BufferLength*TS_PACKET_SIZE];
	Reset();
}


CTsSrcStream::~CTsSrcStream()
{
	if (m_pBuffer!=NULL)
		delete [] m_pBuffer;
}


bool CTsSrcStream::InputMedia(const CMediaData *pMediaData)
{
	CBlockLock Lock(&m_Lock);

	if (!m_bEnableSync) {
		AddData(pMediaData);
		return true;
	}

	/*
		PTSÇìØä˙Ç∑ÇÈèàóùÇÕÅATBSñ‚ëËëŒçÙàƒ(up0357)Çå≥Ç…ÇµÇƒÇ¢Ç‹Ç∑ÅB
	*/

	const CTsPacket *pPacket=static_cast<const CTsPacket*>(pMediaData);
	const WORD PID=pPacket->GetPID();
	if (PID!=m_VideoPID && PID!=m_AudioPID) {
		AddData(pMediaData);
		return true;
	}

	if (pPacket->m_Header.bPayloadUnitStartIndicator) {
		const DWORD Size=pPacket->GetPayloadSize();
		if (Size<14) {
			AddData(pMediaData);
			return true;
		}
		const BYTE *pData=pPacket->GetPayloadData();
		if (pData==NULL) {
			AddData(pMediaData);
			return true;
		}
		if (pData[0]!=0 || pData[1]!=0 || pData[2]!=1) {	// !PES
			AddData(pMediaData);
			return true;
		}
		if ((pData[7]&0x80)!=0) {	// pts_flag
			if (PID==m_VideoPID) {
				m_VideoPTSPrev=m_VideoPTS;
				m_VideoPTS=GET_PTS(&pData[9]);
			} else {
				m_AudioPTSPrev=m_AudioPTS;
				m_AudioPTS=GET_PTS(&pData[9]);
			}
		}
	}

#ifdef BONTSENGINE_1SEG_SUPPORT

	LONGLONG AudioPTS=m_AudioPTS;
	if (m_AudioPTSPrev>=0) {
		if (AudioPTS<m_AudioPTSPrev)
			AudioPTS+=0x200000000LL;
		if (AudioPTS>=m_AudioPTSPrev+ERR_PTS_DIFF) {
			TRACE(TEXT("Reset Audio PTS : Adj=%X%08X Cur=%X%08X Prev=%X%08X\n"),
				(DWORD)(AudioPTS>>32),(DWORD)AudioPTS,
				(DWORD)(m_AudioPTS>>32),(DWORD)m_AudioPTS,
				(DWORD)(m_AudioPTSPrev>>32),(DWORD)m_AudioPTSPrev);
			AddPoolPackets();
			ResetSync();
			AudioPTS=-1;
		}
	}

	if (PID == m_VideoPID) {
		if (m_VideoPTS>=0) {
			PacketData *pData=(PacketData*)m_Allocator.Allocate();
			if (pData!=NULL) {
				::CopyMemory(pData->m_Data,pMediaData->GetData(),TS_PACKET_SIZE);
				pData->m_PTS=m_VideoPTS;
				m_PoolPacketList.push_back(pData);
			}
		} else {
			AddData(pMediaData);
		}
	} else {
		AddData(pMediaData);
	}

	while (!m_PoolPacketList.empty() && m_BufferUsed<m_BufferLength) {
		PacketData *pData=m_PoolPacketList.front();
		if (AudioPTS<0
				|| pData->m_PTS<=AudioPTS+MAX_AUDIO_DELAY
				|| pData->m_PTS>=AudioPTS+ERR_PTS_DIFF) {
			AddPacket(pData);
			m_Allocator.Free(pData);
			m_PoolPacketList.erase(m_PoolPacketList.begin());
		} else {
			break;
		}
	}

#else	// BONTSENGINE_1SEG_SUPPORT

	LONGLONG VideoPTS=m_VideoPTS;
	if (m_VideoPTSPrev>=0 && _abs64(VideoPTS-m_VideoPTSPrev)>=ERR_PTS_DIFF) {
		if (VideoPTS<m_VideoPTSPrev)
			VideoPTS+=0x200000000LL;
		if (VideoPTS>=m_VideoPTSPrev+ERR_PTS_DIFF) {
			TRACE(TEXT("Reset Video PTS : Adj=%X%08X Cur=%X%08X Prev=%X%08X\n"),
				(DWORD)(VideoPTS>>32),(DWORD)VideoPTS,
				(DWORD)(m_VideoPTS>>32),(DWORD)m_VideoPTS,
				(DWORD)(m_VideoPTSPrev>>32),(DWORD)m_VideoPTSPrev);
			AddPoolPackets();
			ResetSync();
			VideoPTS=-1;
		}
	}

	if (PID == m_AudioPID) {
		if (m_AudioPTS>=0) {
			PacketData *pData=(PacketData*)m_Allocator.Allocate();
			if (pData!=NULL) {
				::CopyMemory(pData->m_Data,pMediaData->GetData(),TS_PACKET_SIZE);
				pData->m_PTS=m_AudioPTS;
				m_PoolPacketList.push_back(pData);
			}
		} else {
			AddData(pMediaData);
		}
	} else {
		AddData(pMediaData);
	}

	while (!m_PoolPacketList.empty() && m_BufferUsed<m_BufferLength) {
		PacketData *pData=m_PoolPacketList.front();
		if (VideoPTS<0
				|| pData->m_PTS+MIN_AUDIO_DELAY<=VideoPTS
				|| pData->m_PTS>=VideoPTS+ERR_PTS_DIFF) {
			AddPacket(pData);
			m_Allocator.Free(pData);
			m_PoolPacketList.erase(m_PoolPacketList.begin());
		} else {
			break;
		}
	}

#endif	// ndef BONTSENGINE_1SEG_SUPPORT

#ifdef _DEBUG
	/*
	static DWORD Time;
	DWORD CurTime=::GetTickCount();
	if (CurTime-Time>=5000) {
		DWORD VideoMs=(DWORD)(m_VideoPTS/90);
		DWORD AudioMs=(DWORD)(m_AudioPTS/90);
		TRACE(TEXT("PTS Video %lu.%03lu / Audio %lu.%03lu / Pool %lu / Buffer %lu/%lu\n"),
			  VideoMs/1000,VideoMs%1000,AudioMs/1000,AudioMs%1000,(DWORD)m_PoolPacketList.size(),m_BufferUsed,m_BufferLength);
		Time=CurTime;
	}
	*/
#endif

	return true;
}


void CTsSrcStream::AddData(const CMediaData *pMediaData)
{
	::CopyMemory(m_pBuffer+((m_BufferPos+m_BufferUsed)%m_BufferLength)*TS_PACKET_SIZE,pMediaData->GetData(),TS_PACKETSIZE);
	if (m_BufferUsed<m_BufferLength)
		m_BufferUsed++;
	else
		m_BufferPos++;
}


void CTsSrcStream::AddPacket(const PacketData *pPacket)
{
	::CopyMemory(m_pBuffer+((m_BufferPos+m_BufferUsed)%m_BufferLength)*TS_PACKET_SIZE,pPacket->m_Data,TS_PACKETSIZE);
	if (m_BufferUsed<m_BufferLength)
		m_BufferUsed++;
	else
		m_BufferPos++;
}


void CTsSrcStream::AddPoolPackets()
{
	while (!m_PoolPacketList.empty()) {
		PacketData *pData=m_PoolPacketList.front();
		AddPacket(pData);
		m_Allocator.Free(pData);
		m_PoolPacketList.erase(m_PoolPacketList.begin());
	}
}


bool CTsSrcStream::GetData(BYTE *pData,DWORD *pSize)
{
	CBlockLock Lock(&m_Lock);
	DWORD Size;

	if (pData==NULL || m_BufferUsed==0) {
		*pSize=0;
		return false;
	}
	if (m_BufferUsed<=m_BufferLength-m_BufferPos)
		Size=m_BufferUsed;
	else
		Size=m_BufferLength-m_BufferPos;
	if (Size>*pSize)
		Size=*pSize;
	else
		*pSize=Size;
	::CopyMemory(pData,m_pBuffer+m_BufferPos*TS_PACKET_SIZE,Size*TS_PACKET_SIZE);
	m_BufferPos+=Size;
	if (m_BufferPos==m_BufferLength)
		m_BufferPos=0;
	m_BufferUsed-=Size;
	return true;
}


bool CTsSrcStream::IsDataAvailable()
{
	CBlockLock Lock(&m_Lock);

	return m_BufferUsed>0;
}


bool CTsSrcStream::IsBufferFull()
{
	CBlockLock Lock(&m_Lock);

	return m_BufferUsed==m_BufferLength;
}


void CTsSrcStream::Reset()
{
	CBlockLock Lock(&m_Lock);

	m_BufferUsed=0;
	m_BufferPos=0;

	ResetSync();
}


void CTsSrcStream::ResetSync()
{
	m_VideoPTS=-1;
	m_VideoPTSPrev=-1;
	m_AudioPTS=-1;
	m_AudioPTSPrev=-1;
	m_PoolPacketList.clear();
	m_Allocator.FreeAll();
}


bool CTsSrcStream::EnableSync(bool bEnable)
{
	CBlockLock Lock(&m_Lock);

	if (m_bEnableSync!=bEnable) {
		TRACE(TEXT("CTsSrcStream::EnableSync(%s)\n"),bEnable?TEXT("true"):TEXT("false"));
		ResetSync();
		m_bEnableSync=bEnable;
	}
	return true;
}


void CTsSrcStream::SetVideoPID(WORD PID)
{
	CBlockLock Lock(&m_Lock);

	m_VideoPID=PID;
}


void CTsSrcStream::SetAudioPID(WORD PID)
{
	CBlockLock Lock(&m_Lock);

	m_AudioPID=PID;
}




CTsSrcStream::CAllocator::CAllocator(size_t BlockSize,size_t BufferLength)
	: m_BlockSize(BlockSize)
	, m_BufferLength(BufferLength)
	, m_AllocCount(0)
	, m_AllocPos(0)
{
	m_pBuffer=new BYTE[BlockSize*BufferLength];
	m_pBlockUsed=new bool[BufferLength];
	::ZeroMemory(m_pBlockUsed,BufferLength*sizeof(bool));
}


CTsSrcStream::CAllocator::~CAllocator()
{
	delete [] m_pBuffer;
	delete [] m_pBlockUsed;
}


void *CTsSrcStream::CAllocator::Allocate()
{
	if (m_AllocCount<m_BufferLength) {
		while (true) {
			if (m_AllocPos==m_BufferLength)
				m_AllocPos=0;
			if (!m_pBlockUsed[m_AllocPos])
				break;
			m_AllocPos++;
		}
	} else {
		TRACE(TEXT("CTsSrcStream::CAllocator::Allocate() No more block\n"));
		return NULL;
	}
	m_AllocCount++;
	m_pBlockUsed[m_AllocPos]=true;
	return &m_pBuffer[(m_AllocPos++)*m_BlockSize];
}


void CTsSrcStream::CAllocator::Free(void *pBlock)
{
	size_t Pos=((BYTE*)pBlock-m_pBuffer)/m_BlockSize;

#ifdef _DEBUG
	if (Pos>=m_BufferLength || !m_pBlockUsed[Pos]) {
		::DebugBreak();
		return;
	}
#endif
	m_pBlockUsed[Pos]=false;
	m_AllocCount--;
}


void CTsSrcStream::CAllocator::FreeAll()
{
	::ZeroMemory(m_pBlockUsed,m_BufferLength*sizeof(bool));
	m_AllocCount=0;
	m_AllocPos=0;
}
