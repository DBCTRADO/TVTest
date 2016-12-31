#include "StdAfx.h"
#include <initguid.h>
#include "BonSrcFilter.h"
#include "BonSrcPin.h"
#include "../Common/DebugDef.h"




CBonSrcFilter::CBonSrcFilter(LPUNKNOWN pUnk, HRESULT *phr)
	: CBaseFilter(TEXT("Bon Source Filter"), pUnk, &m_cStateLock, CLSID_BonSrcFilter)
	, m_pSrcPin(NULL)
	, m_bOutputWhenPaused(false)
{
	TRACE(TEXT("CBonSrcFilter::CBonSrcFilter %p\n"), this);

	// �s���̃C���X�^���X����
	m_pSrcPin = new CBonSrcPin(phr, this);

	//*phr = (m_pSrcPin)? S_OK : E_OUTOFMEMORY;
	*phr=S_OK;
}


CBonSrcFilter::~CBonSrcFilter()
{
	// �s���̃C���X�^���X���폜����
	if(m_pSrcPin)delete m_pSrcPin;
}


IBaseFilter* WINAPI CBonSrcFilter::CreateInstance(LPUNKNOWN pUnk, HRESULT *phr)
{
	// �C���X�^���X���쐬����
	CBonSrcFilter *pNewFilter = new CBonSrcFilter(pUnk, phr);
	if (FAILED(*phr)) {
		delete pNewFilter;
		return NULL;
	}

	IBaseFilter *pFilter;
	*phr = pNewFilter->QueryInterface(IID_IBaseFilter, pointer_cast<void**>(&pFilter));
	if (FAILED(*phr)) {
		delete pNewFilter;
		return NULL;
	}

	return pFilter;
}


int CBonSrcFilter::GetPinCount()
{
	// �s������Ԃ�
	return 1;
}


CBasePin * CBonSrcFilter::GetPin(int n)
{
	// �s���̃C���X�^���X��Ԃ�
	return (n == 0)? m_pSrcPin : NULL;
}


#ifdef _DEBUG

STDMETHODIMP CBonSrcFilter::Run(REFERENCE_TIME tStart)
{
	TRACE(L"��CBonSrcFilter::Run()\n");

	return CBaseFilter::Run(tStart);
}


STDMETHODIMP CBonSrcFilter::Pause()
{
	TRACE(L"��CBonSrcFilter::Pause()\n");

	return CBaseFilter::Pause();
}


STDMETHODIMP CBonSrcFilter::Stop()
{
	TRACE(L"��CBonSrcFilter::Stop()\n");

	return CBaseFilter::Stop();
}

#endif


STDMETHODIMP CBonSrcFilter::GetState(DWORD dw, FILTER_STATE *pState)
{
	*pState = m_State;
	if (m_State==State_Paused && !m_bOutputWhenPaused)
		return VFW_S_CANT_CUE;
	return S_OK;
}


bool CBonSrcFilter::InputMedia(CMediaData *pMediaData)
{
	{
		CAutoLock Lock(&m_cStateLock);

		if (!m_pSrcPin
				|| m_State==State_Stopped
				|| (m_State==State_Paused && !m_bOutputWhenPaused)) {
			return false;
		}
	}

	return m_pSrcPin->InputMedia(pMediaData);
}


void CBonSrcFilter::Reset()
{
	if (m_pSrcPin)
		m_pSrcPin->Reset();
}


void CBonSrcFilter::Flush()
{
	CAutoLock Lock(m_pLock);

	if (m_pSrcPin)
		m_pSrcPin->Flush();
}


bool CBonSrcFilter::EnableSync(bool bEnable,bool b1Seg)
{
	if (m_pSrcPin)
		return m_pSrcPin->EnableSync(bEnable,b1Seg);
	return false;
}


bool CBonSrcFilter::IsSyncEnabled() const
{
	if (m_pSrcPin)
		return m_pSrcPin->IsSyncEnabled();
	return false;
}


void CBonSrcFilter::SetVideoPID(WORD PID)
{
	if (m_pSrcPin)
		return m_pSrcPin->SetVideoPID(PID);
}


void CBonSrcFilter::SetAudioPID(WORD PID)
{
	if (m_pSrcPin)
		return m_pSrcPin->SetAudioPID(PID);
}


void CBonSrcFilter::SetOutputWhenPaused(bool bOutput)
{
	m_bOutputWhenPaused=bOutput;
	if (m_pSrcPin)
		m_pSrcPin->SetOutputWhenPaused(bOutput);
}


bool CBonSrcFilter::SetBufferSize(size_t Size)
{
	if (m_pSrcPin)
		return m_pSrcPin->SetBufferSize(Size);
	return false;
}


bool CBonSrcFilter::SetInitialPoolPercentage(int Percentage)
{
	if (m_pSrcPin)
		return m_pSrcPin->SetInitialPoolPercentage(Percentage);
	return false;
}


int CBonSrcFilter::GetBufferFillPercentage() const
{
	if (m_pSrcPin)
		return m_pSrcPin->GetBufferFillPercentage();
	return 0;
}


bool CBonSrcFilter::SetInputWait(DWORD Wait)
{
	if (m_pSrcPin)
		return m_pSrcPin->SetInputWait(Wait);
	return false;
}


bool CBonSrcFilter::MapAudioPID(WORD AudioPID, WORD MapPID)
{
	if (m_pSrcPin)
		return m_pSrcPin->MapAudioPID(AudioPID, MapPID);
	return false;
}
