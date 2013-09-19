#pragma once

// EVRを利用しない場合はコメントアウトする
//#define USE_MEDIA_FOUNDATION

/*
	TVTestでの注意:
	上のコメントはMeru-co氏によるオリジナルにおけるものです。
	TVTestでは関係ありません。
*/

#include <vector>
#include <d3d9.h>
#include <vmr9.h>
#ifdef USE_MEDIA_FOUNDATION
#include <mfapi.h>
#include <mfidl.h>
#include <mferror.h>
#include <evr9.h>
#endif


// guidに入っていない追加分GUID
#ifndef WAVE_FORMAT_AAC
#define WAVE_FORMAT_AAC 0x00FF
#endif

// AAC Mediatype {000000FF-0000-0010-8000-00AA00389B71}
#ifndef MEDIASUBTYPE_AAC
DEFINE_GUID(MEDIASUBTYPE_AAC,WAVE_FORMAT_AAC, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71);
#endif

// ColorSpace Converter {1643E180-90F5-11CE-97D5-00AA0055595A}
#ifndef CLSID_ColorSpaceConverter
DEFINE_GUID(CLSID_ColorSpaceConverter, 0x1643E180, 0x90F5, 0x11CE, 0x97, 0xD5, 0x00, 0xAA, 0x00, 0x55, 0x59, 0x5A);
#endif

#ifdef USE_MEDIA_FOUNDATION
// IMFGetService FA993888-4383-415A-A930-DD472A8CF6F7}
#ifndef IID_IMFGetService
DEFINE_GUID(IID_IMFGetService , 0xFA993888, 0x4383, 0x415A, 0xA9, 0x30, 0xDD, 0x47, 0x2A, 0x8C, 0xF6, 0xF7);
#endif
#endif

#ifdef TVH264
//DEFINE_GUID(MEDIASUBTYPE_H264,
//0x34363248, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71);

DEFINE_GUID(MEDIASUBTYPE_h264,
0x34363268, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71);

DEFINE_GUID(MEDIASUBTYPE_H264_bis,
0x8D2D71CB, 0x243F, 0x45E3, 0xB2, 0xD8, 0x5F, 0xD7, 0x96, 0x7E, 0xC0, 0x9B);

DEFINE_GUID(MEDIASUBTYPE_AVC1,
0x31435641, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71);

DEFINE_GUID(MEDIASUBTYPE_avc1,
0x31637661, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71);
#endif

// Release
#ifndef SAFE_RELEASE
#define SAFE_RELEASE(pOblect)			if((pOblect)) {(pOblect)->Release(); (pOblect) = NULL;}
#endif
#ifndef SAFE_RELEASE_EX
#define SAFE_RELEASE_EX(pOblect,nRef)	{nRef=0;if((pOblect)) {nRef=(pOblect)->Release(); (pOblect) = NULL;}}
#endif

class CDirectShowFilterFinder
{
public:
	CDirectShowFilterFinder();
	~CDirectShowFilterFinder();

	void Clear();
	bool FindFilter(const GUID *pInTypes,int InTypeCount,
					const GUID *pOutTypes=NULL,int OutTypeCount=0,
					DWORD Merit=MERIT_DO_NOT_USE+1);
	bool FindFilter(const GUID *pidInType,const GUID *pidInSubType,
					const GUID *pidOutType=NULL,const GUID *pidOutubType=NULL,
					DWORD Merit=MERIT_DO_NOT_USE+1);
	bool PriorityFilterGoToHead(const CLSID idPriorityClass);
	bool IgnoreFilterGoToTail(const CLSID idIgnoreClass,bool bRemoveIt=false);
	int GetFilterCount();
	bool GetFilterInfo(const int iIndex,CLSID *pidClass=NULL,LPWSTR pwszFriendlyName=NULL,int iBufLen=0);
protected:
	class CFilterInfo {
	public:
		LPWSTR m_pwszFriendlyName;
		CLSID m_clsid;
		CFilterInfo();
		CFilterInfo(const CFilterInfo &Info);
		~CFilterInfo();
		CFilterInfo &operator=(const CFilterInfo &Info);
		void SetFriendlyName(LPCWSTR pwszFriendlyName);
	};
	std::vector<CFilterInfo> m_FilterList;
};

