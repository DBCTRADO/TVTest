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


#ifndef TVTEST_CAPTURE_H
#define TVTEST_CAPTURE_H


#include <memory>
#include "BasicWindow.h"
#include "StatusView.h"


namespace TVTest
{

	class CCaptureImage
	{
		HGLOBAL m_hData;
		bool m_fLocked = false;
		LibISDB::DateTime m_CaptureTime;
		String m_Comment;

	public:
		CCaptureImage(HGLOBAL hData);
		CCaptureImage(const BITMAPINFO *pbmi, const void *pBits);
		~CCaptureImage();

		CCaptureImage(const CCaptureImage &) = delete;
		CCaptureImage &operator=(const CCaptureImage &) = delete;

		bool SetClipboard(HWND hwnd);
		bool GetBitmapInfoHeader(BITMAPINFOHEADER *pbmih) const;
		bool LockData(BITMAPINFO **ppbmi, BYTE **ppBits);
		bool UnlockData();
		const LibISDB::DateTime &GetCaptureTime() const { return m_CaptureTime; }
		void SetComment(LPCTSTR pszComment);
		LPCTSTR GetComment() const;
	};

	class CCapturePreview
		: public CCustomWindow
	{
	public:
		class ABSTRACT_CLASS(CEventHandler)
		{
		protected:
			CCapturePreview *m_pCapturePreview = nullptr;

		public:
			virtual ~CEventHandler() = 0;

			virtual void OnLButtonDown(int x, int y) {}
			virtual void OnRButtonUp(int x, int y) {}
			virtual bool OnKeyDown(UINT KeyCode, UINT Flags) { return false; }
			friend class CCapturePreview;
		};

		static bool Initialize(HINSTANCE hinst);

		~CCapturePreview();

	// CBasicWindow
		bool Create(HWND hwndParent, DWORD Style, DWORD ExStyle = 0, int ID = 0) override;

	// CCapturePreview
		bool SetImage(const std::shared_ptr<CCaptureImage> &Image);
		bool ClearImage();
		bool HasImage() const;
		bool SetEventHandler(CEventHandler *pEventHandler);

	private:
		static HINSTANCE m_hinst;

		std::shared_ptr<CCaptureImage> m_Image;
		COLORREF m_crBackColor = RGB(0, 0, 0);
		CEventHandler *m_pEventHandler = nullptr;

	// CCustomWindow
		LRESULT OnMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;
	};

	class CCaptureWindow
		: public CPopupWindow
		, public CUIBase
	{
	public:
		class ABSTRACT_CLASS(CEventHandler)
		{
		protected:
			CCaptureWindow *m_pCaptureWindow = nullptr;

		public:
			virtual ~CEventHandler() = 0;

			virtual void OnRestoreSettings() {}
			virtual bool OnClose() { return true; }
			virtual bool OnSave(CCaptureImage * pImage) { return false; }
			virtual bool OnKeyDown(UINT KeyCode, UINT Flags) { return false; }
			virtual bool OnActivate(bool fActive) { return false; }
			friend class CCaptureWindow;
		};

		static bool Initialize(HINSTANCE hinst);

		CCaptureWindow();
		~CCaptureWindow();

	// CBasicWindow
		bool Create(HWND hwndParent, DWORD Style, DWORD ExStyle = 0, int ID = 0) override;

	// CUIBase
		void SetTheme(const Theme::CThemeManager *pThemeManager) override;

	// CCaptureWindow
		bool SetImage(const BITMAPINFO *pbmi, const void *pBits);
		bool SetImage(CCaptureImage *pImage);
		bool ClearImage();
		bool HasImage() const;
		bool SetEventHandler(CEventHandler *pEventHandler);
		void ShowStatusBar(bool fShow);
		bool IsStatusBarVisible() const { return m_fShowStatusBar; }

	private:
		class CPreviewEventHandler
			: public CCapturePreview::CEventHandler
		{
			CCaptureWindow *m_pCaptureWindow;

		public:
			CPreviewEventHandler(CCaptureWindow *pCaptureWindow);
			void OnRButtonUp(int x, int y) override;
			bool OnKeyDown(UINT KeyCode, UINT Flags) override;
		};

		enum {
			STATUS_ITEM_CAPTURE,
			//STATUS_ITEM_CONTINUOUS,
			STATUS_ITEM_SAVE,
			STATUS_ITEM_COPY
		};

		class CCaptureStatusItem
			: public CIconStatusItem
		{
			CCaptureWindow *m_pCaptureWindow;
			Theme::IconList &m_Icons;

		public:
			CCaptureStatusItem(CCaptureWindow *pCaptureWindow, Theme::IconList &Icons);

			LPCTSTR GetIDText() const override { return TEXT("Capture"); }
			LPCTSTR GetName() const override { return TEXT("キャプチャ"); }
			void Draw(HDC hdc, const RECT &ItemRect, const RECT &DrawRect, DrawFlag Flags) override;
			void OnLButtonDown(int x, int y) override;
			void OnRButtonDown(int x, int y) override;
		};

		class CSaveStatusItem
			: public CIconStatusItem
		{
			CCaptureWindow *m_pCaptureWindow;
			Theme::IconList &m_Icons;

		public:
			CSaveStatusItem(CCaptureWindow *pCaptureWindow, Theme::IconList &Icons);

			LPCTSTR GetIDText() const override { return TEXT("Save"); }
			LPCTSTR GetName() const override { return TEXT("保存"); }
			void Draw(HDC hdc, const RECT &ItemRect, const RECT &DrawRect, DrawFlag Flags) override;
			void OnLButtonDown(int x, int y) override;
		};

		class CCopyStatusItem
			: public CIconStatusItem
		{
			CCaptureWindow *m_pCaptureWindow;
			Theme::IconList &m_Icons;

		public:
			CCopyStatusItem(CCaptureWindow *pCaptureWindow, Theme::IconList &Icons);

			LPCTSTR GetIDText() const override { return TEXT("Copy"); }
			LPCTSTR GetName() const override { return TEXT("コピー"); }
			void Draw(HDC hdc, const RECT &ItemRect, const RECT &DrawRect, DrawFlag Flags) override;
			void OnLButtonDown(int x, int y) override;
		};

		static HINSTANCE m_hinst;

		Style::CStyleScaling m_StyleScaling;
		CCapturePreview m_Preview;
		CPreviewEventHandler m_PreviewEventHandler{this};
		CStatusView m_Status;
		bool m_fShowStatusBar = true;
		Theme::IconList m_StatusIcons;
		std::shared_ptr<CCaptureImage> m_Image;
		CEventHandler *m_pEventHandler = nullptr;
		bool m_fCreateFirst = true;

		void SetTitle();

	// CCustomWindow
		LRESULT OnMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

	// CUIBase
		void RealizeStyle() override;
	};

} // namespace TVTest


#endif
