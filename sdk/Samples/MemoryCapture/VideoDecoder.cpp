#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <windows.h>
#include <shlwapi.h>
#include <initguid.h>
#include <dshow.h>
#include "VideoDecoder.h"


CVideoDecoder::~CVideoDecoder()
{
	Finalize();
}


bool CVideoDecoder::Initialize()
{
	if (m_hLib == nullptr) {
		TCHAR szPath[MAX_PATH];

		// まず実行ファイルと同じフォルダ内の TVTestVideoDecoder.ax を探す
		DWORD Length = ::GetModuleFileName(nullptr, szPath, _countof(szPath));
		if (Length > 0 && Length < _countof(szPath)) {
			::PathRemoveFileSpec(szPath);
			if (::PathAppend(szPath, TEXT("TVTestVideoDecoder.ax")))
				m_hLib = ::LoadLibrary(szPath);
		}

		if (m_hLib == nullptr) {
			// インストールされているモジュールを探す
			HKEY hKey;
			if (::RegOpenKeyEx(
					HKEY_CLASSES_ROOT,
					TEXT("CLSID\\{AE0BF9FF-EBCE-4412-9EFC-C6EE86B20855}\\InprocServer32"),
					0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS) {
				DWORD Type, Size = sizeof(szPath);
				::ZeroMemory(szPath, sizeof(szPath));
				if (::RegQueryValueEx(
							hKey, nullptr, nullptr, &Type,
							reinterpret_cast<LPBYTE>(szPath), &Size) == ERROR_SUCCESS
						&& Type == REG_SZ)
					m_hLib = ::LoadLibrary(szPath);
				::RegCloseKey(hKey);
			}
			if (m_hLib == nullptr)
				return false;
		}
	}

	if (m_pCreateInstance == nullptr) {
		auto pGetInfo =
			reinterpret_cast<decltype(TVTestVideoDecoder_GetInfo)*>(
				::GetProcAddress(m_hLib, "TVTestVideoDecoder_GetInfo"));
		if (pGetInfo == nullptr)
			return false;

		// バージョンチェック
		TVTestVideoDecoderInfo Info;
		Info.HostVersion = TVTVIDEODEC_HOST_VERSION;
		if (!pGetInfo(&Info)
				|| Info.InterfaceVersion != TVTVIDEODEC_INTERFACE_VERSION)
			return false;

		m_pCreateInstance =
			reinterpret_cast<decltype(TVTestVideoDecoder_CreateInstance)*>(
				::GetProcAddress(m_hLib, "TVTestVideoDecoder_CreateInstance"));
		if (m_pCreateInstance == nullptr)
			return false;
	}

	return true;
}


void CVideoDecoder::Finalize()
{
	Close();

	if (m_hLib != nullptr) {
		::FreeLibrary(m_hLib);
		m_hLib = nullptr;
		m_pCreateInstance = nullptr;
	}
}


bool CVideoDecoder::Open(DWORD Format)
{
	Close();

	if (m_pCreateInstance == nullptr)
		return false;

	// 現在 MPEG-2 Video のみ対応
	if (Format != MAKEFOURCC('m', 'p', '2', 'v'))
		return false;

	ITVTestVideoFrameDecoder *pDecoder;
	HRESULT hr = m_pCreateInstance(IID_PPV_ARGS(&pDecoder));
	if (FAILED(hr))
		return false;

	pDecoder->SetWaitForKeyFrame(TRUE);
	pDecoder->SetDeinterlaceMethod((TVTVIDEODEC_DeinterlaceMethod)m_Deinterlace);
	pDecoder->SetFrameCapture(this, MEDIASUBTYPE_RGB24);
	pDecoder->Open(MEDIASUBTYPE_MPEG2_VIDEO);

	m_pDecoder = pDecoder;

	return true;
}


void CVideoDecoder::Close()
{
	if (m_pDecoder != nullptr) {
		m_pDecoder->Release();
		m_pDecoder = nullptr;
	}
}


void CVideoDecoder::SetFrameCapture(CFrameCapture *pFrameCapture)
{
	m_pFrameCapture = pFrameCapture;
}


bool CVideoDecoder::InputStream(const void *pData, SIZE_T Size)
{
	if (m_pDecoder == nullptr)
		return false;

	return m_pDecoder->InputStream(pData, Size) == S_OK;
}


void CVideoDecoder::SetDeinterlaceMethod(DeinterlaceMethod Deinterlace)
{
	m_Deinterlace = Deinterlace;

	if (m_pDecoder != nullptr)
		m_pDecoder->SetDeinterlaceMethod((TVTVIDEODEC_DeinterlaceMethod)Deinterlace);
}


STDMETHODIMP_(ULONG) CVideoDecoder::AddRef()
{
	return ::InterlockedIncrement(&m_RefCount); // ダミー
}


STDMETHODIMP_(ULONG) CVideoDecoder::Release()
{
	return ::InterlockedDecrement(&m_RefCount); // ダミー
}


STDMETHODIMP CVideoDecoder::QueryInterface(REFIID riid, void **ppvObject)
{
	if (ppvObject == nullptr)
		return E_POINTER;

	if (riid == IID_IUnknown) {
		*ppvObject = static_cast<IUnknown*>(this);
	} else if (riid == __uuidof(ITVTestVideoDecoderFrameCapture)) {
		*ppvObject = static_cast<ITVTestVideoDecoderFrameCapture*>(this);
	} else {
		*ppvObject = nullptr;
		return E_NOINTERFACE;
	}

	AddRef();

	return S_OK;
}


// フレームの取得
STDMETHODIMP CVideoDecoder::OnFrame(const TVTVIDEODEC_FrameInfo *pFrameInfo)
{
	if (m_pFrameCapture != nullptr) {
		FrameInfo Info;

		Info.Width = pFrameInfo->Width;
		Info.Height = pFrameInfo->Height;
		Info.BitsPerPixel = 24;
		Info.AspectRatioX = pFrameInfo->AspectX;
		Info.AspectRatioY = pFrameInfo->AspectY;
		Info.Flags = pFrameInfo->Flags;
		Info.Buffer = pFrameInfo->Buffer[0];
		Info.Pitch = pFrameInfo->Pitch[0];

		return m_pFrameCapture->OnFrame(Info) ? S_OK : E_ABORT;
	}

	return S_OK;
}
