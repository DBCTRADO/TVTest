#pragma once

#include "../BonTsEngine/TsMedia.h"
#include "VideoParser.h"


// InPlaceフィルタにする
#define MPEG2PARSERFILTER_INPLACE

// テンプレート名
#define MPEG2PARSERFILTER_NAME L"Mpeg2 Parser Filter"

// このフィルタのGUID {3F8400DA-65F1-4694-BB05-303CDE739680}
DEFINE_GUID(CLSID_Mpeg2ParserFilter, 0x3f8400da, 0x65f1, 0x4694, 0xbb, 0x5, 0x30, 0x3c, 0xde, 0x73, 0x96, 0x80);

class __declspec(uuid("3F8400DA-65F1-4694-BB05-303CDE739680")) CMpeg2ParserFilter
#ifndef MPEG2PARSERFILTER_INPLACE
	: public CTransformFilter
#else
	: public CTransInPlaceFilter
#endif
	, public CVideoParser
	, protected CMpeg2Parser::ISequenceHandler
{
public:
	DECLARE_IUNKNOWN

	static IBaseFilter* WINAPI CreateInstance(LPUNKNOWN pUnk, HRESULT *phr);

// CTransformFilter
#ifndef MPEG2PARSERFILTER_INPLACE
	HRESULT CheckInputType(const CMediaType* mtIn) override;
	HRESULT CheckTransform(const CMediaType* mtIn, const CMediaType* mtOut override);
	HRESULT DecideBufferSize(IMemAllocator * pAllocator, ALLOCATOR_PROPERTIES *pprop) override;
	HRESULT GetMediaType(int iPosition, CMediaType *pMediaType) override;
#else
// CTransInPlaceFilter
	HRESULT CheckInputType(const CMediaType* mtIn) override;
#endif
	HRESULT StartStreaming() override;
	HRESULT StopStreaming() override;
	HRESULT BeginFlush() override;

// CMpeg2ParserFilter
#ifndef MPEG2PARSERFILTER_INPLACE
	void SetFixSquareDisplay(bool bFix);
#endif
	void SetAttachMediaType(bool bAttach);

protected:
	CMpeg2ParserFilter(LPUNKNOWN pUnk, HRESULT *phr);
	~CMpeg2ParserFilter();

#ifndef MPEG2PARSERFILTER_INPLACE
// CTransformFilter
	HRESULT Transform(IMediaSample * pIn, IMediaSample *pOut) override;
#else
// CTransInPlaceFilter
	HRESULT Transform(IMediaSample *pSample) override;
	HRESULT Receive(IMediaSample *pSample) override;
#endif

// CMpeg2Parser::ISequenceHandler
	virtual void OnMpeg2Sequence(const CMpeg2Parser *pMpeg2Parser, const CMpeg2Sequence *pSequence) override;

	CMpeg2Parser m_Mpeg2Parser;
	bool m_bAttachMediaType;
	IMediaSample *m_pOutSample;
};
