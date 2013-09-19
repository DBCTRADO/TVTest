#include "stdafx.h"
#include <initguid.h>
#include "DirectShowUtil.h"
#include "../HelperClass/StdUtil.h"

#ifdef USE_MEDIA_FOUNDATION
#pragma comment(lib,"mf.lib")
#pragma comment(lib,"mfplat.lib")
#pragma comment(lib,"mfuuid.lib")
#endif

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif




CDirectShowFilterFinder::CDirectShowFilterFinder()
{
	//::CoInitialize(NULL);
}

CDirectShowFilterFinder::~CDirectShowFilterFinder()
{
	//::CoUninitialize();
}

void CDirectShowFilterFinder::Clear()
{
	m_FilterList.clear();
}

int CDirectShowFilterFinder::GetFilterCount()
{
	return (int)m_FilterList.size();
}

bool CDirectShowFilterFinder::GetFilterInfo(const int iIndex,CLSID *pidClass,LPWSTR pwszFriendlyName,int iBufLen)
{
	if (iIndex<0 || iIndex>=GetFilterCount())
		return false;
	CFilterInfo &Info = m_FilterList[iIndex];
	if (pidClass)
		*pidClass = Info.m_clsid;
	if (pwszFriendlyName) {
		if (Info.m_pwszFriendlyName) {
			::lstrcpynW(pwszFriendlyName,Info.m_pwszFriendlyName,iBufLen);
		} else if (iBufLen>0) {
			pwszFriendlyName[0]='\0';
		}
	}
	return true;
}

bool CDirectShowFilterFinder::FindFilter(const GUID *pInTypes,int InTypeCount,
	const GUID *pOutTypes,int OutTypeCount,DWORD Merit)
{
	// フィルタを検索する
	bool bRet = false;
	IFilterMapper2 *pMapper=NULL;
	HRESULT hr=::CoCreateInstance(CLSID_FilterMapper2, NULL, CLSCTX_INPROC, IID_IFilterMapper2, (void **)&pMapper);

	if (SUCCEEDED(hr)) {
		IEnumMoniker *pEnum=NULL;

		hr = pMapper->EnumMatchingFilters(
			&pEnum,
			0,					// 予約済み
			TRUE,				// 完全一致を使用するか
			Merit,				// 最小のメリット
			TRUE,				// 1 つ以上の入力ピンか?
			InTypeCount,		// 入力のメジャータイプ/サブタイプの対の数
			pInTypes,			// 入力のメジャータイプ/サブタイプの対の配列
			NULL,				// 入力メディア
			NULL,				// 入力ピンのカテゴリ
			FALSE,				// レンダラでなければならないか
			TRUE,				// 1 つ以上の出力ピンか
			OutTypeCount,		// 出力のメジャータイプ/サブタイプの対の数
			pOutTypes,			// 出力のメジャータイプ/サブタイプの対の配列
			NULL,				// 出力メディア
			NULL);				// 出力ピンのカテゴリ
		if (SUCCEEDED(hr)) {
			IMoniker *pMoniker;
			ULONG cFetched;
			while (pEnum->Next(1, &pMoniker, &cFetched) == S_OK) {
				IPropertyBag *pPropBag;
				hr=pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void **)&pPropBag);
				if (SUCCEEDED(hr)) {
					VARIANT varName,varID;
					::VariantInit(&varName);
					::VariantInit(&varID);
					hr=pPropBag->Read(L"FriendlyName", &varName, 0);
					if (SUCCEEDED(hr)) {
						hr=pPropBag->Read(L"CLSID", &varID, 0);
						if (SUCCEEDED(hr)) {
							bRet = true;
							CFilterInfo FilterInfo;
							FilterInfo.SetFriendlyName(varName.bstrVal);
							::CLSIDFromString(varID.bstrVal,&FilterInfo.m_clsid);
							m_FilterList.push_back(FilterInfo);
							::SysFreeString(varID.bstrVal);
						}
						SysFreeString(varName.bstrVal);
					}
					pPropBag->Release();
				}
				pMoniker->Release();
			}
			pEnum->Release();
		}
		pMapper->Release();
	}
	return bRet;
}

