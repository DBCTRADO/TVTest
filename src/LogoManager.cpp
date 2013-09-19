#include "stdafx.h"
#include "TVTest.h"
#include "AppMain.h"
#include "LogoManager.h"
#include "HelperClass/NFile.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


#define MAX_LOGO_BYTES 1296

#define PNG_SIGNATURE		"\x89PNG\r\n\x1A\n"
#define PNG_SIGNATURE_BYTES	8


#include <pshpack1.h>

/*
	ロゴファイルのフォーマット
	┌──────────┐
	│LogoFileHeader      │
	├──────────┤
	│┌────────┐│
	││LogoImageHeader ││
	│├────────┤│
	││PNGデータ       ││
	│├────────┤│
	││CRC32           ││
	│└────────┘│
	│...                 │
	└──────────┘
*/

struct LogoFileHeader {
	char Type[8];
	DWORD Version;
	DWORD NumImages;
};

#define LOGOFILEHEADER_TYPE		"LogoData"
#define LOGOFILEHEADER_VERSION	1

struct LogoImageHeader {
	WORD NetworkID;
	WORD LogoID;
	WORD LogoVersion;
	BYTE LogoType;
	BYTE Reserved;
	WORD DataSize;
};

struct LogoImageHeader2 {
	WORD NetworkID;
	WORD LogoID;
	WORD LogoVersion;
	BYTE LogoType;
	BYTE Reserved1;
	WORD DataSize;
	BYTE Reserved2[6];
	ULONGLONG Time;
};

#include <poppack.h>


static int CompareLogoVersion(WORD Version1,WORD Version2)
{
	if (Version1==Version2)
		return 0;
	if ((Version1<=2047 && Version2<=2047) || (Version1>=2048 && Version2>=2048))
		return Version1<Version2?-1:1;
	if (Version1<=2047)
		return 1;
	return -1;
}


static ULONGLONG SystemTimeToUInt64(const SYSTEMTIME &Time)
{
	if (Time.wYear==0)
		return 0;
	FILETIME ft;
	if (!::SystemTimeToFileTime(&Time,&ft))
		return 0;
	return (((ULONGLONG)ft.dwHighDateTime<<32) | ft.dwLowDateTime)/FILETIME_SECOND;
}


static SYSTEMTIME UInt64ToSystemTime(ULONGLONG Time)
{
	SYSTEMTIME st;
	::ZeroMemory(&st,sizeof(st));
	if (Time!=0) {
		FILETIME ft;
		Time*=FILETIME_SECOND;
		ft.dwLowDateTime=(DWORD)(Time>>32);
		ft.dwHighDateTime=(DWORD)(Time&0xFFFFFFFFUL);
		::FileTimeToSystemTime(&ft,&st);
	}
	return st;
}




CLogoManager::CLogoManager()
	: m_fSaveLogo(false)
	, m_fSaveBmp(false)
	, m_fLogoUpdated(false)
	, m_fLogoIDMapUpdated(false)
	, m_LogoFileLastWriteTime(FILETIME_NULL)
	, m_LogoIDMapFileLastWriteTime(FILETIME_NULL)
{
	::lstrcpy(m_szLogoDirectory,TEXT(".\\Logo"));
}


CLogoManager::~CLogoManager()
{
	Clear();
}


void CLogoManager::Clear()
{
	for (LogoMap::iterator itr=m_LogoMap.begin();itr!=m_LogoMap.end();++itr)
		delete itr->second;
	m_LogoMap.clear();
}


bool CLogoManager::SetLogoDirectory(LPCTSTR pszDirectory)
{
	CBlockLock Lock(&m_Lock);

	::lstrcpy(m_szLogoDirectory,pszDirectory);
	return true;
}


bool CLogoManager::SetSaveLogo(bool fSave)
{
	CBlockLock Lock(&m_Lock);

	m_fSaveLogo=fSave;
	return true;
}


