// MediaViewer.cpp: CMediaViewer �N���X�̃C���v�������e�[�V����
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <Dvdmedia.h>
#include "MediaViewer.h"
#include "StdUtil.h"
#include "../DirectShowFilter/BonSrcFilter.h"
#ifdef BONTSENGINE_MPEG2_SUPPORT
#include "../DirectShowFilter/Mpeg2ParserFilter.h"
#endif
#ifdef BONTSENGINE_H264_SUPPORT
#include "../DirectShowFilter/H264ParserFilter.h"
#endif
#ifdef BONTSENGINE_H265_SUPPORT
#include "../DirectShowFilter/H265ParserFilter.h"
#endif

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

#pragma comment(lib,"quartz.lib")


//const CLSID CLSID_NullRenderer = {0xc1f400a4, 0x3f08, 0x11d3, {0x9f, 0x0b, 0x00, 0x60, 0x08, 0x03, 0x9e, 0x37}};
EXTERN_C const CLSID CLSID_NullRenderer;

static const DWORD LOCK_TIMEOUT = 2000;


static HRESULT SetVideoMediaType(CMediaType *pMediaType, BYTE VideoStreamType, int Width, int Height)
{
	static const REFERENCE_TIME TIME_PER_FRAME =
		static_cast<REFERENCE_TIME>(10000000.0 / 29.97 + 0.5);

	switch (VideoStreamType) {
#ifdef BONTSENGINE_MPEG2_SUPPORT
	case STREAM_TYPE_MPEG2_VIDEO:
		// MPEG-2
		{
			// �f�����f�B�A�t�H�[�}�b�g�ݒ�
			pMediaType->InitMediaType();
			pMediaType->SetType(&MEDIATYPE_Video);
			pMediaType->SetSubtype(&MEDIASUBTYPE_MPEG2_VIDEO);
			pMediaType->SetVariableSize();
			pMediaType->SetTemporalCompression(TRUE);
			pMediaType->SetSampleSize(0);
			pMediaType->SetFormatType(&FORMAT_MPEG2Video);
			// �t�H�[�}�b�g�\���̊m��
			MPEG2VIDEOINFO *pVideoInfo =
				pointer_cast<MPEG2VIDEOINFO *>(pMediaType->AllocFormatBuffer(sizeof(MPEG2VIDEOINFO)));
			if (!pVideoInfo)
				return E_OUTOFMEMORY;
			::ZeroMemory(pVideoInfo, sizeof(MPEG2VIDEOINFO));
			// �r�f�I�w�b�_�ݒ�
			VIDEOINFOHEADER2 &VideoHeader = pVideoInfo->hdr;
			//::SetRect(&VideoHeader.rcSource, 0, 0, Width, Height);
			VideoHeader.AvgTimePerFrame = TIME_PER_FRAME;
			VideoHeader.bmiHeader.biSize = sizeof(BITMAPINFOHEADER); 
			VideoHeader.bmiHeader.biWidth = Width;
			VideoHeader.bmiHeader.biHeight = Height;
		}
		break;
#endif	// BONTSENGINE_MPEG2_SUPPORT

#ifdef BONTSENGINE_H264_SUPPORT
	case STREAM_TYPE_H264:
		// H.264
		{
			pMediaType->InitMediaType();
			pMediaType->SetType(&MEDIATYPE_Video);
			pMediaType->SetSubtype(&MEDIASUBTYPE_H264);
			pMediaType->SetVariableSize();
			pMediaType->SetTemporalCompression(TRUE);
			pMediaType->SetSampleSize(0);
			pMediaType->SetFormatType(&FORMAT_VideoInfo);
			VIDEOINFOHEADER *pVideoInfo =
				pointer_cast<VIDEOINFOHEADER *>(pMediaType->AllocFormatBuffer(sizeof(VIDEOINFOHEADER)));
			if (!pVideoInfo)
				return E_OUTOFMEMORY;
			::ZeroMemory(pVideoInfo, sizeof(VIDEOINFOHEADER));
			pVideoInfo->dwBitRate = 32000000;
			pVideoInfo->AvgTimePerFrame = TIME_PER_FRAME;
			pVideoInfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
			pVideoInfo->bmiHeader.biWidth = Width;
			pVideoInfo->bmiHeader.biHeight = Height;
			pVideoInfo->bmiHeader.biCompression = MAKEFOURCC('h','2','6','4');
		}
		break;
#endif	// BONTSENGINE_H264_SUPPORT

#ifdef BONTSENGINE_H265_SUPPORT
	case STREAM_TYPE_H265:
		// H.265
		{
			pMediaType->InitMediaType();
			pMediaType->SetType(&MEDIATYPE_Video);
			pMediaType->SetSubtype(&MEDIASUBTYPE_HEVC);
			pMediaType->SetVariableSize();
			pMediaType->SetTemporalCompression(TRUE);
			pMediaType->SetSampleSize(0);
			pMediaType->SetFormatType(&FORMAT_VideoInfo);
			VIDEOINFOHEADER *pVideoInfo =
				pointer_cast<VIDEOINFOHEADER *>(pMediaType->AllocFormatBuffer(sizeof(VIDEOINFOHEADER)));
			if (!pVideoInfo)
				return E_OUTOFMEMORY;
			::ZeroMemory(pVideoInfo, sizeof(VIDEOINFOHEADER));
			pVideoInfo->dwBitRate = 32000000;
			pVideoInfo->AvgTimePerFrame = TIME_PER_FRAME;
			pVideoInfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
			pVideoInfo->bmiHeader.biWidth = Width;
			pVideoInfo->bmiHeader.biHeight = Height;
			pVideoInfo->bmiHeader.biCompression = MAKEFOURCC('H','E','V','C');
		}
		break;
#endif	// BONTSENGINE_H265_SUPPORT

	default:
		return E_UNEXPECTED;
	}

	return S_OK;
}


//////////////////////////////////////////////////////////////////////
// �\�z/����
//////////////////////////////////////////////////////////////////////

CMediaViewer::CMediaViewer(CMediaDecoder::IEventHandler *pEventHandler)
	: CMediaDecoder(pEventHandler, 1UL, 0UL)
	, m_bInit(false)

	, m_pFilterGraph(NULL)
	, m_pMediaControl(NULL)

	, m_pSrcFilter(NULL)

	, m_pAudioDecoder(NULL)

	, m_pVideoDecoderFilter(NULL)

	, m_pVideoRenderer(NULL)
	, m_pAudioRenderer(NULL)

	, m_pszAudioFilterName(NULL)
	, m_pAudioFilter(NULL)

	, m_pVideoParserFilter(NULL)
	, m_pVideoParser(NULL)

	, m_pszVideoDecoderName(NULL)

	, m_pMp2DemuxFilter(NULL)
	, m_pMp2DemuxVideoMap(NULL)
	, m_pMp2DemuxAudioMap(NULL)

	, m_wVideoEsPID(PID_INVALID)
	, m_wAudioEsPID(PID_INVALID)
	, m_MapAudioPID(PID_INVALID)

	, m_wVideoWindowX(0)
	, m_wVideoWindowY(0)
	, m_VideoInfo()
	, m_hOwnerWnd(NULL)
#ifdef _DEBUG
	, m_dwRegister(0)
#endif

	, m_VideoRendererType(CVideoRenderer::RENDERER_UNDEFINED)
	, m_pszAudioRendererName(NULL)
	, m_VideoStreamType(STREAM_TYPE_UNINITIALIZED)
	, m_ForceAspectX(0)
	, m_ForceAspectY(0)
	, m_ViewStretchMode(STRETCH_KEEPASPECTRATIO)
	, m_bNoMaskSideCut(false)
	, m_bIgnoreDisplayExtension(false)
	, m_bUseAudioRendererClock(true)
	, m_b1SegMode(false)
	, m_bAdjustAudioStreamTime(false)
	, m_bEnablePTSSync(false)
	, m_bAdjust1SegVideoSampleTime(true)
	, m_bAdjust1SegFrameRate(true)
	, m_BufferSize(0)
	, m_InitialPoolPercentage(0)
	, m_PacketInputWait(0)
	, m_pAudioStreamCallback(NULL)
	, m_pAudioStreamCallbackParam(NULL)
	, m_pImageMixer(NULL)
{
	// COM���C�u����������
	//::CoInitialize(NULL);
}

CMediaViewer::~CMediaViewer()
{
	CloseViewer();

	if (m_pszAudioFilterName)
		delete [] m_pszAudioFilterName;

	// COM���C�u�����J��
	//::CoUninitialize();
}


void CMediaViewer::Reset()
{
	TRACE(TEXT("CMediaViewer::Reset()\n"));

	CTryBlockLock Lock(&m_DecoderLock);
	Lock.TryLock(LOCK_TIMEOUT);

	Flush();

	SetVideoPID(PID_INVALID);
	SetAudioPID(PID_INVALID);

	//m_VideoInfo.Reset();
}

const bool CMediaViewer::InputMedia(CMediaData *pMediaData, const DWORD dwInputIndex)
{
	CBlockLock Lock(&m_DecoderLock);

	/*
	if(dwInputIndex >= GetInputNum())return false;

	CTsPacket *pTsPacket = dynamic_cast<CTsPacket *>(pMediaData);

	// ���̓��f�B�A�f�[�^�͌݊������Ȃ�
	if(!pTsPacket)return false;
	*/
	CTsPacket *pTsPacket = static_cast<CTsPacket *>(pMediaData);

	// �t�B���^�O���t�ɓ���
	if (m_pSrcFilter
			&& pTsPacket->GetPID() != 0x1FFF
			&& !pTsPacket->IsScrambled()) {
		return m_pSrcFilter->InputMedia(pTsPacket);
	}

	return false;
}

