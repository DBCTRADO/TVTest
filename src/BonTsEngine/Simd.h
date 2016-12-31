#pragma once


#include "MediaData.h"
#include <malloc.h>
#ifdef BONTSENGINE_SSE
#include <xmmintrin.h>
#endif
#ifdef BONTSENGINE_SSE2
#include <emmintrin.h>
#endif


template<size_t Align> class CAlignedMediaData : public CMediaData
{
public:
	~CAlignedMediaData()
	{
		ClearBuffer();
	}

protected:
	void *Allocate(size_t Size) override
	{
		return ::_aligned_malloc(Size, Align);
	}

	void Free(void *pBuffer) override
	{
		::_aligned_free(pBuffer);
	}

	void *ReAllocate(void *pBuffer, size_t Size) override
	{
		return ::_aligned_realloc(pBuffer, Size, Align);
	}
};

typedef CAlignedMediaData<16> CSimdMediaData;


namespace TsEngine
{

#if defined(_M_AMD64)
	inline bool IsSSE2Avaliable() { return true; }
#else
	bool IsSSE2Available();
#endif
	bool IsSSE2Enabled();
	void SetSSE2Enabled(bool bEnabled);

}
