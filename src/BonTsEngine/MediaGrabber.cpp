// MediaGrabber.cpp: CMediaGrabber クラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "MediaGrabber.h"
#include "../Common/DebugDef.h"

//////////////////////////////////////////////////////////////////////
// 構築/消滅
//////////////////////////////////////////////////////////////////////

CMediaGrabber::CMediaGrabber(IEventHandler *pEventHandler)
	: CMediaDecoder(pEventHandler, 1UL, 1UL)
{
}

CMediaGrabber::~CMediaGrabber()
{
}

void CMediaGrabber::Reset(void)
{
	CBlockLock Lock(&m_DecoderLock);

	// コールバックに通知する
	for (auto itr = m_GrabberList.begin(); itr != m_GrabberList.end(); ++itr)
		(*itr)->OnReset();
}

const bool CMediaGrabber::InputMedia(CMediaData *pMediaData, const DWORD dwInputIndex)
{
	CBlockLock Lock(&m_DecoderLock);

	/*
	if (dwInputIndex >= GetInputNum())
		return false;
	*/

	bool bFail = false;

	// コールバックに通知する
	for (auto itr = m_GrabberList.begin(); itr != m_GrabberList.end(); ++itr) {
		if (!(*itr)->OnInputMedia(pMediaData))
			bFail = true;
	}

	if (bFail)
		return false;

	// 下位デコーダにデータを渡す
	OutputMedia(pMediaData);

	return true;
}

bool CMediaGrabber::AddGrabber(IGrabber *pGrabber)
{
	if (!pGrabber)
		return false;

	CBlockLock Lock(&m_DecoderLock);

	m_GrabberList.push_back(pGrabber);

	return true;
}

bool CMediaGrabber::RemoveGrabber(IGrabber *pGrabber)
{
	CBlockLock Lock(&m_DecoderLock);

	for (auto itr = m_GrabberList.begin(); itr != m_GrabberList.end(); ++itr) {
		if (*itr == pGrabber) {
			m_GrabberList.erase(itr);
			return true;
		}
	}

	return false;
}