bool CLogoManager::SetSaveLogoBmp(bool fSave)
{
	CBlockLock Lock(&m_Lock);

	m_fSaveBmp=fSave;
	return true;
}


bool CLogoManager::AssociateLogoID(WORD NetworkID,WORD ServiceID,WORD LogoID)
{
	CBlockLock Lock(&m_Lock);

	SetLogoIDMap(NetworkID,ServiceID,LogoID);
	return true;
}


bool CLogoManager::SaveLogoFile(LPCTSTR pszFileName)
{
	CBlockLock Lock(&m_Lock);

	if (m_LogoMap.empty())
		return true;

	// ファイルが読み込んだ時から更新されている場合読み込み直す
	// (複数起動して他のプロセスが更新した可能性があるため)
	if (m_LogoFileLastWriteTime.dwLowDateTime!=0
			|| m_LogoFileLastWriteTime.dwHighDateTime!=0) {
		WIN32_FIND_DATA fd;
		HANDLE hFind=::FindFirstFile(pszFileName,&fd);
		if (hFind!=INVALID_HANDLE_VALUE) {
			::FindClose(hFind);
			if (::CompareFileTime(&fd.ftLastWriteTime,&m_LogoFileLastWriteTime)>0) {
				TRACE(TEXT("CLogoManager::SaveLogoFile() : Reload file\n"));
				LoadLogoFile(pszFileName);
			}
		}
	}

	CNFile File;

	if (!File.Open(pszFileName,CNFile::CNF_WRITE | CNFile::CNF_NEW))
		return false;

	LogoFileHeader FileHeader;
	::CopyMemory(FileHeader.Type,LOGOFILEHEADER_TYPE,8);
	FileHeader.Version=LOGOFILEHEADER_VERSION;
	FileHeader.NumImages=(DWORD)m_LogoMap.size();
	if (!File.Write(&FileHeader,sizeof(FileHeader)))
		goto OnError;

	for (LogoMap::const_iterator itr=m_LogoMap.begin();itr!=m_LogoMap.end();++itr) {
		LogoImageHeader2 ImageHeader;

		::ZeroMemory(&ImageHeader,sizeof(ImageHeader));
		ImageHeader.NetworkID=itr->second->GetNetworkID();
		ImageHeader.LogoID=itr->second->GetLogoID();
		ImageHeader.LogoVersion=itr->second->GetLogoVersion();
		ImageHeader.LogoType=itr->second->GetLogoType();
		ImageHeader.DataSize=itr->second->GetDataSize();
		ImageHeader.Time=SystemTimeToUInt64(itr->second->GetTime());
		DWORD CRC=CCrcCalculator::CalcCrc32((const BYTE*)&ImageHeader,sizeof(ImageHeader));
		CRC=CCrcCalculator::CalcCrc32(itr->second->GetData(),ImageHeader.DataSize,CRC);
		if (!File.Write(&ImageHeader,sizeof(ImageHeader))
				|| !File.Write(itr->second->GetData(),ImageHeader.DataSize)
				|| !File.Write(&CRC,sizeof(CRC)))
			goto OnError;
	}

	return true;

OnError:
	File.Close();
	::DeleteFile(pszFileName);
	return false;
}