bool CMediaViewer::OpenViewer(
	HWND hOwnerHwnd, HWND hMessageDrainHwnd,
	CVideoRenderer::RendererType RendererType,
	BYTE VideoStreamType,
	LPCWSTR pszVideoDecoder, LPCWSTR pszAudioDevice)
{
	bool bNoVideo = false;

	switch (VideoStreamType) {
	default:
		SetError(TEXT("�Ή����Ă��Ȃ��f���`���ł��B"));
		return false;
	case STREAM_TYPE_INVALID:
		bNoVideo = true;
		break;
#ifdef BONTSENGINE_MPEG2_SUPPORT
	case STREAM_TYPE_MPEG2_VIDEO:
#endif
#ifdef BONTSENGINE_H264_SUPPORT
	case STREAM_TYPE_H264:
#endif
#ifdef BONTSENGINE_H265_SUPPORT
	case STREAM_TYPE_H265:
#endif
		break;
	}

	CTryBlockLock Lock(&m_DecoderLock);
	if (!Lock.TryLock(LOCK_TIMEOUT)) {
		SetError(TEXT("�^�C���A�E�g�G���[�ł��B"));
		return false;
	}

	if (m_bInit) {
		SetError(TEXT("���Ƀt�B���^�O���t���\�z����Ă��܂��B"));
		return false;
	}

	TRACE(TEXT("CMediaViewer::OpenViewer() �t�B���^�O���t�쐬�J�n\n"));

	HRESULT hr=S_OK;

	IPin *pOutput=NULL;
	IPin *pOutputVideo=NULL;
	IPin *pOutputAudio=NULL;

	try {
		// �t�B���^�O���t�}�l�[�W�����\�z����
		hr=::CoCreateInstance(CLSID_FilterGraph,NULL,CLSCTX_INPROC_SERVER,
				IID_IGraphBuilder,pointer_cast<LPVOID*>(&m_pFilterGraph));
		if (FAILED(hr)) {
			throw CBonException(hr,TEXT("�t�B���^�O���t�}�l�[�W�����쐬�ł��܂���B"));
		}

		SendDecoderEvent(EID_FILTER_GRAPH_INITIALIZE, m_pFilterGraph);

#ifdef _DEBUG
		AddToRot(m_pFilterGraph, &m_dwRegister);
#endif

		// IMediaControl�C���^�t�F�[�X�̃N�G���[
		hr=m_pFilterGraph->QueryInterface(IID_IMediaControl, pointer_cast<void**>(&m_pMediaControl));
		if (FAILED(hr)) {
			throw CBonException(hr,TEXT("���f�B�A�R���g���[�����擾�ł��܂���B"));
		}

		Trace(CTracer::TYPE_INFORMATION, TEXT("�\�[�X�t�B���^�̐ڑ���..."));

		/* CBonSrcFilter */
		{
			// �C���X�^���X�쐬
			m_pSrcFilter = static_cast<CBonSrcFilter*>(CBonSrcFilter::CreateInstance(NULL, &hr));
			if (m_pSrcFilter == NULL || FAILED(hr))
				throw CBonException(hr, TEXT("�\�[�X�t�B���^���쐬�ł��܂���B"));
			m_pSrcFilter->SetOutputWhenPaused(RendererType == CVideoRenderer::RENDERER_DEFAULT);
			// �t�B���^�O���t�ɒǉ�
			hr = m_pFilterGraph->AddFilter(m_pSrcFilter, L"BonSrcFilter");
			if (FAILED(hr))
				throw CBonException(hr, TEXT("�\�[�X�t�B���^���t�B���^�O���t�ɒǉ��ł��܂���B"));
			// �o�̓s�����擾
			pOutput = DirectShowUtil::GetFilterPin(m_pSrcFilter, PINDIR_OUTPUT);
			if (pOutput==NULL)
				throw CBonException(TEXT("�\�[�X�t�B���^�̏o�̓s�����擾�ł��܂���B"));
			m_pSrcFilter->EnableSync(m_bEnablePTSSync, m_b1SegMode);
			if (m_BufferSize != 0)
				m_pSrcFilter->SetBufferSize(m_BufferSize);
			m_pSrcFilter->SetInitialPoolPercentage(m_InitialPoolPercentage);
			m_pSrcFilter->SetInputWait(m_PacketInputWait);
		}

		Trace(CTracer::TYPE_INFORMATION, TEXT("MPEG-2 Demultiplexer�t�B���^�̐ڑ���..."));

		/* MPEG-2 Demultiplexer */
		{
			IMpeg2Demultiplexer *pMpeg2Demuxer;

			hr=::CoCreateInstance(CLSID_MPEG2Demultiplexer,NULL,
					CLSCTX_INPROC_SERVER,IID_IBaseFilter,
					pointer_cast<LPVOID*>(&m_pMp2DemuxFilter));
			if (FAILED(hr))
				throw CBonException(hr,TEXT("MPEG-2 Demultiplexer�t�B���^���쐬�ł��܂���B"),
									TEXT("MPEG-2 Demultiplexer�t�B���^���C���X�g�[������Ă��邩�m�F���Ă��������B"));
			hr=DirectShowUtil::AppendFilterAndConnect(m_pFilterGraph,
								m_pMp2DemuxFilter,L"Mpeg2Demuxer",&pOutput);
			if (FAILED(hr))
				throw CBonException(hr,TEXT("MPEG-2 Demultiplexer���t�B���^�O���t�ɒǉ��ł��܂���B"));
			// ���̎��_��pOutput==NULL�̂͂������O�̂���
			SAFE_RELEASE(pOutput);

			// IMpeg2Demultiplexer�C���^�t�F�[�X�̃N�G���[
			hr=m_pMp2DemuxFilter->QueryInterface(IID_IMpeg2Demultiplexer,
												 pointer_cast<void**>(&pMpeg2Demuxer));
			if (FAILED(hr))
				throw CBonException(hr,TEXT("MPEG-2 Demultiplexer�C���^�[�t�F�[�X���擾�ł��܂���B"),
									TEXT("�݊����̂Ȃ��X�v���b�^�̗D��x��MPEG-2 Demultiplexer��荂���Ȃ��Ă���\��������܂��B"));

			if (!bNoVideo) {
				CMediaType MediaTypeVideo;

				// �f�����f�B�A�t�H�[�}�b�g�ݒ�
				hr = SetVideoMediaType(&MediaTypeVideo, VideoStreamType, 1920, 1080);
				if (FAILED(hr))
					throw CBonException(TEXT("���������m�ۂł��܂���B"));
				// �f���o�̓s���쐬
				hr = pMpeg2Demuxer->CreateOutputPin(&MediaTypeVideo, L"Video", &pOutputVideo);
				if (FAILED(hr)) {
					pMpeg2Demuxer->Release();
					throw CBonException(hr, TEXT("MPEG-2 Demultiplexer�̉f���o�̓s�����쐬�ł��܂���B"));
				}
			}

			// �������f�B�A�t�H�[�}�b�g�ݒ�
			CMediaType MediaTypeAudio;
			MediaTypeAudio.InitMediaType();
			MediaTypeAudio.SetType(&MEDIATYPE_Audio);
			MediaTypeAudio.SetSubtype(&MEDIASUBTYPE_NULL);
			MediaTypeAudio.SetVariableSize();
			MediaTypeAudio.SetTemporalCompression(TRUE);
			MediaTypeAudio.SetSampleSize(0);
			MediaTypeAudio.SetFormatType(&FORMAT_None);
			// �����o�̓s���쐬
			hr=pMpeg2Demuxer->CreateOutputPin(&MediaTypeAudio,L"Audio",&pOutputAudio);
			pMpeg2Demuxer->Release();
			if (FAILED(hr))
				throw CBonException(hr,TEXT("MPEG-2 Demultiplexer�̉����o�̓s�����쐬�ł��܂���B"));
			if (pOutputVideo) {
				// �f���o�̓s����IMPEG2PIDMap�C���^�t�F�[�X�̃N�G���[
				hr=pOutputVideo->QueryInterface(__uuidof(IMPEG2PIDMap),pointer_cast<void**>(&m_pMp2DemuxVideoMap));
				if (FAILED(hr))
					throw CBonException(hr,TEXT("�f���o�̓s����IMPEG2PIDMap���擾�ł��܂���B"));
			}
			// �����o�̓s����IMPEG2PIDMap�C���^�t�F�[�X�̃N�G��
			hr=pOutputAudio->QueryInterface(__uuidof(IMPEG2PIDMap),pointer_cast<void**>(&m_pMp2DemuxAudioMap));
			if (FAILED(hr))
				throw CBonException(hr,TEXT("�����o�̓s����IMPEG2PIDMap���擾�ł��܂���B"));
		}

		// �f���p�[�T�t�B���^�̐ڑ�
		switch (VideoStreamType) {
#ifdef BONTSENGINE_MPEG2_SUPPORT
		case STREAM_TYPE_MPEG2_VIDEO:
			{
				Trace(CTracer::TYPE_INFORMATION, TEXT("MPEG-2�p�[�T�t�B���^�̐ڑ���..."));

				// �C���X�^���X�쐬
				CMpeg2ParserFilter *pMpeg2Parser =
					static_cast<CMpeg2ParserFilter*>(CMpeg2ParserFilter::CreateInstance(NULL, &hr));
				if (!pMpeg2Parser || FAILED(hr))
					throw CBonException(hr, TEXT("MPEG-2�p�[�T�t�B���^���쐬�ł��܂���B"));
				m_pVideoParserFilter = pMpeg2Parser;
				m_pVideoParser = pMpeg2Parser;
				// �t�B���^�̒ǉ��Ɛڑ�
				hr = DirectShowUtil::AppendFilterAndConnect(
					m_pFilterGraph, pMpeg2Parser, L"Mpeg2ParserFilter", &pOutputVideo);
				if (FAILED(hr))
					throw CBonException(hr, TEXT("MPEG-2�p�[�T�t�B���^���t�B���^�O���t�ɒǉ��ł��܂���B"));
			}
			break;
#endif	// BONTSENGINE_MPEG2_SUPPORT

#ifdef BONTSENGINE_H264_SUPPORT
		case STREAM_TYPE_H264:
			{
				Trace(CTracer::TYPE_INFORMATION, TEXT("H.264�p�[�T�t�B���^�̐ڑ���..."));

				// �C���X�^���X�쐬
				CH264ParserFilter *pH264Parser =
					static_cast<CH264ParserFilter*>(CH264ParserFilter::CreateInstance(NULL, &hr));
				if (!pH264Parser || FAILED(hr))
					throw CBonException(TEXT("H.264�p�[�T�t�B���^���쐬�ł��܂���B"));
				m_pVideoParserFilter = pH264Parser;
				m_pVideoParser = pH264Parser;
				// �t�B���^�̒ǉ��Ɛڑ�
				hr = DirectShowUtil::AppendFilterAndConnect(
					m_pFilterGraph, pH264Parser, L"H264ParserFilter", &pOutputVideo);
				if (FAILED(hr))
					throw CBonException(hr,TEXT("H.264�p�[�T�t�B���^���t�B���^�O���t�ɒǉ��ł��܂���B"));
			}
			break;
#endif	// BONTSENGINE_H264_SUPPORT

#ifdef BONTSENGINE_H265_SUPPORT
		case STREAM_TYPE_H265:
			{
				Trace(CTracer::TYPE_INFORMATION, TEXT("H.265�p�[�T�t�B���^�̐ڑ���..."));

				// �C���X�^���X�쐬
				CH265ParserFilter *pH265Parser =
					static_cast<CH265ParserFilter*>(CH265ParserFilter::CreateInstance(NULL, &hr));
				if (!pH265Parser || FAILED(hr))
					throw CBonException(TEXT("H.265�p�[�T�t�B���^���쐬�ł��܂���B"));
				m_pVideoParserFilter = pH265Parser;
				m_pVideoParser = pH265Parser;
				// �t�B���^�̒ǉ��Ɛڑ�
				hr = DirectShowUtil::AppendFilterAndConnect(
					m_pFilterGraph, pH265Parser, L"H265ParserFilter", &pOutputVideo);
				if (FAILED(hr))
					throw CBonException(hr,TEXT("H.265�p�[�T�t�B���^���t�B���^�O���t�ɒǉ��ł��܂���B"));
			}
			break;
#endif	// BONTSENGINE_H265_SUPPORT
		}

		if (m_pVideoParser) {
			m_pVideoParser->SetVideoInfoCallback(OnVideoInfo, this);
			// madVR �͉f���T�C�Y�̕ω����� MediaType ��ݒ肵�Ȃ��ƐV�����T�C�Y���K�p����Ȃ�
			m_pVideoParser->SetAttachMediaType(RendererType == CVideoRenderer::RENDERER_madVR);
			ApplyAdjustVideoSampleOptions();
		}

		Trace(CTracer::TYPE_INFORMATION, TEXT("�����f�R�[�_�̐ڑ���..."));

#if 1
		/* CAudioDecFilter */
		{
			// CAudioDecFilter�C���X�^���X�쐬
			m_pAudioDecoder = static_cast<CAudioDecFilter*>(CAudioDecFilter::CreateInstance(NULL, &hr));
			if (!m_pAudioDecoder || FAILED(hr))
				throw CBonException(hr,TEXT("�����f�R�[�_�t�B���^���쐬�ł��܂���B"));
			// �t�B���^�̒ǉ��Ɛڑ�
			hr=DirectShowUtil::AppendFilterAndConnect(
				m_pFilterGraph,m_pAudioDecoder,L"AudioDecFilter",&pOutputAudio);
			if (FAILED(hr))
				throw CBonException(hr,TEXT("�����f�R�[�_�t�B���^���t�B���^�O���t�ɒǉ��ł��܂���B"));

			m_pAudioDecoder->SetEventHandler(this);
			m_pAudioDecoder->SetJitterCorrection(m_bAdjustAudioStreamTime);
			if (m_pAudioStreamCallback)
				m_pAudioDecoder->SetStreamCallback(m_pAudioStreamCallback,
												   m_pAudioStreamCallbackParam);
		}
#else
		/*
			�O��AAC�f�R�[�_�𗘗p����ƁA�`�����l�������؂�ւ�����ۂɉ����o�Ȃ��Ȃ�A
			�f���A�����m�������X�e���I�Ƃ��čĐ������A�Ƃ�������肪�o��
		*/

		/* CAacParserFilter */
		{
			CAacParserFilter *m_pAacParser;
			// CAacParserFilter�C���X�^���X�쐬
			m_pAacParser=static_cast<CAacParserFilter*>(CAacParserFilter::CreateInstance(NULL, &hr));
			if (!m_pAacParser || FAILED(hr))
				throw CBonException(hr,TEXT("AAC�p�[�T�t�B���^���쐬�ł��܂���B"));
			// �t�B���^�̒ǉ��Ɛڑ�
			hr=DirectShowUtil::AppendFilterAndConnect(
				m_pFilterGraph,m_pAacParser,L"AacParserFilter",&pOutputAudio);
			if (FAILED(hr))
				throw CBonException(TEXT("AAC�p�[�T�t�B���^���t�B���^�O���t�ɒǉ��ł��܂���B"));
			m_pAacParser->Release();
		}

		/* AAC�f�R�[�_�[ */
		{
			CDirectShowFilterFinder FilterFinder;

			// ����
			if(!FilterFinder.FindFilter(&MEDIATYPE_Audio,&MEDIASUBTYPE_AAC))
				throw CBonException(TEXT("AAC�f�R�[�_�����t����܂���B"),
									TEXT("AAC�f�R�[�_���C���X�g�[������Ă��邩�m�F���Ă��������B"));

			WCHAR szAacDecoder[128];
			CLSID idAac;
			bool bConnectSuccess=false;
			IBaseFilter *pAacDecFilter=NULL;

			for (int i=0;i<FilterFinder.GetFilterCount();i++){
				if (FilterFinder.GetFilterInfo(i,&idAac,szAacDecoder,128)) {
					if (pszAudioDecoder!=NULL && pszAudioDecoder[0]!='\0'
							&& ::lstrcmpi(szAacDecoder,pszAudioDecoder)!=0)
						continue;
					hr=DirectShowUtil::AppendFilterAndConnect(m_pFilterGraph,
							idAac,szAacDecoder,&pAacDecFilter,
							&pOutputAudio);
					if (SUCCEEDED(hr)) {
						TRACE(TEXT("AAC decoder connected : %s\n"),szAacDecoder);
						bConnectSuccess=true;
						break;
					}
				}
			}
			// �ǂꂩ�̃t�B���^�Őڑ��ł�����
			if (bConnectSuccess) {
				SAFE_RELEASE(pAacDecFilter);
				//m_pszAacDecoderName=StdUtil::strdup(szAacDecoder);
			} else {
				throw CBonException(TEXT("AAC�f�R�[�_�t�B���^���t�B���^�O���t�ɒǉ��ł��܂���B"),
									TEXT("�ݒ�ŗL����AAC�f�R�[�_���I������Ă��邩�m�F���Ă��������B"));
			}
		}
#endif

		/* ���[�U�[�w��̉����t�B���^�̐ڑ� */
		if (m_pszAudioFilterName) {
			Trace(CTracer::TYPE_INFORMATION, TEXT("�����t�B���^�̐ڑ���..."));

			// ����
			bool bConnectSuccess=false;
			CDirectShowFilterFinder FilterFinder;
			if (FilterFinder.FindFilter(&MEDIATYPE_Audio,&MEDIASUBTYPE_PCM)) {
				WCHAR szAudioFilter[128];
				CLSID idAudioFilter;

				for (int i=0;i<FilterFinder.GetFilterCount();i++) {
					if (FilterFinder.GetFilterInfo(i,&idAudioFilter,szAudioFilter,128)) {
						if (::lstrcmpi(m_pszAudioFilterName,szAudioFilter)!=0)
							continue;
						hr=DirectShowUtil::AppendFilterAndConnect(m_pFilterGraph,
								idAudioFilter,szAudioFilter,&m_pAudioFilter,
								&pOutputAudio,NULL,true);
						if (SUCCEEDED(hr)) {
							TRACE(TEXT("�����t�B���^�ڑ� : %s\n"),szAudioFilter);
							bConnectSuccess=true;
						}
						break;
					}
				}
			}
			if (!bConnectSuccess) {
				throw CBonException(hr,
					TEXT("�����t�B���^���t�B���^�O���t�ɒǉ��ł��܂���B"),
					TEXT("�����t�B���^�����p�ł��Ȃ����A�����f�o�C�X�ɑΉ����Ă��Ȃ��\��������܂��B"));
			}
		}

		// �f���f�R�[�_�̐ڑ�
		switch (VideoStreamType) {
#ifdef BONTSENGINE_MPEG2_SUPPORT
		case STREAM_TYPE_MPEG2_VIDEO:
			ConnectVideoDecoder(TEXT("MPEG-2"), MEDIASUBTYPE_MPEG2_VIDEO,
								pszVideoDecoder, &pOutputVideo);
			break;
#endif	// BONTSENGINE_MPEG2_SUPPORT

#ifdef BONTSENGINE_H264_SUPPORT
		case STREAM_TYPE_H264:
			ConnectVideoDecoder(TEXT("H.264"), MEDIASUBTYPE_H264,
								pszVideoDecoder, &pOutputVideo);
			break;
#endif	// BONTSENGINE_H264_SUPPORT

#ifdef BONTSENGINE_H265_SUPPORT
		case STREAM_TYPE_H265:
			ConnectVideoDecoder(TEXT("H.265"), MEDIASUBTYPE_HEVC,
								pszVideoDecoder, &pOutputVideo);
			break;
#endif	// BONTSENGINE_H265_SUPPORT
		}

		m_VideoStreamType = VideoStreamType;

		if (!bNoVideo) {
			Trace(CTracer::TYPE_INFORMATION, TEXT("�f�������_���̍\�z��..."));

			if (!CVideoRenderer::CreateRenderer(RendererType, &m_pVideoRenderer)) {
				throw CBonException(TEXT("�f�������_�����쐬�ł��܂���B"),
									TEXT("�ݒ�ŗL���ȃ����_�����I������Ă��邩�m�F���Ă��������B"));
			}
			if (!m_pVideoRenderer->Initialize(m_pFilterGraph, pOutputVideo,
											  hOwnerHwnd, hMessageDrainHwnd)) {
				throw CBonException(m_pVideoRenderer->GetLastErrorException());
			}
			m_VideoRendererType = RendererType;
		}

		Trace(CTracer::TYPE_INFORMATION, TEXT("���������_���̍\�z��..."));

		// ���������_���\�z
		{
			bool fOK = false;

			if (pszAudioDevice != NULL && pszAudioDevice[0] != '\0') {
				CDirectShowDeviceEnumerator DevEnum;

				if (DevEnum.CreateFilter(CLSID_AudioRendererCategory,
										 pszAudioDevice, &m_pAudioRenderer)) {
					m_pszAudioRendererName=StdUtil::strdup(pszAudioDevice);
					fOK = true;
				}
			}
			if (!fOK) {
				hr = ::CoCreateInstance(CLSID_DSoundRender, NULL,
										CLSCTX_INPROC_SERVER, IID_IBaseFilter,
										pointer_cast<LPVOID*>(&m_pAudioRenderer));
				if (SUCCEEDED(hr)) {
					m_pszAudioRendererName=StdUtil::strdup(TEXT("DirectSound Renderer"));
					fOK = true;
				}
			}
			if (fOK) {
				hr = DirectShowUtil::AppendFilterAndConnect(m_pFilterGraph,
						m_pAudioRenderer, L"Audio Renderer", &pOutputAudio);
				if (SUCCEEDED(hr)) {
#ifdef _DEBUG
					if (pszAudioDevice != NULL && pszAudioDevice[0] != '\0')
						TRACE(TEXT("�����f�o�C�X %s ��ڑ�\n"), pszAudioDevice);
#endif
					if (m_bUseAudioRendererClock) {
						IMediaFilter *pMediaFilter;

						if (SUCCEEDED(m_pFilterGraph->QueryInterface(IID_IMediaFilter,
								pointer_cast<void**>(&pMediaFilter)))) {
							IReferenceClock *pReferenceClock;

							if (SUCCEEDED(m_pAudioRenderer->QueryInterface(IID_IReferenceClock,
									pointer_cast<void**>(&pReferenceClock)))) {
								pMediaFilter->SetSyncSource(pReferenceClock);
								pReferenceClock->Release();
								TRACE(TEXT("�O���t�̃N���b�N�ɉ��������_����I��\n"));
							}
							pMediaFilter->Release();
						}
					}
					fOK = true;
				} else {
					fOK = false;
				}
				if (!fOK) {
					hr = m_pFilterGraph->Render(pOutputAudio);
					if (FAILED(hr))
						throw CBonException(hr, TEXT("���������_����ڑ��ł��܂���B"),
							TEXT("�ݒ�ŗL���ȉ����f�o�C�X���I������Ă��邩�m�F���Ă��������B"));
				}
			} else {
				// �����f�o�C�X������?
				// Null�����_�����q���Ă���
				hr = ::CoCreateInstance(CLSID_NullRenderer, NULL,
										CLSCTX_INPROC_SERVER, IID_IBaseFilter,
										pointer_cast<LPVOID*>(&m_pAudioRenderer));
				if (SUCCEEDED(hr)) {
					hr = DirectShowUtil::AppendFilterAndConnect(m_pFilterGraph,
						m_pAudioRenderer, L"Null Audio Renderer", &pOutputAudio);
					if (FAILED(hr)) {
						throw CBonException(hr, TEXT("Null���������_����ڑ��ł��܂���B"));
					}
					m_pszAudioRendererName=StdUtil::strdup(TEXT("Null Renderer"));
					TRACE(TEXT("Null�����_����ڑ�\n"));
				}
			}
		}

		/*
			�f�t�H���g��MPEG-2 Demultiplexer���O���t�̃N���b�N��
			�ݒ肳���炵�����A�ꉞ�ݒ肵�Ă���
		*/
		if (!m_bUseAudioRendererClock) {
			IMediaFilter *pMediaFilter;

			if (SUCCEEDED(m_pFilterGraph->QueryInterface(
					IID_IMediaFilter,pointer_cast<void**>(&pMediaFilter)))) {
				IReferenceClock *pReferenceClock;

				if (SUCCEEDED(m_pMp2DemuxFilter->QueryInterface(
						IID_IReferenceClock,pointer_cast<void**>(&pReferenceClock)))) {
					pMediaFilter->SetSyncSource(pReferenceClock);
					pReferenceClock->Release();
					TRACE(TEXT("�O���t�̃N���b�N��MPEG-2 Demultiplexer��I��\n"));
				}
				pMediaFilter->Release();
			}
		}

		// �I�[�i�E�B���h�E�ݒ�
		m_hOwnerWnd = hOwnerHwnd;
		RECT rc;
		::GetClientRect(hOwnerHwnd, &rc);
		m_wVideoWindowX = (WORD)rc.right;
		m_wVideoWindowY = (WORD)rc.bottom;

		m_bInit=true;

		if (m_pMp2DemuxVideoMap && m_wVideoEsPID != PID_INVALID) {
			if (!MapVideoPID(m_wVideoEsPID))
				m_wVideoEsPID = PID_INVALID;
		}
		if (m_wAudioEsPID != PID_INVALID) {
			if (!MapAudioPID(m_wAudioEsPID))
				m_wAudioEsPID = PID_INVALID;
		}
	} catch (CBonException &Exception) {
		SetError(Exception);
		if (Exception.GetErrorCode()!=0) {
			TCHAR szText[MAX_ERROR_TEXT_LEN+32];
			int Length;

			Length=::AMGetErrorText(Exception.GetErrorCode(),szText,MAX_ERROR_TEXT_LEN);
			StdUtil::snprintf(szText+Length,_countof(szText)-Length,
							  TEXT("\n�G���[�R�[�h(HRESULT) 0x%08X"),Exception.GetErrorCode());
			SetErrorSystemMessage(szText);
		}

		SAFE_RELEASE(pOutput);
		SAFE_RELEASE(pOutputVideo);
		SAFE_RELEASE(pOutputAudio);
		CloseViewer();

		TRACE(TEXT("�t�B���^�O���t�\�z���s : %s\n"), GetLastErrorText());
		return false;
	}

	SAFE_RELEASE(pOutputVideo);
	SAFE_RELEASE(pOutputAudio);

	SendDecoderEvent(EID_FILTER_GRAPH_INITIALIZED, m_pFilterGraph);

	ClearError();

	TRACE(TEXT("�t�B���^�O���t�\�z����\n"));
	return true;
}

