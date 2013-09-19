// CasProcessor.cpp: CCasProcessor クラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////

/*
	このクラスはstubです
	CAS処理は実装されていません
*/

#include "stdafx.h"
#include "Common.h"
#include "CasProcessor.h"
#include "TsStream.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


//////////////////////////////////////////////////////////////////////
// CCasProcessor 構築/消滅
//////////////////////////////////////////////////////////////////////

CCasProcessor::CCasProcessor(IEventHandler *pEventHandler)
	: CMediaDecoder(pEventHandler, 1UL, 1UL)
	, m_InputPacketCount(0)
	, m_ScramblePacketCount(0)
{
}

CCasProcessor::~CCasProcessor()
{
	FreeCasLibrary();
}

void CCasProcessor::Reset(void)
{
	CBlockLock Lock(&m_DecoderLock);

	m_InputPacketCount = 0;
	m_ScramblePacketCount = 0;
}

const bool CCasProcessor::InputMedia(CMediaData *pMediaData, const DWORD dwInputIndex)
{
	CBlockLock Lock(&m_DecoderLock);

	m_InputPacketCount++;

	CTsPacket *pPacket = static_cast<CTsPacket *>(pMediaData);

	if (pPacket->IsScrambled())
		m_ScramblePacketCount++;

	// パケットを下流デコーダにデータを渡す
	OutputMedia(pMediaData);

	return true;
}

bool CCasProcessor::LoadCasLibrary(LPCTSTR pszFileName)
{
	SetError(TEXT("Unimplemented"));

	return false;
}

bool CCasProcessor::FreeCasLibrary()
{
	return true;
}

bool CCasProcessor::IsCasLibraryLoaded() const
{
	return false;
}

bool CCasProcessor::GetCasModuleInfo(CasModuleInfo *pInfo) const
{
	return false;
}

bool CCasProcessor::EnableDescramble(bool bEnable)
{
	return false;
}

bool CCasProcessor::EnableContract(bool bEnable)
{
	return false;
}

int CCasProcessor::GetCasDeviceCount() const
{
	return 0;
}

bool CCasProcessor::GetCasDeviceInfo(int Device, CasDeviceInfo *pInfo) const
{
	return false;
}

bool CCasProcessor::GetCasDeviceCardList(int Device, StringList *pList)
{
	return false;
}

bool CCasProcessor::IsCasDeviceAvailable(int Device)
{
	return false;
}

bool CCasProcessor::CheckCasDeviceAvailability(int Device, bool *pbAvailable, LPWSTR pszMessage, int MaxLength)
{
	if (pszMessage == NULL || MaxLength < 1)
		return false;
	pszMessage[0] = L'\0';
	return false;
}

int CCasProcessor::GetDefaultCasDevice()
{
	return -1;
}

int CCasProcessor::GetCasDeviceByID(DWORD DeviceID) const
{
	return -1;
}

int CCasProcessor::GetCasDeviceByName(LPCWSTR pszName) const
{
	return -1;
}

bool CCasProcessor::OpenCasCard(int Device, LPCWSTR pszName)
{
	return false;
}

bool CCasProcessor::CloseCasCard()
{
	return false;
}

bool CCasProcessor::IsCasCardOpen() const
{
	return false;
}

int CCasProcessor::GetCasDevice() const
{
	return -1;
}

int CCasProcessor::GetCasCardName(LPWSTR pszName, int MaxName) const
{
	return 0;
}

bool CCasProcessor::GetCasCardInfo(CasCardInfo *pInfo) const
{
	return false;
}

bool CCasProcessor::SendCasCommand(const void *pSendData, DWORD SendSize, void *pRecvData, DWORD *pRecvSize)
{
	return false;
}

ULONGLONG CCasProcessor::GetInputPacketCount() const
{
	return m_InputPacketCount;
}

ULONGLONG CCasProcessor::GetScramblePacketCount() const
{
	return m_ScramblePacketCount;
}

void CCasProcessor::ResetScramblePacketCount()
{
	m_ScramblePacketCount = 0;
}

bool CCasProcessor::SetTargetServiceID(WORD ServiceID)
{
	return false;
}

WORD CCasProcessor::GetTargetServiceID() const
{
	return 0;
}

WORD CCasProcessor::GetEcmPIDByServiceID(WORD ServiceID) const
{
	return 0xFFFF;
}

CCasProcessor::ContractStatus CCasProcessor::GetContractStatus(WORD NetworkID, WORD ServiceID, const SYSTEMTIME *pTime)
{
	return CONTRACT_ERROR;
}

CCasProcessor::ContractStatus CCasProcessor::GetContractPeriod(WORD NetworkID, WORD ServiceID, SYSTEMTIME *pTime)
{
	return CONTRACT_ERROR;
}

bool CCasProcessor::HasContractInfo(WORD NetworkID, WORD ServiceID) const
{
	return false;
}

int CCasProcessor::GetInstructionName(int Instruction, LPWSTR pszName, int MaxName) const
{
	return 0;
}

UINT CCasProcessor::GetAvailableInstructions() const
{
	return 0;
}

bool CCasProcessor::SetInstruction(int Instruction)
{
	return false;
}

int CCasProcessor::GetInstruction() const
{
	return 0;
}

bool CCasProcessor::DescrambleBenchmarkTest(int Instruction, DWORD Round, DWORD *pTime)
{
	return false;
}
