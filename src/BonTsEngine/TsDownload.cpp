#include "stdafx.h"
#include "TsDownload.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


inline DWORD MSBFirst32(const BYTE *p)
{
	return ((DWORD)p[0] << 24) | ((DWORD)p[1] << 16) | ((DWORD)p[2] << 8) | (DWORD)p[3];
}


CDataModule::CDataModule(DWORD DownloadID, WORD BlockSize, WORD ModuleID, DWORD ModuleSize, BYTE ModuleVersion)
	: m_DownloadID(DownloadID)
	, m_BlockSize(BlockSize)
	, m_ModuleID(ModuleID)
	, m_ModuleSize(ModuleSize)
	, m_ModuleVersion(ModuleVersion)
	, m_NumBlocks((WORD)((ModuleSize - 1) / BlockSize + 1))
	, m_NumDownloadedBlocks(0)
	, m_pData(NULL)
	, m_BlockDownloaded(0)
	, m_pBlockDownloaded(NULL)
{
	if (m_NumBlocks > sizeof(m_BlockDownloaded) * 8) {
		SIZE_T Length = (m_NumBlocks + 31) / 32;
		m_pBlockDownloaded = new DWORD[Length];
		::ZeroMemory(m_pBlockDownloaded, Length * sizeof(DWORD));
	}
}

CDataModule::~CDataModule()
{
	delete [] m_pBlockDownloaded;
	delete [] m_pData;
}

bool CDataModule::StoreBlock(const WORD BlockNumber, const void *pData, const WORD DataSize)
{
	if (BlockNumber >= m_NumBlocks)
		return false;

	if (IsBlockDownloaded(BlockNumber))
		return true;

	if (!m_pData)
		m_pData = new BYTE[m_ModuleSize];

	SIZE_T Offset, Size;
	Offset = (SIZE_T)BlockNumber * m_BlockSize;
	if (BlockNumber < m_NumBlocks - 1)
		Size = m_BlockSize;
	else
		Size = m_ModuleSize - Offset;
	if (DataSize < Size)
		return false;

	::CopyMemory(&m_pData[Offset], pData, Size);

	SetBlockDownloaded(BlockNumber);
	m_NumDownloadedBlocks++;

	if (IsComplete()) {
		TRACE(TEXT("Download complete : Download ID %08lX / Module ID %04X\n"),
			  m_DownloadID,m_ModuleID);
		OnComplete(m_pData, m_ModuleSize);
	}

	return true;
}

bool CDataModule::IsBlockDownloaded(const WORD BlockNumber) const
{
	if (BlockNumber >= m_NumBlocks)
		return false;
	if (m_NumBlocks <= sizeof(m_BlockDownloaded) * 8)
		return (m_BlockDownloaded & ((ULONG_PTR)1 << BlockNumber)) != 0;
	return (m_pBlockDownloaded[BlockNumber >> 5] & (1UL << (BlockNumber & 0x1F))) != 0;
}

bool CDataModule::SetBlockDownloaded(const WORD BlockNumber)
{
	if (BlockNumber >= m_NumBlocks)
		return false;
	if (m_NumBlocks <= sizeof(m_BlockDownloaded) * 8)
		m_BlockDownloaded |= (ULONG_PTR)1 << BlockNumber;
	else
		m_pBlockDownloaded[BlockNumber >> 5] |= 1UL << (BlockNumber & 0x1F);
	return true;
}




CDownloadInfoIndicationParser::CDownloadInfoIndicationParser(IEventHandler *pHandler)
	: m_pEventHandler(pHandler)
{
}