bool CDirectShowFilterFinder::FindFilter(const GUID *pidInType,const GUID *pidInSubType,const GUID *pidOutType,const GUID *pidOutSubType,DWORD Merit)
{
	GUID arInType[2],arOutType[2];
	GUID *pInTypes=NULL,*pOutTypes=NULL;

	if (pidInType || pidInSubType) {
		arInType[0] = pidInType ? *pidInType : GUID_NULL;
		arInType[1] = pidInSubType ? *pidInSubType : GUID_NULL;
		pInTypes = arInType;
	}
	if (pidOutType || pidOutSubType) {
		arOutType[0] = pidOutType ? *pidOutType : GUID_NULL;
		arOutType[1] = pidOutSubType ? *pidOutSubType : GUID_NULL;
		pOutTypes = arOutType;
	}

	return FindFilter(pInTypes, pInTypes ? 1 : 0,
					  pOutTypes, pOutTypes ? 1 : 0,
					  Merit);
}

// 優先するフィルタをリスト先端に持ってくる
bool CDirectShowFilterFinder::PriorityFilterGoToHead(const CLSID idPriorityClass)
{
	std::vector<CFilterInfo> TmpList;
	size_t i;

	for (i=0;i<m_FilterList.size();i++) {
		if (m_FilterList[i].m_clsid == idPriorityClass) {
			// 優先するものを発見
			TmpList.push_back(m_FilterList[i]);
		}
	}
	if (!TmpList.empty()) {
		for (i=0;i<m_FilterList.size();i++) {
			if(m_FilterList[i].m_clsid != idPriorityClass) {
				// 優先するもの以外
				TmpList.push_back(m_FilterList[i]);
			}
		}
	}
	m_FilterList=TmpList;
	return true;
}

// 無視するフィルタをリスト終端に持ってくる
bool CDirectShowFilterFinder::IgnoreFilterGoToTail(const CLSID idIgnoreClass,bool bRemoveIt)
{
	std::vector<CFilterInfo> TmpList;
	size_t i;

	for (i=0;i<m_FilterList.size();i++) {
		if (m_FilterList[i].m_clsid != idIgnoreClass) {
			// 無視するもの以外
			TmpList.push_back(m_FilterList[i]);
		}
	}
	if (!bRemoveIt) {
		for (i=0;i<m_FilterList.size();i++) {
			if (m_FilterList[i].m_clsid == idIgnoreClass) {
				TmpList.push_back(m_FilterList[i]);
			}
		}
	}
	m_FilterList=TmpList;
	return true;
}


CDirectShowFilterFinder::CFilterInfo::CFilterInfo()
	: m_pwszFriendlyName(NULL)
{
}

CDirectShowFilterFinder::CFilterInfo::CFilterInfo(const CFilterInfo &Info)
	: m_pwszFriendlyName(NULL)
{
	*this=Info;
}

CDirectShowFilterFinder::CFilterInfo::~CFilterInfo()
{
	delete m_pwszFriendlyName;
}

CDirectShowFilterFinder::CFilterInfo &CDirectShowFilterFinder::CFilterInfo::operator=(const CFilterInfo &Info)
{
	if (&Info!=this) {
		SetFriendlyName(Info.m_pwszFriendlyName);
		m_clsid=Info.m_clsid;
	}
	return *this;
}

void CDirectShowFilterFinder::CFilterInfo::SetFriendlyName(LPCWSTR pwszFriendlyName)
{
	if (m_pwszFriendlyName) {
		delete [] m_pwszFriendlyName;
		m_pwszFriendlyName=NULL;
	}
	if (pwszFriendlyName) {
		m_pwszFriendlyName=StdUtil::strdup(pwszFriendlyName);
	}
}




CDirectShowDeviceEnumerator::CDirectShowDeviceEnumerator()
{
}

CDirectShowDeviceEnumerator::~CDirectShowDeviceEnumerator()
{
}

void CDirectShowDeviceEnumerator::Clear()
{
	m_DeviceList.clear();
}

