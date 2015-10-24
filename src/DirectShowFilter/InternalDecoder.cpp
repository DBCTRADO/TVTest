#include "stdafx.h"
#include "InternalDecoder.h"
#include "../Common/DebugDef.h"


CInternalDecoderManager::CInternalDecoderManager()
	: m_hLib(nullptr)
{
}


CInternalDecoderManager::~CInternalDecoderManager()
{
	FreeDecoderModule();
}


HRESULT CInternalDecoderManager::CreateInstance(const GUID &MediaSubType, IBaseFilter **ppFilter)
{
	*ppFilter = nullptr;

	if (!IsMediaSupported(MediaSubType))
		return E_INVALIDARG;

	HRESULT hr;

	hr = LoadDecoderModule();
	if (FAILED(hr))
		return hr;

	auto pGetInfo = reinterpret_cast<decltype(TVTestVideoDecoder_GetInfo)*>(
		::GetProcAddress(m_hLib, "TVTestVideoDecoder_GetInfo"));
	auto pCreateInstance = reinterpret_cast<decltype(TVTestVideoDecoder_CreateInstance)*>(
		::GetProcAddress(m_hLib, "TVTestVideoDecoder_CreateInstance"));
	if (!pGetInfo || !pCreateInstance)
		return E_FAIL;

	TVTestVideoDecoderInfo DecoderInfo;
	DecoderInfo.HostVersion = TVTVIDEODEC_HOST_VERSION;
	if (!pGetInfo(&DecoderInfo)
			|| DecoderInfo.InterfaceVersion != TVTVIDEODEC_INTERFACE_VERSION)
		return E_FAIL;

	ITVTestVideoDecoder *pDecoder;

	hr = pCreateInstance(&pDecoder);
	if (FAILED(hr))
		return hr;
	hr = pDecoder->QueryInterface(IID_PPV_ARGS(ppFilter));
	if (FAILED(hr)) {
		pDecoder->Release();
		return hr;
	}

	pDecoder->SetEnableDeinterlace(m_VideoDecoderSettings.bEnableDeinterlace);
	pDecoder->SetDeinterlaceMethod(m_VideoDecoderSettings.DeinterlaceMethod);
	pDecoder->SetAdaptProgressive(m_VideoDecoderSettings.bAdaptProgressive);
	pDecoder->SetAdaptTelecine(m_VideoDecoderSettings.bAdaptTelecine);
	pDecoder->SetInterlacedFlag(m_VideoDecoderSettings.bSetInterlacedFlag);
	pDecoder->SetBrightness(m_VideoDecoderSettings.Brightness);
	pDecoder->SetContrast(m_VideoDecoderSettings.Contrast);
	pDecoder->SetHue(m_VideoDecoderSettings.Hue);
	pDecoder->SetSaturation(m_VideoDecoderSettings.Saturation);
	pDecoder->SetNumThreads(m_VideoDecoderSettings.NumThreads);
	pDecoder->SetEnableDXVA2(m_VideoDecoderSettings.bEnableDXVA2);

	pDecoder->Release();

	return S_OK;
}


void CInternalDecoderManager::SetVideoDecoderSettings(const VideoDecoderSettings &Settings)
{
	m_VideoDecoderSettings = Settings;
}


bool CInternalDecoderManager::GetVideoDecoderSettings(VideoDecoderSettings *pSettings) const
{
	if (!pSettings)
		return false;
	*pSettings = m_VideoDecoderSettings;
	return true;
}


bool CInternalDecoderManager::SaveVideoDecoderSettings(IBaseFilter *pFilter)
{
	if (!pFilter)
		return false;

	ITVTestVideoDecoder *pDecoder;
	HRESULT hr = pFilter->QueryInterface(IID_PPV_ARGS(&pDecoder));
	if (FAILED(hr))
		return false;

	m_VideoDecoderSettings.bEnableDeinterlace = pDecoder->GetEnableDeinterlace() != FALSE;
	m_VideoDecoderSettings.DeinterlaceMethod = pDecoder->GetDeinterlaceMethod();
	m_VideoDecoderSettings.bAdaptProgressive = pDecoder->GetAdaptProgressive() != FALSE;
	m_VideoDecoderSettings.bAdaptTelecine = pDecoder->GetAdaptTelecine() != FALSE;
	m_VideoDecoderSettings.bSetInterlacedFlag = pDecoder->GetInterlacedFlag() != FALSE;
	m_VideoDecoderSettings.Brightness = pDecoder->GetBrightness();
	m_VideoDecoderSettings.Contrast = pDecoder->GetContrast();
	m_VideoDecoderSettings.Hue = pDecoder->GetHue();
	m_VideoDecoderSettings.Saturation = pDecoder->GetSaturation();
	m_VideoDecoderSettings.NumThreads = pDecoder->GetNumThreads();
	m_VideoDecoderSettings.bEnableDXVA2 = pDecoder->GetEnableDXVA2() != FALSE;

	pDecoder->Release();

	return true;
}


bool CInternalDecoderManager::IsMediaSupported(const GUID &MediaSubType)
{
	return (MediaSubType == MEDIASUBTYPE_MPEG2_VIDEO) != FALSE;
}


bool CInternalDecoderManager::IsDecoderAvailable(const GUID &MediaSubType)
{
	if (!IsMediaSupported(MediaSubType))
		return false;

	TCHAR szPath[MAX_PATH];

	if (!GetModulePath(szPath))
		return false;

	return ::PathFileExists(szPath) != FALSE;
}


LPCWSTR CInternalDecoderManager::GetDecoderName(const GUID &MediaSubType)
{
	if (MediaSubType == MEDIASUBTYPE_MPEG2_VIDEO)
		return TVTVIDEODEC_FILTER_NAME;
	return nullptr;
}


HRESULT CInternalDecoderManager::LoadDecoderModule()
{
	if (!m_hLib) {
		TCHAR szPath[MAX_PATH];

		GetModulePath(szPath);
		m_hLib = ::LoadLibrary(szPath);
		if (!m_hLib)
			return HRESULT_FROM_WIN32(::GetLastError());
	}

	return S_OK;
}


void CInternalDecoderManager::FreeDecoderModule()
{
	if (m_hLib) {
		::FreeLibrary(m_hLib);
		m_hLib = nullptr;
	}
}


bool CInternalDecoderManager::GetModulePath(LPTSTR pszPath)
{
	::GetModuleFileName(nullptr, pszPath, MAX_PATH);
	::PathRemoveFileSpec(pszPath);
	::PathAppend(pszPath, TEXT("TVTestVideoDecoder.ax"));
	return true;
}
