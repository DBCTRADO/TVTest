#pragma once


#include "TsSrcStream.h"
#include "MediaData.h"


class CBonSrcFilter;

class CBonSrcPin : public CBaseOutputPin
{
public:
	CBonSrcPin(HRESULT *phr, CBonSrcFilter *pFilter);
	virtual ~CBonSrcPin();

// CBasePin
	HRESULT GetMediaType(int iPosition, CMediaType *pMediaType) override;
	HRESULT CheckMediaType(const CMediaType *pMediaType) override;

	HRESULT Active() override;
	HRESULT Inactive() override;
	HRESULT Run(REFERENCE_TIME tStart) override;

// CBaseOutputPin
	HRESULT DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pRequest) override;

// CBonSrcPin
	bool InputMedia(CMediaData *pMediaData);

	void Reset();
	void Flush();
	bool EnableSync(bool bEnable);
	bool IsSyncEnabled() const;
	void SetVideoPID(WORD PID);
	void SetAudioPID(WORD PID);
	void SetOutputWhenPaused(bool bOutput) { m_bOutputWhenPaused = bOutput; }

protected:
	void EndStreamThread();
	static unsigned int __stdcall StreamThread(LPVOID lpParameter);

	CBonSrcFilter* m_pFilter;

	HANDLE m_hThread;
	HANDLE m_hEndEvent;
	CTsSrcStream m_SrcStream;

	bool m_bOutputWhenPaused;
};
