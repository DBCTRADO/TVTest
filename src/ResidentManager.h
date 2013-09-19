#ifndef RESIDENT_MANAGER_H
#define RESIDENT_MANAGER_H


class CResidentManager
{
	HWND m_hwnd;
	UINT m_TrayIconMessage;
	bool m_fResident;
	bool m_fMinimizeToTray;
	UINT m_Status;
	UINT m_TaskbarCreatedMessage;
	CDynamicString m_TipText;
	bool AddTrayIcon();
	bool RemoveTrayIcon();
	bool ChangeTrayIcon();
	bool UpdateTipText();
	bool IsTrayIconVisible() const;

public:
	enum {
		STATUS_RECORDING	=0x00000001,
		STATUS_MINIMIZED	=0x00000002
	};
	CResidentManager();
	~CResidentManager();
	bool Initialize(HWND hwnd,UINT Message);
	void Finalize();
	bool SetResident(bool fResident);
	bool GetResident() const { return m_fResident; }
	bool SetMinimizeToTray(bool fMinimizeToTray);
	bool GetMinimizeToTray() const { return m_fMinimizeToTray; }
	bool SetStatus(UINT Status,UINT Mask=~0U);
	UINT GetStatus() const { return m_Status; }
	bool SetTipText(LPCTSTR pszText);
	enum {
		MESSAGE_ICON_NONE,
		MESSAGE_ICON_INFO,
		MESSAGE_ICON_WARNING,
		MESSAGE_ICON_ERROR
	};
	bool ShowMessage(LPCTSTR pszText,LPCTSTR pszTitle,int Icon=0,DWORD TimeOut=5000);
	bool HandleMessage(UINT Message,WPARAM wParam,LPARAM lParam);
};


#endif