bool CDownloadInfoIndicationParser::ParseData(const BYTE *pData, const WORD DataSize)
{
	if (DataSize < 34)
		return false;

	MessageInfo Message;

	Message.ProtocolDiscriminator = pData[0];
	Message.DsmccType = pData[1];
	Message.MessageID = ((WORD)pData[2] << 8) | (WORD)pData[3];
	Message.TransactionID = MSBFirst32(&pData[4]);

	const BYTE AdaptationLength = pData[9];
	//const WORD MessageLength = ((WORD)pData[10] << 8) | (WORD)pData[11];
	if (12 + (WORD)AdaptationLength > DataSize)
		return false;
	WORD Pos = 12 + AdaptationLength;

	Message.DownloadID = MSBFirst32(&pData[Pos]);
	Message.BlockSize = ((WORD)pData[Pos + 4] << 8) | (WORD)pData[Pos + 5];
	if (Message.BlockSize == 0 || Message.BlockSize > 4066)
		return false;
	Message.WindowSize = pData[Pos + 6];
	Message.AckPeriod = pData[Pos + 7];
	Message.TCDownloadWindow = MSBFirst32(&pData[Pos + 8]);
	Message.TCDownloadScenario = MSBFirst32(&pData[Pos + 12]);

	// Compatibility Descriptor
	const WORD DescLength = ((WORD)pData[Pos + 16] << 8) | (WORD)pData[Pos + 17];
	if (Pos + 18 + DescLength + 2 > DataSize)
		return false;
	Pos += 18 + DescLength;

	const WORD NumberOfModules = ((WORD)pData[Pos + 0] << 8) | (WORD)pData[Pos + 1];
	Pos += 2;

	TRACE(TEXT("DII : Download ID %08X / Block size %d / %d modules\n"),
		  Message.DownloadID, Message.BlockSize, NumberOfModules);

	for (WORD i = 0; i < NumberOfModules; i++) {
		if (Pos + 8 > DataSize)
			return false;

		ModuleInfo Module;

		Module.ModuleID = ((WORD)pData[Pos + 0] << 8) | (WORD)pData[Pos + 1];
		Module.ModuleSize = MSBFirst32(&pData[Pos + 2]);
		Module.ModuleVersion = pData[Pos + 6];

		const BYTE ModuleInfoLength = pData[Pos + 7];
		Pos += 8;
		if (Pos + ModuleInfoLength > DataSize)
			return false;

		Module.ModuleDesc.Name.Length = 0;
		Module.ModuleDesc.Name.pText = NULL;
		Module.ModuleDesc.CRC.bValid = false;

		for (BYTE DescPos = 0; DescPos + 2 < ModuleInfoLength;) {
			const BYTE DescTag = pData[Pos + DescPos + 0];
			const BYTE DescLength = pData[Pos + DescPos + 1];

			DescPos += 2;

			if (DescPos + DescLength > ModuleInfoLength)
				break;

			switch (DescTag) {
			case 0x02:	// Name descriptor
				Module.ModuleDesc.Name.Length = DescLength;
				Module.ModuleDesc.Name.pText = (const char*)(&pData[Pos + DescPos]);
				break;

			case 0x05:	// CRC32 descriptor
				if (DescLength == 4) {
					Module.ModuleDesc.CRC.bValid = true;
					Module.ModuleDesc.CRC.CRC32 = MSBFirst32(&pData[Pos + DescPos]);
				}
				break;
			}

			DescPos += DescLength;
		}

#ifdef _DEBUG
		char szName[256];
		if (Module.ModuleDesc.Name.pText)
			::lstrcpynA(szName,
						Module.ModuleDesc.Name.pText,
						Module.ModuleDesc.Name.Length + 1);
		else
			szName[0]='\0';
		TRACE(TEXT("[%3d] Module ID %04X / Size %lu / Version %d / Name \"%hs\"\n"),
			  i, Module.ModuleID, Module.ModuleSize, Module.ModuleVersion, szName);
#endif

		m_pEventHandler->OnDataModule(&Message, &Module);

		Pos += ModuleInfoLength;
	}

	return true;
}




CDownloadDataBlockParser::CDownloadDataBlockParser(IEventHandler *pHandler)
	: m_pEventHandler(pHandler)
{
}

bool CDownloadDataBlockParser::ParseData(const BYTE *pData, const WORD DataSize)
{
	if (DataSize < 12)
		return false;

	DataBlockInfo DataBlock;

	DataBlock.ProtocolDiscrimnator = pData[0];
	DataBlock.DsmccType = pData[1];
	DataBlock.MessageID = ((WORD)pData[2] << 8) | (WORD)pData[3];
	DataBlock.DownloadID = MSBFirst32(&pData[4]);

	const BYTE AdaptationLength = pData[9];
	//const WORD MessageLength = ((WORD)pData[10] << 8) | (WORD)pData[11];
	if (12 + (WORD)AdaptationLength + 6 >= DataSize)
		return false;

	WORD Pos = 12 + AdaptationLength;

	DataBlock.ModuleID = ((WORD)pData[Pos + 0] << 8) | (WORD)pData[Pos + 1];
	DataBlock.ModuleVersion = pData[Pos + 2];
	DataBlock.BlockNumber = ((WORD)pData[Pos + 4] << 8) | (WORD)pData[Pos + 5];
	Pos += 6;
	DataBlock.DataSize = DataSize - Pos;
	DataBlock.pData = &pData[Pos];

	m_pEventHandler->OnDataBlock(&DataBlock);

	/*
	TRACE(TEXT("DDB : Download ID %08X / Module ID %04X / Version %d / Block no %d / Data size %d\n"),
		  DataBlock.DownloadID, DataBlock.ModuleID, DataBlock.ModuleVersion, DataBlock.BlockNumber, DataBlock.DataSize);
	*/

	return true;
}
