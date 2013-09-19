// FileWriter.cpp: CFileWriter クラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "FileReader.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// 構築/消滅
//////////////////////////////////////////////////////////////////////

CFileReader::CFileReader(IEventHandler *pEventHandler)
	: CMediaDecoder(pEventHandler, 0UL, 1UL)
	, m_ReadBuffer((BYTE)0x00U, DEF_READSIZE)
	, m_hReadAnsyncThread(NULL)
	, m_dwReadAnsyncThreadID(0UL)
	, m_bKillSignal(true)
	, m_bIsAnsyncRead(false)
{

}

CFileReader::~CFileReader()
{
	CloseFile();

	if(m_hReadAnsyncThread)::CloseHandle(m_hReadAnsyncThread);
}

const bool CFileReader::InputMedia(CMediaData *pMediaData, const DWORD dwInputIndex)
{
	// ソースデコーダのため常にエラーを返す
	return false;
}

const bool CFileReader::OpenFile(LPCTSTR lpszFileName)
{
	// 一旦閉じる
	CloseFile();

	// ファイルを開く
	return (m_InFile.Open(lpszFileName, CNCachedFile::CNF_READ | CNCachedFile::CNF_SHAREREAD | CNCachedFile::CNF_SHAREWRITE))? true : false;
}

void CFileReader::CloseFile(void)
{
	// ファイルを閉じる
	StopReadAnsync();
	m_InFile.Close();
}

const bool CFileReader::IsOpen() const
{
	return m_InFile.IsOpen();
}

const DWORD CFileReader::ReadSync(const DWORD dwReadSize)
{
	// 読み込みサイズ計算
	ULONGLONG llRemainSize = m_InFile.GetSize() - m_InFile.GetPos();
	const DWORD dwReqSize = (llRemainSize > (ULONGLONG)dwReadSize)? dwReadSize : (DWORD)llRemainSize;
	if(!dwReqSize)return 0UL;

	// バッファ確保
	m_ReadBuffer.SetSize(dwReqSize);

	// ファイル読み込み
	if(m_InFile.Read(m_ReadBuffer.GetData(), dwReqSize) != dwReqSize)return 0UL;

	// データ出力
	OutputMedia(&m_ReadBuffer);

	return dwReqSize;
}

const DWORD CFileReader::ReadSync(const DWORD dwReadSize, const ULONGLONG llReadPos)
{
	// ファイルシーク
	if(!m_InFile.SetPos(llReadPos))return 0UL;

	// データ出力
	return ReadSync(dwReadSize);
}

const bool CFileReader::StartReadAnsync(const DWORD dwReadSize, const ULONGLONG llReadPos)
{
	if(!m_bKillSignal)return false;
	
	if(m_hReadAnsyncThread){
		::CloseHandle(m_hReadAnsyncThread);
		m_hReadAnsyncThread = NULL;
		}

	// ファイルシーク
	if(!m_InFile.SetPos(llReadPos))return false;

	// 非同期リードスレッド起動
	m_dwReadAnsyncThreadID = 0UL;
	m_bKillSignal = false;
	m_bIsAnsyncRead = false;

	if(!(m_hReadAnsyncThread = ::CreateThread(NULL, 0UL, CFileReader::ReadAnsyncThread, (LPVOID)this, 0UL, &m_dwReadAnsyncThreadID))){
		return false;
		}

	return true;
}

void CFileReader::StopReadAnsync(void)
{
	// 非同期リード停止
	m_bKillSignal = true;
	if(m_hReadAnsyncThread)
	{
		if(WaitForSingleObject(m_hReadAnsyncThread,5000)!=WAIT_OBJECT_0)
		{
			// スレッドを強制解放
			::TerminateThread(m_hReadAnsyncThread,-1);
			m_hReadAnsyncThread=NULL;
		}
	}
}

const bool CFileReader::IsAnsyncReadBusy(void) const
{
	// 非同期リード中有無を返す
	return m_bIsAnsyncRead;
}

const ULONGLONG CFileReader::GetReadPos(void)
{
	// ファイルポジションを返す
	return m_InFile.GetPos();
}

const ULONGLONG CFileReader::GetFileSize(void)
{
	// ファイルサイズを返す
	return m_InFile.GetSize();
}

const bool CFileReader::SetReadPos(const ULONGLONG llReadPos)
{
	// ファイルシーク
	return m_InFile.SetPos(llReadPos);
}

DWORD WINAPI CFileReader::ReadAnsyncThread(LPVOID pParam)
{
	// 非同期リードスレッド(ファイルリードとグラフ処理を別スレッドにするとより性能が向上する)
	CFileReader *pThis = static_cast<CFileReader *>(pParam);

	pThis->m_bIsAnsyncRead = true;

	// 「非同期リード開始」イベント通知
	pThis->SendDecoderEvent(EID_READ_ASYNC_START);

	while(!pThis->m_bKillSignal && (pThis->m_InFile.GetPos() < pThis->m_InFile.GetSize())){
		
		// 「非同期リード前」イベント通知
		if(pThis->SendDecoderEvent(EID_READ_ASYNC_PREREAD))break;
		
		// ファイル同期リード
		pThis->ReadSync();
			
		// 「非同期リード後」イベント通知
		if(pThis->SendDecoderEvent(EID_READ_ASYNC_POSTREAD))break;
		}

	// 「非同期リード終了」イベント通知
	pThis->SendDecoderEvent(EID_READ_ASYNC_END);

	pThis->m_bKillSignal = true;
	pThis->m_bIsAnsyncRead = false;

	return 0UL;
}
