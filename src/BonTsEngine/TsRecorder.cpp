#include "stdafx.h"
#include "TsRecorder.h"
#include "BasicFile.h"
#include "../Common/DebugDef.h"


template<typename T> T ROUND_PACKET_SIZE(T Size)
{
	return (Size + 187) / 188 * 188;
}




class CFileWriter : public CTsRecorder::CWriter
{
public:
	CFileWriter();
	~CFileWriter();

// CWriter
	bool Open(LPCTSTR pszFileName, UINT Flags, ULONGLONG PreAllocationUnit) override;
	bool ReOpen(LPCTSTR pszFileName, UINT Flags, ULONGLONG PreAllocationUnit) override;
	void Close() override;
	bool IsOpen() const override;
	DWORD Write(const void *pBuffer, DWORD Size) override;
	int GetFileName(LPTSTR pszFileName, int MaxFileName) const override;
	ULONGLONG GetWriteSize() const override;
	bool IsWriteSizeAvailable() const override;
	bool SetPreAllocationUnit(ULONGLONG PreAllocationUnit) override;

private:
	CBasicFile *OpenFile(LPCTSTR pszFileName, UINT Flags, ULONGLONG PreAllocationUnit);

	CBasicFile *m_pFile;
	ULONGLONG m_WriteSize;
};


CFileWriter::CFileWriter()
	: m_pFile(nullptr)
	, m_WriteSize(0)
{
}


CFileWriter::~CFileWriter()
{
	Close();
}


bool CFileWriter::Open(LPCTSTR pszFileName, UINT Flags, ULONGLONG PreAllocationUnit)
{
	if (m_pFile)
		return false;

	CBasicFile *pFile = OpenFile(pszFileName, Flags, PreAllocationUnit);
	if (!pFile)
		return false;

	m_pFile = pFile;
	m_WriteSize = 0;

	ClearError();

	return true;
}


bool CFileWriter::ReOpen(LPCTSTR pszFileName, UINT Flags, ULONGLONG PreAllocationUnit)
{
	CBasicFile *pFile = OpenFile(pszFileName, Flags, PreAllocationUnit);

	if (!pFile)
		return false;

	Close();

	m_pFile = pFile;

	return true;
}


void CFileWriter::Close()
{
	if (m_pFile) {
		m_pFile->Close();
		delete m_pFile;
		m_pFile = nullptr;
	}
}


bool CFileWriter::IsOpen() const
{
	return m_pFile != nullptr;
}


DWORD CFileWriter::Write(const void *pBuffer, DWORD Size)
{
	if (!m_pFile) {
		SetErrorCode(ERROR_INVALID_FUNCTION);
		return 0;
	}

	DWORD Write = m_pFile->Write(pBuffer, Size);

	m_WriteSize += Write;

	SetErrorCode(m_pFile->GetLastError());

	return Write;
}


int CFileWriter::GetFileName(LPTSTR pszFileName, int MaxFileName) const
{
	if (!m_pFile)
		return 0;

	LPCTSTR pszPath = m_pFile->GetFileName();

	if (!pszPath)
		return 0;

	int PathLength = ::lstrlen(pszPath);

	if (!pszFileName)
		return PathLength;
	if (MaxFileName <= PathLength)
		return 0;

	::lstrcpyn(pszFileName, pszPath, MaxFileName);

	return PathLength;
}


ULONGLONG CFileWriter::GetWriteSize() const
{
	return m_WriteSize;
}


bool CFileWriter::IsWriteSizeAvailable() const
{
	return m_pFile != nullptr;
}


bool CFileWriter::SetPreAllocationUnit(ULONGLONG PreAllocationUnit)
{
	return m_pFile && m_pFile->SetPreAllocationUnit(PreAllocationUnit);
}