void CMediaViewer::CloseViewer()
{
	CTryBlockLock Lock(&m_DecoderLock);
	Lock.TryLock(LOCK_TIMEOUT);

	/*
	if (!m_bInit)
		return;
	*/

	if (m_pFilterGraph) {
		Trace(CTracer::TYPE_INFORMATION, TEXT("�t�B���^�O���t���~���Ă��܂�..."));
		m_pFilterGraph->Abort();
		Stop();

		SendDecoderEvent(EID_FILTER_GRAPH_FINALIZE, m_pFilterGraph);
	}

	Trace(CTracer::TYPE_INFORMATION, TEXT("COM�C���X�^���X��������Ă��܂�..."));

	// COM�C���X�^���X���J������
	if (m_pVideoRenderer!=NULL) {
		m_pVideoRenderer->Finalize();
	}

	if (m_pImageMixer!=NULL) {
		delete m_pImageMixer;
		m_pImageMixer=NULL;
	}

	if (m_pszVideoDecoderName!=NULL) {
		delete [] m_pszVideoDecoderName;
		m_pszVideoDecoderName=NULL;
	}

	SAFE_RELEASE(m_pVideoDecoderFilter);

	SAFE_RELEASE(m_pAudioDecoder);

	SAFE_RELEASE(m_pAudioRenderer);

	SAFE_RELEASE(m_pVideoParserFilter);
	m_pVideoParser = NULL;

	SAFE_RELEASE(m_pMp2DemuxAudioMap);
	SAFE_RELEASE(m_pMp2DemuxVideoMap);
	SAFE_RELEASE(m_pMp2DemuxFilter);
	m_MapAudioPID = PID_INVALID;

	SAFE_RELEASE(m_pSrcFilter);

	SAFE_RELEASE(m_pAudioFilter);

	SAFE_RELEASE(m_pMediaControl);

#ifdef _DEBUG
	if(m_dwRegister!=0){
		RemoveFromRot(m_dwRegister);
		m_dwRegister = 0;
	}
#endif

	if (m_pFilterGraph) {
		Trace(CTracer::TYPE_INFORMATION, TEXT("�t�B���^�O���t��������Ă��܂�..."));
		SendDecoderEvent(EID_FILTER_GRAPH_FINALIZED, m_pFilterGraph);
#ifdef _DEBUG
		TRACE(TEXT("FilterGraph RefCount = %d\n"),DirectShowUtil::GetRefCount(m_pFilterGraph));
#endif
		SAFE_RELEASE(m_pFilterGraph);
	}

	if (m_pVideoRenderer!=NULL) {
		delete m_pVideoRenderer;
		m_pVideoRenderer=NULL;
	}

	if (m_pszAudioRendererName!=NULL) {
		delete [] m_pszAudioRendererName;
		m_pszAudioRendererName=NULL;
	}

	m_VideoStreamType = STREAM_TYPE_UNINITIALIZED;

	m_bInit=false;
}


