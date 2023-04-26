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
		void CalcSize(int Width, SIZE *pSize) override;
		void Draw(HDC hdc, const RECT &Rect) override;
		void OnLButtonDown(int x, int y) override;
		void OnRButtonDown(int x, int y) override;
	};

	class CChannelControlItem
		: public CControlPanelItem
	{
	public:
	// CControlPanelItem
		void CalcSize(int Width, SIZE *pSize) override;
		void Draw(HDC hdc, const RECT &Rect) override;
		void OnLButtonDown(int x, int y) override;
		void OnRButtonDown(int x, int y) override;
	};

	class CVideoControlItem
		: public CControlPanelItem
	{
	public:
	// CControlPanelItem
		void CalcSize(int Width, SIZE *pSize) override;
		void Draw(HDC hdc, const RECT &Rect) override;
		void OnLButtonDown(int x, int y) override;
		void OnRButtonDown(int x, int y) override;
	};

	class CVolumeControlItem
		: public CControlPanelItem
	{
	public:
	// CControlPanelItem
		void CalcSize(int Width, SIZE *pSize) override;
		void Draw(HDC hdc, const RECT &Rect) override;
		void OnLButtonDown(int x, int y) override;
		void OnRButtonDown(int x, int y) override;
		void OnMouseMove(int x, int y) override;
		void SetStyle(const Style::CStyleManager *pStyleManager) override;
		void NormalizeStyle(
			const Style::CStyleManager *pStyleManager,
			const Style::CStyleScaling *pStyleScaling) override;

	private:
		struct VolumeControlStyle
		{
			Style::IntValue BarHeight{8};
			Style::Margins BarPadding{1};
			Style::IntValue BarBorderWidth{1};
		};

		VolumeControlStyle m_Style;
	};

	class CAudioControlItem
		: public CControlPanelItem
	{
	public:
	// CControlPanelItem
		void CalcSize(int Width, SIZE *pSize) override;
		void Draw(HDC hdc, const RECT &Rect) override;
		void OnLButtonDown(int x, int y) override;
		void OnRButtonDown(int x, int y) override;

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
		virtual void CalcSize(int Width, SIZE *pSize) override;
		virtual void Draw(HDC hdc, const RECT &Rect) override;
	};

}	// namespace TVTest

#endif