CBasicFile *CFileWriter::OpenFile(LPCTSTR pszFileName, UINT Flags, ULONGLONG PreAllocationUnit)
{
	CBasicFile *pFile = new CBasicFile;

	if (!pFile->Open(pszFileName,
					 CBasicFile::OPEN_WRITE | CBasicFile::OPEN_SHAREREAD |
					 (!(Flags & CTsRecorder::OPEN_OVERWRITE) ? CBasicFile::OPEN_NEW : 0))) {
		TCHAR szMessage[1024];
		pFile->GetLastErrorMessage(szMessage, _countof(szMessage));
		SetError(pFile->GetLastError(), TEXT("ファイルが開けません。"), nullptr, szMessage);
		delete pFile;
		return nullptr;
	}

	pFile->SetPreAllocationUnit(PreAllocationUnit);

	return pFile;
}




class CEdcbPluginWriter : public CTsRecorder::CWriter
{
public:
	CEdcbPluginWriter();
	~CEdcbPluginWriter();
	bool Load(LPCTSTR pszFileName);
	void Free();

// CWriter
	bool Open(LPCTSTR pszFileName, UINT Flags, ULONGLONG PreAllocationUnit) override;
	bool ReOpen(LPCTSTR pszFileName, UINT Flags, ULONGLONG PreAllocationUnit) override;
	void Close() override;
	bool IsOpen() const override;
	DWORD Write(const void *pBuffer, DWORD Size) override;
	int GetFileName(LPTSTR pszFileName, int MaxFileName) const override;
	ULONGLONG GetWriteSize() const override;
	bool IsWriteSizeAvailable() const override;

private:
	typedef BOOL (WINAPI *GetPlugInName)(WCHAR *name, DWORD *nameSize);
	typedef void (WINAPI *Setting)(HWND parentWnd);
	typedef BOOL (WINAPI *CreateCtrl)(DWORD *id);
	typedef BOOL (WINAPI *DeleteCtrl)(DWORD id);
	typedef BOOL (WINAPI *StartSave)(DWORD id, LPCWSTR fileName, BOOL overWriteFlag, ULONGLONG createSize);
	typedef BOOL (WINAPI *StopSave)(DWORD id);
	typedef BOOL (WINAPI *GetSaveFilePath)(DWORD id, WCHAR *filePath, DWORD *filePathSize);
	typedef BOOL (WINAPI *AddTSBuff)(DWORD id, BYTE *data, DWORD size, DWORD *writeSize);

	HMODULE m_hLib;
	DeleteCtrl m_pDeleteCtrl;
	StartSave m_pStartSave;
	StopSave m_pStopSave;
	GetSaveFilePath m_pGetSaveFilePath;
	AddTSBuff m_pAddTSBuff;
	DWORD m_ID;
	bool m_bOpen;
	HANDLE m_hFile;
};


CEdcbPluginWriter::CEdcbPluginWriter()
	: m_hLib(nullptr)
	, m_ID(0)
	, m_bOpen(false)
	, m_hFile(INVALID_HANDLE_VALUE)
{
}


CEdcbPluginWriter::~CEdcbPluginWriter()
{
	Free();
}


bool CEdcbPluginWriter::Load(LPCTSTR pszFileName)
{
	if (m_hLib)
		return false;

	HMODULE hLib = ::LoadLibrary(pszFileName);
	if (!hLib) {
		SetError(TEXT("DLLをロードできません。"));
		return false;
	}

	CreateCtrl pCreateCtrl = (CreateCtrl)::GetProcAddress(hLib, "CreateCtrl");
	m_pDeleteCtrl = (DeleteCtrl)::GetProcAddress(hLib, "DeleteCtrl");
	m_pStartSave = (StartSave)::GetProcAddress(hLib, "StartSave");
	m_pStopSave = (StopSave)::GetProcAddress(hLib, "StopSave");
	m_pGetSaveFilePath = (GetSaveFilePath)::GetProcAddress(hLib, "GetSaveFilePath");
	m_pAddTSBuff = (AddTSBuff)::GetProcAddress(hLib, "AddTSBuff");

	if (!pCreateCtrl || !m_pDeleteCtrl || !m_pStartSave || !m_pStopSave
			|| !m_pGetSaveFilePath || !m_pAddTSBuff) {
		::FreeLibrary(hLib);
		SetError(TEXT("プラグインから必要な関数を取得できません。"));
		return false;
	}

	DWORD ID;
	if (!pCreateCtrl(&ID)) {
		::FreeLibrary(hLib);
		SetError(TEXT("保存用インスタンスを作成できません。"));
		return false;
	}

	m_hLib = hLib;
	m_ID = ID;

	return true;
}


