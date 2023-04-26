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


#ifndef TVTEST_DIALOG_H
#define TVTEST_DIALOG_H


#include "UIBase.h"
#include "DrawUtil.h"
#include <vector>
#include <map>


namespace TVTest
{

	class CBasicDialog
		: public CUIBase
	{
	public:
		struct Position
		{
			int x = 0, y = 0;
			int Width = 0, Height = 0;

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
		HWND GetHandle() const noexcept { return m_hDlg; }

		bool IsDarkMode() const noexcept { return m_fDarkMode; }

	protected:
		HWND m_hDlg = nullptr;
		bool m_fModeless = false;
		bool m_fSetPosition = false;
		Position m_Position;
		Style::CStyleScaling m_StyleScaling;

		struct ItemInfo
		{
			HWND hwnd;
			RECT rcOriginal;
		};
		std::vector<ItemInfo> m_ItemList;
		int m_OriginalDPI = 0;
		int m_CurrentDPI = 0;
		HFONT m_hOriginalFont = nullptr;
		LOGFONT m_lfOriginalFont{};
		DrawUtil::CFont m_Font;
		bool m_fInitializing = false;
		bool m_fOwnDPIScaling = false;

		bool m_fDisableDarkMode = false;
		bool m_fAllowDarkMode = false;
		bool m_fDarkMode = false;
		DrawUtil::CBrush m_BackBrush;
		DrawUtil::CBrush m_FaceBrush;

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

		virtual bool IsDisableThemingControl(HWND hwnd) const { return false; }
		virtual void OnDarkModeChanged(bool fDarkMode) {}
		COLORREF GetThemeColor(int Type) const;

	// CUIBase
		void ApplyStyle() override;
		void RealizeStyle() override;

	private:
		struct ControlThemeInfo {
			CUxTheme Theme;
			int State = -1;
			bool fDisabled = false;
		};

		INT_PTR HandleDarkModeMessage(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
		void HandleDarkModeChanged(bool fDarkMode);
		LRESULT HandleDarkModeCtlColor(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
		bool HandleDarkModeNotifyMessage(NMHDR *pnmh);
		LRESULT CustomDrawDarkModeButton(NMCUSTOMDRAW *pnmcd);
		LRESULT CustomDrawDarkModeListView(NMLVCUSTOMDRAW *pnmcd);
		LRESULT CustomDrawDarkModeTreeView(NMTVCUSTOMDRAW *pnmcd);
		LRESULT CustomDrawDarkModeTrackBar(NMCUSTOMDRAW *pnmcd);
		void UpdateColors(bool fDarkMode);
		void InitializeControls(HWND hDlg, bool fDark);
		void UpdateControlsTheme(HWND hDlg, bool fDark);
		void UpdateControlsColors(HWND hDlg, bool fDark);
		bool InitializeControl(HWND hwnd, bool fDark);
		bool SetControlDarkTheme(HWND hwnd, bool fDark);
		bool UpdateControlColors(HWND hwnd, bool fDark);
		void DrawDarkModeStatic(HWND hwnd, HDC hdc, const RECT &PaintRect, DWORD Style);
		void DrawDarkModeGroupBox(HWND hwnd, HDC hdc);
		void DrawDarkModeTab(HWND hwnd, HDC hdc, const RECT &PaintRect);
		void DrawDarkModeUpDown(HWND hwnd, HDC hdc, const RECT &PaintRect);
		LRESULT DarkModeHeaderCustomDraw(NMCUSTOMDRAW *pnmcd);
		ControlThemeInfo * GetThemeInfo(HWND hwnd, LPCWSTR pszClassList);
		void DeleteTheme(HWND hwnd);

		static LRESULT CALLBACK StaticSubclassProc(
			HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
			UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
		static LRESULT CALLBACK ButtonSubclassProc(
			HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
			UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
		static LRESULT CALLBACK ListViewSubclassProc(
			HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
			UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
		static LRESULT CALLBACK TabSubclassProc(
			HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
			UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
		static LRESULT CALLBACK UpDownSubclassProc(
			HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
			UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

		static constexpr UINT_PTR SUBCLASS_ID = 1;

		static const LPCTSTR PROP_NAME;

		COLORREF m_TextColor;
		COLORREF m_BackColor;
		COLORREF m_FaceColor;
		std::map<HWND, ControlThemeInfo> m_ControlThemeMap;
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
			LeftHalf    = 0x0010U,
			TopHalf     = 0x0020U,
			RightHalf   = 0x0040U,
			BottomHalf  = 0x0080U,
			TVTEST_ENUM_FLAGS_TRAILER,
			BottomRight = Right | Bottom,
			Horz        = Left | Right,
			Vert        = Top | Bottom,
			HorzTop     = Horz | Top,
			HorzBottom  = Horz | Bottom,
			VertLeft    = Vert | Left,
			VertRight   = Vert | Right,
			All         = Horz | Vert,
		};

		struct ControlAlignInfo
		{
			int ID;
			AlignFlag Align;
		};

		virtual ~CResizableDialog();

	protected:
		struct LayoutItem
		{
			int ID;
			RECT rcOriginal;
			int DPI;
			AlignFlag Align;
		};

		SIZE m_MinSize{};
		SIZE m_OriginalClientSize{};
		SIZE m_ScaledClientSize{};
		int m_BaseDPI = 0;
		HWND m_hwndSizeGrip = nullptr;
		std::vector<LayoutItem> m_ControlList;

		INT_PTR HandleMessage(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) override;
		bool ApplyPosition() override;
		void DoLayout();
		bool AddControl(int ID, AlignFlag Align);
		bool AddControls(int FirstID, int LastID, AlignFlag Align);
		void AddControls(std::initializer_list<ControlAlignInfo> List);
		bool UpdateControlPosition(int ID);

	// CUIBase
		void ApplyStyle() override;
		void RealizeStyle() override;
	};

}	// namespace TVTest


#endif
