// MediaGrabber.h: CMediaGrabber クラスのインターフェイス
//
//////////////////////////////////////////////////////////////////////

#pragma once


#include "MediaDecoder.h"


/////////////////////////////////////////////////////////////////////////////
// サンプルグラバ(グラフを通過するCMediaDataにアクセスする手段を提供する)
/////////////////////////////////////////////////////////////////////////////
// Input	#0	: CMediaData		入力データ
// Output	#0	: CMediaData		出力データ
/////////////////////////////////////////////////////////////////////////////

class CMediaGrabber : public CMediaDecoder  
{
public:
	typedef bool (CALLBACK * MEDIAGRABFUNC)(const CMediaData *pMediaData, const PVOID pParam);	// メディア受け取りコールバック型
	typedef void (CALLBACK * RESETGRABFUNC)(const PVOID pParam);								// リセット受け取りコールバック型

	CMediaGrabber(IEventHandler *pEventHandler = NULL);
	virtual ~CMediaGrabber();

// IMediaDecoder
	virtual void Reset(void);
	virtual const bool InputMedia(CMediaData *pMediaData, const DWORD dwInputIndex = 0UL);

// CMediaGrabber
	void SetMediaGrabCallback(const MEDIAGRABFUNC pCallback, const PVOID pParam = NULL);
	void SetResetGrabCallback(const RESETGRABFUNC pCallback, const PVOID pParam = NULL);

protected:
	MEDIAGRABFUNC m_pfnMediaGrabFunc;
	RESETGRABFUNC m_pfnResetGrabFunc;

	PVOID m_pMediaGrabParam;
	PVOID m_pResetGrabParam;
};