bool CMediaViewer::IsOpen() const
{
	return m_bInit;
}


bool CMediaViewer::Play()
{
	TRACE(TEXT("CMediaViewer::Play()\n"));

	CTryBlockLock Lock(&m_DecoderLock);
	if (!Lock.TryLock(LOCK_TIMEOUT))
		return false;

	if(!m_pMediaControl)return false;

	// �t�B���^�O���t���Đ�����

	//return m_pMediaControl->Run()==S_OK;

	if (m_pMediaControl->Run()!=S_OK) {
		int i;
		OAFilterState fs;

		for (i=0;i<20;i++) {
			if (m_pMediaControl->GetState(100,&fs)==S_OK && fs==State_Running)
				return true;
		}
		return false;
	}
	return true;
}


bool CMediaViewer::Stop()
{
	TRACE(TEXT("CMediaViewer::Stop()\n"));

	CTryBlockLock Lock(&m_DecoderLock);
	if (!Lock.TryLock(LOCK_TIMEOUT))
		return false;

	if (!m_pMediaControl)
		return false;

	if (m_pSrcFilter)
		//m_pSrcFilter->Reset();
		m_pSrcFilter->Flush();

	// �t�B���^�O���t���~����
	return m_pMediaControl->Stop()==S_OK;
}


bool CMediaViewer::Pause()
{
	TRACE(TEXT("CMediaViewer::Pause()\n"));

	CTryBlockLock Lock(&m_DecoderLock);
	if (!Lock.TryLock(LOCK_TIMEOUT))
		return false;

	if (!m_pMediaControl)
		return false;

	if (m_pSrcFilter)
		//m_pSrcFilter->Reset();
		m_pSrcFilter->Flush();

	if (m_pMediaControl->Pause()!=S_OK) {
		int i;
		OAFilterState fs;
		HRESULT hr;

		for (i=0;i<20;i++) {
			hr=m_pMediaControl->GetState(100,&fs);
			if ((hr==S_OK || hr==VFW_S_CANT_CUE) && fs==State_Paused)
				return true;
		}
		return false;
	}
	return true;
}


