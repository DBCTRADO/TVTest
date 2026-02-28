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


#ifndef TVTEST_CHANNEL_DISPLAY_H
#define TVTEST_CHANNEL_DISPLAY_H


#include <vector>
#include <memory>
#include "View.h"
#include "DriverManager.h"
#include "LogoManager.h"
#include "DrawUtil.h"
#include "Theme.h"
#include "WindowUtil.h"
#include "GUIUtil.h"
#include "LibISDB/LibISDB/EPG/EPGDatabase.hpp"


namespace TVTest
{

	class CChannelDisplay
		: public CDisplayView
	{
	public:
		class ABSTRACT_CLASS(CChannelDisplayEventHandler)
			: public CDisplayView::CEventHandler
		{
		protected:
			class CChannelDisplay *m_pChannelDisplay = nullptr;
		public:
			static constexpr int SPACE_NOTSPECIFIED = -2;
			static constexpr int SPACE_ALL          = -1;

			virtual void OnTunerSelect(LPCTSTR pszDriverFileName, int TuningSpace) = 0;
			virtual void OnChannelSelect(LPCTSTR pszDriverFileName, const CChannelInfo *pChannelInfo) = 0;
			virtual void OnClose() = 0;
			friend class CChannelDisplay;
		};

		CChannelDisplay(LibISDB::EPGDatabase *pEPGDatabase);
		~CChannelDisplay();

	// CBasicWindow
		bool Create(HWND hwndParent, DWORD Style, DWORD ExStyle = 0, int ID = 0) override;

	// CUIBase
		void SetStyle(const Style::CStyleManager *pStyleManager) override;
		void NormalizeStyle(
			const Style::CStyleManager *pStyleManager,
			const Style::CStyleScaling *pStyleScaling) override;

	// CDisplayView
		bool Close() override;
		bool IsMessageNeed(const MSG *pMsg) const override;
		bool OnMouseWheel(UINT Msg, WPARAM wParam, LPARAM lParam) override;

	// CChannelDisplay
		void Clear();
		bool SetDriverManager(CDriverManager *pDriverManager);
		void SetLogoManager(CLogoManager *pLogoManager);
		void SetEventHandler(CChannelDisplayEventHandler *pEventHandler);
		bool SetSelect(LPCTSTR pszDriverFileName, const CChannelInfo *pChannelInfo);
		bool SetFont(const Style::Font &Font, bool fAutoSize);

		static bool Initialize(HINSTANCE hinst);

	private:
		class CTuner
		{
		public:
			class CChannel
				: public CChannelInfo
			{
				HBITMAP m_hbmSmallLogo = nullptr;
				HBITMAP m_hbmBigLogo = nullptr;
				LibISDB::EventInfo m_Event[2];

			public:
				CChannel(const CChannelInfo &Info)
					: CChannelInfo(Info)
				{
				}
				void SetSmallLogo(HBITMAP hbm) { m_hbmSmallLogo = hbm; }
				HBITMAP GetSmallLogo() const { return m_hbmSmallLogo; }
				void SetBigLogo(HBITMAP hbm) { m_hbmBigLogo = hbm; }
				HBITMAP GetBigLogo() const { return m_hbmBigLogo; }
				bool HasLogo() const { return m_hbmSmallLogo != nullptr || m_hbmBigLogo != nullptr; }
				bool SetEvent(int Index, const LibISDB::EventInfo *pEvent);
				const LibISDB::EventInfo *GetEvent(int Index) const;
			};

			CTuner(const CDriverInfo *pDriverInfo);

			void Clear();
			LPCTSTR GetDriverFileName() const { return m_DriverFileName.c_str(); }
			LPCTSTR GetTunerName() const { return m_TunerName.c_str(); }
			LPCTSTR GetDisplayName() const;
			void SetDisplayName(LPCTSTR pszName);
			void GetDisplayName(int Space, LPTSTR pszName, int MaxName) const;
			int NumSpaces() const;
			CTuningSpaceInfo *GetTuningSpaceInfo(int Index);
			const CTuningSpaceInfo *GetTuningSpaceInfo(int Index) const;
			void SetIcon(HICON hico);
			HICON GetIcon() const { return m_Icon; }

		private:
			std::vector<std::unique_ptr<CTuningSpaceInfo>> m_TuningSpaceList;
			String m_DriverFileName;
			String m_TunerName;
			String m_DisplayName;
			CIcon m_Icon;
		};

		struct ChannelDisplayStyle
		{
			Style::Margins TunerItemPadding;
			Style::Size TunerIconSize;
			Style::IntValue TunerIconTextMargin;
			Style::Margins ChannelItemPadding;
			Style::IntValue ChannelEventMargin;
			Style::Margins ClockPadding;
			Style::Margins ClockMargin;

			ChannelDisplayStyle();
			void SetStyle(const Style::CStyleManager *pStyleManager);
			void NormalizeStyle(
				const Style::CStyleManager *pStyleManager,
				const Style::CStyleScaling *pStyleScaling);
		};

		ChannelDisplayStyle m_ChannelDisplayStyle;
		Theme::BackgroundStyle m_TunerAreaBackStyle;
		Theme::BackgroundStyle m_ChannelAreaBackStyle;
		Theme::Style m_TunerItemStyle;
		Theme::Style m_TunerItemSelStyle;
		Theme::Style m_TunerItemCurStyle;
		Theme::Style m_ChannelItemStyle[2];
		Theme::Style m_ChannelItemCurStyle;
		Theme::Style m_ClockStyle;
		Style::Font m_StyleFont;
		DrawUtil::CFont m_Font;
		bool m_fAutoFontSize = true;
		int m_FontHeight;
		int m_TunerItemWidth;
		int m_TunerItemHeight;
		int m_TunerAreaWidth;
		int m_TunerItemLeft;
		int m_TunerItemTop;
		int m_ChannelItemWidth;
		int m_ChannelItemHeight;
		int m_ChannelItemLeft;
		int m_ChannelItemTop;
		int m_ChannelNameWidth;
		int m_VisibleTunerItems;
		int m_TunerScrollPos;
		int m_VisibleChannelItems;
		int m_ChannelScrollPos;
		HWND m_hwndTunerScroll = nullptr;
		HWND m_hwndChannelScroll = nullptr;
		CMouseWheelHandler m_MouseWheel;
		LibISDB::DateTime m_EpgBaseTime;
		LibISDB::DateTime m_ClockTime;
		static constexpr UINT TIMER_CLOCK = 1;

		std::vector<std::unique_ptr<CTuner>> m_TunerList;
		int m_TotalTuningSpaces = 0;
		int m_CurTuner = -1;
		int m_CurChannel = -1;
		LibISDB::EPGDatabase *m_pEPGDatabase;
		CLogoManager *m_pLogoManager = nullptr;
		CChannelDisplayEventHandler *m_pChannelDisplayEventHandler = nullptr;
		POINT m_LastCursorPos;

		struct TunerInfo
		{
			TCHAR DriverMasks[MAX_PATH];
			TCHAR szDisplayName[64];
			TCHAR szIconFile[MAX_PATH];
			int Index;
			bool fUseDriverChannel;
		};
		std::vector<TunerInfo> m_TunerInfoList;

		static const LPCTSTR m_pszWindowClass;
		static HINSTANCE m_hinst;

		void LoadSettings();
		void Layout();
		const CTuningSpaceInfo *GetTuningSpaceInfo(int Index) const;
		CTuningSpaceInfo *GetTuningSpaceInfo(int Index);
		const CTuner *GetTuner(int Index, int *pSpace = nullptr) const;
		void GetTunerItemRect(int Index, RECT *pRect) const;
		void GetChannelItemRect(int Index, RECT *pRect) const;
		void UpdateTunerItem(int Index) const;
		void UpdateChannelItem(int Index) const;
		int TunerItemHitTest(int x, int y) const;
		int ChannelItemHitTest(int x, int y) const;
		bool SetCurTuner(int Index, bool fUpdate = false);
		bool UpdateChannelInfo(int Index);
		bool SetCurChannel(int Index);
		void SetTunerScrollPos(int Pos, bool fScroll);
		void SetChannelScrollPos(int Pos, bool fScroll);
		void Draw(HDC hdc, const RECT *pPaintRect);
		void DrawClock(HDC hdc) const;
		void NotifyTunerSelect() const;
		void NotifyChannelSelect() const;

	// CCustomWindow
		LRESULT OnMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

	// CUIBase
		void ApplyStyle() override;
		void RealizeStyle() override;
	};

} // namespace TVTest


#endif
