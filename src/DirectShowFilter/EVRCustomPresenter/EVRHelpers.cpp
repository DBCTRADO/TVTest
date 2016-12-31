#include "stdafx.h"
#include "EVRPresenterBase.h"
#include "EVRHelpers.h"
#include "../../Common/DebugDef.h"


CSamplePool::CSamplePool()
	: m_bInitialized(false)
	, m_PendingCount(0)
{
}


CSamplePool::~CSamplePool()
{
}


HRESULT CSamplePool::GetSample(IMFSample **ppSample)
{
	*ppSample = nullptr;

	CAutoLock Lock(&m_Lock);

	if (!m_bInitialized) {
		return MF_E_NOT_INITIALIZED;
	}

	if (m_VideoSampleQueue.IsEmpty()) {
		return MF_E_SAMPLEALLOCATOR_EMPTY;
	}

	IMFSample *pSample = nullptr;

	HRESULT hr = m_VideoSampleQueue.RemoveFront(&pSample);

	if (SUCCEEDED(hr)) {
		m_PendingCount++;

		*ppSample = pSample;
	}

	return hr;
}


HRESULT CSamplePool::ReturnSample(IMFSample *pSample)
{
	CAutoLock Lock(&m_Lock);

	_ASSERT(m_PendingCount > 0);

	if (!m_bInitialized) {
		return MF_E_NOT_INITIALIZED;
	}

	HRESULT hr = m_VideoSampleQueue.InsertBack(pSample);

	if (SUCCEEDED(hr)) {
		m_PendingCount--;
	}

	return hr;
}


bool CSamplePool::AreSamplesPending()
{
	CAutoLock Lock(&m_Lock);

	if (!m_bInitialized) {
		return false;
	}

	return m_PendingCount > 0;
}


HRESULT CSamplePool::Initialize(VideoSampleList &Samples)
{
	CAutoLock Lock(&m_Lock);

	if (m_bInitialized) {
		return MF_E_INVALIDREQUEST;
	}

	HRESULT hr = S_OK;

	for (VideoSampleList::Position Pos = Samples.FrontPosition();
			Pos != Samples.EndPosition();
			Pos = Samples.Next(Pos)) {
		IMFSample *pSample = nullptr;

		hr = Samples.GetItemPos(Pos, &pSample);
		if (SUCCEEDED(hr)) {
			hr = m_VideoSampleQueue.InsertBack(pSample);
			pSample->Release();
		}

		if (FAILED(hr)) {
			break;
		}
	}

	if (SUCCEEDED(hr)) {
		m_bInitialized = true;
	}

	Samples.Clear();

	return hr;
}


HRESULT CSamplePool::Clear()
{
	CAutoLock Lock(&m_Lock);

	m_VideoSampleQueue.Clear();
	m_bInitialized = false;
	m_PendingCount = 0;

	return S_OK;
}