void CEdcbPluginWriter::Free()
{
	if (m_hLib) {
		Close();

		m_pDeleteCtrl(m_ID);
		m_ID = 0;

		::FreeLibrary(m_hLib);
		m_hLib = nullptr;
	}
}


bool CEdcbPluginWriter::Open(LPCTSTR pszFileName, UINT Flags, ULONGLONG PreAllocationUnit)
{
	if (!m_hLib || m_bOpen) {
		return false;
	}

	if (!m_pStartSave(m_ID, pszFileName,
					  (Flags & CTsRecorder::OPEN_OVERWRITE) != 0,
					  PreAllocationUnit)) {
		SetError(TEXT("プラグインでの保存を開始できません。"));
		return false;
	}

	m_bOpen = true;

	WCHAR szActualPath[MAX_PATH];
	DWORD PathLength = _countof(szActualPath);
	if (m_pGetSaveFilePath(m_ID, szActualPath, &PathLength)) {
		m_hFile = ::CreateFileW(szActualPath,
								FILE_READ_ATTRIBUTES,
								FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
								nullptr, OPEN_EXISTING, 0, nullptr);
	}

	ClearError();

	return true;
}


bool CEdcbPluginWriter::ReOpen(LPCTSTR pszFileName, UINT Flags, ULONGLONG PreAllocationUnit)
{
	Close();

	return Open(pszFileName, Flags, PreAllocationUnit);
}


void CEdcbPluginWriter::Close()
{
	if (m_bOpen) {
		m_pStopSave(m_ID);
		m_bOpen = false;

		if (m_hFile != INVALID_HANDLE_VALUE) {
			::CloseHandle(m_hFile);
			m_hFile = INVALID_HANDLE_VALUE;
		}
	}
}


bool CEdcbPluginWriter::IsOpen() const
{
	return m_bOpen;
}


DWORD CEdcbPluginWriter::Write(const void *pBuffer, DWORD Size)
{
	if (!m_bOpen) {
		SetErrorCode(ERROR_INVALID_FUNCTION);
		return 0;
	}

	DWORD Write = 0;
	if (!m_pAddTSBuff(m_ID, static_cast<BYTE*>(const_cast<void*>(pBuffer)), Size, &Write))
		SetErrorCode(ERROR_WRITE_FAULT);
	else
		SetErrorCode(ERROR_SUCCESS);

	return Write;
}


int CEdcbPluginWriter::GetFileName(LPTSTR pszFileName, int MaxFileName) const
{
	if (!m_bOpen)
		return 0;

	DWORD Size = MaxFileName;
	if (!m_pGetSaveFilePath(m_ID, pszFileName, &Size) || Size == 0)
		return 0;

	return Size - 1;
}


ULONGLONG CEdcbPluginWriter::GetWriteSize() const
{
	LARGE_INTEGER FileSize;

	if (m_hFile == INVALID_HANDLE_VALUE
			|| !::GetFileSizeEx(m_hFile, &FileSize))
		return 0;
	return FileSize.QuadPart;
}


bool CEdcbPluginWriter::IsWriteSizeAvailable() const
{
	return m_hFile != INVALID_HANDLE_VALUE;
}




CTsRecorder::CTsRecorder(IEventHandler *pEventHandler)
	: CMediaDecoder(pEventHandler, 1UL, 0UL)
	, m_pWriter(nullptr)

	, m_BufferSize(0x200000)
	, m_BufferUsed(0)
	, m_pWriteBuffer(nullptr)

	, m_PreAllocationUnit(0)

	, m_InputSize(0)
	, m_OutputSize(0)
	, m_OutputCount(0)
	, m_WriteErrorCount(0)

	, m_bWriteError(false)
	, m_bPause(false)

	, m_bEnableQueueing(false)
	, m_QueueBlockSize(ROUND_PACKET_SIZE(m_BufferSize))
	, m_MaxQueueSize(16)
	, m_MaxPendingSize(0x20000000)
	, m_hThread(nullptr)
{
	m_EndEvent.Create();
}


