// MediaGrabber.cpp: CMediaGrabber �N���X�̃C���v�������e�[�V����
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "MediaGrabber.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// �\�z/����
//////////////////////////////////////////////////////////////////////

CMediaGrabber::CMediaGrabber(IEventHandler *pEventHandler)
	: CMediaDecoder(pEventHandler, 1UL, 1UL)
{
}

CMediaGrabber::~CMediaGrabber()
{
}

void CMediaGrabber::Reset(void)
{
	CBlockLock Lock(&m_DecoderLock);

	// �R�[���o�b�N�ɒʒm����
	for (auto itr = m_GrabberList.begin(); itr != m_GrabberList.end(); ++itr)
		(*itr)->OnReset();
}

const bool CMediaGrabber::InputMedia(CMediaData *pMediaData, const DWORD dwInputIndex)
{
	CBlockLock Lock(&m_DecoderLock);

	/*
	if (dwInputIndex >= GetInputNum())
		return false;
	*/

	bool bFail = false;

	// �R�[���o�b�N�ɒʒm����
	for (auto itr = m_GrabberList.begin(); itr != m_GrabberList.end(); ++itr) {
		if (!(*itr)->OnInputMedia(pMediaData))
			bFail = true;
	}

	if (bFail)
		return false;

	// ���ʃf�R�[�_�Ƀf�[�^��n��
	OutputMedia(pMediaData);

	return true;
}

bool CMediaGrabber::AddGrabber(IGrabber *pGrabber)
{
	if (!pGrabber)
		return false;

	CBlockLock Lock(&m_DecoderLock);

	m_GrabberList.push_back(pGrabber);

	return true;
}

bool CMediaGrabber::RemoveGrabber(IGrabber *pGrabber)
{
	CBlockLock Lock(&m_DecoderLock);

	for (auto itr = m_GrabberList.begin(); itr != m_GrabberList.end(); ++itr) {
		if (*itr == pGrabber) {
			m_GrabberList.erase(itr);
			return true;
		}
	}

	return false;
}