bool CMediaViewer::Flush()
{
	TRACE(TEXT("CMediaViewer::Flush()\n"));

	/*
	CTryBlockLock Lock(&m_DecoderLock);
	if (!Lock.TryLock(LOCK_TIMEOUT))
		return false;
	*/

	if (!m_pSrcFilter)
		return false;

	m_pSrcFilter->Flush();
	return true;
}


bool CMediaViewer::SetVisible(bool fVisible)
{
	if (m_pVideoRenderer)
		return m_pVideoRenderer->SetVisible(fVisible);
	return false;
}


void CMediaViewer::HideCursor(bool bHide)
{
	if (m_pVideoRenderer)
		m_pVideoRenderer->ShowCursor(!bHide);
}


bool CMediaViewer::RepaintVideo(HWND hwnd,HDC hdc)
{
	if (m_pVideoRenderer)
		return m_pVideoRenderer->RepaintVideo(hwnd,hdc);
	return false;
}


bool CMediaViewer::DisplayModeChanged()
{
	if (m_pVideoRenderer)
		return m_pVideoRenderer->DisplayModeChanged();
	return false;
}


void CMediaViewer::Set1SegMode(bool b1Seg)
{
	if (m_b1SegMode != b1Seg) {
		TRACE(TEXT("CMediaViewer::Set1SegMode(%d)\n"), b1Seg);

		m_b1SegMode = b1Seg;

		if (m_pSrcFilter != NULL)
			m_pSrcFilter->EnableSync(m_bEnablePTSSync, m_b1SegMode);
		ApplyAdjustVideoSampleOptions();
	}
}


bool CMediaViewer::Is1SegMode() const
{
	return m_b1SegMode;
}


bool CMediaViewer::SetVideoPID(const WORD wPID)
{
	// �f���o�̓s����PID���}�b�s���O����

	CBlockLock Lock(&m_DecoderLock);

	if (wPID == m_wVideoEsPID)
		return true;

	TRACE(TEXT("CMediaViewer::SetVideoPID() %04X <- %04X\n"), wPID, m_wVideoEsPID);

	if (m_pMp2DemuxVideoMap) {
		// ���݂�PID���A���}�b�v
		if (m_wVideoEsPID != PID_INVALID) {
			ULONG TempPID = m_wVideoEsPID;
			if (m_pMp2DemuxVideoMap->UnmapPID(1UL, &TempPID) != S_OK)
				return false;
		}
	}

	if (!MapVideoPID(wPID)) {
		m_wVideoEsPID = PID_INVALID;
		return false;
	}

	m_wVideoEsPID = wPID;

	return true;
}


bool CMediaViewer::SetAudioPID(const WORD wPID, const bool bUseMap)
{
	// �����o�̓s����PID���}�b�s���O����

	CBlockLock Lock(&m_DecoderLock);

	if (wPID == m_wAudioEsPID
			&& (bUseMap || wPID == m_MapAudioPID))
		return true;

	TRACE(TEXT("CMediaViewer::SetAudioPID() %04X <- %04X\n"), wPID, m_wAudioEsPID);

	if (bUseMap && wPID != PID_INVALID && m_MapAudioPID != PID_INVALID) {
		/*
			bUseMap �� true �̏ꍇ�APID �����������ĉ����X�g���[����ύX����
			IMPEG2PIDMap::MapPID() ���ĂԂƍĐ�����u�~�܂�̂ŁA�����������邽��
		*/
		if (m_pSrcFilter)
			m_pSrcFilter->MapAudioPID(wPID, m_MapAudioPID);
	} else {
		if (m_pMp2DemuxAudioMap) {
			// ���݂�PID���A���}�b�v
			if (m_MapAudioPID != PID_INVALID) {
				ULONG TempPID = m_MapAudioPID;
				if (m_pMp2DemuxAudioMap->UnmapPID(1UL, &TempPID) != S_OK)
					return false;
				m_MapAudioPID = PID_INVALID;
			}
		}

		if (!MapAudioPID(wPID)) {
			m_wAudioEsPID = PID_INVALID;
			return false;
		}
	}

	m_wAudioEsPID = wPID;

	return true;
}


WORD CMediaViewer::GetVideoPID() const
{
	return m_wVideoEsPID;
}


WORD CMediaViewer::GetAudioPID() const
{
	return m_wAudioEsPID;
}


void CMediaViewer::OnVideoInfo(const CVideoParser::VideoInfo *pVideoInfo, const LPVOID pParam)
{
	// �r�f�I���̍X�V
	CMediaViewer *pThis=static_cast<CMediaViewer*>(pParam);

	/*if (pThis->m_VideoInfo != *pVideoInfo)*/ {
		// �r�f�I���̍X�V
		CBlockLock Lock(&pThis->m_ResizeLock);

		pThis->m_VideoInfo = *pVideoInfo;
		//pThis->AdjustVideoPosition();
	}

	pThis->SendDecoderEvent(EID_VIDEO_SIZE_CHANGED);
}


bool CMediaViewer::AdjustVideoPosition()
{
	// �f���̈ʒu�𒲐�����
	if (m_pVideoRenderer && m_wVideoWindowX > 0 && m_wVideoWindowY > 0
			&& m_VideoInfo.m_OrigWidth > 0 && m_VideoInfo.m_OrigHeight > 0) {
		long WindowWidth, WindowHeight, DestWidth, DestHeight;

		WindowWidth = m_wVideoWindowX;
		WindowHeight = m_wVideoWindowY;
		if (m_ViewStretchMode == STRETCH_FIT) {
			// �E�B���h�E�T�C�Y�ɍ��킹��
			DestWidth = WindowWidth;
			DestHeight = WindowHeight;
		} else {
			int AspectX, AspectY;

			if (m_ForceAspectX > 0 && m_ForceAspectY > 0) {
				// �A�X�y�N�g�䂪�w�肳��Ă���
				AspectX = m_ForceAspectX;
				AspectY = m_ForceAspectY;
			} else if (m_VideoInfo.m_AspectRatioX > 0 && m_VideoInfo.m_AspectRatioY > 0) {
				// �f���̃A�X�y�N�g����g�p����
				AspectX = m_VideoInfo.m_AspectRatioX;
				AspectY = m_VideoInfo.m_AspectRatioY;
				if (m_bIgnoreDisplayExtension
						&& m_VideoInfo.m_DisplayWidth > 0
						&& m_VideoInfo.m_DisplayHeight > 0) {
					AspectX = AspectX * 3 * m_VideoInfo.m_OrigWidth / m_VideoInfo.m_DisplayWidth;
					AspectY = AspectY * 3 * m_VideoInfo.m_OrigHeight / m_VideoInfo.m_DisplayHeight;
				}
			} else {
				// �A�X�y�N�g��s��
				if (m_VideoInfo.m_DisplayHeight == 1080) {
					AspectX = 16;
					AspectY = 9;
				} else if (m_VideoInfo.m_DisplayWidth > 0 && m_VideoInfo.m_DisplayHeight > 0) {
					AspectX = m_VideoInfo.m_DisplayWidth;
					AspectY = m_VideoInfo.m_DisplayHeight;
				} else {
					AspectX = WindowWidth;
					AspectY = WindowHeight;
				}
			}
			double WindowRatio = (double)WindowWidth / (double)WindowHeight;
			double AspectRatio = (double)AspectX / (double)AspectY;
			if ((m_ViewStretchMode == STRETCH_KEEPASPECTRATIO && AspectRatio > WindowRatio)
					|| (m_ViewStretchMode == STRETCH_CUTFRAME && AspectRatio < WindowRatio)) {
				DestWidth = WindowWidth;
				DestHeight = ::MulDiv(DestWidth, AspectY, AspectX);
			} else {
				DestHeight = WindowHeight;
				DestWidth = ::MulDiv(DestHeight, AspectX, AspectY);
			}
		}

		RECT rcSrc,rcDst,rcWindow;
		CalcSourceRect(&rcSrc);
#if 0
		// ���W�l���}�C�i�X�ɂȂ�ƃ}���`�f�B�X�v���C�ł��������Ȃ�?
		rcDst.left = (WindowWidth - DestWidth) / 2;
		rcDst.top = (WindowHeight - DestHeight) / 2,
		rcDst.right = rcDst.left + DestWidth;
		rcDst.bottom = rcDst.top + DestHeight;
#else
		if (WindowWidth < DestWidth) {
			rcDst.left = 0;
			rcDst.right = WindowWidth;
			rcSrc.left += ::MulDiv(DestWidth - WindowWidth, rcSrc.right - rcSrc.left, DestWidth) / 2;
			rcSrc.right = m_VideoInfo.m_OrigWidth - rcSrc.left;
		} else {
			if (m_bNoMaskSideCut
					&& WindowWidth > DestWidth
					&& rcSrc.right - rcSrc.left < m_VideoInfo.m_OrigWidth) {
				int NewDestWidth = ::MulDiv(m_VideoInfo.m_OrigWidth, DestWidth, rcSrc.right-rcSrc.left);
				if (NewDestWidth > WindowWidth)
					NewDestWidth = WindowWidth;
				int NewSrcWidth = ::MulDiv(rcSrc.right-rcSrc.left, NewDestWidth, DestWidth);
				rcSrc.left = (m_VideoInfo.m_OrigWidth - NewSrcWidth) / 2;
				rcSrc.right = rcSrc.left + NewSrcWidth;
				TRACE(TEXT("Adjust %d x %d -> %d x %d [%d - %d (%d)]\n"),
					  DestWidth, DestHeight, NewDestWidth, DestHeight,
					  rcSrc.left, rcSrc.right, NewSrcWidth);
				DestWidth = NewDestWidth;
			}
			rcDst.left = (WindowWidth - DestWidth) / 2;
			rcDst.right = rcDst.left + DestWidth;
		}
		if (WindowHeight < DestHeight) {
			rcDst.top = 0;
			rcDst.bottom = WindowHeight;
			rcSrc.top += ::MulDiv(DestHeight - WindowHeight, rcSrc.bottom - rcSrc.top, DestHeight) / 2;
			rcSrc.bottom = m_VideoInfo.m_OrigHeight - rcSrc.top;
		} else {
			rcDst.top = (WindowHeight - DestHeight) / 2,
			rcDst.bottom = rcDst.top + DestHeight;
		}
#endif

		rcWindow.left = 0;
		rcWindow.top = 0;
		rcWindow.right = WindowWidth;
		rcWindow.bottom = WindowHeight;

#if 0
		TRACE(TEXT("SetVideoPosition %d,%d,%d,%d -> %d,%d,%d,%d [%d,%d,%d,%d]\n"),
			  rcSrc.left, rcSrc.top, rcSrc.right, rcSrc.bottom,
			  rcDst.left, rcDst.top, rcDst.right, rcDst.bottom,
			  rcWindow.left, rcWindow.top, rcWindow.right, rcWindow.bottom);
#endif

		return m_pVideoRenderer->SetVideoPosition(
			m_VideoInfo.m_OrigWidth, m_VideoInfo.m_OrigHeight, &rcSrc, &rcDst, &rcWindow);
	}

	return false;
}


