// MediaTee.cpp: CMediaTee クラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "MediaTee.h"
#include "../Common/DebugDef.h"

//////////////////////////////////////////////////////////////////////
// 構築/消滅
//////////////////////////////////////////////////////////////////////

CMediaTee::CMediaTee(IEventHandler *pEventHandler, const DWORD dwOutputNum)
	: CMediaDecoder(pEventHandler, 1UL, dwOutputNum)
{
}

CMediaTee::~CMediaTee()
{
}

const bool CMediaTee::InputMedia(CMediaData *pMediaData, const DWORD dwInputIndex)
{
	CBlockLock Lock(&m_DecoderLock);

	/*
	if (dwInputIndex >= GetInputNum())
		return false;
	*/

	// 下位デコーダにデータを渡す
	for (DWORD dwOutIndex = 0UL ; dwOutIndex < GetOutputNum() ; dwOutIndex++) {
		OutputMedia(pMediaData, dwOutIndex);
	}

	return true;
}