bool CDirectShowDeviceEnumerator::EnumDevice(REFCLSID clsidDeviceClass)
{
	HRESULT hr;
	ICreateDevEnum *pDevEnum;

	hr=::CoCreateInstance(CLSID_SystemDeviceEnum,NULL,CLSCTX_INPROC_SERVER,
						  IID_ICreateDevEnum,reinterpret_cast<void**>(&pDevEnum));
	if (FAILED(hr))
		return false;

	IEnumMoniker *pEnumCategory;
	hr=pDevEnum->CreateClassEnumerator(clsidDeviceClass,&pEnumCategory,0);
	if (hr==S_OK) {
		IMoniker *pMoniker;
		ULONG cFetched;

		while (pEnumCategory->Next(1,&pMoniker,&cFetched)==S_OK) {
			IPropertyBag *pPropBag;

			hr=pMoniker->BindToStorage(0,0,IID_IPropertyBag,reinterpret_cast<void**>(&pPropBag));
			if (SUCCEEDED(hr)) {
				VARIANT varName;

				::VariantInit(&varName);
				hr=pPropBag->Read(L"FriendlyName",&varName,0);
				if (SUCCEEDED(hr)) {
					m_DeviceList.push_back(CDeviceInfo(varName.bstrVal));
				}
				::VariantClear(&varName);
				pPropBag->Release();
			}
			pMoniker->Release();
		}
		pEnumCategory->Release();
	}
	pDevEnum->Release();
	return true;
}

bool CDirectShowDeviceEnumerator::CreateFilter(REFCLSID clsidDeviceClass,LPCWSTR pszFriendlyName,IBaseFilter **ppFilter)
{
	HRESULT hr;
	ICreateDevEnum *pDevEnum;

	hr=::CoCreateInstance(CLSID_SystemDeviceEnum,NULL,CLSCTX_INPROC_SERVER,
						  IID_ICreateDevEnum,reinterpret_cast<void**>(&pDevEnum));
	if (FAILED(hr))
		return false;

	IEnumMoniker *pEnumCategory;
	hr=pDevEnum->CreateClassEnumerator(clsidDeviceClass,&pEnumCategory,0);
	bool bFound=false;
	if (hr==S_OK) {
		IMoniker *pMoniker;
		ULONG cFetched;

		while (pEnumCategory->Next(1,&pMoniker,&cFetched)==S_OK) {
			IPropertyBag *pPropBag;

			hr=pMoniker->BindToStorage(0,0,IID_IPropertyBag,reinterpret_cast<void**>(&pPropBag));
			if (SUCCEEDED(hr)) {
				VARIANT varName;

				::VariantInit(&varName);
				hr=pPropBag->Read(L"FriendlyName",&varName,0);
				if (SUCCEEDED(hr)) {
					if (::lstrcmpiW(varName.bstrVal,pszFriendlyName)==0) {
						hr=pMoniker->BindToObject(NULL,NULL,IID_IBaseFilter,
										reinterpret_cast<void**>(ppFilter));
						bFound=true;
					}
				}
				::VariantClear(&varName);
				pPropBag->Release();
			}
			pMoniker->Release();
			if (bFound)
				break;
		}
		pEnumCategory->Release();
	}
	pDevEnum->Release();

	if (!bFound)
		return false;
	return SUCCEEDED(hr);
}

int CDirectShowDeviceEnumerator::GetDeviceCount() const
{
	return (int)m_DeviceList.size();
}

LPCWSTR CDirectShowDeviceEnumerator::GetDeviceFriendlyName(int Index) const
{
	if (Index<0 || Index>=(int)m_DeviceList.size())
		return false;
	return m_DeviceList[Index].GetFriendlyName();
}


CDirectShowDeviceEnumerator::CDeviceInfo::CDeviceInfo(LPCWSTR pszFriendlyName)
{
	m_pszFriendlyName=StdUtil::strdup(pszFriendlyName);
}

CDirectShowDeviceEnumerator::CDeviceInfo::CDeviceInfo(const CDeviceInfo &Info)
{
	m_pszFriendlyName=StdUtil::strdup(Info.m_pszFriendlyName);
}

CDirectShowDeviceEnumerator::CDeviceInfo::~CDeviceInfo()
{
	delete [] m_pszFriendlyName;
}

