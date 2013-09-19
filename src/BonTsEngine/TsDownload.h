#pragma once


#include "Common.h"


class CDataModule
{
public:
	CDataModule(DWORD DownloadID, WORD BlockSize, WORD ModuleID, DWORD ModuleSize, BYTE ModuleVersion);
	virtual ~CDataModule();
	bool StoreBlock(const WORD BlockNumber, const void *pData, const WORD DataSize);
	DWORD GetDownloadID() const { return m_DownloadID; }
	WORD GetBlockSize() const { return m_BlockSize; }
	WORD GetModuleID() const { return m_ModuleID; }
	DWORD GetModuleSize() const { return m_ModuleSize; }
	BYTE GetModuleVersion() const { return m_ModuleVersion; }
	bool IsComplete() const { return m_NumDownloadedBlocks == m_NumBlocks; }
	bool IsBlockDownloaded(const WORD BlockNumber) const;

protected:
	bool SetBlockDownloaded(const WORD BlockNumber);

	virtual void OnComplete(const BYTE *pData, const DWORD ModuleSize) {}

	DWORD m_DownloadID;
	WORD m_BlockSize;
	WORD m_ModuleID;
	DWORD m_ModuleSize;
	BYTE m_ModuleVersion;
	WORD m_NumBlocks;
	WORD m_NumDownloadedBlocks;
	BYTE *m_pData;
	UINT_PTR m_BlockDownloaded;
	DWORD *m_pBlockDownloaded;
};

class CDownloadInfoIndicationParser
{
public:
	struct MessageInfo {
		BYTE ProtocolDiscriminator;
		BYTE DsmccType;
		WORD MessageID;
		DWORD TransactionID;
		DWORD DownloadID;
		WORD BlockSize;
		BYTE WindowSize;
		BYTE AckPeriod;
		DWORD TCDownloadWindow;
		DWORD TCDownloadScenario;
	};

	struct ModuleInfo {
		WORD ModuleID;
		DWORD ModuleSize;
		BYTE ModuleVersion;
		// ‚±‚Ì•Ó‚è‚ÍŽb’è
		struct {
			struct {
				BYTE Length;
				const char *pText;
			} Name;
			struct {
				bool bValid;
				DWORD CRC32;
			} CRC;
		} ModuleDesc;
	};

	class ABSTRACT_CLASS_DECL IEventHandler {
	public:
		virtual ~IEventHandler() {}
		virtual void OnDataModule(const MessageInfo *pMessageInfo, const ModuleInfo *pModuleInfo) = 0;
	};

	CDownloadInfoIndicationParser(IEventHandler *pHandler);
	bool ParseData(const BYTE *pData, const WORD DataSize);

private:
	IEventHandler *m_pEventHandler;
};

class CDownloadDataBlockParser
{
public:
	struct DataBlockInfo {
		BYTE ProtocolDiscrimnator;
		BYTE DsmccType;
		WORD MessageID;
		DWORD DownloadID;
		WORD MessageLength;
		WORD ModuleID;
		BYTE ModuleVersion;
		BYTE Reserved;
		WORD BlockNumber;
		WORD DataSize;
		const BYTE *pData;
	};

	class ABSTRACT_CLASS_DECL IEventHandler {
	public:
		virtual ~IEventHandler() {}
		virtual void OnDataBlock(const DataBlockInfo *pDataBlock) = 0;
	};

	CDownloadDataBlockParser(IEventHandler *pHandler);
	bool ParseData(const BYTE *pData, const WORD DataSize);

private:
	IEventHandler *m_pEventHandler;
};
