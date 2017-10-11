#ifndef TVTEST_CONTROL_PANEL_ITEMS_H
#define TVTEST_CONTROL_PANEL_ITEMS_H


#include "ControlPanel.h"


namespace TVTest
{

	enum {
		CONTROLPANEL_ITEM_TUNER,
		CONTROLPANEL_ITEM_CHANNEL,
		CONTROLPANEL_ITEM_CHANNEL_1,
		CONTROLPANEL_ITEM_CHANNEL_2,
		CONTROLPANEL_ITEM_CHANNEL_3,
		CONTROLPANEL_ITEM_CHANNEL_4,
		CONTROLPANEL_ITEM_CHANNEL_5,
		CONTROLPANEL_ITEM_CHANNEL_6,
		CONTROLPANEL_ITEM_CHANNEL_7,
		CONTROLPANEL_ITEM_CHANNEL_8,
		CONTROLPANEL_ITEM_CHANNEL_9,
		CONTROLPANEL_ITEM_CHANNEL_10,
		CONTROLPANEL_ITEM_CHANNEL_11,
		CONTROLPANEL_ITEM_CHANNEL_12,
		CONTROLPANEL_ITEM_VIDEO,
		CONTROLPANEL_ITEM_VOLUME,
		CONTROLPANEL_ITEM_AUDIO
	};

	class CTunerControlItem
		: public CControlPanelItem
	{
	public:
	// CControlPanelItem
		void CalcSize(int Width, SIZE *pSize);
		void Draw(HDC hdc, const RECT &Rect);
		void OnLButtonDown(int x, int y);
		void OnRButtonDown(int x, int y);
	};

	class CChannelControlItem
		: public CControlPanelItem
	{
	public:
	// CControlPanelItem
		void CalcSize(int Width, SIZE *pSize);
		void Draw(HDC hdc, const RECT &Rect);
		void OnLButtonDown(int x, int y);
		void OnRButtonDown(int x, int y);
	};

	class CVideoControlItem
		: public CControlPanelItem
	{
	public:
	// CControlPanelItem
		void CalcSize(int Width, SIZE *pSize);
		void Draw(HDC hdc, const RECT &Rect);
		void OnLButtonDown(int x, int y);
		void OnRButtonDown(int x, int y);
	};

	class CVolumeControlItem
		: public CControlPanelItem
	{
	public:
	// CControlPanelItem
		void CalcSize(int Width, SIZE *pSize);
		void Draw(HDC hdc, const RECT &Rect);
		void OnLButtonDown(int x, int y);
		void OnRButtonDown(int x, int y);
		void OnMouseMove(int x, int y);
		void SetStyle(const Style::CStyleManager *pStyleManager);
		void NormalizeStyle(
			const Style::CStyleManager *pStyleManager,
			const Style::CStyleScaling *pStyleScaling);

	private:
		struct VolumeControlStyle
		{
			Style::IntValue BarHeight;
			Style::Margins BarPadding;
			Style::IntValue BarBorderWidth;

			VolumeControlStyle();
		};

		VolumeControlStyle m_Style;
	};

	class CAudioControlItem
		: public CControlPanelItem
	{
	public:
	// CControlPanelItem
		void CalcSize(int Width, SIZE *pSize);
		void Draw(HDC hdc, const RECT &Rect);
		void OnLButtonDown(int x, int y);
		void OnRButtonDown(int x, int y);

	private:
		Theme::IconList m_Icons;
	};

	class CControlPanelButton
		: public CControlPanelItem
	{
	protected:
		String m_Text;
		int m_Width;

	public:
		CControlPanelButton(int Command, LPCTSTR pszText, bool fBreak = true, int Width = -1);
	// CControlPanelItem
		virtual void CalcSize(int Width, SIZE *pSize);
		virtual void Draw(HDC hdc, const RECT &Rect);
	};

}	// namespace TVTest

#endif