CDirectShowDeviceEnumerator::CDeviceInfo &CDirectShowDeviceEnumerator::CDeviceInfo::operator=(const CDeviceInfo &Info)
{
	if (&Info!=this) {
		if (m_pszFriendlyName) {
			delete [] m_pszFriendlyName;
			m_pszFriendlyName=NULL;
		}
		if (Info.m_pszFriendlyName)
			m_pszFriendlyName=StdUtil::strdup(Info.m_pszFriendlyName);
	}
	return *this;
}




HRESULT DirectShowUtil::AddToRot(IUnknown *pUnkGraph, DWORD *pdwRegister)
{
	IMoniker * pMoniker;
	IRunningObjectTable *pROT;

	if (FAILED(::GetRunningObjectTable(0, &pROT)))
		return E_FAIL;

	wchar_t wsz[256];
	swprintf_s(wsz,256, L"FilterGraph %08p pid %08x", (DWORD_PTR)pUnkGraph, ::GetCurrentProcessId());

	HRESULT hr = ::CreateItemMoniker(L"!", wsz, &pMoniker);

	if (SUCCEEDED(hr)) {
		hr = pROT->Register(0, pUnkGraph, pMoniker, pdwRegister);
		pMoniker->Release();
	}
	pROT->Release();
	return hr;
}


void DirectShowUtil::RemoveFromRot(const DWORD dwRegister)
{
	IRunningObjectTable *pROT;

	if (SUCCEEDED(::GetRunningObjectTable(0, &pROT))) {
		pROT->Revoke(dwRegister);
		pROT->Release();
	}
}


// フィルタから指定メディアのピンを検索する
IPin* DirectShowUtil::GetFilterPin(IBaseFilter *pFilter, const PIN_DIRECTION dir, const AM_MEDIA_TYPE *pMediaType)
{
	IEnumPins *pEnumPins=NULL;
	IPin *pPin;
	IPin *pRetPin=NULL;

	if(pFilter->EnumPins(&pEnumPins)==S_OK ){
		ULONG cFetched;
		while(!pRetPin&&pEnumPins->Next(1,&pPin,&cFetched)==S_OK){
			PIN_INFO stPin;
			if(pPin->QueryPinInfo(&stPin)==S_OK){
				if(stPin.dir==dir){
					if(!pMediaType){
						// 方向さえあっていればOK
						pRetPin=pPin;
						pRetPin->AddRef();
						} else {
						// DirectShowにまかせてピンを検索
						if(pPin->QueryAccept(pMediaType)==S_OK){
							pRetPin=pPin;
							pRetPin->AddRef();
							}
						}
					}
					if(stPin.pFilter) stPin.pFilter->Release();
				}
				pPin->Release();
			}
		pEnumPins->Release();
		}
	return pRetPin;
}

// フィルタのプロパティページを開く
bool DirectShowUtil::ShowPropertyPage(IBaseFilter *pFilter, HWND hWndParent)
{
	if (!pFilter)
		return false;

	bool bRet = false;
	ISpecifyPropertyPages *pProp = NULL;

	HRESULT hr = pFilter->QueryInterface(IID_ISpecifyPropertyPages, (void **)&pProp);
	if (SUCCEEDED(hr)) {
		CAUUID caGUID = {0, NULL};

		if (SUCCEEDED(pProp->GetPages(&caGUID))) {
			FILTER_INFO stFilterInfo;

			hr = pFilter->QueryFilterInfo(&stFilterInfo);
			if (SUCCEEDED(hr)) {
				IUnknown *pFilterUnk=NULL;

				hr = pFilter->QueryInterface(IID_IUnknown, (void **)&pFilterUnk);
				if (SUCCEEDED(hr)) {
					::OleCreatePropertyFrame(
						hWndParent,             // 親ウィンドウ。
						0, 0,                   // 予約済み。
						stFilterInfo.achName,   // ダイアログ ボックスのキャプション。
						1,                      // オブジェクト数 (フィルタのみ)。
						&pFilterUnk,            // オブジェクト ポインタの配列。
						caGUID.cElems,          // プロパティ ページ数。
						caGUID.pElems,          // プロパティ ページ CLSID の配列。
						0,                      // ロケール識別子。
						0, NULL                 // 予約済み。
					);
					SAFE_RELEASE(pFilterUnk);
					bRet = true;
				}
				SAFE_RELEASE(stFilterInfo.pGraph);
			}
			::CoTaskMemFree(caGUID.pElems);
		}
		SAFE_RELEASE(pProp);
	}
	return bRet;
}

