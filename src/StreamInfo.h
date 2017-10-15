#ifndef TVTEST_STREAM_INFO_H
#define TVTEST_STREAM_INFO_H


#include "BasicWindow.h"
#include "UIBase.h"
#include "Settings.h"
#include "Dialog.h"


namespace TVTest
{

	class CStreamInfo
		: public CCustomWindow
		, public CUIBase
	{
	public:
		class CEventHandler
		{
		public:
			virtual ~CEventHandler() = default;

			virtual void OnRestoreSettings() {}
			virtual bool OnClose() { return true; }
		};

		static bool Initialize(HINSTANCE hinst);

		CStreamInfo();
		~CStreamInfo();

	// CBasicWindow
		bool Create(
			HWND hwndParent,
			DWORD Style = WS_OVERLAPPED | WS_SYSMENU | WS_THICKFRAME | WS_CLIPCHILDREN,
			DWORD ExStyle = 0, int ID = 0) override;

	// CStreamInfo
		void SetEventHandler(CEventHandler *pHandler);
		bool IsPositionSet() const;
		void LoadSettings(CSettings &Settings);
		void SaveSettings(CSettings &Settings) const;

	private:
		struct PageInfo
		{
			LPCTSTR pszTitle;
			std::unique_ptr<CResizableDialog> Dialog;
		};

		enum {
			PAGE_STREAMINFO,
			PAGE_PIDINFO,
			NUM_PAGES
		};

	// CCustomWindow
		LRESULT OnMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

	// CUIBase
		void ApplyStyle() override;
		void RealizeStyle() override;

	// CStreamInfo
		void OnSizeChanged(int Width, int Height);
		bool CreatePage(int Page);
		bool SetPage(int Page);
		void GetPagePosition(RECT *pPosition) const;

		PageInfo m_PageList[NUM_PAGES];
		int m_CurrentPage;
		HWND m_hwndTab;
		DrawUtil::CFont m_TabFont;
		CEventHandler *m_pEventHandler;
		bool m_fCreateFirst;
		SIZE m_DefaultPageSize;

		static HINSTANCE m_hinst;
	};

}	// namespace TVTest


#endif
