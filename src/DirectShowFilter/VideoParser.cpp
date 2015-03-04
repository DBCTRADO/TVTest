#include "StdAfx.h"
#include "VideoParser.h"
#include "../Common/DebugDef.h"


CVideoParser::CVideoParser()
	: m_pVideoInfoCallback(NULL)
	, m_pCallbackParam(NULL)
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


DWORD CVideoParser::GetBitRate() const
{
	CAutoLock Lock(&m_ParserLock);

	return m_BitRateCalculator.GetBitRate();
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


bool CVideoParser::SARToDAR(WORD SarX, WORD SarY, WORD Width, WORD Height,
							BYTE *pDarX, BYTE *pDarY)
{
	DWORD DispWidth = Width * SarX, DispHeight = Height * SarY;

	// ‚Æ‚è‚ ‚¦‚¸ 16:9 ‚Æ 4:3 ‚¾‚¯
	if (DispWidth * 9 == DispHeight * 16) {
		*pDarX = 16;
		*pDarY = 9;
	} else if (DispWidth * 3 == DispHeight * 4) {
		*pDarX = 4;
		*pDarY = 3;
	} else {
		return false;
	}

	return true;
}