CTsRecorder::~CTsRecorder()
{
	ClearQueue();
	CloseFile();
}


void CTsRecorder::Reset(void)
{
	CBlockLock Lock(&m_DecoderLock);

	ClearQueue();
}


const bool CTsRecorder::InputMedia(CMediaData *pMediaData, const DWORD dwInputIndex)
{
	CBlockLock Lock(&m_DecoderLock);

	if (m_bEnableQueueing || (m_pWriter && !m_bPause))
		PushData(pMediaData->GetData(), pMediaData->GetSize());

	OutputMedia(pMediaData);

	return true;
}


bool CTsRecorder::OpenFile(LPCTSTR pszFileName, UINT Flags)
{
	return OpenFile(nullptr, pszFileName, Flags);
}


bool CTsRecorder::OpenFile(LPCTSTR pszPluginName, LPCTSTR pszFileName, UINT Flags)
{
	if (!pszFileName) {
		SetError(TEXT("引数が不正です。"));
		return false;
	}

	CBlockLock Lock(&m_DecoderLock);

	// 一旦閉じる
	CloseFile();

	m_InputSize = 0;
	m_OutputSize = 0;
	m_OutputCount = 0;
	m_WriteErrorCount = 0;

	// ファイルを開く
	if (pszPluginName && pszPluginName[0]) {
		CEdcbPluginWriter *pWriter = new CEdcbPluginWriter;
		if (!pWriter->Load(pszPluginName)) {
			SetError(pWriter->GetLastErrorException());
			delete pWriter;
			return false;
		}
		m_pWriter = pWriter;
	} else {
		m_pWriter = new CFileWriter;
	}
	if (!m_pWriter->Open(pszFileName, Flags, m_PreAllocationUnit)) {
		SetError(m_pWriter->GetLastErrorException());
		delete m_pWriter;
		m_pWriter = nullptr;
		return false;
	}

	m_bWriteError = false;
	m_bPause = false;

	if (m_BufferSize > 0) {
		try {
			m_pWriteBuffer = new BYTE[m_BufferSize];
		} catch (...) {
#if 0
			CloseFile();
			SetError(CTracer::TYPE_ERROR, TEXT("メモリが確保できません。"));
			return false;
#else
			Trace(CTracer::TYPE_WARNING, TEXT("書き出しバッファのメモリを確保できません。"));
			// バッファリングなしで続行
#endif
		}
	}
	m_BufferUsed = 0;

	// 書き出しスレッド開始
	m_EndEvent.Reset();
	m_hThread = (HANDLE)::_beginthreadex(nullptr, 0, WriteThread, this, 0, nullptr);
	if (!m_hThread) {
		CloseFile();
		SetError(TEXT("スレッドが作成できません。"));
		return false;
	}

	ClearError();

	return true;
}


bool CTsRecorder::RelayFile(LPCTSTR pszFileName, UINT Flags)
{
	if (!pszFileName) {
		SetError(TEXT("引数が不正です。"));
		return false;
	}

	if (!m_pWriter)
		return OpenFile(pszFileName, Flags);

	CBlockLock Lock1(&m_DecoderLock);
	CBlockLock Lock2(&m_WriterLock);

	if (!m_pWriter->ReOpen(pszFileName, Flags, m_PreAllocationUnit)) {
		SetError(m_pWriter->GetLastErrorException());
		if (!m_pWriter->IsOpen()) {
			delete m_pWriter;
			m_pWriter = nullptr;
		}
		return false;
	}

	m_bWriteError = false;

	ClearError();

	return true;
}


