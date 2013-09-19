#ifndef NETWORK_REMOCON_H
#define NETWORK_REMOCON_H


#include <winsock2.h>
#include "CoreEngine.h"
#include "ChannelManager.h"
#include "Options.h"


class CNetworkRemoconReceiver
{
public:
	virtual void OnReceive(LPCSTR pszText) = 0;
};

class CNetworkRemocon {
	bool m_fInitialized;
	WSADATA m_WSAData;
	DWORD m_Address;
	WORD m_Port;
	HANDLE m_hThread;
	SOCKET m_Socket;
	bool m_fConnected;
	CChannelList m_ChannelList;
	static DWORD WINAPI SendProc(LPVOID pParam);
	bool Send(const char *pBuffer,int Length,
										CNetworkRemoconReceiver *pReceiver=NULL);
public:
	CNetworkRemocon();
	~CNetworkRemocon();
	bool Init(LPCSTR pszAddress,WORD Port);
	bool Shutdown();
	DWORD GetAddress() const { return m_Address; }
	WORD GetPort() const { return m_Port; }
	bool SetChannel(int ChannelNo);
	bool GetChannel(CNetworkRemoconReceiver *pReceiver);
	bool SetService(int Service);
	bool GetDriverList(CNetworkRemoconReceiver *pReceiver);
	bool LoadChannelText(LPCTSTR pszFileName,const CChannelList *pChannelList);
	const CChannelList &GetChannelList() const { return m_ChannelList; }
	CChannelList &GetChannelList() { return m_ChannelList; }
};

class CNetworkRemoconOptions : public COptions
{
	enum {
		UPDATE_NETWORKREMOCON = 0x00000001UL
	};
	bool m_fUseNetworkRemocon;
	char m_szAddress[16];
	unsigned int m_Port;
	TCHAR m_szChannelFileName[MAX_PATH];
	TCHAR m_szDefaultChannelFileName[MAX_PATH];
	bool m_fTempEnable;
	unsigned int m_TempPort;

// CBasicDialog
	INT_PTR DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam) override;

	bool GetChannelFilePath(LPTSTR pszPath) const;

public:
	CNetworkRemoconOptions();
	~CNetworkRemoconOptions();
// COptions
	//bool Apply(DWORD Flags) override;
// CSettingsBase
	bool ReadSettings(CSettings &Settings) override;
	bool WriteSettings(CSettings &Settings) override;
// CBasicDialog
	bool Create(HWND hwndOwner) override;
// CNetworkRemoconOptions
	unsigned int GetPort() const { return m_Port; }
	bool SetTempEnable(bool fEnable);
	bool SetTempPort(unsigned int Port);
	LPCTSTR GetChannelFileName() const { return m_szChannelFileName; }
	bool SetDefaultChannelFileName(LPCTSTR pszFileName);
	bool IsEnable() const;
	bool IsSettingValid() const;
	bool CreateNetworkRemocon(CNetworkRemocon **ppNetworkRemocon);
	bool InitNetworkRemocon(CNetworkRemocon **ppNetworkRemocon,
		const CCoreEngine *pCoreEngine,CChannelManager *pChannelManager) const;
	bool FindChannelFile(LPCTSTR pszDriverName,LPTSTR pszFileName) const;
};


#endif
