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

int CDirectShowFilterFinder::GetFilterCount() const
{
	return (int)m_FilterList.size();
}

bool CDirectShowFilterFinder::GetFilterInfo(
	const int iIndex,CLSID *pidClass,LPWSTR pwszFriendlyName,int iBufLen) const
{
	if (iIndex<0 || iIndex>=GetFilterCount())
		return false;
	const CFilterInfo &Info = m_FilterList[iIndex];
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

bool CDirectShowFilterFinder::GetFilterInfo(
	const int iIndex,CLSID *pidClass,std::wstring *pFriendlyName) const
{
	if (iIndex<0 || iIndex>=GetFilterCount())
		return false;
	const CFilterInfo &Info = m_FilterList[iIndex];
	if (pidClass)
		*pidClass = Info.m_clsid;
	if (pFriendlyName) {
		if (Info.m_pwszFriendlyName)
			*pFriendlyName = Info.m_pwszFriendlyName;
		else
			pFriendlyName->clear();
	}
	return true;
}

bool CDirectShowFilterFinder::FindFilter(const GUID *pInTypes,int InTypeCount,
	const GUID *pOutTypes,int OutTypeCount,DWORD Merit)
{
	// �t�B���^����������
	bool bRet = false;
	IFilterMapper2 *pMapper=NULL;
	HRESULT hr=::CoCreateInstance(CLSID_FilterMapper2, NULL, CLSCTX_INPROC, IID_IFilterMapper2, (void **)&pMapper);

	if (SUCCEEDED(hr)) {
		IEnumMoniker *pEnum=NULL;

		hr = pMapper->EnumMatchingFilters(
			&pEnum,
			0,					// �\��ς�
			TRUE,				// ���S��v���g�p���邩
			Merit,				// �ŏ��̃����b�g
			TRUE,				// 1 �ȏ�̓��̓s����?
			InTypeCount,		// ���͂̃��W���[�^�C�v/�T�u�^�C�v�̑΂̐�
			pInTypes,			// ���͂̃��W���[�^�C�v/�T�u�^�C�v�̑΂̔z��
			NULL,				// ���̓��f�B�A
			NULL,				// ���̓s���̃J�e�S��
			FALSE,				// �����_���łȂ���΂Ȃ�Ȃ���
			TRUE,				// 1 �ȏ�̏o�̓s����
			OutTypeCount,		// �o�͂̃��W���[�^�C�v/�T�u�^�C�v�̑΂̐�
			pOutTypes,			// �o�͂̃��W���[�^�C�v/�T�u�^�C�v�̑΂̔z��
			NULL,				// �o�̓��f�B�A
			NULL);				// �o�̓s���̃J�e�S��
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

// �D�悷��t�B���^�����X�g��[�Ɏ����Ă���
bool CDirectShowFilterFinder::PriorityFilterGoToHead(const CLSID idPriorityClass)
{
	std::vector<CFilterInfo> TmpList;
	size_t i;

	for (i=0;i<m_FilterList.size();i++) {
		if (m_FilterList[i].m_clsid == idPriorityClass) {
			// �D�悷����̂𔭌�
			TmpList.push_back(m_FilterList[i]);
		}
	}
	if (!TmpList.empty()) {
		for (i=0;i<m_FilterList.size();i++) {
			if(m_FilterList[i].m_clsid != idPriorityClass) {
				// �D�悷����̈ȊO
				TmpList.push_back(m_FilterList[i]);
			}
		}
	}
	m_FilterList=TmpList;
	return true;
}

// ��������t�B���^�����X�g�I�[�Ɏ����Ă���
bool CDirectShowFilterFinder::IgnoreFilterGoToTail(const CLSID idIgnoreClass,bool bRemoveIt)
{
	std::vector<CFilterInfo> TmpList;
	size_t i;

	for (i=0;i<m_FilterList.size();i++) {
		if (m_FilterList[i].m_clsid != idIgnoreClass) {
			// ����������̈ȊO
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


// �t�B���^����w�胁�f�B�A�̃s������������
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
						// �������������Ă����OK
						pRetPin=pPin;
						pRetPin->AddRef();
						} else {
						// DirectShow�ɂ܂����ăs��������
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

// �t�B���^�̃v���p�e�B�y�[�W���J��
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
						hWndParent,             // �e�E�B���h�E�B
						0, 0,                   // �\��ς݁B
						stFilterInfo.achName,   // �_�C�A���O �{�b�N�X�̃L���v�V�����B
						1,                      // �I�u�W�F�N�g�� (�t�B���^�̂�)�B
						&pFilterUnk,            // �I�u�W�F�N�g �|�C���^�̔z��B
						caGUID.cElems,          // �v���p�e�B �y�[�W���B
						caGUID.pElems,          // �v���p�e�B �y�[�W CLSID �̔z��B
						0,                      // ���P�[�����ʎq�B
						0, NULL                 // �\��ς݁B
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
// Mpeg�f�R�[�_��ǉ����ăs���ڑ����s���B
// pUtil�ɐ���ς݃f�R�[�_�񋓂����Ă����Ƃ��̐��񏇂ɐڑ����s����BNULL�Ȃ�f�t�H���g���B
//   �킴�킴�w�肷�闝�R : ���̃N���X�Ɍ������ꂽ�t�B���^��Mpeg2�̂��̂Ƃ͌���Ȃ��ׁB
//
// ppMpeg2DecoderFilter �͐ڑ��Ɏg��ꂽMpeg2�C���^�[�t�F�[�X���󂯂Ƃ�B
// ppCurrentOutputPin ���㗬�ɐڑ����ꂽ�L���ȃs���ł���̂��O��B
// ppCurrentOutputPin �͐���I���Ȃ�������A�f�R�[�_�t�B���^�̏o�̓s����*ppNewOutputPin�ɂȂ�B
// ppNewOutputPin==NULL �̏ꍇ�A�f�R�[�_�t�B���^�̏o�̓s����*ppCurrentOutputPin�ɂȂ�B����*ppCurrentOutputPin�͉�������
//
// ���s�����ꍇ�ł��t�B���^����͍s���Ȃ��p�X������(���ɐڑ��ς݂̏ꍇ)�B�߂�t�B���^�͊m�F���ĉ�����邱��
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

	// �|�C���^�`�F�b�N
	if(!pFilterGraph || !ppMpeg2DecoderFilter || !ppCurrentOutputPin) return false;
	// �����s���A�h���X�Ȃ� New==NULL �œ��͂��ꂽ�̂Ɠ��`
	if(ppCurrentOutputPin==ppNewOutputPin) ppNewOutputPin = NULL;
	// �w�肪�Ȃ��ꍇ�̃t�B���^����
	if(!pUtil){
		pUtil = &cUtil;
		if(!pUtil->FindFilter(&MEDIATYPE_Video,&MEDIASUBTYPE_MPEG2_VIDEO)) return false;
		}
	// �߂�l���N���A
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
		// �S�g�ݍ��킹�œK���f�R�[�_����������
		return false;
	}
	// �ڑ��Ɏg�����s�����
	SAFE_RELEASE(*ppCurrentOutputPin);
	SAFE_RELEASE(pInput);
	// ���t�B���^�ւ̏o�̓s����T��
	IPin *pNewOutput = GetFilterPin(*ppMpeg2DecoderFilter, PINDIR_OUTPUT);
	if(!pNewOutput){
		// �o�̓s����������Ȃ�
		return false;
		}
	if(ppNewOutputPin){
		// �V�o�̓s���ɏo��
		// ���̏o�̓s���͊��ɉ���ς�
		*ppNewOutputPin = pNewOutput;
		}else{
		// �o�̓s�����X�V(ppCurrentOutputPin==ppNewOutputPin�̏ꍇ�ł����̂��㏑��������X�V�ƂȂ�)
		// ���̏o�̓s���͊��ɉ���ς�
		*ppCurrentOutputPin = pNewOutput;
		}
	return true;
}
#endif

// �w��t�B���^�o�R���ăs���ڑ����s��(��ɓ���=1/�o��=1�̌o�R�^�t�B���^�ڑ��p)
// Filter�w���
//
// lpwszFilterName �� NULL �ł��ǂ��B
// ppCurrentOutputPin ���㗬�ɐڑ����ꂽ�L���ȃs���ł���̂��O��B
// ppCurrentOutputPin �͐���I���Ȃ�������A�t�B���^�̏o�̓s����*ppNewOutputPin�ɂȂ�B
// ppNewOutputPin==NULL �̏ꍇ�A�t�B���^�̏o�̓s����*ppCurrentOutputPin�ɂȂ�B����*ppCurrentOutputPin�͉�������
//
HRESULT DirectShowUtil::AppendFilterAndConnect(IGraphBuilder *pFilterGraph,
				IBaseFilter *pFilter,LPCWSTR lpwszFilterName,
				IPin **ppCurrentOutputPin,IPin **ppNewOutputPin,bool fDirect)
{
	HRESULT hr;

	// �|�C���^�`�F�b�N
	if (!pFilterGraph || !pFilter || !ppCurrentOutputPin)
		return E_INVALIDARG;
	// �����s���A�h���X�Ȃ� New==NULL �œ��͂��ꂽ�̂Ɠ��`
	if (ppCurrentOutputPin==ppNewOutputPin)
		ppNewOutputPin = NULL;
	if (!lpwszFilterName)
		lpwszFilterName = L"No Name";
	hr = pFilterGraph->AddFilter(pFilter, lpwszFilterName);
	if (FAILED(hr)) {
		// �t�B���^�ɒǉ����s
		return hr;
	}
	IPin *pInput = GetFilterPin(pFilter, PINDIR_INPUT);
	if (!pInput) {
		// ���̓s����������Ȃ�
		pFilterGraph->RemoveFilter(pFilter);
		return E_FAIL;
	}
	// �ڑ�
	if (fDirect)
		hr = pFilterGraph->ConnectDirect(*ppCurrentOutputPin,pInput,NULL);
	else
		hr = pFilterGraph->Connect(*ppCurrentOutputPin,pInput);
	if (FAILED(hr)) {
		// �ڑ��ł��Ȃ�
		SAFE_RELEASE(pInput);
		pFilterGraph->RemoveFilter(pFilter);
		return hr;
	}
	// �ڑ��Ɏg�����s�����
	SAFE_RELEASE(*ppCurrentOutputPin);
	SAFE_RELEASE(pInput);
	// ���t�B���^�ւ̏o�̓s����T��(�o�̓s���������Ă������͑��s)
	IPin *pNewOutput = GetFilterPin(pFilter, PINDIR_OUTPUT);
	if (ppNewOutputPin) {
		// �V�o�̓s���ɏo��
		// ���̏o�̓s���͊��ɉ���ς�
		*ppNewOutputPin = pNewOutput;
	} else {
		// �o�̓s�����X�V(ppCurrentOutputPin==ppNewOutputPin�̏ꍇ�ł����̂��㏑��������X�V�ƂȂ�)
		// ���̏o�̓s���͊��ɉ���ς�
		*ppCurrentOutputPin = pNewOutput;
	}
#ifdef _DEBUG
	LONG refCount = GetRefCount(pFilter);
#endif
	return S_OK;
}

// �w��t�B���^�o�R���ăs���ڑ����s��(��ɓ���=1/�o��=1�̌o�R�^�t�B���^�ڑ��p)
// CLSID�w���
//
// AppendFilterAndConnect(Filter�w���)�̐������Q�ƁB
// guidFilter�͗L����DirectShow�t�B���^��GUID���w�肷��B
// ppAppendedFilter �͒ǉ������t�B���^���󂯎��B
HRESULT DirectShowUtil::AppendFilterAndConnect(IGraphBuilder *pFilterGraph,
	const CLSID guidFilter,LPCWSTR lpwszFilterName,IBaseFilter **ppAppendedFilter,
	IPin **ppCurrentOutputPin,IPin **ppNewOutputPin,bool fDirect)
{
	// �t�B���^�C���X�^���X�쐬
	HRESULT hr = ::CoCreateInstance(guidFilter, NULL, CLSCTX_INPROC_SERVER,
				IID_IBaseFilter, reinterpret_cast<LPVOID*>(ppAppendedFilter));
	if (FAILED(hr)) {
		// �C���X�^���X�쐬���s
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

// �F��ԕϊ��t�B���^���o�R���ăs���ڑ����s��(����G�t�F�N�g�t�B���^�ւ̑Ή��̂��߂̐F��ԕϊ����K�v�ȏꍇ�̂���)
//
// AppendFilterAndConnect(Filter�w���)�̐������Q��
// ppColorSpaceConverterFilter �͒ǉ������t�B���^���󂯎��ׂ̃|�C���^
HRESULT DirectShowUtil::AppendColorSpaceConverterFilter_and_Connect(IGraphBuilder *pFilterGraph, IBaseFilter **ppColorSpaceConverterFilter, IPin **ppCurrentOutputPin, IPin **ppNewOutputPin)
{
	return AppendFilterAndConnect(pFilterGraph,CLSID_ColorSpaceConverter,L"Color Space Converter",ppColorSpaceConverterFilter,ppCurrentOutputPin,ppNewOutputPin);
}

// �����_���̏o�͐�r�f�I�E�B���h�E�̃C���^�[�t�F�[�X�擾
// EVR�g�p����MF_GetVideoDisplayControl()���g��
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
// �ȉ� EVR��p���[�e�B���e�B

#ifdef USE_MEDIA_FOUNDATION

// EVR��p : ����������
void DirectShowUtil::MF_Init()
{
	::MFStartup(MF_VERSION);
};

// EVR��p : �I������
void DirectShowUtil::MF_Term() {
	::MFShutdown();
};

// EVR��p : EVR�ݒ�p�C���^�[�t�F�[�X�̎擾
IEVRFilterConfig* DirectShowUtil::MF_GetEVRFilterConfig(IBaseFilter *pEvr)
{
	IEVRFilterConfig *pEvrFilterConfig;
	HRESULT hr = pEvr->QueryInterface(IID_IEVRFilterConfig,(void**)&pEvrFilterConfig);
	if(SUCCEEDED(hr)) {
		return pEvrFilterConfig;
		}
	return NULL;
}

// EVR��p : EVR����MF�̃T�[�r�X�擾�p�C���^�[�t�F�[�X�̎擾
IMFGetService* DirectShowUtil::MF_GetService(IBaseFilter *pEvr)
{
	IMFGetService *pMFGetService;
	HRESULT hr = pEvr->QueryInterface(IID_IMFGetService,(void**)&pMFGetService);
	if(SUCCEEDED(hr)) {
		return pMFGetService;
		}
	return NULL;
}

// EVR��p : ���̓X�g���[������ݒ肷��i�ʏ�͂P�j
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

// EVR��p : �f�B�X�v���C����p�C���^�[�t�F�[�X�̎擾
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

// EVR��p : �r�f�I�~�L�T����p�C���^�[�t�F�[�X�̎擾
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

// EVR��p : �r�f�I�v���Z�b�T����p�C���^�[�t�F�[�X�̎擾
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