void CTsRecorder::CloseFile()
{
	CBlockLock Lock(&m_DecoderLock);

	// 書き出しスレッド終了
	if (m_hThread) {
		m_EndEvent.Set();
		if (::WaitForSingleObject(m_hThread, 10000) != WAIT_OBJECT_0) {
			Trace(CTracer::TYPE_WARNING, TEXT("ファイル書き出しスレッドが応答しないため強制終了します。"));
			::TerminateThread(m_hThread, -1);
		}
		::CloseHandle(m_hThread);
		m_hThread = nullptr;
	}

	// ファイルを閉じる
	if (m_pWriter) {
		m_pWriter->SetPreAllocationUnit(0);
		FlushBuffer();
		m_pWriter->Close();
		delete m_pWriter;
		m_pWriter = nullptr;
	}

	// バッファ解放
	if (m_pWriteBuffer) {
		delete [] m_pWriteBuffer;
		m_pWriteBuffer = nullptr;
		m_BufferUsed = 0;
	}

	if (!m_bEnableQueueing) {
		ClearQueue();
	}
}


bool CTsRecorder::IsOpen() const
{
	return m_pWriter != nullptr;
}


int CTsRecorder::GetFileName(LPTSTR pszFileName, int MaxFileName) const
{
	if (pszFileName) {
		if (MaxFileName < 1)
			return 0;
		pszFileName[0] = _T('\0');
	}

	if (!m_pWriter)
		return 0;

	return m_pWriter->GetFileName(pszFileName, MaxFileName);
}


bool CTsRecorder::GetWriteStatistics(WriteStatistics *pStatistics) const
{
	if (!pStatistics)
		return false;

	pStatistics->InputSize = m_InputSize;
	pStatistics->OutputSize = m_OutputSize;
	pStatistics->OutputCount = m_OutputCount;
	pStatistics->WriteErrorCount = m_WriteErrorCount;

	return true;
}


ULONGLONG CTsRecorder::GetInputSize() const
{
	// 入力されたサイズを返す
	return m_InputSize;
}


ULONGLONG CTsRecorder::GetOutputSize() const
{
	// 出力したTSサイズを返す
	return m_OutputSize;
}


ULONGLONG CTsRecorder::GetOutputCount() const
{
	// 出力した回数を返す
	return m_OutputCount;
}


bool CTsRecorder::IsWriteSizeAvailable() const
{
	// 書き出し済みサイズが取得できるかを返す
	if (!m_pWriter)
		return false;
	return m_pWriter->IsWriteSizeAvailable();
}


ULONGLONG CTsRecorder::GetWriteSize() const
{
	// 書き出し済みサイズを返す
	if (!m_pWriter)
		return 0;
	return m_pWriter->GetWriteSize();
}


DWORD CTsRecorder::GetWriteErrorCount() const
{
	// 書き出しエラー回数を返す
	return m_WriteErrorCount;
}


bool CTsRecorder::Pause()
{
	TRACE(TEXT("CTsRecorder::Pause()\n"));

	CBlockLock Lock(&m_DecoderLock);

	if (!m_pWriter)
		return false;

	m_bPause = true;

	return true;
}


bool CTsRecorder::Resume()
{
	TRACE(TEXT("CTsRecorder::Resume()\n"));

	CBlockLock Lock(&m_DecoderLock);

	if (!m_pWriter)
		return false;

	m_bPause = false;

	return true;
}


bool CTsRecorder::SetBufferSize(DWORD Size)
{
	if (m_pWriter || Size < 188)
		return false;

	m_BufferSize = Size;

	return true;
}


bool CTsRecorder::SetPreAllocationUnit(ULONGLONG PreAllocationUnit)
{
	CBlockLock Lock(&m_WriterLock);

	if (m_pWriter && !m_pWriter->SetPreAllocationUnit(PreAllocationUnit))
		return false;

	m_PreAllocationUnit = PreAllocationUnit;

	return true;
}


bool CTsRecorder::EnableQueueing(bool bEnable)
{
	CBlockLock Lock(&m_DecoderLock);

	if (m_bEnableQueueing != bEnable) {
		if (!bEnable && !m_pWriter)
			ClearQueue();
		m_bEnableQueueing = bEnable;
	}

	return true;
}


bool CTsRecorder::IsQueueingEnabled() const
{
	return m_bEnableQueueing;
}