bool DirectShowUtil::HasPropertyPage(IBaseFilter *pFilter)
{
	bool bRet = false;

	if (pFilter) {
		ISpecifyPropertyPages *pProp = NULL;
		HRESULT hr = pFilter->QueryInterface(IID_ISpecifyPropertyPages, (void**)&pProp);
		if (SUCCEEDED(hr)) {
			CAUUID caGUID = {0, NULL};

			if (SUCCEEDED(pProp->GetPages(&caGUID))) {
				bRet = caGUID.cElems > 0;
				::CoTaskMemFree(caGUID.pElems);
			}
			SAFE_RELEASE(pProp);
		}
	}
	return bRet;
}

#if 0
// Mpegデコーダを追加してピン接続を行う。
// pUtilに整列済みデコーダ列挙を入れておくとその整列順に接続が行われる。NULLならデフォルト順。
//   わざわざ指定する理由 : このクラスに検索されたフィルタがMpeg2のものとは限らない為。
//
// ppMpeg2DecoderFilter は接続に使われたMpeg2インターフェースを受けとる。
// ppCurrentOutputPin が上流に接続された有効なピンであるのが前提。
// ppCurrentOutputPin は正常終了なら解放され、デコーダフィルタの出力ピンが*ppNewOutputPinになる。
// ppNewOutputPin==NULL の場合、デコーダフィルタの出力ピンが*ppCurrentOutputPinになる。元の*ppCurrentOutputPinは解放される
//
// 失敗した場合でもフィルタ解放は行われないパスがある(既に接続済みの場合)。戻りフィルタは確認して解放すること
bool DirectShowUtil::AppendMpeg2Decoder_and_Connect(IGraphBuilder *pFilterGraph, CDirectShowFilterFinder *pUtil, IBaseFilter **ppMpeg2DecoderFilter,wchar_t *lpszDecoderName,int iDecNameBufLen, IPin **ppCurrentOutputPin, IPin **ppNewOutputPin)
{
	HRESULT hr;
	IPin *pInput = NULL;
	CDirectShowFilterFinder cUtil;	
	CLSID guidFilter = CLSID_NULL;
	bool bRet;
	wchar_t tmp[128];
	if(!lpszDecoderName){
		lpszDecoderName = tmp;
		iDecNameBufLen = 128;
		}

	// ポインタチェック
	if(!pFilterGraph || !ppMpeg2DecoderFilter || !ppCurrentOutputPin) return false;
	// 同じピンアドレスなら New==NULL で入力されたのと同義
	if(ppCurrentOutputPin==ppNewOutputPin) ppNewOutputPin = NULL;
	// 指定がない場合のフィルタ検索
	if(!pUtil){
		pUtil = &cUtil;
		if(!pUtil->FindFilter(&MEDIATYPE_Video,&MEDIASUBTYPE_MPEG2_VIDEO)) return false;
		}
	// 戻り値をクリア
	*ppMpeg2DecoderFilter = NULL;

	bRet=false;
	for(int i=0;!bRet&&i<pUtil->GetFilterCount();i++){
		if(!pUtil->GetFilterInfo(i,&guidFilter,lpszDecoderName,iDecNameBufLen)) continue;
		hr = ::CoCreateInstance(guidFilter, NULL, CLSCTX_INPROC_SERVER, IID_IBaseFilter, (void**)ppMpeg2DecoderFilter);
		if(FAILED(hr)) continue;
		hr = pFilterGraph->AddFilter(*ppMpeg2DecoderFilter,lpszDecoderName);
		if(FAILED(hr)){
			SAFE_RELEASE(*ppMpeg2DecoderFilter);
			continue;
			}
		pInput = CDirectShowFilterFinder::GetFilterPin(*ppMpeg2DecoderFilter,PINDIR_INPUT);
		if(!pInput){
			SAFE_RELEASE(*ppMpeg2DecoderFilter);
			continue;
		}
		hr = pFilterGraph->Connect(*ppCurrentOutputPin,pInput);
		if(FAILED(hr)){
			SAFE_RELEASE(pInput);
			pFilterGraph->RemoveFilter(*ppMpeg2DecoderFilter);			
			SAFE_RELEASE(*ppMpeg2DecoderFilter);
			continue;
		} else {
			bRet=true;
		}
		}
	if(!bRet) {
		// 全組み合わせで適合デコーダが無かった
		return false;
	}
	// 接続に使ったピン解放
	SAFE_RELEASE(*ppCurrentOutputPin);
	SAFE_RELEASE(pInput);
	// 次フィルタへの出力ピンを探す
	IPin *pNewOutput = GetFilterPin(*ppMpeg2DecoderFilter, PINDIR_OUTPUT);
	if(!pNewOutput){
		// 出力ピンが見つからない
		return false;
		}
	if(ppNewOutputPin){
		// 新出力ピンに出力
		// 元の出力ピンは既に解放済み
		*ppNewOutputPin = pNewOutput;
		}else{
		// 出力ピンを更新(ppCurrentOutputPin==ppNewOutputPinの場合でも実体が上書きだから更新となる)
		// 元の出力ピンは既に解放済み
		*ppCurrentOutputPin = pNewOutput;
		}
	return true;
}
#endif

