#include "StdAfx.h"
#include "VideoParser.h"
#include "../Common/DebugDef.h"


CVideoParser::CVideoParser()
	: m_pVideoInfoCallback(NULL)
	, m_pCallbackParam(NULL)
	, m_pStreamCallback(NULL)
	, m_bAttachMediaType(false)
{
}


CVideoParser::~CVideoParser()
{
}


bool CVideoParser::GetVideoInfo(VideoInfo *pInfo) const
{
	if (!pInfo)
		return false;

	CAutoLock Lock(&m_ParserLock);

	*pInfo = m_VideoInfo;

	return true;
}


void CVideoParser::ResetVideoInfo()
{
	CAutoLock Lock(&m_ParserLock);

	m_VideoInfo.Reset();
}


void CVideoParser::SetVideoInfoCallback(VideoInfoCallback pCallback, const PVOID pParam)
{
	CAutoLock Lock(&m_ParserLock);

	m_pVideoInfoCallback = pCallback;
	m_pCallbackParam = pParam;
}


void CVideoParser::SetStreamCallback(IStreamCallback *pCallback)
{
	CAutoLock Lock(&m_ParserLock);

	m_pStreamCallback = pCallback;
}


void CVideoParser::SetAttachMediaType(bool bAttach)
{
	CAutoLock Lock(&m_ParserLock);

	m_bAttachMediaType = bAttach;
}


void CVideoParser::NotifyVideoInfo() const
{
	if (m_pVideoInfoCallback)
		m_pVideoInfoCallback(&m_VideoInfo, m_pCallbackParam);
}


// GCD (Greatest Common Denominator)
template<typename T> T GCD(T m, T n)
{
	if (m != 0 && n != 0) {
		do {
			T r;

			r = m % n;
			m = n;
			n = r;
		} while (n != 0);
	} else {
		m = 0;
	}

	return m;
}

bool CVideoParser::SARToDAR(WORD SarX, WORD SarY, WORD Width, WORD Height,
							BYTE *pDarX, BYTE *pDarY)
{
	DWORD DispWidth = Width * SarX, DispHeight = Height * SarY;
	DWORD Denom = GCD(DispWidth, DispHeight);

	if (Denom != 0) {
		DWORD DarX = DispWidth / Denom, DarY = DispHeight / Denom;
		if (DarX <= 255 && DarY <= 255) {
			*pDarX = (BYTE)DarX;
			*pDarY = (BYTE)DarY;
			return true;
		}
	}

	return false;
}
