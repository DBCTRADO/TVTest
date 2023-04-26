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


#ifndef TVTEST_UI_SKIN_H
#define TVTEST_UI_SKIN_H


#include "AppEvent.h"
#include "NotificationBar.h"
#include "LibISDB/LibISDB/Base/ErrorHandler.hpp"


namespace TVTest
{

	class CUICore;

	class ABSTRACT_CLASS(CUISkin)
		: public CAppEventHandler
	{
	protected:
		CUICore *m_pCore = nullptr;
		bool m_fWheelChannelChanging = false;

		virtual bool InitializeViewer(BYTE VideoStreamType = 0) = 0;
		virtual bool FinalizeViewer() = 0;
		virtual bool EnableViewer(bool fEnable) = 0;
		virtual bool IsViewerEnabled() const = 0;
		virtual HWND GetViewerWindow() const = 0;
		virtual bool SetZoomRate(int Rate, int Factor) = 0;
		virtual bool GetZoomRate(int *pRate, int *pFactor) = 0;
		virtual void SetTitleText(LPCTSTR pszTitleText, LPCTSTR pszWindowText) = 0;
		virtual bool SetTitleFont(const Style::Font & Font) = 0;
		virtual bool SetLogo(HBITMAP hbm) = 0;
		virtual bool SetAlwaysOnTop(bool fTop) = 0;
		virtual bool SetFullscreen(bool fFullscreen) = 0;
		virtual bool SetStandby(bool fStandby) = 0;
		virtual bool ShowVolumeOSD() = 0;
		virtual void BeginWheelChannelSelect(DWORD Delay) {}
		virtual void EndWheelChannelSelect() {}

		void SetWheelChannelChanging(bool fChanging, DWORD Delay = 0);

	public:
		CUISkin() = default;
		virtual ~CUISkin() = default;

		CUISkin(const CUISkin &) = delete;
		CUISkin &operator=(const CUISkin &) = delete;

		virtual HWND GetMainWindow() const = 0;
		virtual HWND GetFullscreenWindow() const = 0;
		virtual HWND GetVideoHostWindow() const = 0;
		virtual const CUIBase * GetUIBase() const = 0;
		virtual const CUIBase * GetFullscreenUIBase() const = 0;
		virtual int ShowMessage(
			LPCTSTR pszText, LPCTSTR pszCaption = nullptr,
			UINT Type = MB_OK | MB_ICONEXCLAMATION) const;
		virtual void ShowErrorMessage(LPCTSTR pszText) const;
		virtual void ShowErrorMessage(
			const LibISDB::ErrorHandler * pErrorHandler,
			LPCTSTR pszTitle = nullptr) const;
		virtual void ShowNotificationBar(
			LPCTSTR pszText,
			CNotificationBar::MessageType Type = CNotificationBar::MessageType::Info,
			DWORD Duration = 0, bool fSkippable = false) = 0;
		bool IsWheelChannelChanging() const { return m_fWheelChannelChanging; }

		friend CUICore;
	};

}	// namespace TVTest


#endif
