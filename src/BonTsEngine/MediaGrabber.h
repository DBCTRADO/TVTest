// MediaGrabber.h: CMediaGrabber クラスのインターフェイス
//
//////////////////////////////////////////////////////////////////////

#pragma once


#include "MediaDecoder.h"
#include <vector>


/////////////////////////////////////////////////////////////////////////////
// サンプルグラバ(グラフを通過するCMediaDataにアクセスする手段を提供する)
/////////////////////////////////////////////////////////////////////////////
// Input	#0	: CMediaData		入力データ
// Output	#0	: CMediaData		出力データ
/////////////////////////////////////////////////////////////////////////////

class CMediaGrabber : public CMediaDecoder
{
public:
	class ABSTRACT_CLASS_DECL IGrabber
	{
	public:
		virtual ~IGrabber() {}
		virtual bool OnInputMedia(CMediaData *pMediaData) { return true; }
		virtual void OnReset() {}
	};

	CMediaGrabber(IEventHandler *pEventHandler = NULL);
	virtual ~CMediaGrabber();

// IMediaDecoder
	virtual void Reset(void) override;
	virtual const bool InputMedia(CMediaData *pMediaData, const DWORD dwInputIndex = 0UL) override;

// CMediaGrabber
	bool AddGrabber(IGrabber *pGrabber);
	bool RemoveGrabber(IGrabber *pGrabber);

protected:
	std::vector<IGrabber*> m_GrabberList;
};