bool CTsRecorder::SetQueueSize(SIZE_T BlockSize, SIZE_T MaxBlocks)
{
	if (BlockSize < 188 || MaxBlocks == 0)
		return false;

	CBlockLock Lock(&m_QueueLock);

	BlockSize = ROUND_PACKET_SIZE(BlockSize);

	if (m_QueueBlockSize != BlockSize) {
		if (m_pWriter)
			return false;

		std::deque<QueueBlock> Queue;
		m_WriteQueue.swap(Queue);
		size_t AliveBlocks = Queue.size() * BlockSize / m_QueueBlockSize;
		m_QueueBlockSize = BlockSize;
		m_MaxQueueSize = MaxBlocks;
		while (!Queue.empty()) {
			QueueBlock Info = Queue.front();
			if (Queue.size() <= AliveBlocks) {
				SIZE_T Pos = Info.Offset;
				do {
					PushData(Info.pData + Pos, min(Info.Size - Pos, BlockSize));
					Pos += BlockSize;
				} while (Pos < Info.Size);
			}
			delete [] Info.pData;
			Queue.pop_front();
		}
	} else if (m_MaxQueueSize != MaxBlocks) {
		m_MaxQueueSize = MaxBlocks;
		if (!m_pWriter || m_bPause) {
			while (m_WriteQueue.size() > MaxBlocks) {
				delete [] m_WriteQueue.front().pData;
				m_WriteQueue.pop_front();
			}
		}
	}

	return true;
}


void CTsRecorder::ClearQueue()
{
	TRACE(TEXT("CTsRecorder::ClearQueue()\n"));

	CBlockLock Lock(&m_QueueLock);

	while (!m_WriteQueue.empty()) {
		delete [] m_WriteQueue.front().pData;
		m_WriteQueue.pop_front();
	}
}


bool CTsRecorder::SetMaxPendingSize(SIZE_T Size)
{
	if (Size == 0)
		return false;

	CBlockLock Lock(&m_QueueLock);

	m_MaxPendingSize = Size;

	return true;
}


bool CTsRecorder::PushData(const BYTE *pData, SIZE_T DataSize)
{
	CBlockLock Lock(&m_QueueLock);

	if (!pData || DataSize == 0 || DataSize > m_QueueBlockSize)
		return false;

	m_InputSize += DataSize;

	if (!m_WriteQueue.empty()) {
		QueueBlock &Last = m_WriteQueue.back();
		if (Last.Size < m_QueueBlockSize) {
			SIZE_T Size = min(m_QueueBlockSize - Last.Size, DataSize);
			::CopyMemory(Last.pData + Last.Size, pData, Size);
			Last.Size += Size;
			if (Size == DataSize)
				return true;
			DataSize -= Size;
			pData += Size;
		}
	}

	ULONGLONG MaxQueueSize = 0;
	if (m_bEnableQueueing)
		MaxQueueSize = m_MaxQueueSize * m_QueueBlockSize;
	if (m_pWriter && !m_bPause)
		MaxQueueSize += m_MaxPendingSize;
#ifndef _WIN64
	if (MaxQueueSize > 0x80000000ULL)
		MaxQueueSize = 0x80000000ULL;
#endif

	QueueBlock Block;

	if ((ULONGLONG)(m_WriteQueue.size() * m_QueueBlockSize) < MaxQueueSize) {
		try {
			Block.pData = new BYTE[m_QueueBlockSize];
		} catch (...) {
			return false;
		}
	} else {
		Block = m_WriteQueue.front();
		m_WriteQueue.pop_front();
	}
	Block.Size = DataSize;
	Block.Offset = 0;
	::CopyMemory(Block.pData, pData, DataSize);
	m_WriteQueue.push_back(Block);

	return true;
}


bool CTsRecorder::FlushBuffer()
{
	if (!m_pWriter)
		return false;

	TRACE(TEXT("CTsRecorder::FlushBuffer()\n"));

	const DWORD StartTime = ::GetTickCount();

	while (!m_WriteQueue.empty()) {
		if (::GetTickCount() - StartTime >= 10000) {
			Trace(CTracer::TYPE_WARNING, TEXT("書き出し待ちデータを全て書き出すのに時間が掛かり過ぎているため中止します。"));
			return false;
		}

		QueueBlock &Block = m_WriteQueue.front();
		DWORD Size = (DWORD)(Block.Size - Block.Offset), Written = 0;
		DWORD Error = WriteData(Block.pData + Block.Offset, Size, &Written);
		if (Written < Size) {
			Block.Offset += Written;
		} else {
			delete [] Block.pData;
			m_WriteQueue.pop_front();
		}
		if (Error != ERROR_SUCCESS) {
			m_WriteErrorCount++;
			return false;
		}
	}

	if (m_pWriteBuffer && m_BufferUsed) {
		const DWORD Written = m_pWriter->Write(m_pWriteBuffer, m_BufferUsed);
		if (Written) {
			m_OutputSize += Written;
			m_OutputCount++;
		}
		if (Written < m_BufferUsed) {
			m_WriteErrorCount++;
			return false;
		}
	}

	return true;
}


