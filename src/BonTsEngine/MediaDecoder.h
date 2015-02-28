// MediaDecoder.h: CMediaDecoder クラスのインターフェイス
//
//////////////////////////////////////////////////////////////////////

#pragma once


#include "Common.h"
#include "BonBaseClass.h"
#include "MediaData.h"
#include "TsUtilClass.h"


//////////////////////////////////////////////////////////////////////
// メディアデコーダ基底クラス
//////////////////////////////////////////////////////////////////////

class ABSTRACT_CLASS_DECL CMediaDecoder : public CBonBaseClass
{
public:
	class ABSTRACT_CLASS_DECL IEventHandler
	{
	public:
		virtual const DWORD OnDecoderEvent(CMediaDecoder *pDecoder, const DWORD dwEventID, PVOID pParam) = 0;
	};

	CMediaDecoder(IEventHandler *pEventHandler = NULL, const DWORD dwInputNum = 1UL, const DWORD dwOutputNum = 1UL);
	virtual ~CMediaDecoder() = 0;

	virtual bool Initialize();
	virtual void Finalize();

	virtual void Reset();
	virtual void ResetGraph();

	virtual DWORD GetInputNum() const;
	virtual DWORD GetOutputNum() const;

	bool SetOutputDecoder(CMediaDecoder *pDecoder, const DWORD dwOutputIndex = 0UL, const DWORD dwInputIndex = 0UL);
	CMediaDecoder *GetOutputDecoder(const DWORD Index) const;
	virtual const bool InputMedia(CMediaData *pMediaData, const DWORD dwInputIndex = 0UL);

	virtual bool SetActiveServiceID(WORD ServiceID);
	virtual WORD GetActiveServiceID() const;

	void SetEventHandler(IEventHandler *pEventHandler);

protected:
	bool OutputMedia(CMediaData *pMediaData, const DWORD dwOutptIndex = 0UL);
	void ResetDownstreamDecoder();
	DWORD SendDecoderEvent(const DWORD dwEventID, PVOID pParam = NULL);

	// 出力ピンデータベース
	struct TAG_OUTPUTDECODER
	{
		CMediaDecoder *pDecoder;
		DWORD dwInputIndex;
	} m_aOutputDecoder[4];

	IEventHandler *m_pEventHandler;

	const DWORD m_dwInputNum;
	const DWORD m_dwOutputNum;

	mutable CCriticalLock m_DecoderLock;
};