bool CLogoManager::LoadLogoFile(LPCTSTR pszFileName)
{
	CBlockLock Lock(&m_Lock);

	CNFile File;

	if (!File.Open(pszFileName,
			CNFile::CNF_READ | CNFile::CNF_SHAREREAD | CNFile::CNF_SEQUENTIALREAD)) {
		TRACE(TEXT("CLogoManager::LoadLogoFile() : File open error \"%s\"\n"),pszFileName);
		return false;
	}

	LogoFileHeader FileHeader;
	if (File.Read(&FileHeader,sizeof(FileHeader))!=sizeof(FileHeader)
			|| std::memcmp(FileHeader.Type,LOGOFILEHEADER_TYPE,8)!=0
			|| FileHeader.Version>LOGOFILEHEADER_VERSION
			|| FileHeader.NumImages==0) {
		TRACE(TEXT("CLogoManager::LoadLogoFile() : File header error\n"));
		return false;
	}

	const DWORD ImageHeaderSize=
		FileHeader.Version==0?sizeof(LogoImageHeader):sizeof(LogoImageHeader2);
	BYTE Buffer[MAX_LOGO_BYTES];

	for (DWORD i=0;i<FileHeader.NumImages;i++) {
		LogoImageHeader2 ImageHeader;

		if (File.Read(&ImageHeader,ImageHeaderSize)!=ImageHeaderSize
				|| ImageHeader.LogoType>0x05
				|| ImageHeader.DataSize<=PNG_SIGNATURE_BYTES
				|| ImageHeader.DataSize>MAX_LOGO_BYTES) {
			TRACE(TEXT("CLogoManager::LoadLogoFile() : Image header error\n"));
			return false;
		}

		DWORD CRC;
		if (File.Read(Buffer,ImageHeader.DataSize)!=ImageHeader.DataSize
				|| std::memcmp(Buffer,PNG_SIGNATURE,PNG_SIGNATURE_BYTES)!=0
				|| File.Read(&CRC,sizeof(CRC))!=sizeof(CRC)
				|| CRC!=CCrcCalculator::CalcCrc32(Buffer,ImageHeader.DataSize,
					CCrcCalculator::CalcCrc32((const BYTE*)&ImageHeader,ImageHeaderSize))) {
			return false;
		}

		CLogoDownloader::LogoData Data;
		Data.OriginalNetworkID=ImageHeader.NetworkID;
		Data.LogoID=ImageHeader.LogoID;
		Data.LogoVersion=ImageHeader.LogoVersion;
		Data.LogoType=ImageHeader.LogoType;
		Data.DataSize=ImageHeader.DataSize;
		Data.pData=Buffer;
		if (FileHeader.Version==0)
			::ZeroMemory(&Data.Time,sizeof(SYSTEMTIME));
		else
			Data.Time=UInt64ToSystemTime(ImageHeader.Time);

		ULONGLONG Key=GetMapKey(Data.OriginalNetworkID,Data.LogoID,Data.LogoType);
		LogoMap::iterator itr=m_LogoMap.find(Key);
		CLogoData *pLogoData;
		if (itr!=m_LogoMap.end()) {
			if (CompareLogoVersion(itr->second->GetLogoVersion(),Data.LogoVersion)<0) {
				delete itr->second;
				pLogoData=new CLogoData(&Data);
				m_LogoMap[Key]=pLogoData;
			}
		} else {
			pLogoData=new CLogoData(&Data);
			m_LogoMap.insert(std::pair<ULONGLONG,CLogoData*>(Key,pLogoData));
		}
	}

	File.GetTime(NULL,NULL,&m_LogoFileLastWriteTime);

	return true;
}


bool CLogoManager::SaveLogoIDMap(LPCTSTR pszFileName)
{
	CBlockLock Lock(&m_Lock);

	if (m_LogoIDMap.empty())
		return true;

	// ファイルが読み込んだ時から更新されている場合読み込み直す
	// (複数起動して他のプロセスが更新した可能性があるため)
	if (m_LogoIDMapFileLastWriteTime.dwLowDateTime!=0
			|| m_LogoIDMapFileLastWriteTime.dwHighDateTime!=0) {
		WIN32_FIND_DATA fd;
		HANDLE hFind=::FindFirstFile(pszFileName,&fd);
		if (hFind!=INVALID_HANDLE_VALUE) {
			::FindClose(hFind);
			if (::CompareFileTime(&fd.ftLastWriteTime,&m_LogoIDMapFileLastWriteTime)>0) {
				TRACE(TEXT("CLogoManager::SaveLogoIDMap() : Reload file\n"));
				LoadLogoIDMap(pszFileName);
			}
		}
	}

	for (LogoIDMap::const_iterator itr=m_LogoIDMap.begin();itr!=m_LogoIDMap.end();++itr) {
		TCHAR szKey[16],szText[16];

		::wsprintf(szKey,TEXT("%08lX"),itr->first);
		::wsprintf(szText,TEXT("%d"),itr->second);
		::WritePrivateProfileString(TEXT("LogoIDMap"),szKey,szText,pszFileName);
	}
	return true;
}