class CDirectShowDeviceEnumerator
{
	class CDeviceInfo {
		LPWSTR m_pszFriendlyName;
	public:
		CDeviceInfo(LPCWSTR pszFriendlyName);
		CDeviceInfo(const CDeviceInfo &Info);
		~CDeviceInfo();
		CDeviceInfo &operator=(const CDeviceInfo &Info);
		LPCWSTR GetFriendlyName() const { return m_pszFriendlyName; }
	};
	std::vector<CDeviceInfo> m_DeviceList;
public:
	CDirectShowDeviceEnumerator();
	~CDirectShowDeviceEnumerator();
	void Clear();
	bool EnumDevice(REFCLSID clsidDeviceClass);
	bool CreateFilter(REFCLSID clsidDeviceClass,LPCWSTR pszFriendlyName,IBaseFilter **ppFilter);
	int GetDeviceCount() const;
	LPCWSTR GetDeviceFriendlyName(int Index) const;
};

namespace DirectShowUtil {

// 以下、便利関数
// デバッグ用(実行テーブルに追加・削除)
// ・RemoveFromRot()時にはGrapheditを閉じておく必要があります。
// ・WindowsVistaでこの機能を使うには、Windows SDK for Vista の C:\Program Files\Microsoft SDKs\Windows\v6.0\Bin にある
//   proppage.dll を regsvr32 で登録しておく必要があります。
HRESULT AddToRot(IUnknown *pUnkGraph, DWORD *pdwRegister);
void RemoveFromRot(const DWORD dwRegister);

// 構築用ユーティリティ
IPin* GetFilterPin(IBaseFilter *pFilter, const PIN_DIRECTION dir, const AM_MEDIA_TYPE *pMediaType=NULL);
bool ShowPropertyPage(IBaseFilter *pFilter, HWND hWndParent);
bool HasPropertyPage(IBaseFilter *pFilter);
//bool AppendMpeg2Decoder_and_Connect(IGraphBuilder *pFilterGraph, CDirectShowUtil *pUtil, IBaseFilter **ppMpeg2DecoderFilter,wchar_t *lpszDecoderName,int iDecNameBufLen, IPin **ppCurrentOutputPin, IPin **ppNewOutputPin=NULL);
HRESULT AppendFilterAndConnect(IGraphBuilder *pFilterGraph,
	IBaseFilter *pFilter,LPCWSTR lpwszFilterName,
	IPin **ppCurrentOutputPin,IPin **ppNewOutputPin=NULL,bool fDirect=false);
HRESULT AppendFilterAndConnect(IGraphBuilder *pFilterGraph,
	const CLSID guidFilter,LPCWSTR lpwszFilterName,IBaseFilter **ppAppendedFilter,
	IPin **ppCurrentOutputPin, IPin **ppNewOutputPin=NULL, bool fDirect=false);
HRESULT AppendColorSpaceConverterFilter_and_Connect(IGraphBuilder *pFilterGraph, IBaseFilter **ppColorSpaceConverterFilter, IPin **ppCurrentOutputPin, IPin **ppNewOutputPin=NULL);

// 汎用ユーティリティ
IVideoWindow* GetVideoWindow(IGraphBuilder *pGraph);
IBasicVideo2* GetBasicVideo2(IGraphBuilder *pGraph);
IMediaControl* GetMediaControl(IGraphBuilder *pGraph);
bool FilterGrapph_Play(IGraphBuilder *pFilterGraph);
bool FilterGrapph_Stop(IGraphBuilder *pFilterGraph);
bool FilterGrapph_Pause(IGraphBuilder *pFilterGraph);

// フィルタ参照カウンタ取得
inline LONG GetRefCount(IUnknown *pUkn)
{
	if (!pUkn) {
		return 0;
	} else {
		pUkn->AddRef();
		LONG ret = pUkn->Release();
		return ret;
	}
};

#ifdef DEBUG
#define CHECK_RELEASE(pObj)									\
	if (pObj) {												\
		LONG RefCount=DirectShowUtil::GetRefCount(pObj);	\
		if (RefCount!=1)									\
			TRACE(TEXT("%s %d : RefCount = %d\n"),TEXT(__FILE__),__LINE__,RefCount);	\
		pObj->Release();									\
		pObj=NULL;											\
	}
#else
#define CHECK_RELEASE SAFE_RELEASE
#endif

//////////////////////////////////////////////////////////////////////
// 以下 EVR専用ユーティリティ
#ifdef USE_MEDIA_FOUNDATION
void						MF_Init();
void						MF_Term();
IEVRFilterConfig*		MF_GetEVRFilterConfig(IBaseFilter *pEvr);
IMFGetService*			MF_GetService(IBaseFilter *pEvr);
bool						MF_SetNumberOfStreams(IBaseFilter *pEvr,int iStreamNumber=1);
IMFVideoDisplayControl*	MF_GetVideoDisplayControl(IBaseFilter *pEvr);
IMFVideoMixerControl*	MF_GetVideoMixerControl(IBaseFilter *pEvr);
IMFVideoProcessor*		MF_GetVideoProcessor(IBaseFilter *pEvr);
#endif

}	// namespace DirectShowUtil
