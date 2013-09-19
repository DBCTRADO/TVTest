#ifndef LOGO_MANAGER_H
#define LOGO_MANAGER_H


#include <map>
#include "BonTsEngine/LogoDownloader.h"
#include "DrawUtil.h"
#include "Image.h"


class CLogoManager : public CLogoDownloader::ILogoHandler
{
public:
	CLogoManager();
	~CLogoManager();
	void Clear();
	bool SetLogoDirectory(LPCTSTR pszDirectory);
	LPCTSTR GetLogoDirectory() const { return m_szLogoDirectory; }
	bool SetSaveLogo(bool fSave);
	bool GetSaveLogo() const { return m_fSaveLogo; }
	bool SetSaveLogoBmp(bool fSave);
	bool GetSaveLogoBmp() const { return m_fSaveBmp; }
	bool AssociateLogoID(WORD NetworkID,WORD ServiceID,WORD LogoID);
	bool SaveLogoFile(LPCTSTR pszFileName);
	bool LoadLogoFile(LPCTSTR pszFileName);
	bool IsLogoDataUpdated() const { return m_fLogoUpdated; }
	bool SaveLogoIDMap(LPCTSTR pszFileName);
	bool LoadLogoIDMap(LPCTSTR pszFileName);
	bool IsLogoIDMapUpdated() const { return m_fLogoIDMapUpdated; }
	HBITMAP GetLogoBitmap(WORD NetworkID,WORD LogoID,BYTE LogoType);
	HBITMAP GetAssociatedLogoBitmap(WORD NetworkID,WORD ServiceID,BYTE LogoType);
	const CGdiPlus::CImage *GetLogoImage(WORD NetworkID,WORD LogoID,BYTE LogoType);
	const CGdiPlus::CImage *GetAssociatedLogoImage(WORD NetworkID,WORD ServiceID,BYTE LogoType);
	HICON CreateLogoIcon(WORD NetworkID,WORD ServiceID,int Width,int Height);
	bool IsLogoAvailable(WORD NetworkID,WORD ServiceID,BYTE LogoType);
	DWORD GetAvailableLogoType(WORD NetworkID,WORD ServiceID);

	enum {
		LOGOTYPE_SMALL	=0xFF,	// éÊìæÇ≈Ç´ÇÈíÜÇ©ÇÁè¨Ç≥Ç¢Ç‡ÇÃóDêÊ
		LOGOTYPE_BIG	=0xFE	// éÊìæÇ≈Ç´ÇÈíÜÇ©ÇÁëÂÇ´Ç¢Ç‡ÇÃóDêÊ
	};

private:
	class CLogoData
	{
		WORD m_NetworkID;
		WORD m_LogoID;
		WORD m_LogoVersion;
		BYTE m_LogoType;
		WORD m_DataSize;
		BYTE *m_pData;
		SYSTEMTIME m_Time;
		HBITMAP m_hbm;
		CGdiPlus::CImage m_Image;

	public:
		CLogoData(const CLogoDownloader::LogoData *pData);
		CLogoData(const CLogoData &Src);
		~CLogoData();
		CLogoData &operator=(const CLogoData &Src);
		WORD GetNetworkID() const { return m_NetworkID; }
		WORD GetLogoID() const { return m_LogoID; }
		WORD GetLogoVersion() const { return m_LogoVersion; }
		void SetLogoVersion(WORD Version) { m_LogoVersion=Version; }
		BYTE GetLogoType() const { return m_LogoType; }
		WORD GetDataSize() const { return m_DataSize; }
		const BYTE *GetData() const { return m_pData; }
		const SYSTEMTIME &GetTime() const { return m_Time; }
		HBITMAP GetBitmap(CImageCodec *pCodec);
		const CGdiPlus::CImage *GetImage(CImageCodec *pCodec);
		bool SaveToFile(LPCTSTR pszFileName) const;
		bool SaveBmpToFile(CImageCodec *pCodec,LPCTSTR pszFileName) const;
	};

	inline ULONGLONG GetMapKey(WORD NID,WORD LogoID,BYTE LogoType) {
		return ((ULONGLONG)NID<<24) | ((ULONGLONG)LogoID<<8) | LogoType;
	}
	typedef std::map<ULONGLONG,CLogoData*> LogoMap;
	inline DWORD GetIDMapKey(WORD NID,WORD SID) {
		return ((DWORD)NID<<16) | (DWORD)SID;
	}
	typedef std::map<DWORD,WORD> LogoIDMap;

	TCHAR m_szLogoDirectory[MAX_PATH];
	bool m_fSaveLogo;
	bool m_fSaveBmp;
	LogoMap m_LogoMap;
	LogoIDMap m_LogoIDMap;
	CImageCodec m_ImageCodec;
	CCriticalLock m_Lock;
	bool m_fLogoUpdated;
	bool m_fLogoIDMapUpdated;
	FILETIME m_LogoFileLastWriteTime;
	FILETIME m_LogoIDMapFileLastWriteTime;

	bool SetLogoIDMap(WORD NetworkID,WORD ServiceID,WORD LogoID,bool fUpdate=true);
	CLogoData *LoadLogoData(WORD NetworkID,WORD LogoID,BYTE LogoType);

// CLogoDownloader::ILogoHandler
	void OnLogoDownloaded(const CLogoDownloader::LogoData *pData) override;
};


#endif
