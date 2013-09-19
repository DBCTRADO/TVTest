// MediaDecoder.cpp: CMediaDecoder クラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "MediaDecoder.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


//////////////////////////////////////////////////////////////////////
// CMediaDecoder 構築/消滅
//////////////////////////////////////////////////////////////////////

CMediaDecoder::CMediaDecoder(IEventHandler *pEventHandler, const DWORD dwInputNum, const DWORD dwOutputNum)
	: m_pEventHandler(pEventHandler)
	, m_dwInputNum(dwInputNum)
	, m_dwOutputNum(dwOutputNum)
{
	// 出力フィルタ配列をクリアする
	::ZeroMemory(m_aOutputDecoder, sizeof(m_aOutputDecoder));
}

CMediaDecoder::~CMediaDecoder()
{
}

void CMediaDecoder::Reset()
{
}

void CMediaDecoder::ResetGraph(void)
{
	CBlockLock Lock(&m_DecoderLock);

	Reset();
	ResetDownstreamDecoder();
}

const DWORD CMediaDecoder::GetInputNum(void) const
{
	// 入力数を返す
	return m_dwInputNum;
}

const DWORD CMediaDecoder::GetOutputNum(void) const
{
	// 出力数を返す
	return m_dwOutputNum;
}

const bool CMediaDecoder::SetOutputDecoder(CMediaDecoder *pDecoder, const DWORD dwOutputIndex, const DWORD dwInputIndex)
{
	CBlockLock Lock(&m_DecoderLock);

	if (dwOutputIndex >= m_dwOutputNum)
		return false;

	// 出力フィルタをセットする
	m_aOutputDecoder[dwOutputIndex].pDecoder = pDecoder;
	m_aOutputDecoder[dwOutputIndex].dwInputIndex = dwInputIndex;
	return true;
}

const bool CMediaDecoder::InputMedia(CMediaData *pMediaData, const DWORD dwInputIndex)
{
	CBlockLock Lock(&m_DecoderLock);

	OutputMedia(pMediaData, dwInputIndex);
	return true;
}

const bool CMediaDecoder::OutputMedia(CMediaData *pMediaData, const DWORD dwOutptIndex)
{
	// 出力処理
	if (dwOutptIndex >= m_dwOutputNum)
		return false;

	// 次のフィルタにデータを渡す
	if (m_aOutputDecoder[dwOutptIndex].pDecoder) {
		return m_aOutputDecoder[dwOutptIndex].pDecoder->InputMedia(pMediaData, m_aOutputDecoder[dwOutptIndex].dwInputIndex);
	}
	return false;
}

void CMediaDecoder::ResetDownstreamDecoder(void)
{
	// 次のフィルタをリセットする
	for (DWORD dwOutputIndex = 0UL ; dwOutputIndex < m_dwOutputNum ; dwOutputIndex++) {
		if (m_aOutputDecoder[dwOutputIndex].pDecoder)
			m_aOutputDecoder[dwOutputIndex].pDecoder->ResetGraph();
	}
}

const DWORD CMediaDecoder::SendDecoderEvent(const DWORD dwEventID, PVOID pParam)
{
	// イベントを通知する
	if (m_pEventHandler==NULL)
		return 0;
	return m_pEventHandler->OnDecoderEvent(this, dwEventID, pParam);
}
