#include "stdafx.h"
#include "AudioDecoder.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


CAudioDecoder::CAudioDecoder()
{
	ClearAudioInfo();
}


CAudioDecoder::~CAudioDecoder()
{
}


bool CAudioDecoder::GetAudioInfo(AudioInfo *pInfo) const
{
	if (pInfo == NULL || m_AudioInfo.Frequency == 0)
		return false;

	*pInfo = m_AudioInfo;

	return true;
}


void CAudioDecoder::ClearAudioInfo()
{
	::ZeroMemory(&m_AudioInfo, sizeof(m_AudioInfo));
}
