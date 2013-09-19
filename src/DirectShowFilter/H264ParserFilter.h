#pragma once

#include "../BonTsEngine/MediaData.h"
#include "../BonTsEngine/TsMedia.h"
#include "VideoParser.h"
#include <deque>


#define H264PARSERFILTER_NAME L"H264 Parser Filter"

// ‚±‚ÌƒtƒBƒ‹ƒ^‚ÌGUID {46941C5F-AD0A-47fc-A35A-155ECFCEB4BA}
DEFINE_GUID(CLSID_H264ParserFilter, 0x46941c5f, 0xad0a, 0x47fc, 0xa3, 0x5a, 0x15, 0x5e, 0xcf, 0xce, 0xb4, 0xba);

class __declspec(uuid("46941C5F-AD0A-47fc-A35A-155ECFCEB4BA")) CH264ParserFilter
	: public CTransformFilter
	, public CVideoParser
	, protected CH264Parser::IAccessUnitHandler
{
public:
	DECLARE_IUNKNOWN

	static IBaseFilter* WINAPI CreateInstance(LPUNKNOWN pUnk, HRESULT *phr);

// CTransInplaceFilter
	HRESULT CheckInputType(const CMediaType* mtIn) override;
	HRESULT CheckTransform(const CMediaType* mtIn, const CMediaType* mtOut) override;
	HRESULT DecideBufferSize(IMemAllocator * pAllocator, ALLOCATOR_PROPERTIES *pprop) override;
	HRESULT GetMediaType(int iPosition, CMediaType *pMediaType) override;
	HRESULT StartStreaming() override;
	HRESULT StopStreaming() override;
	HRESULT BeginFlush() override;

// CH264ParserFilter
	bool SetAdjustTime(bool bAdjust);
	bool SetAdjustFrameRate(bool bAdjust);
	void SetAttachMediaType(bool bAttach);

protected:
	CH264ParserFilter(LPUNKNOWN pUnk, HRESULT *phr);
	~CH264ParserFilter();

// CTransformFilter
	HRESULT Transform(IMediaSample *pIn, IMediaSample *pOut) override;
	HRESULT Receive(IMediaSample *pSample) override;

// CH264Parser::IAccessUnitHandler
	virtual void OnAccessUnit(const CH264Parser *pParser, const CH264AccessUnit *pAccessUnit) override;

// CH264ParserFilter
	class CSampleData : public CMediaData
	{
	public:
		REFERENCE_TIME m_StartTime;
		REFERENCE_TIME m_EndTime;
		bool m_bChangeSize;
		int m_Width;
		int m_Height;

		CSampleData(const CMediaData &Data);
		void ResetProperties();
		void SetTime(const REFERENCE_TIME &StartTime, const REFERENCE_TIME &EndTime);
		bool HasTimestamp() const { return m_StartTime >= 0; }
		void ChangeSize(int Width, int Height);
	};

	typedef std::deque<CSampleData*> CSampleDataQueue;

	class CSampleDataPool
	{
	public:
		CSampleDataPool();
		~CSampleDataPool();
		void Clear();
		CSampleData *Get(const CMediaData &Data);
		void Restore(CSampleData *pData);

	private:
		size_t m_MaxData;
		size_t m_DataCount;
		CSampleDataQueue m_Queue;
		CCritSec m_Lock;
	};

	void Reset();
	void ClearSampleDataQueue(CSampleDataQueue *pQueue);
	HRESULT AttachMediaType(IMediaSample *pSample, int Width, int Height);

	CMediaType m_MediaType;
	CH264Parser m_H264Parser;
	CSampleDataPool m_SampleDataPool;
	CSampleDataQueue m_OutSampleQueue;
	bool m_bAdjustTime;
	bool m_bAdjustFrameRate;
	REFERENCE_TIME m_PrevTime;
	DWORD m_SampleCount;
	CSampleDataQueue m_SampleQueue;
	bool m_bAttachMediaType;
	bool m_bChangeSize;
};
