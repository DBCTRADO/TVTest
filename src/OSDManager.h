#ifndef TVTEST_OSD_MANAGER_H
#define TVTEST_OSD_MANAGER_H


#include "UIBase.h"
#include "OSDOptions.h"
#include "PseudoOSD.h"
#include "ChannelList.h"


class COSDManager
	: public TVTest::CUIBase
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

	enum {
		SHOW_NO_FADE = 0x0001U,
		SHOW_PSEUDO  = 0x0002U
	};

	COSDManager(const COSDOptions *pOptions);
	~COSDManager();
	bool Initialize();
	void SetEventHandler(CEventHandler *pEventHandler);
	void Reset();
	void ClearOSD();
	void OnParentMove();
	bool ShowOSD(LPCTSTR pszText, unsigned int Flags = 0);
	void HideOSD();
	bool ShowChannelOSD(const CChannelInfo *pInfo, LPCTSTR pszText, bool fChanging = false);
	void HideChannelOSD();
	bool ShowVolumeOSD(int Volume);
	void HideVolumeOSD();

private:
	struct OSDStyle
	{
		TVTest::Style::Margins Margin;
		TVTest::Style::IntValue TextSizeRatio;
		TVTest::Style::IntValue CompositeTextSizeRatio;
		TVTest::Style::Size LogoSize;
		TVTest::String LogoEffect;
		bool fChannelAnimation;
		TVTest::Style::Margins VolumeMargin;
		TVTest::Style::IntValue VolumeTextSizeMin;

		OSDStyle();
		void SetStyle(const TVTest::Style::CStyleManager *pStyleManager);
		void NormalizeStyle(
			const TVTest::Style::CStyleManager *pStyleManager,
			const TVTest::Style::CStyleScaling *pStyleScaling);
	};

	const COSDOptions *m_pOptions;
	OSDStyle m_Style;
	CEventHandler *m_pEventHandler;
	CPseudoOSD m_OSD;
	CPseudoOSD m_VolumeOSD;

	bool CompositeText(LPCTSTR pszText, const RECT &rcClient, int LeftOffset, DWORD FadeTime);

// CUIBase
	void SetStyle(const TVTest::Style::CStyleManager *pStyleManager) override;
	void NormalizeStyle(
		const TVTest::Style::CStyleManager *pStyleManager,
		const TVTest::Style::CStyleScaling *pStyleScaling) override;
};


#endif
