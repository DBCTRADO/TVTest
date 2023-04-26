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


#ifndef TVTEST_MAIN_DISPLAY_H
#define TVTEST_MAIN_DISPLAY_H


#include "View.h"
#include "HomeDisplay.h"
#include "ChannelDisplay.h"


namespace TVTest
{

	class CAppMain;

	class CMainDisplay
	{
	public:
		class CHomeDisplayEventHandler
			: public CHomeDisplay::CHomeDisplayEventHandler
			, protected CDisplayEventHandlerBase
		{
			void OnClose() override;
			void OnMouseMessage(UINT Msg, int x, int y) override;
		};

		class CChannelDisplayEventHandler
			: public CChannelDisplay::CChannelDisplayEventHandler
			, protected CDisplayEventHandlerBase
		{
		public:
			CChannelDisplayEventHandler(CAppMain &App);

			void OnTunerSelect(LPCTSTR pszDriverFileName, int TuningSpace) override;
			void OnChannelSelect(LPCTSTR pszDriverFileName, const CChannelInfo *pChannelInfo) override;
			void OnClose() override;
			void OnMouseMessage(UINT Msg, int x, int y) override;

		private:
			CAppMain &m_App;
		};

		CMainDisplay(CAppMain &App);

		bool Create(HWND hwndParent, int ViewID, int ContainerID, HWND hwndMessage);
		bool EnableViewer(bool fEnable);
		bool IsViewerEnabled() const { return m_fViewerEnabled; }
		bool BuildViewer(BYTE VideoStreamType = 0);
		bool CloseViewer();
		CViewWindow &GetViewWindow() { return m_ViewWindow; }
		const CViewWindow &GetViewWindow() const { return m_ViewWindow; }
		CVideoContainerWindow &GetVideoContainer() { return m_VideoContainer; }
		const CVideoContainerWindow &GetVideoContainer() const { return m_VideoContainer; }
		CDisplayBase &GetDisplayBase() { return m_DisplayBase; }
		const CDisplayBase &GetDisplayBase() const { return m_DisplayBase; }
		HWND GetDisplayViewParent() const;

		CHomeDisplayEventHandler HomeDisplayEventHandler;
		CChannelDisplayEventHandler ChannelDisplayEventHandler;

	protected:
		CAppMain &m_App;
		bool m_fViewerEnabled = false;
		CViewWindow m_ViewWindow;
		CVideoContainerWindow m_VideoContainer;
		CDisplayBase m_DisplayBase;
	};

}	// namespace TVTest


#endif