bool CLogoManager::LoadLogoIDMap(LPCTSTR pszFileName)
{
	CBlockLock Lock(&m_Lock);

	m_fLogoIDMapUpdated=false;

	WIN32_FIND_DATA fd;
	HANDLE hFind=::FindFirstFile(pszFileName,&fd);
	if (hFind!=INVALID_HANDLE_VALUE) {
		::FindClose(hFind);
		m_LogoIDMapFileLastWriteTime=fd.ftLastWriteTime;
	}

	LPTSTR pszSection=new TCHAR[32767];
	if (::GetPrivateProfileSection(TEXT("LogoIDMap"),pszSection,32767,pszFileName)>2) {
		for (LPTSTR p=pszSection;*p!=_T('\0');p+=::lstrlen(p)+1) {
			int i;
			DWORD Key=0;
			for (i=0;i<8;i++) {
				Key<<=4;
				if (p[i]>=_T('0') && p[i]<=_T('9'))
					Key|=p[i]-_T('0');
				else if (p[i]>=_T('a') && p[i]<=_T('z'))
					Key|=p[i]-_T('a')+10;
				else if (p[i]>=_T('A') && p[i]<=_T('Z'))
					Key|=p[i]-_T('A')+10;
				else
					break;
			}
			if (i==8 && p[i]==_T('=')) {
				m_LogoIDMap.insert(std::pair<DWORD,WORD>(Key,(WORD)::StrToInt(&p[i+1])));
			}
		}
	}
	delete [] pszSection;

	return true;
}


HBITMAP CLogoManager::GetLogoBitmap(WORD NetworkID,WORD LogoID,BYTE LogoType)
{
	CBlockLock Lock(&m_Lock);

	ULONGLONG Key;
	LogoMap::iterator itr;
	if (LogoType==LOGOTYPE_SMALL || LogoType==LOGOTYPE_BIG) {
		static const BYTE SmallLogoPriority[] = {2, 0, 1, 5, 3, 4};
		static const BYTE BigLogoPriority[] = {5, 3, 4, 2, 0, 1};
		const BYTE *pPriority=LogoType==LOGOTYPE_SMALL?SmallLogoPriority:BigLogoPriority;
		for (BYTE i=0;i<=5;i++) {
			Key=GetMapKey(NetworkID,LogoID,pPriority[i]);
			itr=m_LogoMap.find(Key);
			if (itr!=m_LogoMap.end())
				break;
		}
		if (itr==m_LogoMap.end())
			LogoType=pPriority[0];
	} else {
		Key=GetMapKey(NetworkID,LogoID,LogoType);
		itr=m_LogoMap.find(Key);
	}
	if (itr==m_LogoMap.end()) {
		CLogoData *pLogoData=LoadLogoData(NetworkID,LogoID,LogoType);
		if (pLogoData!=NULL) {
			m_LogoMap.insert(std::pair<ULONGLONG,CLogoData*>(Key,pLogoData));
			m_fLogoUpdated=true;
			return pLogoData->GetBitmap(&m_ImageCodec);
		}
		return NULL;
	}
	return itr->second->GetBitmap(&m_ImageCodec);
}


HBITMAP CLogoManager::GetAssociatedLogoBitmap(WORD NetworkID,WORD ServiceID,BYTE LogoType)
{
	CBlockLock Lock(&m_Lock);
	LogoIDMap::iterator itr=m_LogoIDMap.find(GetIDMapKey(NetworkID,ServiceID));
	if (itr==m_LogoIDMap.end())
		return NULL;
	return GetLogoBitmap(NetworkID,itr->second,LogoType);
}