// �f���E�B���h�E�̃T�C�Y��ݒ肷��
bool CMediaViewer::SetViewSize(const int Width, const int Height)
{
	CBlockLock Lock(&m_ResizeLock);

	if (Width > 0 && Height > 0) {
		m_wVideoWindowX = Width;
		m_wVideoWindowY = Height;
		return AdjustVideoPosition();
	}
	return false;
}


// �f���̃T�C�Y���擾����
bool CMediaViewer::GetVideoSize(WORD *pwWidth, WORD *pwHeight) const
{
	if (m_bIgnoreDisplayExtension)
		return GetOriginalVideoSize(pwWidth, pwHeight);

	CBlockLock Lock(&m_ResizeLock);

	if (m_VideoInfo.m_DisplayWidth > 0 && m_VideoInfo.m_DisplayHeight > 0) {
		if (pwWidth)
			*pwWidth = m_VideoInfo.m_DisplayWidth;
		if (pwHeight)
			*pwHeight = m_VideoInfo.m_DisplayHeight;
		return true;
	}
	return false;
}


// �f���̃A�X�y�N�g����擾����
bool CMediaViewer::GetVideoAspectRatio(BYTE *pbyAspectRatioX, BYTE *pbyAspectRatioY) const
{
	CBlockLock Lock(&m_ResizeLock);

	if (m_VideoInfo.m_AspectRatioX > 0 && m_VideoInfo.m_AspectRatioY > 0) {
		if (pbyAspectRatioX)
			*pbyAspectRatioX = m_VideoInfo.m_AspectRatioX;
		if (pbyAspectRatioY)
			*pbyAspectRatioY = m_VideoInfo.m_AspectRatioY;
		return true;
	}
	return false;
}


// �f���̃A�X�y�N�g���ݒ肷��
bool CMediaViewer::ForceAspectRatio(int AspectX, int AspectY)
{
	m_ForceAspectX=AspectX;
	m_ForceAspectY=AspectY;
	return true;
}


// �ݒ肳�ꂽ�A�X�y�N�g����擾����
bool CMediaViewer::GetForceAspectRatio(int *pAspectX, int *pAspectY) const
{
	if (pAspectX)
		*pAspectX=m_ForceAspectX;
	if (pAspectY)
		*pAspectY=m_ForceAspectY;
	return true;
}


// �L���ȃA�X�y�N�g����擾����
bool CMediaViewer::GetEffectiveAspectRatio(BYTE *pAspectX, BYTE *pAspectY) const
{
	if (m_ForceAspectX > 0 && m_ForceAspectY > 0) {
		if (pAspectX)
			*pAspectX = m_ForceAspectX;
		if (pAspectY)
			*pAspectY = m_ForceAspectY;
		return true;
	}
	BYTE AspectX, AspectY;
	if (!GetVideoAspectRatio(&AspectX, &AspectY))
		return false;
	if (m_bIgnoreDisplayExtension
			&& (m_VideoInfo.m_DisplayWidth != m_VideoInfo.m_OrigWidth
				|| m_VideoInfo.m_DisplayHeight != m_VideoInfo.m_OrigHeight)) {
		if (m_VideoInfo.m_DisplayWidth == 0
				|| m_VideoInfo.m_DisplayHeight == 0)
			return false;
		AspectX = AspectX * 3 * m_VideoInfo.m_OrigWidth / m_VideoInfo.m_DisplayWidth;
		AspectY = AspectY * 3 * m_VideoInfo.m_OrigHeight / m_VideoInfo.m_DisplayHeight;
		if (AspectX % 4 == 0 && AspectY % 4 == 0) {
			AspectX /= 4;
			AspectY /= 4;
		}
	}
	if (pAspectX)
		*pAspectX = AspectX;
	if (pAspectY)
		*pAspectY = AspectY;
	return true;
}


bool CMediaViewer::SetPanAndScan(int AspectX,int AspectY,const ClippingInfo *pClipping)
{
	if (m_ForceAspectX!=AspectX || m_ForceAspectY!=AspectY || pClipping!=NULL) {
		CBlockLock Lock(&m_ResizeLock);

		m_ForceAspectX=AspectX;
		m_ForceAspectY=AspectY;
		if (pClipping!=NULL)
			m_Clipping=*pClipping;
		else
			::ZeroMemory(&m_Clipping,sizeof(m_Clipping));
		AdjustVideoPosition();
	}
	return true;
}


bool CMediaViewer::GetClippingInfo(ClippingInfo *pClipping) const
{
	if (pClipping==NULL)
		return false;
	*pClipping=m_Clipping;
	return true;
}


bool CMediaViewer::SetViewStretchMode(ViewStretchMode Mode)
{
	if (m_ViewStretchMode!=Mode) {
		CBlockLock Lock(&m_ResizeLock);

		m_ViewStretchMode=Mode;
		return AdjustVideoPosition();
	}
	return true;
}


bool CMediaViewer::SetNoMaskSideCut(bool bNoMask, bool bAdjust)
{
	if (m_bNoMaskSideCut != bNoMask) {
		CBlockLock Lock(&m_ResizeLock);

		m_bNoMaskSideCut = bNoMask;
		if (bAdjust)
			AdjustVideoPosition();
	}
	return true;
}


bool CMediaViewer::SetIgnoreDisplayExtension(bool bIgnore)
{
	if (bIgnore != m_bIgnoreDisplayExtension) {
		CBlockLock Lock(&m_ResizeLock);

		m_bIgnoreDisplayExtension = bIgnore;
		if (m_VideoInfo.m_DisplayWidth != m_VideoInfo.m_OrigWidth
				|| m_VideoInfo.m_DisplayHeight != m_VideoInfo.m_OrigHeight)
			AdjustVideoPosition();
	}
	return true;
}


bool CMediaViewer::GetOriginalVideoSize(WORD *pWidth, WORD *pHeight) const
{
	CBlockLock Lock(&m_ResizeLock);

	if (m_VideoInfo.m_OrigWidth > 0 && m_VideoInfo.m_OrigHeight > 0) {
		if (pWidth)
			*pWidth = m_VideoInfo.m_OrigWidth;
		if (pHeight)
			*pHeight = m_VideoInfo.m_OrigHeight;
		return true;
	}
	return false;
}


bool CMediaViewer::GetCroppedVideoSize(WORD *pWidth, WORD *pHeight) const
{
	RECT rc;

	if (!GetSourceRect(&rc))
		return false;
	if (pWidth)
		*pWidth = (WORD)(rc.right - rc.left);
	if (pHeight)
		*pHeight = (WORD)(rc.bottom - rc.top);
	return true;
}


bool CMediaViewer::GetSourceRect(RECT *pRect) const
{
	CBlockLock Lock(&m_ResizeLock);

	if (!pRect)
		return false;
	return CalcSourceRect(pRect);
}


bool CMediaViewer::CalcSourceRect(RECT *pRect) const
{
	if (m_VideoInfo.m_OrigWidth == 0 || m_VideoInfo.m_OrigHeight == 0)
		return false;

	long SrcX, SrcY, SrcWidth, SrcHeight;

	if (m_Clipping.HorzFactor != 0) {
		long ClipLeft, ClipRight;
		ClipLeft = ::MulDiv(m_VideoInfo.m_OrigWidth, m_Clipping.Left, m_Clipping.HorzFactor);
		ClipRight = ::MulDiv(m_VideoInfo.m_OrigWidth, m_Clipping.Right, m_Clipping.HorzFactor);
		SrcWidth = m_VideoInfo.m_OrigWidth - (ClipLeft + ClipRight);
		SrcX = ClipLeft;
	} else if (m_bIgnoreDisplayExtension) {
		SrcWidth = m_VideoInfo.m_OrigWidth;
		SrcX = 0;
	} else {
		SrcWidth = m_VideoInfo.m_DisplayWidth;
		SrcX = m_VideoInfo.m_PosX;
	}
	if (m_Clipping.VertFactor != 0) {
		long ClipTop, ClipBottom;
		ClipTop = ::MulDiv(m_VideoInfo.m_OrigHeight, m_Clipping.Top, m_Clipping.VertFactor);
		ClipBottom = ::MulDiv(m_VideoInfo.m_OrigHeight, m_Clipping.Bottom, m_Clipping.VertFactor);
		SrcHeight = m_VideoInfo.m_OrigHeight - (ClipTop + ClipBottom);
		SrcY = ClipTop;
	} else if (m_bIgnoreDisplayExtension) {
		SrcHeight = m_VideoInfo.m_OrigHeight;
		SrcY = 0;
	} else {
		SrcHeight = m_VideoInfo.m_DisplayHeight;
		SrcY = m_VideoInfo.m_PosY;
	}

	pRect->left = SrcX;
	pRect->top = SrcY;
	pRect->right = SrcX + SrcWidth;
	pRect->bottom = SrcY + SrcHeight;

	return true;
}


