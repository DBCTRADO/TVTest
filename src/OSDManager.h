#ifndef TVTEST_OSD_MANAGER_H
#define TVTEST_OSD_MANAGER_H


#include "UIBase.h"
#include "OSDOptions.h"
#include "PseudoOSD.h"
#include "ChannelList.h"


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
		};

		COSDManager(const COSDOptions *pOptions);
		~COSDManager();

		bool Initialize();
		void SetEventHandler(CEventHandler *pEventHandler);
		void Reset();
		void ClearOSD();
		void OnParentMove();
		bool ShowOSD(LPCTSTR pszText, ShowFlag Flags = ShowFlag::None);
		void HideOSD();
		bool ShowChannelOSD(const CChannelInfo *pInfo, LPCTSTR pszText, bool fChanging = false);
		void HideChannelOSD();
		bool ShowVolumeOSD(int Volume);
		void HideVolumeOSD();

	private:
		struct OSDStyle
		{
			Style::Margins Margin;
			Style::IntValue TextSizeRatio;
			Style::IntValue CompositeTextSizeRatio;
			Style::Size LogoSize;
			String LogoEffect;
			bool fChannelAnimation;
			Style::Margins VolumeMargin;
			Style::IntValue VolumeTextSizeMin;

			OSDStyle();
			void SetStyle(const Style::CStyleManager *pStyleManager);
			void NormalizeStyle(
				const Style::CStyleManager *pStyleManager,
				const Style::CStyleScaling *pStyleScaling);
		};

		const COSDOptions *m_pOptions;
		OSDStyle m_Style;
		CEventHandler *m_pEventHandler;
		CPseudoOSD m_OSD;
		CPseudoOSD m_VolumeOSD;

		bool CompositeText(LPCTSTR pszText, const RECT &rcClient, int LeftOffset, DWORD FadeTime);

	// CUIBase
		void SetStyle(const Style::CStyleManager *pStyleManager) override;
		void NormalizeStyle(
			const Style::CStyleManager *pStyleManager,
			const Style::CStyleScaling *pStyleScaling) override;
	};

	TVTEST_ENUM_FLAGS(COSDManager::ShowFlag)

}	// namespace TVTest


#endif