const CGdiPlus::CImage *CLogoManager::GetLogoImage(WORD NetworkID,WORD LogoID,BYTE LogoType)
{
	CBlockLock Lock(&m_Lock);
	ULONGLONG Key=GetMapKey(NetworkID,LogoID,LogoType);
	LogoMap::iterator itr=m_LogoMap.find(Key);
	if (itr==m_LogoMap.end())
		return NULL;
	return itr->second->GetImage(&m_ImageCodec);
}


const CGdiPlus::CImage *CLogoManager::GetAssociatedLogoImage(WORD NetworkID,WORD ServiceID,BYTE LogoType)
{
	CBlockLock Lock(&m_Lock);
	LogoIDMap::iterator itr=m_LogoIDMap.find(GetIDMapKey(NetworkID,ServiceID));
	if (itr==m_LogoIDMap.end())
		return NULL;
	return GetLogoImage(NetworkID,itr->second,LogoType);
}


HICON CLogoManager::CreateLogoIcon(WORD NetworkID,WORD ServiceID,int Width,int Height)
{
	if (Width<1 || Height<1)
		return NULL;

	HBITMAP hbm=GetAssociatedLogoBitmap(NetworkID,ServiceID,
		(Width<=36 && Height<=24)?LOGOTYPE_SMALL:LOGOTYPE_BIG);
	if (hbm==NULL)
		return NULL;

	// 本来の比率より縦長にしている(見栄えのため)
	int ImageWidth=Height*16/10;
	int ImageHeight=Width*10/16;
	return CreateIconFromBitmap(hbm,Width,Height,
								min(Width,ImageWidth),
								min(Height,ImageHeight));
}


bool CLogoManager::IsLogoAvailable(WORD NetworkID,WORD ServiceID,BYTE LogoType)
{
	CBlockLock Lock(&m_Lock);
	LogoMap::iterator itr=m_LogoMap.find(GetMapKey(NetworkID,ServiceID,LogoType));
	return itr!=m_LogoMap.end();
}


DWORD CLogoManager::GetAvailableLogoType(WORD NetworkID,WORD ServiceID)
{
	CBlockLock Lock(&m_Lock);

	LogoIDMap::const_iterator itrID=m_LogoIDMap.find(GetIDMapKey(NetworkID,ServiceID));
	if (itrID==m_LogoIDMap.end())
		return 0;
	const WORD LogoID=itrID->second;
	DWORD Flags=0;
	for (BYTE i=0;i<=5;i++) {
		LogoMap::const_iterator itrLogo=m_LogoMap.find(GetMapKey(NetworkID,LogoID,i));
		if (itrLogo!=m_LogoMap.end())
			Flags|=1<<i;
	}
	return Flags;
}


