#pragma once

#include "../BonTsEngine/TsMedia.h"
#include "VideoParser.h"


// テンプレート名
#define H265PARSERFILTER_NAME L"H.265 Parser Filter"

// このフィルタのGUID {0F3CFFD1-C30D-43f7-B4DC-57E5F73D074A}
DEFINE_GUID(CLSID_H265ParserFilter, 0xf3cffd1, 0xc30d, 0x43f7, 0xb4, 0xdc, 0x57, 0xe5, 0xf7, 0x3d, 0x7, 0x4a);

class __declspec(uuid("0F3CFFD1-C30D-43f7-B4DC-57E5F73D074A")) CH265ParserFilter
	: public CTransInPlaceFilter
	, public CVideoParser
	, protected CH265Parser::IAccessUnitHandler
{
public:
	DECLARE_IUNKNOWN

	static IBaseFilter* WINAPI CreateInstance(LPUNKNOWN pUnk, HRESULT *phr);

// CTransInPlaceFilter
	HRESULT CheckInputType(const CMediaType* mtIn) override;
	HRESULT StartStreaming() override;
	HRESULT StopStreaming() override;
	HRESULT BeginFlush() override;

protected:
	CH265ParserFilter(LPUNKNOWN pUnk, HRESULT *phr);
	~CH265ParserFilter();

// CTransInPlaceFilter
	HRESULT Transform(IMediaSample *pSample) override;
	HRESULT Receive(IMediaSample *pSample) override;

// CH265Parser::IAccessUnitHandler
	virtual void OnAccessUnit(const CH265Parser *pParser, const CH265AccessUnit *pAccessUnit) override;

	CH265Parser m_H265Parser;
};