// 指定フィルタ経由してピン接続を行う(主に入力=1/出力=1の経由型フィルタ接続用)
// Filter指定版
//
// lpwszFilterName は NULL でも良い。
// ppCurrentOutputPin が上流に接続された有効なピンであるのが前提。
// ppCurrentOutputPin は正常終了なら解放され、フィルタの出力ピンが*ppNewOutputPinになる。
// ppNewOutputPin==NULL の場合、フィルタの出力ピンが*ppCurrentOutputPinになる。元の*ppCurrentOutputPinは解放される
//
HRESULT DirectShowUtil::AppendFilterAndConnect(IGraphBuilder *pFilterGraph,
				IBaseFilter *pFilter,LPCWSTR lpwszFilterName,
				IPin **ppCurrentOutputPin,IPin **ppNewOutputPin,bool fDirect)
{
	HRESULT hr;

	// ポインタチェック
	if (!pFilterGraph || !pFilter || !ppCurrentOutputPin)
		return E_INVALIDARG;
	// 同じピンアドレスなら New==NULL で入力されたのと同義
	if (ppCurrentOutputPin==ppNewOutputPin)
		ppNewOutputPin = NULL;
	if (!lpwszFilterName)
		lpwszFilterName = L"No Name";
	hr = pFilterGraph->AddFilter(pFilter, lpwszFilterName);
	if (FAILED(hr)) {
		// フィルタに追加失敗
		return hr;
	}
	IPin *pInput = GetFilterPin(pFilter, PINDIR_INPUT);
	if (!pInput) {
		// 入力ピンが見つからない
		pFilterGraph->RemoveFilter(pFilter);
		return E_FAIL;
	}
	// 接続
	if (fDirect)
		hr = pFilterGraph->ConnectDirect(*ppCurrentOutputPin,pInput,NULL);
	else
		hr = pFilterGraph->Connect(*ppCurrentOutputPin,pInput);
	if (FAILED(hr)) {
		// 接続できない
		SAFE_RELEASE(pInput);
		pFilterGraph->RemoveFilter(pFilter);
		return hr;
	}
	// 接続に使ったピン解放
	SAFE_RELEASE(*ppCurrentOutputPin);
	SAFE_RELEASE(pInput);
	// 次フィルタへの出力ピンを探す(出力ピンが無くても処理は続行)
	IPin *pNewOutput = GetFilterPin(pFilter, PINDIR_OUTPUT);
	if (ppNewOutputPin) {
		// 新出力ピンに出力
		// 元の出力ピンは既に解放済み
		*ppNewOutputPin = pNewOutput;
	} else {
		// 出力ピンを更新(ppCurrentOutputPin==ppNewOutputPinの場合でも実体が上書きだから更新となる)
		// 元の出力ピンは既に解放済み
		*ppCurrentOutputPin = pNewOutput;
	}
#ifdef _DEBUG
	LONG refCount = GetRefCount(pFilter);
#endif
	return S_OK;
}

