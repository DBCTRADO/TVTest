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


#ifndef TVTEST_OSD_MANAGER_H
#define TVTEST_OSD_MANAGER_H


#include "UIBase.h"
#include "OSDOptions.h"
#include "PseudoOSD.h"
#include "ChannelList.h"
#include "EventInfoOSD.h"


namespace TVTest
{

	class COSDManager
		: public CUIBase
	{
	public:
		struct OSDClientInfo
		{
			HWND hwndParent;
			RECT ClientRect;
			bool fForcePseudoOSD;
			bool fAnimation;
		};

		class ABSTRACT_CLASS(CEventHandler)
		{
		public:
			virtual ~CEventHandler() = default;

			virtual bool GetOSDClientInfo(OSDClientInfo * pInfo) = 0;
			virtual bool SetOSDHideTimer(DWORD Delay) = 0;
		};

		enum class ShowFlag : unsigned int {
			None   = 0x0000U,
			NoFade = 0x0001U,
			Pseudo = 0x0002U,
			TVTEST_ENUM_FLAGS_TRAILER
		};

		enum class EventInfoOSDFlag : unsigned int {
			None   = 0x0000U,
			Manual = 0x0001U,
			Auto   = 0x0002U,
			Next   = 0x0004U,
			TVTEST_ENUM_FLAGS_TRAILER
		};

		COSDManager(const COSDOptions *pOptions);

		bool Initialize();
		void SetEventHandler(CEventHandler *pEventHandler);
		void Reset();
		void ClearOSD();
		void ClearCompositedOSD();
		void OnParentMove();
		void AdjustPosition();
		bool ShowOSD(LPCTSTR pszText, ShowFlag Flags = ShowFlag::None);
		void HideOSD();
		bool ShowChannelOSD(const CChannelInfo *pInfo, LPCTSTR pszText, bool fChanging = false);
		void HideChannelOSD();
		bool ShowVolumeOSD(int Volume);
		void HideVolumeOSD();
		bool ShowEventInfoOSD(const LibISDB::EventInfo &EventInfo, EventInfoOSDFlag Flags);
		void HideEventInfoOSD();
		bool IsEventInfoOSDVisible() const;
		bool IsEventInfoOSDCreated() const;
		void OnOSDFontChanged();
		void OnEventInfoOSDFontChanged();

	private:
		struct OSDStyle
		{
			Style::Margins Margin{8};
			Style::IntValue TextSizeRatio{28};
			Style::IntValue TextSizeMin{12};
			Style::IntValue TextSizeMax{100};
			Style::IntValue CompositeTextSizeRatio{24};
			Style::IntValue CompositeTextSizeMin{12};
			Style::IntValue CompositeTextSizeMax{100};
			Style::Size LogoSize{64, 36};
			String LogoEffect;
			bool fChannelAnimation = true;
			Style::Margins VolumeMargin{16};
			Style::IntValue VolumeTextSizeMin{10};
			Style::IntValue VolumeTextSizeMax{50};
			Style::IntValue VolumeHorizontalScale{60};
			Style::IntValue VolumeSteps{20};
			String VolumeTextFill{TEXT("■")};
			String VolumeTextRemain{TEXT("□")};

			void SetStyle(const Style::CStyleManager *pStyleManager);
			void NormalizeStyle(
				const Style::CStyleManager *pStyleManager,
				const Style::CStyleScaling *pStyleScaling);
		};

		const COSDOptions *m_pOptions;
		OSDStyle m_Style;
		CEventHandler *m_pEventHandler = nullptr;
		CPseudoOSD m_OSD;
		CPseudoOSD m_VolumeOSD;
		int m_VolumeOSDMaxWidth = 0;
		CEventInfoOSD m_EventInfoOSD;
		EventInfoOSDFlag m_EventInfoOSDFlags = EventInfoOSDFlag::None;

		bool CompositeText(LPCTSTR pszText, const RECT &rcClient, int LeftOffset, DWORD FadeTime);
		bool CreateTextOSD(
			LPCTSTR pszText, const OSDClientInfo &ClientInfo,
			int ImageWidth = 0, int ImageHeight = 0);

	// CUIBase
		void SetStyle(const Style::CStyleManager *pStyleManager) override;
		void NormalizeStyle(
			const Style::CStyleManager *pStyleManager,
			const Style::CStyleScaling *pStyleScaling) override;
	};

}	// namespace TVTest


#endif