bool CMediaViewer::GetDestRect(RECT *pRect) const
{
	if (m_pVideoRenderer && pRect) {
		if (m_pVideoRenderer->GetDestPosition(pRect))
			return true;
	}
	return false;
}


bool CMediaViewer::GetDestSize(WORD *pWidth, WORD *pHeight) const
{
	RECT rc;

	if (!GetDestRect(&rc))
		return false;
	if (pWidth)
		*pWidth=(WORD)(rc.right-rc.left);
	if (pHeight)
		*pHeight=(WORD)(rc.bottom-rc.top);
	return true;
}


bool CMediaViewer::SetVolume(const float fVolume)
{
	// �I�[�f�B�I�{�����[����dB�Őݒ肷��( -100.0(����) < fVolume < 0(�ő�) )
	IBasicAudio *pBasicAudio;
	bool fOK=false;

	if (m_pFilterGraph) {
		if (SUCCEEDED(m_pFilterGraph->QueryInterface(
				IID_IBasicAudio, pointer_cast<void**>(&pBasicAudio)))) {
			long lVolume = (long)(fVolume * 100.0f);

			if (lVolume>=-10000 && lVolume<=0) {
					TRACE(TEXT("Volume Control = %d\n"),lVolume);
				if (SUCCEEDED(pBasicAudio->put_Volume(lVolume)))
					fOK=true;
			}
			pBasicAudio->Release();
		}
	}
	return fOK;
}


BYTE CMediaViewer::GetAudioChannelNum() const
{
	// �I�[�f�B�I�̓��̓`�����l�������擾����
	if (m_pAudioDecoder)
		return m_pAudioDecoder->GetCurrentChannelNum();
	return AUDIO_CHANNEL_INVALID;
}


bool CMediaViewer::SetDualMonoMode(CAudioDecFilter::DualMonoMode Mode)
{
	if (m_pAudioDecoder)
		return m_pAudioDecoder->SetDualMonoMode(Mode);
	return false;
}


CAudioDecFilter::DualMonoMode CMediaViewer::GetDualMonoMode() const
{
	if (m_pAudioDecoder)
		return m_pAudioDecoder->GetDualMonoMode();
	return CAudioDecFilter::DUALMONO_INVALID;
}


bool CMediaViewer::SetStereoMode(CAudioDecFilter::StereoMode Mode)
{
	// �X�e���I�o�̓`�����l���̐ݒ�
	if (m_pAudioDecoder)
		return m_pAudioDecoder->SetStereoMode(Mode);
	return false;
}


CAudioDecFilter::StereoMode CMediaViewer::GetStereoMode() const
{
	if (m_pAudioDecoder)
		return m_pAudioDecoder->GetStereoMode();
	return CAudioDecFilter::STEREOMODE_STEREO;
}


bool CMediaViewer::SetSpdifOptions(const CAudioDecFilter::SpdifOptions *pOptions)
{
	if (m_pAudioDecoder)
		return m_pAudioDecoder->SetSpdifOptions(pOptions);
	return false;
}


bool CMediaViewer::GetSpdifOptions(CAudioDecFilter::SpdifOptions *pOptions) const
{
	if (m_pAudioDecoder)
		return m_pAudioDecoder->GetSpdifOptions(pOptions);
	return false;
}


bool CMediaViewer::IsSpdifPassthrough() const
{
	if (m_pAudioDecoder)
		return m_pAudioDecoder->IsSpdifPassthrough();
	return false;
}


bool CMediaViewer::SetDownMixSurround(bool bDownMix)
{
	if (m_pAudioDecoder)
		return m_pAudioDecoder->SetDownMixSurround(bDownMix);
	return false;
}


bool CMediaViewer::GetDownMixSurround() const
{
	if (m_pAudioDecoder)
		return m_pAudioDecoder->GetDownMixSurround();
	return false;
}


bool CMediaViewer::SetAudioGainControl(bool bGainControl, float Gain, float SurroundGain)
{
	if (m_pAudioDecoder==NULL)
		return false;
	return m_pAudioDecoder->SetGainControl(bGainControl, Gain, SurroundGain);
}


bool CMediaViewer::GetAudioDecFilter(CAudioDecFilter **ppFilter)
{
	if (ppFilter == NULL)
		return false;

	if (m_pAudioDecoder == NULL) {
		*ppFilter = NULL;
		return false;
	}

	*ppFilter = m_pAudioDecoder;
	m_pAudioDecoder->AddRef();

	return true;
}


bool CMediaViewer::GetVideoDecoderName(LPWSTR pszName, int Length) const
{
	// �I������Ă���r�f�I�f�R�[�_�[���̎擾
	if (pszName == NULL || Length < 1)
		return false;

	if (m_pszVideoDecoderName == NULL) {
		pszName[0] = '\0';
		return false;
	}

	::lstrcpynW(pszName, m_pszVideoDecoderName, Length);
	return true;
}


bool CMediaViewer::GetVideoRendererName(LPTSTR pszName, int Length) const
{
	if (pszName == NULL || Length < 1)
		return false;

	LPCTSTR pszRenderer = CVideoRenderer::EnumRendererName((int)m_VideoRendererType);
	if (pszRenderer == NULL) {
		pszName[0] = '\0';
		return false;
	}

	::lstrcpyn(pszName, pszRenderer, Length);
	return true;
}


bool CMediaViewer::GetAudioRendererName(LPWSTR pszName, int Length) const
{
	if (pszName == NULL || Length < 1)
		return false;

	if (m_pszAudioRendererName==NULL) {
		pszName[0] = '\0';
		return false;
	}

	::lstrcpyn(pszName, m_pszAudioRendererName, Length);
	return true;
}


CVideoRenderer::RendererType CMediaViewer::GetVideoRendererType() const
{
	return m_VideoRendererType;
}


BYTE CMediaViewer::GetVideoStreamType() const
{
	return m_VideoStreamType;
}


bool CMediaViewer::DisplayFilterProperty(PropertyFilter Filter, HWND hwndOwner)
{
	switch (Filter) {
	case PROPERTY_FILTER_VIDEODECODER:
		if (m_pVideoDecoderFilter)
			return DirectShowUtil::ShowPropertyPage(m_pVideoDecoderFilter,hwndOwner);
		break;
	case PROPERTY_FILTER_VIDEORENDERER:
		if (m_pVideoRenderer)
			return m_pVideoRenderer->ShowProperty(hwndOwner);
		break;
	case PROPERTY_FILTER_MPEG2DEMULTIPLEXER:
		if (m_pMp2DemuxFilter)
			return DirectShowUtil::ShowPropertyPage(m_pMp2DemuxFilter,hwndOwner);
		break;
	case PROPERTY_FILTER_AUDIOFILTER:
		if (m_pAudioFilter)
			return DirectShowUtil::ShowPropertyPage(m_pAudioFilter,hwndOwner);
		break;
	case PROPERTY_FILTER_AUDIORENDERER:
		if (m_pAudioRenderer)
			return DirectShowUtil::ShowPropertyPage(m_pAudioRenderer,hwndOwner);
		break;
	}
	return false;
}

bool CMediaViewer::FilterHasProperty(PropertyFilter Filter)
{
	switch (Filter) {
	case PROPERTY_FILTER_VIDEODECODER:
		if (m_pVideoDecoderFilter)
			return DirectShowUtil::HasPropertyPage(m_pVideoDecoderFilter);
		break;
	case PROPERTY_FILTER_VIDEORENDERER:
		if (m_pVideoRenderer)
			return m_pVideoRenderer->HasProperty();
		break;
	case PROPERTY_FILTER_MPEG2DEMULTIPLEXER:
		if (m_pMp2DemuxFilter)
			return DirectShowUtil::HasPropertyPage(m_pMp2DemuxFilter);
		break;
	case PROPERTY_FILTER_AUDIOFILTER:
		if (m_pAudioFilter)
			return DirectShowUtil::HasPropertyPage(m_pAudioFilter);
		break;
	case PROPERTY_FILTER_AUDIORENDERER:
		if (m_pAudioRenderer)
			return DirectShowUtil::HasPropertyPage(m_pAudioRenderer);
		break;
	}
	return false;
}


bool CMediaViewer::SetUseAudioRendererClock(bool bUse)
{
	m_bUseAudioRendererClock = bUse;
	return true;
}


bool CMediaViewer::SetAdjustAudioStreamTime(bool bAdjust)
{
	m_bAdjustAudioStreamTime = bAdjust;
	if (m_pAudioDecoder == NULL)
		return true;
	return m_pAudioDecoder->SetJitterCorrection(bAdjust);
}


bool CMediaViewer::SetAudioStreamCallback(CAudioDecFilter::StreamCallback pCallback, void *pParam)
{
	m_pAudioStreamCallback=pCallback;
	m_pAudioStreamCallbackParam=pParam;
	if (m_pAudioDecoder == NULL)
		return true;
	return m_pAudioDecoder->SetStreamCallback(pCallback, pParam);
}


bool CMediaViewer::SetAudioFilter(LPCWSTR pszFilterName)
{
	if (m_pszAudioFilterName) {
		delete [] m_pszAudioFilterName;
		m_pszAudioFilterName = NULL;
	}
	if (pszFilterName && pszFilterName[0] != '\0')
		m_pszAudioFilterName = StdUtil::strdup(pszFilterName);
	return true;
}


