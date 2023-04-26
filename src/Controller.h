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


#ifndef TVTEST_CONTROLLER_H
#define TVTEST_CONTROLLER_H


#include <vector>
#include <memory>
#include "Options.h"
#include "Tooltip.h"


namespace TVTest
{

	class ABSTRACT_CLASS(CController)
	{
	public:
		class ABSTRACT_CLASS(CEventHandler)
		{
		public:
			virtual ~CEventHandler() = default;
			virtual bool OnButtonDown(CController * pController, int Index) = 0;
		};

		struct ButtonInfo
		{
			LPCTSTR pszName;
			int DefaultCommand;

			struct
			{
				WORD Left, Top, Width, Height;
			} ImageButtonRect;

			struct
			{
				WORD Left, Top;
			} ImageSelButtonPos;
		};

		enum class ImageType {
			Controller,
			SelButtons,
		};

		virtual ~CController() = default;

		virtual LPCTSTR GetName() const = 0;
		virtual LPCTSTR GetText() const = 0;
		virtual int NumButtons() const = 0;
		virtual bool GetButtonInfo(int Index, ButtonInfo * pInfo) const = 0;
		virtual bool Enable(bool fEnable) = 0;
		virtual bool IsEnabled() const = 0;
		virtual bool IsActiveOnly() const { return false; }
		virtual bool SetTargetWindow(HWND hwnd) = 0;
		virtual bool TranslateMessage(HWND hwnd, MSG * pMessage);
		virtual HBITMAP GetImage(ImageType Type) const;
		virtual bool GetIniFileName(LPTSTR pszFileName, int MaxLength) const;
		virtual LPCTSTR GetIniFileSection() const;
		void SetEventHandler(CEventHandler * pEventHandler);

	protected:
		CEventHandler *m_pEventHandler = nullptr;

		bool OnButtonDown(int Index);
	};

	class CControllerManager
		: public COptions
		, public CController::CEventHandler
	{
	public:
		struct ControllerSettings
		{
			std::vector<WORD> AssignList;
			bool fActiveOnly;

			bool operator==(const ControllerSettings &Operand) const;
		};

		~CControllerManager();

	// CSettingsBase
		bool ReadSettings(CSettings &Settings) override;
		bool WriteSettings(CSettings &Settings) override;

	// CBasicDialog
		bool Create(HWND hwndOwner) override;

	// CControllerManager
		bool AddController(CController *pController);
		bool DeleteController(LPCTSTR pszName);
		void DeleteAllControllers();
		bool IsControllerEnabled(LPCTSTR pszName) const;
		bool LoadControllerSettings(LPCTSTR pszName);
		bool SaveControllerSettings(LPCTSTR pszName) const;
		bool TranslateMessage(HWND hwnd, MSG *pMessage);
		bool IsFocus() const { return m_fFocus; }
		bool IsActive() const { return m_fActive; }
		bool OnActiveChange(HWND hwnd, bool fActive);
		bool OnFocusChange(HWND hwnd, bool fFocus);
		bool OnButtonDown(LPCTSTR pszName, int Button) const;
		const ControllerSettings *GetControllerSettings(LPCTSTR pszName) const;

	private:
		struct ControllerInfo
		{
			std::unique_ptr<CController> Controller;
			bool fSettingsLoaded = false;
			bool fSettingsChanged = false;
			ControllerSettings Settings;

			ControllerInfo(CController *p)
				: Controller(p)
			{
			}
		};

		std::vector<ControllerInfo> m_ControllerList;
		std::vector<ControllerSettings> m_CurSettingsList;
		bool m_fFocus = false;
		bool m_fActive = false;
		String m_CurController;
		HBITMAP m_hbmController = nullptr;
		HBITMAP m_hbmSelButtons = nullptr;
		RECT m_ImageRect{};
		CTooltip m_Tooltip;

	// CController::CEventHandler
		bool OnButtonDown(CController *pController, int Index) override;

	// CBasicDialog
		INT_PTR DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

	// CUIBase
		void RealizeStyle() override;

	// CControllerManager
		int FindController(LPCTSTR pszName) const;
		void InitDlgItems();
		void SetButtonCommand(HWND hwndList, int Index, int Command);
		void SetDlgItemStatus();
		CController *GetCurController() const;
	};

}	// namespace TVTest


#endif