// 指定フィルタ経由してピン接続を行う(主に入力=1/出力=1の経由型フィルタ接続用)
// CLSID指定版
//
// AppendFilterAndConnect(Filter指定版)の説明を参照。
// guidFilterは有効なDirectShowフィルタのGUIDを指定する。
// ppAppendedFilter は追加したフィルタを受け取る。
HRESULT DirectShowUtil::AppendFilterAndConnect(IGraphBuilder *pFilterGraph,
	const CLSID guidFilter,LPCWSTR lpwszFilterName,IBaseFilter **ppAppendedFilter,
	IPin **ppCurrentOutputPin,IPin **ppNewOutputPin,bool fDirect)
{
	// フィルタインスタンス作成
	HRESULT hr = ::CoCreateInstance(guidFilter, NULL, CLSCTX_INPROC_SERVER,
				IID_IBaseFilter, reinterpret_cast<LPVOID*>(ppAppendedFilter));
	if (FAILED(hr)) {
		// インスタンス作成失敗
		return hr;
	}
	hr = AppendFilterAndConnect(pFilterGraph,*ppAppendedFilter,lpwszFilterName,
								ppCurrentOutputPin,ppNewOutputPin,fDirect);
	if (FAILED(hr)) {
		SAFE_RELEASE(*ppAppendedFilter);
		return hr;
	}
	return S_OK;
}

// 色空間変換フィルタを経由してピン接続を行う(特定エフェクトフィルタへの対応のための色空間変換が必要な場合のため)
//
// AppendFilterAndConnect(Filter指定版)の説明を参照
// ppColorSpaceConverterFilter は追加したフィルタを受け取る為のポインタ
HRESULT DirectShowUtil::AppendColorSpaceConverterFilter_and_Connect(IGraphBuilder *pFilterGraph, IBaseFilter **ppColorSpaceConverterFilter, IPin **ppCurrentOutputPin, IPin **ppNewOutputPin)
{
	return AppendFilterAndConnect(pFilterGraph,CLSID_ColorSpaceConverter,L"Color Space Converter",ppColorSpaceConverterFilter,ppCurrentOutputPin,ppNewOutputPin);
}

// レンダラの出力先ビデオウィンドウのインターフェース取得
// EVR使用時はMF_GetVideoDisplayControl()を使う
IVideoWindow* DirectShowUtil::GetVideoWindow(IGraphBuilder *pGraph)
{
	IVideoWindow *pVideoWindow;
	HRESULT hr = pGraph->QueryInterface(IID_IVideoWindow,(void**)&pVideoWindow);
	if(SUCCEEDED(hr)){
		return pVideoWindow;
		}
	return NULL;
}

IBasicVideo2* DirectShowUtil::GetBasicVideo2(IGraphBuilder *pGraph)
{
	IBasicVideo2 *pBasicVideo;
	HRESULT hr = pGraph->QueryInterface(IID_IBasicVideo,(void**)&pBasicVideo);
	if(SUCCEEDED(hr)){
		return pBasicVideo;
		}
	return NULL;
}

IMediaControl* DirectShowUtil::GetMediaControl(IGraphBuilder *pGraph)
{
	IMediaControl *pMediaControl;
	HRESULT hr = pGraph->QueryInterface(IID_IMediaControl,(void**)&pMediaControl);
	if(SUCCEEDED(hr)){
		return pMediaControl;
		}
	return NULL;
}

bool DirectShowUtil::FilterGrapph_Play(IGraphBuilder *pFilterGraph)
{
	bool bRet=false;

	if(pFilterGraph){
		IMediaControl *pControl = GetMediaControl(pFilterGraph);
		if(pControl){
			HRESULT hr = pControl->Run();
			SAFE_RELEASE(pControl);
			bRet = true;
		}
	}
	return bRet;
}

bool DirectShowUtil::FilterGrapph_Stop(IGraphBuilder *pFilterGraph)
{
	bool bRet=false;

	if(pFilterGraph){
		IMediaControl *pControl = GetMediaControl(pFilterGraph);
		if(pControl){
			HRESULT hr = pControl->Stop();
			SAFE_RELEASE(pControl);
			bRet = true;
		}
	}
	return bRet;
}

