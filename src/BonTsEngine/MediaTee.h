// MediaTee.h: CMediaTee クラスのインターフェイス
//
//////////////////////////////////////////////////////////////////////

#pragma once


#include "MediaDecoder.h"


/////////////////////////////////////////////////////////////////////////////
// メディアティー(グラフを通過するCMediaDataを分岐させる)
/////////////////////////////////////////////////////////////////////////////
// Input	#0	: CMediaData		入力データ
// Output	#0	: CMediaData		出力データ
// Output	#N	: CMediaData		出力データ
/////////////////////////////////////////////////////////////////////////////

class CMediaTee : public CMediaDecoder  
{
public:
	CMediaTee(IEventHandler *pEventHandler = NULL, const DWORD dwOutputNum = 2UL);
	virtual ~CMediaTee();

// IMediaDecoder
	virtual const bool InputMedia(CMediaData *pMediaData, const DWORD dwInputIndex = 0UL);
};
