/*
  TVTest
  Copyright(c) 2008-2019 DBCTRADO

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


#ifndef TVTEST_LAYOUT_H
#define TVTEST_LAYOUT_H


#include <vector>
#include "BasicWindow.h"
#include "UIBase.h"
#include "DrawUtil.h"


namespace TVTest
{

	namespace Layout
	{

		class CLayoutBase;

		class ABSTRACT_CLASS(CContainer)
			: public CUIBase
		{
		public:
			CContainer(int ID);
			virtual ~CContainer() = 0;

			virtual void SetPosition(const RECT & Pos);
			virtual void GetMinSize(SIZE * pSize) const;
			virtual void SetVisible(bool fVisible);
			virtual int NumChildContainers() const;
			virtual CContainer * GetChildContainer(int Index) const;
			virtual void OnLButtonDown(int x, int y) {}
			virtual void OnLButtonUp(int x, int y) {}
			virtual void OnMouseMove(int x, int y) {}
			CLayoutBase *GetLayoutBase() const { return m_pBase; }
			void SetID(int ID) { m_ID = ID; }
			int GetID() const { return m_ID; }
			bool GetVisible() const { return m_fVisible; }
			const RECT &GetPosition() const { return m_Position; }

		protected:
			CLayoutBase *m_pBase;
			int m_ID;
			bool m_fVisible;
			RECT m_Position;

			friend CLayoutBase;
		};

		class CWindowContainer
			: public CContainer
		{
			CBasicWindow *m_pWindow;
			CUIBase *m_pUIBase;
			int m_MinWidth;
			int m_MinHeight;

		public:
			CWindowContainer(int ID);
			~CWindowContainer();

		// CContainer
			void SetPosition(const RECT &Pos) override;
			void GetMinSize(SIZE *pSize) const override;
			void SetVisible(bool fVisible) override;

		// CWindowContainer
			void SetWindow(CBasicWindow *pWindow, CUIBase *pUIBase = nullptr);
			CBasicWindow *GetWindow() const { return m_pWindow; }
			bool SetMinSize(int Width, int Height);
		};

		class CSplitter
			: public CContainer
		{
		public:
			enum class StyleFlag : unsigned int {
				None  = 0x0000U,
				Horz  = 0x0000U,
				Vert  = 0x0001U,
				Fixed = 0x0002U,
			};

		private:
			struct PaneInfo
			{
				CContainer *pContainer;
				int FixedSize;

				PaneInfo() : pContainer(nullptr), FixedSize(-1) {}
			};
			PaneInfo m_PaneList[2];
			StyleFlag m_Style;
			int m_AdjustPane;
			int m_BarPos;
			int m_BarWidth;

		// CUIBase
			void ApplyStyle() override;
			void RealizeStyle() override;

			void Adjust();
			bool GetBarRect(RECT *pRect) const;

		public:
			CSplitter(int ID);
			~CSplitter();

		// CContainer
			void SetPosition(const RECT &Pos) override;
			void GetMinSize(SIZE *pSize) const override;
			int NumChildContainers() const override;
			CContainer *GetChildContainer(int Index) const override;
			void OnLButtonDown(int x, int y) override;
			void OnLButtonUp(int x, int y) override;
			void OnMouseMove(int x, int y) override;

		// CSplitter
			bool SetPane(int Index, CContainer *pContainer);
			bool ReplacePane(int Index, CContainer *pContainer);
			CContainer *GetPane(int Index) const;
			CContainer *GetPaneByID(int ID) const;
			void SwapPane();
			bool SetPaneSize(int ID, int Size);
			int GetPaneSize(int ID);
			int IDToIndex(int ID) const;
			bool SetStyle(StyleFlag Style, bool fAdjust = true);
			StyleFlag GetStyle() const { return m_Style; }
			bool SetAdjustPane(int ID);
			bool SetBarPos(int Pos);
			int GetBarPos() const { return m_BarPos; }
			int GetBarWidth() const { return m_BarWidth; }
		};

		TVTEST_ENUM_FLAGS(CSplitter::StyleFlag)


		class CLayoutBase
			: public CCustomWindow
			, public CUIBase
		{
		public:
			class ABSTRACT_CLASS(CEventHandler)
			{
			protected:
				CLayoutBase *m_pBase;

			public:
				CEventHandler();
				virtual ~CEventHandler();
				friend CLayoutBase;
			};

			CLayoutBase();
			~CLayoutBase();

			bool Create(HWND hwndParent, DWORD Style, DWORD ExStyle = 0, int ID = 0) override;
			bool SetTopContainer(CContainer *pContainer);
			CContainer *GetTopContainer() const { return m_pContainer; }
			CContainer *GetContainerByID(int ID) const;
			CContainer *GetContainerFromPoint(int x, int y) const;
			bool SetContainerVisible(int ID, bool fVisible);
			bool GetContainerVisible(int ID) const;
			void GetMinSize(SIZE *pSize) const;
			void Adjust();
			void LockLayout();
			void UnlockLayout(bool fAdjust = true);
			bool IsLayoutLocked() const { return m_fLockLayout; }
			void SetEventHandler(CEventHandler *pHandler);
			bool SetBackColor(COLORREF Color);
			COLORREF GetBackColor() const { return m_BackColor; }
			HBRUSH GetBackBrush();

			static bool Initialize(HINSTANCE hinst);

		protected:
			CEventHandler *m_pEventHandler;
			CContainer *m_pContainer;
			CContainer *m_pFocusContainer;
			COLORREF m_BackColor;
			DrawUtil::CBrush m_BackBrush;
			bool m_fLockLayout;

			LRESULT OnMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

			void SetBasePointer(CContainer *pContainer, CLayoutBase *pBase);
			CContainer *GetChildContainerByID(const CContainer *pContainer, int ID) const;
			CContainer *GetChildContainerFromPoint(const CContainer *pContainer, int x, int y) const;

			static const LPCTSTR m_pszWindowClass;
			static HINSTANCE m_hinst;
		};

	}	// namespace Layout

}	// namespace TVTest


#endif
