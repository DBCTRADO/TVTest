#ifndef ACCELERATOR_H
#define ACCELERATOR_H


#include <vector>
#include "Options.h"
#include "Command.h"
#include "RawInput.h"
#include "ChannelInput.h"
#include "ListView.h"


class CMainMenu;

class CAccelerator
	: public COptions
	, public CRawInput::CEventHandler
{
public:
	CAccelerator();
	~CAccelerator();

// COptions
	bool LoadSettings(CSettings &Settings) override;
	bool SaveSettings(CSettings &Settings) override;

// CBasicDialog
	bool Create(HWND hwndOwner) override;

// CAccelerator
	bool Initialize(HWND hwndHotKey,CMainMenu *pMainMenu,
					CSettings &Settings,const CCommandList *pCommandList);
	void Finalize();
	bool TranslateMessage(HWND hwnd,LPMSG pmsg);
	int TranslateHotKey(WPARAM wParam,LPARAM lParam) const;
	int TranslateAppCommand(WPARAM wParam,LPARAM lParam) const;
	LRESULT OnInput(HWND hwnd,WPARAM wParam,LPARAM lParam) {
		return m_RawInput.OnInput(hwnd,wParam,lParam);
	}
	void SetMenuAccel(HMENU hmenu);
	const TVTest::CChannelInputOptions &GetChannelInputOptions() const { return m_ChannelInputOptions; }

private:
	enum {
		COLUMN_COMMAND,
		COLUMN_KEY,
		COLUMN_APPCOMMAND
	};

	HACCEL m_hAccel;
	struct KeyInfo {
		WORD Command;
		WORD KeyCode;
		BYTE Modifiers;
		bool fGlobal;
		bool operator==(const KeyInfo &Info) const {
			return Command==Info.Command && KeyCode==Info.KeyCode
				&& Modifiers==Info.Modifiers && fGlobal==Info.fGlobal;
		}
	};
	std::vector<KeyInfo> m_KeyList;
	enum MediaKeyType {
		MEDIAKEY_APPCOMMAND,
		MEDIAKEY_RAWINPUT
	};
	struct MediaKeyInfo {
		MediaKeyType Type;
		WORD Command;
		LPCTSTR pszText;
	};
	std::vector<MediaKeyInfo> m_MediaKeyList;
	struct AppCommandInfo {
		WORD Command;
		MediaKeyType Type;
		WORD AppCommand;
		bool operator==(const AppCommandInfo &Info) const {
			return Command==Info.Command && Type==Info.Type && AppCommand==Info.AppCommand;
		}
	};
	std::vector<AppCommandInfo> m_AppCommandList;
	HWND m_hwndHotKey;
	CMainMenu *m_pMainMenu;
	const CCommandList *m_pCommandList;
	CRawInput m_RawInput;
	bool m_fRegisterHotKey;
	TVTest::CChannelInputOptions m_ChannelInputOptions;
	TVTest::CListView m_ListView;

// CBasicDialog
	INT_PTR DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam) override;

// CAccelerator
	static const KeyInfo m_DefaultAccelList[];
	static const AppCommandInfo m_DefaultAppCommandList[];
	static void FormatAccelText(LPTSTR pszText,int Key,int Modifiers,bool fGlobal=false);
	void SetMenuAccelText(HMENU hmenu,int Command);
	HACCEL CreateAccel();
	bool RegisterHotKey();
	bool UnregisterHotKey();
	int CheckAccelKey(BYTE Mod,WORD Key);
	int CheckAppCommand(int AppCommand);
	void SetAccelItem(int Index,BYTE Mod,WORD Key,bool fGlobal,BYTE AppCommand);
	void SetDlgItemStatus(HWND hDlg);

// CRawInput::CEventHandler
	void OnInput(int Type) override;
	void OnUnknownInput(const BYTE *pData,int Size) override;
};


#endif
