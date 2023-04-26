/*
  TVTest
  Copyright(c) 2008-2020 DBCTRADO

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


#ifndef TVTEST_ACCELERATOR_H
#define TVTEST_ACCELERATOR_H


#include <vector>
#include "Options.h"
#include "Command.h"
#include "RawInput.h"
#include "ChannelInput.h"
#include "ListView.h"


namespace TVTest
{

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
		bool Initialize(
			HWND hwndHotKey, CMainMenu *pMainMenu,
			CSettings &Settings, const CCommandManager *pCommandManager);
		void Finalize();
		bool TranslateMessage(HWND hwnd, LPMSG pmsg);
		int TranslateHotKey(WPARAM wParam, LPARAM lParam) const;
		int TranslateAppCommand(WPARAM wParam, LPARAM lParam) const;
		LRESULT OnInput(HWND hwnd, WPARAM wParam, LPARAM lParam) {
			return m_RawInput.OnInput(hwnd, wParam, lParam);
		}
		void SetMenuAccel(HMENU hmenu);
		const CChannelInputOptions &GetChannelInputOptions() const { return m_ChannelInputOptions; }

	private:
		enum {
			COLUMN_COMMAND,
			COLUMN_KEY,
			COLUMN_APPCOMMAND
		};

		HACCEL m_hAccel = nullptr;
		struct KeyInfo {
			WORD Command;
			WORD KeyCode;
			BYTE Modifiers;
			bool fGlobal;
			bool operator==(const KeyInfo &Info) const noexcept = default;
		};
		std::vector<KeyInfo> m_KeyList;
		enum class MediaKeyType {
			AppCommand,
			RawInput,
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
			bool operator==(const AppCommandInfo &Info) const noexcept = default;
		};
		std::vector<AppCommandInfo> m_AppCommandList;
		HWND m_hwndHotKey = nullptr;
		CMainMenu *m_pMainMenu = nullptr;
		const CCommandManager *m_pCommandManager = nullptr;
		CRawInput m_RawInput;
		bool m_fRegisterHotKey = false;
		CChannelInputOptions m_ChannelInputOptions;
		CListView m_ListView;

	// CBasicDialog
		INT_PTR DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

	// CUIBase
		void RealizeStyle() override;

	// CAccelerator
		static const KeyInfo m_DefaultAccelList[];
		static const AppCommandInfo m_DefaultAppCommandList[];
		static void FormatAccelText(LPTSTR pszText, size_t MaxText, int Key, int Modifiers, bool fGlobal = false);
		void SetMenuAccelText(HMENU hmenu, int Command);
		HACCEL CreateAccel();
		bool RegisterHotKey();
		bool UnregisterHotKey();
		int CheckAccelKey(BYTE Mod, WORD Key);
		int CheckAppCommand(int AppCommand);
		void SetAccelItem(int Index, BYTE Mod, WORD Key, bool fGlobal, BYTE AppCommand);
		void SetDlgItemStatus(HWND hDlg);

	// CRawInput::CEventHandler
		void OnInput(int Type) override;
		void OnUnknownInput(const BYTE *pData, int Size) override;
	};

}


#endif
