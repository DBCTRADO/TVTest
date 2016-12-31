#include "stdafx.h"
#include "EVRCustomPresenter.h"
#include "EVRPresenterBase.h"
#include "EVRPresenter.h"
#include "../DirectShowUtil.h"
#include "../../Common/DebugDef.h"


CVideoRenderer_EVRCustomPresenter::CVideoRenderer_EVRCustomPresenter()
	: m_pPresenter(nullptr)
{
}


CVideoRenderer_EVRCustomPresenter::~CVideoRenderer_EVRCustomPresenter()
{
	SAFE_RELEASE(m_pPresenter);
}


bool CVideoRenderer_EVRCustomPresenter::Finalize()
{
	SAFE_RELEASE(m_pPresenter);
	return CVideoRenderer_EVR::Finalize();
}


HRESULT CVideoRenderer_EVRCustomPresenter::InitializePresenter(IBaseFilter *pFilter)
{
	HRESULT hr;
	IMFVideoRenderer *pRenderer;

	hr = pFilter->QueryInterface(IID_PPV_ARGS(&pRenderer));

	if (SUCCEEDED(hr)) {
		IMFVideoPresenter *pPresenter;

		hr = CEVRPresenter::CreateInstance(nullptr, IID_PPV_ARGS(&pPresenter));
		if (SUCCEEDED(hr)) {
			hr = pRenderer->InitializeRenderer(nullptr, pPresenter);
			if (SUCCEEDED(hr)) {
				m_pPresenter = pPresenter;
			} else {
				pPresenter->Release();
			}
		}

		pRenderer->Release();
	}

	return hr;
}