void CLogoManager::OnLogoDownloaded(const CLogoDownloader::LogoData *pData)
{
	// 透明なロゴは除外
	if (pData->DataSize<=93)
		return;

	CBlockLock Lock(&m_Lock);

	const ULONGLONG Key=GetMapKey(pData->OriginalNetworkID,pData->LogoID,pData->LogoType);
	LogoMap::iterator itr=m_LogoMap.find(Key);
	bool fUpdated=false,fDataUpdated=false;
	CLogoData *pLogoData;
	if (itr!=m_LogoMap.end()) {
		// バージョンが新しい場合のみ更新
		int VerCmp=CompareLogoVersion(itr->second->GetLogoVersion(),pData->LogoVersion);
		if (VerCmp<0
				|| (VerCmp==0 && CompareSystemTime(&itr->second->GetTime(),&pData->Time)<0)) {
			// BS/CSはバージョンが共通のため、データを比較して更新を確認する
			if (pData->DataSize!=itr->second->GetDataSize()
					|| std::memcmp(pData->pData,itr->second->GetData(),pData->DataSize)!=0) {
				pLogoData=new CLogoData(pData);
				delete itr->second;
				itr->second=pLogoData;
				fUpdated=true;
				fDataUpdated=true;
			} else if (VerCmp<0) {
				itr->second->SetLogoVersion(pData->LogoVersion);
				fUpdated=true;
			}
		}
	} else {
		pLogoData=new CLogoData(pData);
		m_LogoMap.insert(std::pair<ULONGLONG,CLogoData*>(Key,pLogoData));
		fUpdated=true;
		fDataUpdated=true;
	}

	if (fUpdated)
		m_fLogoUpdated=true;

	if (pData->ServiceList.size()>0) {
		for (size_t i=0;i<pData->ServiceList.size();i++) {
			const CLogoDownloader::LogoService &Service=pData->ServiceList[i];
			SetLogoIDMap(Service.OriginalNetworkID,Service.ServiceID,pData->LogoID,fUpdated);
		}
	}

	if (fDataUpdated && (m_fSaveLogo || m_fSaveBmp)) {
		TCHAR szFilePath[MAX_PATH],szDirectory[MAX_PATH],szFileName[MAX_PATH];

		if (!GetAbsolutePath(m_szLogoDirectory,szDirectory,lengthof(szDirectory)))
			return;
		if (!::PathIsDirectory(szDirectory)) {
			// 一階層のみ自動的に作成
			if (!::CreateDirectory(szDirectory,NULL)
					&& ::GetLastError()!=ERROR_ALREADY_EXISTS)
				return;
		}
		if (m_fSaveLogo) {
			::wsprintf(szFileName,TEXT("%04X_%03X_%03X_%02X"),
					   pData->OriginalNetworkID,pData->LogoID,pData->LogoVersion,pData->LogoType);
			::PathCombine(szFilePath,szDirectory,szFileName);
			if (!::PathFileExists(szFilePath))
				pLogoData->SaveToFile(szFilePath);
		}
		if (m_fSaveBmp) {
			::wsprintf(szFileName,TEXT("%04X_%03X_%03X_%02X.bmp"),
					   pData->OriginalNetworkID,pData->LogoID,pData->LogoVersion,pData->LogoType);
			::PathCombine(szFilePath,szDirectory,szFileName);
			if (!::PathFileExists(szFilePath))
				pLogoData->SaveBmpToFile(&m_ImageCodec,szFilePath);
		}
	}
}


bool CLogoManager::SetLogoIDMap(WORD NetworkID,WORD ServiceID,WORD LogoID,bool fUpdate)
{
	const DWORD Key=GetIDMapKey(NetworkID,ServiceID);
	LogoIDMap::iterator i=m_LogoIDMap.find(Key);

	if (i==m_LogoIDMap.end()) {
		TRACE(TEXT("Logo ID mapped : NID %04x / SID %04x / Logo ID %04x\n"),
			  NetworkID,ServiceID,LogoID);
		m_LogoIDMap.insert(std::pair<DWORD,WORD>(Key,LogoID));
		m_fLogoIDMapUpdated=true;
	} else if (fUpdate && i->second!=LogoID) {
		TRACE(TEXT("Logo ID changed : NID %04x / SID %04x / Logo ID %04x -> %04x\n"),
			  NetworkID,ServiceID,i->second,LogoID);
		i->second=LogoID;
		m_fLogoIDMapUpdated=true;
	} else {
		return false;
	}
	return true;
}


