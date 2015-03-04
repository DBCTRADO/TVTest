#include "stdafx.h"
#include "AudioDecoder.h"
#include "../Common/DebugDef.h"


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
