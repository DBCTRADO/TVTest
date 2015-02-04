#ifndef TVTEST_MAIN_DISPLAY_H
#define TVTEST_MAIN_DISPLAY_H


#include "View.h"
#include "HomeDisplay.h"
#include "ChannelDisplay.h"


class CAppMain;

namespace TVTest
{

	class CMainDisplay
	{
	public:
		class CHomeDisplayEventHandler
			: public CHomeDisplay::CHomeDisplayEventHandler
			, protected CDisplayEventHandlerBase
		{
			void OnClose() override;
			void OnMouseMessage(UINT Msg,int x,int y) override;
		};

		class CChannelDisplayEventHandler
			: public CChannelDisplay::CChannelDisplayEventHandler
			, protected CDisplayEventHandlerBase
		{
		public:
			CChannelDisplayEventHandler(CAppMain &App);
			void OnTunerSelect(LPCTSTR pszDriverFileName,int TuningSpace) override;
			void OnChannelSelect(LPCTSTR pszDriverFileName,const CChannelInfo *pChannelInfo) override;
			void OnClose() override;
			void OnMouseMessage(UINT Msg,int x,int y) override;

		private:
			CAppMain &m_App;
		};

		CMainDisplay(CAppMain &App);
		bool Create(HWND hwndParent,int ViewID,int ContainerID,HWND hwndMessage);
		bool EnableViewer(bool fEnable);
		bool IsViewerEnabled() const { return m_fViewerEnabled; }
		bool BuildViewer(BYTE VideoStreamType=0);
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
		bool m_fViewerEnabled;
		CViewWindow m_ViewWindow;
		CVideoContainerWindow m_VideoContainer;
		CDisplayBase m_DisplayBase;
	};

}	// namespace TVTest


#endif