DWORD CTsRecorder::WriteData(const BYTE *pData, DWORD DataSize, DWORD *pWrittenSize)
{
	CBlockLock Lock(&m_WriterLock);

	if (!m_pWriteBuffer) {
		*pWrittenSize = m_pWriter->Write(pData, DataSize);
		return m_pWriter->GetLastErrorCode();
	}

	if (m_BufferUsed == 0 && DataSize >= m_BufferSize) {
		*pWrittenSize = m_pWriter->Write(pData, m_BufferSize);
		return m_pWriter->GetLastErrorCode();
	}

	const DWORD Remain = m_BufferSize - m_BufferUsed;
	if (Remain) {
		const DWORD CopySize = min(Remain, DataSize);
		::CopyMemory(m_pWriteBuffer + m_BufferUsed, pData, CopySize);
		m_BufferUsed += CopySize;
		*pWrittenSize = CopySize;
		if (m_BufferUsed < m_BufferSize)
			return ERROR_SUCCESS;
	} else {
		*pWrittenSize = 0;
	}

	const DWORD Written = m_pWriter->Write(m_pWriteBuffer, m_BufferUsed);
	if (Written) {
		m_OutputSize += Written;
		m_OutputCount++;
	}
	if (Written < m_BufferUsed) {
		if (Written) {
			m_BufferUsed -= Written;
			std::memmove(m_pWriteBuffer, m_pWriteBuffer + Written, m_BufferUsed);
		}
		return m_pWriter->GetLastErrorCode();
	}

	m_BufferUsed = 0;

	return ERROR_SUCCESS;
}


unsigned int __stdcall CTsRecorder::WriteThread(LPVOID lpParameter)
{
	CTsRecorder *pThis = static_cast<CTsRecorder*>(lpParameter);

	try {
		pThis->WriteMain();
	} catch (...) {
		pThis->Trace(CTracer::TYPE_ERROR, TEXT("ファイル書き出しスレッドで例外が発生しました。"));
	}

	return 0;
}


void CTsRecorder::WriteMain()
{
	DWORD Wait = 0;

	while (m_EndEvent.Wait(Wait) == WAIT_TIMEOUT) {
		if (m_bPause) {
			Wait = 100;
			continue;
		}

		m_QueueLock.Lock();
		if (!m_WriteQueue.empty()) {
			QueueBlock Block = m_WriteQueue.front();
			m_WriteQueue.pop_front();
			m_QueueLock.Unlock();

			DWORD Size = (DWORD)(Block.Size - Block.Offset), Written = 0;
			DWORD Error = WriteData(Block.pData + Block.Offset, Size, &Written);

			Wait = 0;

			m_QueueLock.Lock();
			if (Written < Size) {
				Block.Offset += Written;
				m_WriteQueue.push_front(Block);
			} else {
				if (m_WriteQueue.empty()) {
					Block.Size = 0;
					Block.Offset = 0;
					m_WriteQueue.push_front(Block);
					Wait = 50;
				} else {
					delete [] Block.pData;
				}
			}
			m_QueueLock.Unlock();

			if (Error != ERROR_SUCCESS) {
				m_WriteErrorCount++;
				if (!m_bWriteError) {
					m_bWriteError = true;
					SendDecoderEvent(EVENT_WRITE_ERROR, &Error);
				} else if (!Written) {
					Wait = 50;
				}
			}
		} else {
			m_QueueLock.Unlock();
			Wait = 50;
		}
	}
}