bool DirectShowUtil::FilterGrapph_Pause(IGraphBuilder *pFilterGraph)
{
	bool bRet=false;

	if(pFilterGraph){
		IMediaControl *pControl = GetMediaControl(pFilterGraph);
		if(pControl){
			HRESULT hr = pControl->Pause();
			SAFE_RELEASE(pControl);
			bRet = true;
		}
	}
	return bRet;
}


//////////////////////////////////////////////////////////////////////
// 以下 EVR専用ユーティリティ

#ifdef USE_MEDIA_FOUNDATION

// EVR専用 : 初期化処理
void DirectShowUtil::MF_Init()
{
	::MFStartup(MF_VERSION);
};

// EVR専用 : 終了処理
void DirectShowUtil::MF_Term() {
	::MFShutdown();
};

// EVR専用 : EVR設定用インターフェースの取得
IEVRFilterConfig* DirectShowUtil::MF_GetEVRFilterConfig(IBaseFilter *pEvr)
{
	IEVRFilterConfig *pEvrFilterConfig;
	HRESULT hr = pEvr->QueryInterface(IID_IEVRFilterConfig,(void**)&pEvrFilterConfig);
	if(SUCCEEDED(hr)) {
		return pEvrFilterConfig;
		}
	return NULL;
}

// EVR専用 : EVRからMFのサービス取得用インターフェースの取得
IMFGetService* DirectShowUtil::MF_GetService(IBaseFilter *pEvr)
{
	IMFGetService *pMFGetService;
	HRESULT hr = pEvr->QueryInterface(IID_IMFGetService,(void**)&pMFGetService);
	if(SUCCEEDED(hr)) {
		return pMFGetService;
		}
	return NULL;
}

// EVR専用 : 入力ストリーム数を設定する（通常は１）
bool DirectShowUtil::MF_SetNumberOfStreams(IBaseFilter *pEvr,int iStreamNumber)
{
	IEVRFilterConfig *pFilterConfig = MF_GetEVRFilterConfig(pEvr);
	if(pFilterConfig) {
		pFilterConfig->SetNumberOfStreams(iStreamNumber);
		SAFE_RELEASE(pFilterConfig);
		return true;
		}
	return false;
}

// EVR専用 : ディスプレイ操作用インターフェースの取得
IMFVideoDisplayControl* DirectShowUtil::MF_GetVideoDisplayControl(IBaseFilter *pEvr)
{
	IMFGetService *pService = MF_GetService(pEvr);
	if(pService) {
		IMFVideoDisplayControl *pDisplayControl;
		HRESULT hr = pService->GetService(MR_VIDEO_RENDER_SERVICE,IID_IMFVideoDisplayControl,(void**)&pDisplayControl);
		SAFE_RELEASE(pService);
		if(SUCCEEDED(hr)){
			return pDisplayControl;
			}
		}
	return NULL;
}

// EVR専用 : ビデオミキサ操作用インターフェースの取得
IMFVideoMixerControl* DirectShowUtil::MF_GetVideoMixerControl(IBaseFilter *pEvr)
{
	IMFGetService *pService = MF_GetService(pEvr);
	if(pService) {
		IMFVideoMixerControl *pVideoMixerControl;
		HRESULT hr = pService->GetService(MR_VIDEO_MIXER_SERVICE,IID_IMFVideoMixerControl,(void**)&pVideoMixerControl);
		SAFE_RELEASE(pService);
		if(SUCCEEDED(hr)) {
			return pVideoMixerControl;
			}
		}
	return NULL;
}

// EVR専用 : ビデオプロセッサ操作用インターフェースの取得
IMFVideoProcessor* DirectShowUtil::MF_GetVideoProcessor(IBaseFilter *pEvr)
{
	IMFGetService *pService = MF_GetService(pEvr);
	if(pService) {
		IMFVideoProcessor *pVideoProcessor;
		HRESULT hr = pService->GetService(MR_VIDEO_MIXER_SERVICE,IID_IMFVideoProcessor,(void**)&pVideoProcessor);
		SAFE_RELEASE(pService);
		if(SUCCEEDED(hr)) {
			return pVideoProcessor;
			}
		}
	return NULL;
}

#endif
