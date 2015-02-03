#ifndef TVTEST_MAIN_DISPLAY_H
#define TVTEST_MAIN_DISPLAY_H


#include "View.h"


class CAppMain;

namespace TVTest
{

	class CMainDisplay
	{
	public:
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

	protected:
		CAppMain &m_App;
		bool m_fViewerEnabled;
		CViewWindow m_ViewWindow;
		CVideoContainerWindow m_VideoContainer;
		CDisplayBase m_DisplayBase;
	};

}	// namespace TVTest


#endif
