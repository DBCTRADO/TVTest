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


#ifndef TVTEST_VIEW_H
#define TVTEST_VIEW_H


#include "BasicWindow.h"
#include "UIBase.h"
#include "Theme.h"
#include "LibISDB/LibISDB/Windows/Viewer/ViewerFilter.hpp"


namespace TVTest
{

	class ABSTRACT_CLASS(CDisplayView)
		: public CCustomWindow
		, public CUIBase
	{
	public:
		class ABSTRACT_CLASS(CEventHandler)
		{
		public:
			virtual ~CEventHandler() = 0;

			virtual void OnMouseMessage(UINT Msg, int x, int y) {}
		};

		CDisplayView() = default;
		virtual ~CDisplayView() = 0;

		virtual bool Close() = 0;
		virtual bool IsMessageNeed(const MSG *pMsg) const;
		virtual bool OnMouseWheel(UINT Msg, WPARAM wParam, LPARAM lParam) { return false; }

	// CBasicWindow
		void SetVisible(bool fVisible) override;

	// CUIBase
		void SetStyle(const Style::CStyleManager *pStyleManager) override;
		void NormalizeStyle(
			const Style::CStyleManager *pStyleManager,
			const Style::CStyleScaling *pStyleScaling) override;

	protected:
		enum class ItemType {
			Normal,
			Normal1,
			Normal2,
			Hot,
			Selected,
			Current,
		};

		enum class BackgroundType {
			Content,
			Categories,
		};

		struct DisplayViewStyle
		{
			Style::IntValue TextSizeRatioHorz{50};
			Style::IntValue TextSizeRatioVert{30};
			Style::IntValue TextSizeScaleBase{140};
			Style::IntValue TextSizeMin{12};
			Style::IntValue TextSizeMax{72};
			Style::Margins ContentMargin{8, 16, 18, 16};
			Style::Margins CategoriesMargin{8, 32, 8, 32};
			Style::Size CloseButtonSize{14, 14};
			Style::Margins CloseButtonMargin{2};

			void SetStyle(const Style::CStyleManager *pStyleManager);
			void NormalizeStyle(
				const Style::CStyleManager *pStyleManager,
				const Style::CStyleScaling *pStyleScaling);
		};

		class CDisplayBase *m_pDisplayBase = nullptr;
		DisplayViewStyle m_Style;
		CEventHandler *m_pEventHandler = nullptr;

		virtual bool OnVisibleChange(bool fVisible);
		virtual bool GetCloseButtonRect(RECT *pRect) const;
		bool CloseButtonHitTest(int x, int y) const;
		void DrawCloseButton(HDC hdc) const;
		bool GetItemStyle(ItemType Type, Theme::Style *pStyle) const;
		bool GetBackgroundStyle(BackgroundType Type, Theme::BackgroundStyle *pStyle) const;
		int GetDefaultFontSize(int Width, int Height) const;
		void SetEventHandler(CEventHandler *pEventHandler);
		bool HandleMessage(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam, LRESULT *pResult);

	// CCustomWindow
		LRESULT OnMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

	private:
		void SetDisplayVisible(bool fVisible);

		friend class CDisplayBase;
	};

	class CDisplayBase
	{
	public:
		class ABSTRACT_CLASS(CEventHandler)
		{
		public:
			virtual ~CEventHandler() = 0;

			virtual bool OnVisibleChange(bool fVisible) { return true; }
		};

		CDisplayBase() = default;

		CDisplayBase(const CDisplayBase &) = delete;
		CDisplayBase &operator=(const CDisplayBase &) = delete;

		void SetEventHandler(CEventHandler *pHandler);
		void SetParent(CBasicWindow *pWindow);
		CBasicWindow *GetParent() const { return m_pParentWindow; }
		void SetDisplayView(CDisplayView *pView);
		CDisplayView *GetDisplayView() const { return m_pDisplayView; }
		bool SetVisible(bool fVisible);
		bool IsVisible() const;
		void AdjustPosition();
		void SetPosition(int Left, int Top, int Width, int Height);
		void SetPosition(const RECT *pRect);
		void SetFocus();

	private:
		CBasicWindow *m_pParentWindow = nullptr;
		CDisplayView *m_pDisplayView = nullptr;
		CEventHandler *m_pEventHandler = nullptr;
		bool m_fVisible = false;
	};

	class CDisplayEventHandlerBase
	{
	protected:
		void RelayMouseMessage(CDisplayView *pView, UINT Message, int x, int y);
	};

	class CVideoContainerWindow
		: public CCustomWindow
	{
	public:
		class ABSTRACT_CLASS(CEventHandler)
		{
		protected:
			CVideoContainerWindow *m_pVideoContainer = nullptr;

		public:
			virtual ~CEventHandler();

			virtual void OnSizeChanged(int Width, int Height) {}

			friend class CVideoContainerWindow;
		};

		~CVideoContainerWindow();

		bool Create(HWND hwndParent, DWORD Style, DWORD ExStyle, int ID, LibISDB::ViewerFilter *pViewer);
		void SetDisplayBase(CDisplayBase *pDisplayBase);
		void SetEventHandler(CEventHandler *pEventHandler);

		static bool Initialize(HINSTANCE hinst);

	private:
		static HINSTANCE m_hinst;

		LibISDB::ViewerFilter *m_pViewer = nullptr;
		CDisplayBase *m_pDisplayBase = nullptr;
		CEventHandler *m_pEventHandler = nullptr;
		SIZE m_ClientSize{};

	// CBasicWindow
		bool Create(HWND hwndParent, DWORD Style, DWORD ExStyle = 0, int ID = 0) override;

	// CCustomWindow
		LRESULT OnMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;
	};

	class CViewWindow
		: public CCustomWindow
		, public CUIBase
	{
	public:
		class ABSTRACT_CLASS(CEventHandler)
		{
		protected:
			CViewWindow *m_pView = nullptr;

		public:
			virtual ~CEventHandler();

			virtual void OnSizeChanged(int Width, int Height) {}

			friend class CViewWindow;
		};

		~CViewWindow();

	// CBasicWindow
		bool Create(HWND hwndParent, DWORD Style, DWORD ExStyle = 0, int ID = 0) override;

	// CViewWindow
		void SetVideoContainer(CVideoContainerWindow *pVideoContainer);
		void SetMessageWindow(HWND hwnd);
		void SetEventHandler(CEventHandler *pEventHandler);
		bool SetLogo(HBITMAP hbm);
		void SetBorder(const Theme::BorderStyle &Style);
		void SetMargin(const Style::Margins &Margin);
		void SetShowCursor(bool fShow);
		bool CalcClientRect(RECT *pRect) const;
		bool CalcWindowRect(RECT *pRect) const;

		static bool Initialize(HINSTANCE hinst);

	private:
		static HINSTANCE m_hinst;

		CVideoContainerWindow *m_pVideoContainer = nullptr;
		HWND m_hwndMessage = nullptr;
		CEventHandler *m_pEventHandler = nullptr;
		HBITMAP m_hbmLogo = nullptr;
		Theme::BorderStyle m_BorderStyle{Theme::BorderType::None, RGB(128, 128, 128)};
		Style::Margins m_Margin;
		bool m_fShowCursor = true;

	// CCustomWindow
		LRESULT OnMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;
	};

} // namespace TVTest


#endif