CLogoManager::CLogoData *CLogoManager::LoadLogoData(WORD NetworkID,WORD LogoID,BYTE LogoType)
{
	TCHAR szDirectory[MAX_PATH],szFileName[MAX_PATH],szMask[32];

	if (!GetAbsolutePath(m_szLogoDirectory,szDirectory,lengthof(szDirectory)))
		return false;
	// 最もバージョンが新しいロゴを探す
	::wsprintf(szMask,TEXT("%04X_%03X_\?\?\?_%02X"),NetworkID,LogoID,LogoType);
	if (::lstrlen(szDirectory)+1+::lstrlen(szMask)>=lengthof(szFileName))
		return false;
	::PathCombine(szFileName,szDirectory,szMask);
	WIN32_FIND_DATA fd;
	HANDLE hFind=::FindFirstFile(szFileName,&fd);
	if (hFind==INVALID_HANDLE_VALUE)
		return NULL;
	int NewerVersion=-1;
	do {
		if ((fd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)==0) {
			int Version;
			if (::StrToIntEx(&fd.cFileName[9],STIF_SUPPORT_HEX,&Version)) {
				if (Version>NewerVersion) {
					NewerVersion=Version;
					::lstrcpy(szFileName,fd.cFileName);
				}
			}
		}
	} while (::FindNextFile(hFind,&fd));
	::FindClose(hFind);
	if (NewerVersion<0)
		return NULL;

	// ロゴをファイルから読み込む
	::PathAppend(szDirectory,szFileName);
	HANDLE hFile=::CreateFile(szDirectory,GENERIC_READ,FILE_SHARE_READ,NULL,
							  OPEN_EXISTING,0,NULL);
	if (hFile==INVALID_HANDLE_VALUE)
		return NULL;
	LARGE_INTEGER FileSize;
	if (!::GetFileSizeEx(hFile,&FileSize)
			|| FileSize.QuadPart<64 || FileSize.QuadPart>1024) {
		::CloseHandle(hFile);
		return NULL;
	}
	BYTE *pData=new BYTE[FileSize.LowPart];
	DWORD Read;
	if (!::ReadFile(hFile,pData,FileSize.LowPart,&Read,NULL)
			|| Read!=FileSize.LowPart
			|| std::memcmp(pData,PNG_SIGNATURE,PNG_SIGNATURE_BYTES)!=0) {
		delete [] pData;
		::CloseHandle(hFile);
		return NULL;
	}
	::CloseHandle(hFile);
	CLogoDownloader::LogoData LogoData;
	LogoData.OriginalNetworkID=NetworkID;
	LogoData.LogoID=LogoID;
	LogoData.LogoVersion=(WORD)NewerVersion;
	LogoData.LogoType=LogoType;
	LogoData.DataSize=(WORD)Read;
	LogoData.pData=pData;
	::ZeroMemory(&LogoData.Time,sizeof(SYSTEMTIME));
	CLogoData *pLogoData=new CLogoData(&LogoData);
	delete [] pData;
	return pLogoData;
}




CLogoManager::CLogoData::CLogoData(const CLogoDownloader::LogoData *pData)
	: m_NetworkID(pData->OriginalNetworkID)
	, m_LogoID(pData->LogoID)
	, m_LogoVersion(pData->LogoVersion)
	, m_LogoType(pData->LogoType)
	, m_DataSize(pData->DataSize)
	, m_Time(pData->Time)
	, m_hbm(NULL)
{
	m_pData=new BYTE[pData->DataSize];
	::CopyMemory(m_pData,pData->pData,pData->DataSize);
}


CLogoManager::CLogoData::CLogoData(const CLogoData &Src)
	: m_pData(NULL)
	, m_hbm(NULL)
{
	*this=Src;
}


CLogoManager::CLogoData::~CLogoData()
{
	delete [] m_pData;
	if (m_hbm!=NULL)
		::DeleteObject(m_hbm);
}


CLogoManager::CLogoData &CLogoManager::CLogoData::operator=(const CLogoData &Src)
{
	if (&Src!=this) {
		if (m_hbm!=NULL) {
			::DeleteObject(m_hbm);
			m_hbm=NULL;
		}
		m_Image.Free();
		m_NetworkID=Src.m_NetworkID;
		m_LogoID=Src.m_LogoID;
		m_LogoVersion=Src.m_LogoVersion;
		m_LogoType=Src.m_LogoType;
		m_DataSize=Src.m_DataSize;
		m_pData=new BYTE[Src.m_DataSize];
		::CopyMemory(m_pData,Src.m_pData,Src.m_DataSize);
		m_Time=Src.m_Time;
	}
	return *this;
}


