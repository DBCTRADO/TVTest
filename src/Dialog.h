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


#ifndef TVTEST_DIALOG_H
#define TVTEST_DIALOG_H


#include "UIBase.h"
#include <vector>


namespace TVTest
{

	class CBasicDialog
		: public CUIBase
	{
	public:
		struct Position
		{
			int x, y;
			int Width, Height;

			Position() : x(0), y(0), Width(0), Height(0) {}
			void Set(const RECT *pRect) {
				x = pRect->left;
				y = pRect->top;
				Width = pRect->right - x;
				Height = pRect->bottom - y;
			}
			void Get(RECT *pRect) const {
				pRect->left = x;
				pRect->top = y;
				pRect->right = x + Width;
				pRect->bottom = y + Height;
			}
		};

		CBasicDialog();
		virtual ~CBasicDialog();

		CBasicDialog(const CBasicDialog &) = delete;
		CBasicDialog &operator=(const CBasicDialog &) = delete;

		virtual bool Show(HWND hwndOwner) { return false; }
		virtual bool Create(HWND hwndOwner) { return false; }
		bool IsCreated() const;
		bool Destroy();
		bool IsModeless() const { return m_fModeless; }
		bool ProcessMessage(LPMSG pMsg);
		bool IsVisible() const;
		bool SetVisible(bool fVisible);
		bool GetPosition(Position *pPosition) const;
		bool GetPosition(RECT *pPosition) const;
		bool GetPosition(int *pLeft, int *pTop, int *pWidth = nullptr, int *pHeight = nullptr) const;
		bool SetPosition(const Position &Pos);
		bool SetPosition(const RECT *pPosition);
		bool SetPosition(int Left, int Top, int Width, int Height);
		bool SetPosition(int Left, int Top);
		bool IsPositionSet() const { return m_fSetPosition; }
		LRESULT SendMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

	protected:
		HWND m_hDlg;
		bool m_fModeless;
		bool m_fSetPosition;
		Position m_Position;
		Style::CStyleScaling m_StyleScaling;

		struct ItemInfo
		{
			HWND hwnd;
			RECT rcOriginal;
		};
		std::vector<ItemInfo> m_ItemList;
		int m_OriginalDPI;
		int m_CurrentDPI;
		HFONT m_hOriginalFont;
		LOGFONT m_lfOriginalFont;
		DrawUtil::CFont m_Font;
		bool m_fInitializing;
		bool m_fOwnDPIScaling;

		static CBasicDialog *GetThis(HWND hDlg);
		static INT_PTR CALLBACK DialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
		int ShowDialog(HWND hwndOwner, HINSTANCE hinst, LPCTSTR pszTemplate);
		bool CreateDialogWindow(HWND hwndOwner, HINSTANCE hinst, LPCTSTR pszTemplate);
		virtual INT_PTR HandleMessage(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
		virtual INT_PTR DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
		virtual bool ApplyPosition();
		void StorePosition();
		void InitDialog();
		virtual void OnDestroyed() {}

	// CUIBase
		void ApplyStyle() override;
		void RealizeStyle() override;
	};

	class CResizableDialog
		: public CBasicDialog
	{
	public:
		enum class AlignFlag : unsigned int {
			None        = 0x0000U,
			Left        = 0x0001U,
			Top         = 0x0002U,
			Right       = 0x0004U,
			Bottom      = 0x0008U,
			BottomRight = Right | Bottom,
			Horz        = Left | Right,
			Vert        = Top | Bottom,
			HorzTop     = Horz | Top,
			HorzBottom  = Horz | Bottom,
			VertLeft    = Vert | Left,
			VertRight   = Vert | Right,
			All         = Horz | Vert,
		};

		CResizableDialog();
		virtual ~CResizableDialog();

	protected:
		struct LayoutItem
		{
			int ID;
			RECT rcOriginal;
			int DPI;
			AlignFlag Align;
		};

		SIZE m_MinSize;
		SIZE m_OriginalClientSize;
		SIZE m_ScaledClientSize;
		int m_BaseDPI;
		HWND m_hwndSizeGrip;
		std::vector<LayoutItem> m_ControlList;

		INT_PTR HandleMessage(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) override;
		bool ApplyPosition() override;
		void DoLayout();
		bool AddControl(int ID, AlignFlag Align);
		bool AddControls(int FirstID, int LastID, AlignFlag Align);
		bool UpdateControlPosition(int ID);

	// CUIBase
		void ApplyStyle() override;
		void RealizeStyle() override;
	};

	TVTEST_ENUM_FLAGS(CResizableDialog::AlignFlag)

}	// namespace TVTest


#endif