bool CMediaViewer::GetCurrentImage(BYTE **ppDib)
{
	bool fOK=false;

	if (m_pVideoRenderer) {
		void *pBuffer;

		if (m_pVideoRenderer->GetCurrentImage(&pBuffer)) {
			fOK=true;
			*ppDib=static_cast<BYTE*>(pBuffer);
		}
	}
	return fOK;
}


bool CMediaViewer::DrawText(LPCTSTR pszText,int x,int y,
							HFONT hfont,COLORREF crColor,int Opacity)
{
	IBaseFilter *pRenderer;
	int Width,Height;

	if (m_pVideoRenderer==NULL || !IsDrawTextSupported())
		return false;
	pRenderer=m_pVideoRenderer->GetRendererFilter();
	if (pRenderer==NULL)
		return false;
	if (m_pImageMixer==NULL) {
		m_pImageMixer=CImageMixer::CreateImageMixer(m_VideoRendererType,pRenderer);
		if (m_pImageMixer==NULL)
			return false;
	}
	if (!m_pImageMixer->GetMapSize(&Width,&Height))
		return false;
	m_ResizeLock.Lock();
	if (m_VideoInfo.m_OrigWidth==0 || m_VideoInfo.m_OrigHeight==0)
		return false;
	x=x*Width/m_VideoInfo.m_OrigWidth;
	y=y*Height/m_VideoInfo.m_OrigHeight;
	m_ResizeLock.Unlock();
	return m_pImageMixer->SetText(pszText,x,y,hfont,crColor,Opacity);
}


bool CMediaViewer::IsDrawTextSupported() const
{
	return CImageMixer::IsSupported(m_VideoRendererType);
}


bool CMediaViewer::ClearOSD()
{
	if (m_pVideoRenderer==NULL)
		return false;
	if (m_pImageMixer!=NULL)
		m_pImageMixer->Clear();
	return true;
}


bool CMediaViewer::EnablePTSSync(bool bEnable)
{
	TRACE(TEXT("CMediaViewer::EnablePTSSync(%s)\n"), bEnable ? TEXT("true") : TEXT("false"));
	if (m_pSrcFilter != NULL) {
		if (!m_pSrcFilter->EnableSync(bEnable, m_b1SegMode))
			return false;
	}
	m_bEnablePTSSync = bEnable;
	return true;
}


bool CMediaViewer::IsPTSSyncEnabled() const
{
	/*
	if (m_pSrcFilter != NULL)
		return m_pSrcFilter->IsSyncEnabled();
	*/
	return m_bEnablePTSSync;
}


bool CMediaViewer::SetAdjust1SegVideoSample(bool bAdjustTime, bool bAdjustFrameRate)
{
	TRACE(TEXT("CMediaViewer::SetAdjust1SegVideoSample() : Adjust time %d / Adjust frame rate %d\n"),
		  bAdjustTime, bAdjustFrameRate);

	m_bAdjust1SegVideoSampleTime = bAdjustTime;
	m_bAdjust1SegFrameRate = bAdjustFrameRate;
	ApplyAdjustVideoSampleOptions();

	return true;
}


void CMediaViewer::ResetBuffer()
{
	if (m_pSrcFilter != NULL)
		m_pSrcFilter->Reset();
}


bool CMediaViewer::SetBufferSize(size_t Size)
{
	TRACE(TEXT("CMediaViewer::SetBufferSize(%Iu)\n"), Size);

	if (m_pSrcFilter != NULL) {
		if (!m_pSrcFilter->SetBufferSize(Size))
			return false;
	}

	m_BufferSize = Size;

	return true;
}


bool CMediaViewer::SetInitialPoolPercentage(int Percentage)
{
	TRACE(TEXT("CMediaViewer::SetInitialPoolPercentage(%d)\n"), Percentage);

	if (m_pSrcFilter != NULL) {
		if (!m_pSrcFilter->SetInitialPoolPercentage(Percentage))
			return false;
	}

	m_InitialPoolPercentage = Percentage;

	return true;
}


int CMediaViewer::GetBufferFillPercentage() const
{
	if (m_pSrcFilter != NULL)
		return m_pSrcFilter->GetBufferFillPercentage();
	return 0;
}


bool CMediaViewer::SetPacketInputWait(DWORD Wait)
{
	TRACE(TEXT("CMediaViewer::SetPacketInputWait(%u)\n"), Wait);

	if (m_pSrcFilter != NULL) {
		if (!m_pSrcFilter->SetInputWait(Wait))
			return false;
	}

	m_PacketInputWait = Wait;

	return true;
}


DWORD CMediaViewer::GetAudioBitRate() const
{
	if (m_pAudioDecoder != NULL)
		return m_pAudioDecoder->GetBitRate();
	return 0;
}


DWORD CMediaViewer::GetVideoBitRate() const
{
	if (m_pVideoParser != NULL)
		return m_pVideoParser->GetBitRate();
	return 0;
}


void CMediaViewer::ConnectVideoDecoder(
	LPCTSTR pszCodecName, const GUID &MediaSubType, LPCTSTR pszDecoderName, IPin **ppOutputPin)
{
	Trace(CTracer::TYPE_INFORMATION, TEXT("%s�f�R�[�_�̐ڑ���..."), pszCodecName);

	CDirectShowFilterFinder FilterFinder;
	TCHAR szText1[128], szText2[128];

	// ����
	if(!FilterFinder.FindFilter(&MEDIATYPE_Video, &MediaSubType)) {
		StdUtil::snprintf(szText1, _countof(szText1),
						  TEXT("%s�f�R�[�_�����t����܂���B"), pszCodecName);
		StdUtil::snprintf(szText2, _countof(szText2),
						  TEXT("%s�f�R�[�_���C���X�g�[������Ă��邩�m�F���Ă��������B"), pszCodecName);
		throw CBonException(szText1, szText2);
	}

	bool bConnectSuccess = false;
	WCHAR szFilter[256];
	HRESULT hr;

	for (int i = 0; i < FilterFinder.GetFilterCount(); i++) {
		CLSID clsidFilter;

		if (FilterFinder.GetFilterInfo(i, &clsidFilter, szFilter, _countof(szFilter))) {
			if (pszDecoderName != NULL && pszDecoderName[0] != '\0'
					&& ::lstrcmpi(szFilter, pszDecoderName) != 0)
				continue;
			hr = DirectShowUtil::AppendFilterAndConnect(m_pFilterGraph,
				clsidFilter, szFilter, &m_pVideoDecoderFilter,
				ppOutputPin, NULL, true);
			if (SUCCEEDED(hr)) {
				bConnectSuccess = true;
				break;
			}
		}
	}

	// �ǂꂩ�̃t�B���^�Őڑ��ł�����
	if (bConnectSuccess) {
		m_pszVideoDecoderName = StdUtil::strdup(szFilter);
	} else {
		StdUtil::snprintf(szText1, _countof(szText1),
						  TEXT("%s�f�R�[�_�t�B���^���t�B���^�O���t�ɒǉ��ł��܂���B"),
						  pszCodecName);
		throw CBonException(hr, szText1,
			TEXT("�ݒ�ŗL���ȃf�R�[�_���I������Ă��邩�m�F���Ă��������B\n�܂��A�����_����ς��Ă݂Ă��������B"));
	}
}


bool CMediaViewer::MapVideoPID(WORD PID)
{
	if (m_pMp2DemuxVideoMap) {
		// �V�K��PID���}�b�v
		if (PID != PID_INVALID) {
			ULONG TempPID = PID;
			if (m_pMp2DemuxVideoMap->MapPID(1UL, &TempPID, MEDIA_ELEMENTARY_STREAM) != S_OK)
				return false;
		}
	}

	if (m_pSrcFilter)
		m_pSrcFilter->SetVideoPID(PID);

	return true;
}


bool CMediaViewer::MapAudioPID(WORD PID)
{
	if (m_pMp2DemuxAudioMap) {
		// �V�K��PID���}�b�v
		if (PID != PID_INVALID) {
			ULONG TempPID = PID;
			if (m_pMp2DemuxAudioMap->MapPID(1UL, &TempPID, MEDIA_ELEMENTARY_STREAM) != S_OK)
				return false;
			m_MapAudioPID = PID;
		}
	}

	if (m_pSrcFilter)
		m_pSrcFilter->SetAudioPID(PID);

	return true;
}


void CMediaViewer::ApplyAdjustVideoSampleOptions()
{
	if (m_pVideoParser != NULL) {
		unsigned int Flags = 0;

		if (m_b1SegMode) {
			Flags = CVideoParser::ADJUST_SAMPLE_1SEG;
			if (m_bAdjust1SegVideoSampleTime)
				Flags |= CVideoParser::ADJUST_SAMPLE_TIME;
			if (m_bAdjust1SegFrameRate)
				Flags |= CVideoParser::ADJUST_SAMPLE_FRAME_RATE;
		}

		m_pVideoParser->SetAdjustSampleOptions(Flags);
	}
}


void CMediaViewer::OnSpdifPassthroughError(HRESULT hr)
{
	SendDecoderEvent(EID_SPDIF_PASSTHROUGH_ERROR, &hr);
}


#ifdef _DEBUG

HRESULT CMediaViewer::AddToRot(IUnknown *pUnkGraph, DWORD *pdwRegister) const
{
	// �f�o�b�O�p
	IMoniker * pMoniker;
	IRunningObjectTable *pROT;
	if(FAILED(::GetRunningObjectTable(0, &pROT)))return E_FAIL;

	WCHAR wsz[256];
	wsprintfW(wsz, L"FilterGraph %08p pid %08x", (DWORD_PTR)pUnkGraph, ::GetCurrentProcessId());

	HRESULT hr = ::CreateItemMoniker(L"!", wsz, &pMoniker);

	if(SUCCEEDED(hr)){
		hr = pROT->Register(0, pUnkGraph, pMoniker, pdwRegister);
		pMoniker->Release();
		}

	pROT->Release();

	return hr;
}


void CMediaViewer::RemoveFromRot(const DWORD dwRegister) const
{
	// �f�o�b�O�p
	IRunningObjectTable *pROT;

	if(SUCCEEDED(::GetRunningObjectTable(0, &pROT))){
		pROT->Revoke(dwRegister);
		pROT->Release();
		}
}

#endif	// _DEBUG