HBITMAP CLogoManager::CLogoData::GetBitmap(CImageCodec *pCodec)
{
	if (m_hbm==NULL) {
		HGLOBAL hDIB=pCodec->LoadAribPngFromMemory(m_pData,m_DataSize);
		if (hDIB==NULL)
			return NULL;
		BITMAPINFO *pbmi=(BITMAPINFO*)::GlobalLock(hDIB);
		void *pBits;
		m_hbm=::CreateDIBSection(NULL,pbmi,DIB_RGB_COLORS,&pBits,NULL,0);
		if (m_hbm==NULL)
			return NULL;
		::CopyMemory(pBits,(BYTE*)pbmi+CalcDIBInfoSize(&pbmi->bmiHeader),
					 CalcDIBBitsSize(&pbmi->bmiHeader));
		::GlobalUnlock(hDIB);
		::GlobalFree(hDIB);
	}
	return m_hbm;
}


const CGdiPlus::CImage *CLogoManager::CLogoData::GetImage(CImageCodec *pCodec)
{
	if (!m_Image.IsCreated()) {
		HGLOBAL hDIB=pCodec->LoadAribPngFromMemory(m_pData,m_DataSize);
		if (hDIB==NULL)
			return NULL;
		BITMAPINFO *pbmi=(BITMAPINFO*)::GlobalLock(hDIB);
		bool fResult=m_Image.CreateFromDIB(pbmi,(BYTE*)pbmi+CalcDIBInfoSize(&pbmi->bmiHeader));
		::GlobalUnlock(hDIB);
		::GlobalFree(hDIB);
		if (!fResult)
			return NULL;
	}
	return &m_Image;
}


bool CLogoManager::CLogoData::SaveToFile(LPCTSTR pszFileName) const
{
	HANDLE hFile=::CreateFile(pszFileName,GENERIC_WRITE,FILE_SHARE_READ,NULL,
							  CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
	if (hFile==INVALID_HANDLE_VALUE)
		return false;
	DWORD Write;
	if (!WriteFile(hFile,m_pData,m_DataSize,&Write,NULL) || Write!=m_DataSize) {
		::CloseHandle(hFile);
		return false;
	}
	::CloseHandle(hFile);
	return true;
}


bool CLogoManager::CLogoData::SaveBmpToFile(CImageCodec *pCodec,LPCTSTR pszFileName) const
{
	HGLOBAL hDIB=pCodec->LoadAribPngFromMemory(m_pData,m_DataSize);
	if (hDIB==NULL)
		return false;
	HANDLE hFile=::CreateFile(pszFileName,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
	if (hFile==INVALID_HANDLE_VALUE)
		return false;
	BITMAPINFOHEADER *pbmih=static_cast<BITMAPINFOHEADER*>(::GlobalLock(hDIB));
	DWORD InfoSize=(DWORD)CalcDIBInfoSize(pbmih),BitsSize=(DWORD)CalcDIBBitsSize(pbmih);
	DWORD Write;
	BITMAPFILEHEADER bmfh;
	bmfh.bfType=0x4D42;
	bmfh.bfSize=sizeof(BITMAPFILEHEADER)+InfoSize+BitsSize;
	bmfh.bfReserved1=0;
	bmfh.bfReserved2=0;
	bmfh.bfOffBits=sizeof(BITMAPFILEHEADER)+InfoSize;
	bool fResult=
		::WriteFile(hFile,&bmfh,sizeof(bmfh),&Write,NULL) && Write==sizeof(bmfh)
		&& ::WriteFile(hFile,pbmih,InfoSize+BitsSize,&Write,NULL) && Write==InfoSize+BitsSize;
	::CloseHandle(hFile);
	::GlobalUnlock(hDIB);
	::GlobalFree(hDIB);
	return fResult;
}
