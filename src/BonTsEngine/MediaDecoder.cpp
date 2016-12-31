// MediaDecoder.cpp: CMediaDecoder �N���X�̃C���v�������e�[�V����
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "MediaDecoder.h"
#include "../Common/DebugDef.h"


//////////////////////////////////////////////////////////////////////
// CMediaDecoder �\�z/����
//////////////////////////////////////////////////////////////////////

CMediaDecoder::CMediaDecoder(IEventHandler *pEventHandler, const DWORD dwInputNum, const DWORD dwOutputNum)
	: m_pEventHandler(pEventHandler)
	, m_dwInputNum(dwInputNum)
	, m_dwOutputNum(dwOutputNum)
{
	// �o�̓t�B���^�z����N���A����
	::ZeroMemory(m_aOutputDecoder, sizeof(m_aOutputDecoder));
}

CMediaDecoder::~CMediaDecoder()
{
}

bool CMediaDecoder::Initialize()
{
	return true;
}

void CMediaDecoder::Finalize()
{
}

void CMediaDecoder::Reset()
{
}

void CMediaDecoder::ResetGraph()
{
	CBlockLock Lock(&m_DecoderLock);

	Reset();
	ResetDownstreamDecoder();
}

DWORD CMediaDecoder::GetInputNum() const
{
	// ���͐���Ԃ�
	return m_dwInputNum;
}

DWORD CMediaDecoder::GetOutputNum() const
{
	// �o�͐���Ԃ�
	return m_dwOutputNum;
}

bool CMediaDecoder::SetOutputDecoder(CMediaDecoder *pDecoder, const DWORD dwOutputIndex, const DWORD dwInputIndex)
{
	CBlockLock Lock(&m_DecoderLock);

	if (dwOutputIndex >= m_dwOutputNum)
		return false;

	// �o�̓t�B���^���Z�b�g����
	m_aOutputDecoder[dwOutputIndex].pDecoder = pDecoder;
	m_aOutputDecoder[dwOutputIndex].dwInputIndex = dwInputIndex;
	return true;
}

CMediaDecoder *CMediaDecoder::GetOutputDecoder(const DWORD Index) const
{
	if (Index >= m_dwOutputNum)
		return NULL;

	return m_aOutputDecoder[Index].pDecoder;
}

const bool CMediaDecoder::InputMedia(CMediaData *pMediaData, const DWORD dwInputIndex)
{
	CBlockLock Lock(&m_DecoderLock);

	OutputMedia(pMediaData, dwInputIndex);
	return true;
}


bool CMediaDecoder::SetActiveServiceID(WORD ServiceID)
{
	return false;
}

WORD CMediaDecoder::GetActiveServiceID() const
{
	return 0;
}

void CMediaDecoder::SetEventHandler(IEventHandler *pEventHandler)
{
	CBlockLock Lock(&m_DecoderLock);

	m_pEventHandler = pEventHandler;
}

bool CMediaDecoder::OutputMedia(CMediaData *pMediaData, const DWORD dwOutptIndex)
{
	// �o�͏���
	if (dwOutptIndex >= m_dwOutputNum)
		return false;

	// ���̃t�B���^�Ƀf�[�^��n��
	if (m_aOutputDecoder[dwOutptIndex].pDecoder) {
		return m_aOutputDecoder[dwOutptIndex].pDecoder->InputMedia(pMediaData, m_aOutputDecoder[dwOutptIndex].dwInputIndex);
	}
	return false;
}

void CMediaDecoder::ResetDownstreamDecoder()
{
	// ���̃t�B���^�����Z�b�g����
	for (DWORD dwOutputIndex = 0UL ; dwOutputIndex < m_dwOutputNum ; dwOutputIndex++) {
		if (m_aOutputDecoder[dwOutputIndex].pDecoder)
			m_aOutputDecoder[dwOutputIndex].pDecoder->ResetGraph();
	}
}

DWORD CMediaDecoder::SendDecoderEvent(const DWORD dwEventID, PVOID pParam)
{
	// �C�x���g��ʒm����
	if (m_pEventHandler==NULL)
		return 0;
	return m_pEventHandler->OnDecoderEvent(this, dwEventID, pParam);
}
