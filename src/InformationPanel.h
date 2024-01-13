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


#ifndef TVTEST_INFORMATION_PANEL_H
#define TVTEST_INFORMATION_PANEL_H


#include "PanelForm.h"
#include "UIBase.h"
#include "Settings.h"
#include "DrawUtil.h"
#include "RichEditUtil.h"
#include "WindowUtil.h"
#include "Tooltip.h"


namespace TVTest
{

	class CInformationPanel
		: public CPanelForm::CPage
		, public CSettingsBase
	{
	public:
		enum {
			ITEM_VIDEOINFO,
			ITEM_VIDEODECODER,
			ITEM_VIDEORENDERER,
			ITEM_AUDIODEVICE,
			ITEM_SIGNALLEVEL,
			ITEM_MEDIABITRATE,
			ITEM_ERROR,
			ITEM_RECORD,
			ITEM_SERVICE,
			ITEM_PROGRAMINFO,
			NUM_ITEMS,
		};

		class CItem
		{
		public:
			CItem(CInformationPanel *pPanel, bool fVisible, int PropertyID = 0);
			virtual ~CItem() = default;

			virtual int GetID() const = 0;
			virtual LPCTSTR GetName() const = 0;
			virtual bool IsSingleRow() const { return true; }
			virtual void Reset() {}
			virtual bool Update() = 0;
			virtual void Draw(HDC hdc, const RECT &Rect) {}
			virtual void DrawButton(
				HDC hdc, Theme::CThemeDraw &ThemeDraw,
				const Theme::ForegroundStyle Style,
				const RECT &ButtonRect, const RECT &TextRect,
				int Button);
			virtual int GetButtonCount() const;
			virtual bool GetButtonRect(int Button, RECT *pRect) const;
			virtual bool IsButtonEnabled(int Button) const;
			virtual bool OnButtonPushed(int Button);
			virtual bool GetButtonTipText(int Button, LPTSTR pszText, int MaxText) const;
			int ButtonHitTest(int x, int y) const;
			bool IsVisible() const { return m_fVisible; }
			void SetVisible(bool fVisible) { m_fVisible = fVisible; }
			bool HasProperty() const { return m_PropertyID != 0; }

		protected:
			void DrawItem(HDC hdc, const RECT &Rect, LPCTSTR pszText);
			void UpdateItem();
			void Redraw();

			CInformationPanel *m_pPanel;
			bool m_fVisible;
			const int m_PropertyID;
		};

		template<int id> class CItemTemplate
			: public CItem
		{
		public:
			static constexpr int ID = id;
			CItemTemplate(CInformationPanel *pPanel, bool fVisible, int PropertyID = 0)
				: CItem(pPanel, fVisible, PropertyID) {}

			int GetID() const override { return id; }
		};

		class CVideoInfoItem
			: public CItemTemplate<ITEM_VIDEOINFO>
		{
		public:
			CVideoInfoItem(CInformationPanel *pPanel, bool fVisible);

			LPCTSTR GetName() const override { return TEXT("VideoInfo"); }
			void Reset() override;
			bool Update() override;
			void Draw(HDC hdc, const RECT &Rect) override;

		private:
			struct VideoInfo
			{
				int OriginalWidth;
				int OriginalHeight;
				int DisplayWidth;
				int DisplayHeight;
				int AspectX;
				int AspectY;

				bool operator==(const VideoInfo &rhs) const noexcept = default;
			};

			VideoInfo m_VideoInfo;
		};

		class CVideoDecoderItem
			: public CItemTemplate<ITEM_VIDEODECODER>
		{
		public:
			CVideoDecoderItem(CInformationPanel *pPanel, bool fVisible);

			LPCTSTR GetName() const override { return TEXT("VideoDecoder"); }
			void Reset() override;
			bool Update() override;
			void Draw(HDC hdc, const RECT &Rect) override;

		private:
			String m_VideoDecoderName;
		};

		class CVideoRendererItem
			: public CItemTemplate<ITEM_VIDEORENDERER>
		{
		public:
			CVideoRendererItem(CInformationPanel *pPanel, bool fVisible);

			LPCTSTR GetName() const override { return TEXT("VideoRenderer"); }
			void Reset() override;
			bool Update() override;
			void Draw(HDC hdc, const RECT &Rect) override;

		private:
			String m_VideoRendererName;
		};

		class CAudioDeviceItem
			: public CItemTemplate<ITEM_AUDIODEVICE>
		{
		public:
			CAudioDeviceItem(CInformationPanel *pPanel, bool fVisible);

			LPCTSTR GetName() const override { return TEXT("AudioDevice"); }
			void Reset() override;
			bool Update() override;
			void Draw(HDC hdc, const RECT &Rect) override;

		private:
			String m_AudioDeviceName;
		};

		class CSignalLevelItem
			: public CItemTemplate<ITEM_SIGNALLEVEL>
		{
		public:
			CSignalLevelItem(CInformationPanel *pPanel, bool fVisible);

			LPCTSTR GetName() const override { return TEXT("SignalLevel"); }
			void Reset() override;
			bool Update() override;
			void Draw(HDC hdc, const RECT &Rect) override;
			void ShowSignalLevel(bool fShow);

		private:
			bool m_fShowSignalLevel = true;
			float m_SignalLevel;
			DWORD m_BitRate;
		};

		class CMediaBitRateItem
			: public CItemTemplate<ITEM_MEDIABITRATE>
		{
		public:
			CMediaBitRateItem(CInformationPanel *pPanel, bool fVisible);

			LPCTSTR GetName() const override { return TEXT("MediaBitrate"); }
			void Reset() override;
			bool Update() override;
			void Draw(HDC hdc, const RECT &Rect) override;

		private:
			DWORD m_VideoBitRate;
			DWORD m_AudioBitRate;
		};

		class CErrorItem
			: public CItemTemplate<ITEM_ERROR>
		{
		public:
			CErrorItem(CInformationPanel *pPanel, bool fVisible);

			LPCTSTR GetName() const override { return TEXT("Error"); }
			void Reset() override;
			bool Update() override;
			void Draw(HDC hdc, const RECT &Rect) override;

		private:
			bool m_fShowScramble;
			ULONGLONG m_ErrorPacketCount;
			ULONGLONG m_ContinuityErrorPacketCount;
			ULONGLONG m_ScramblePacketCount;
		};

		class CRecordItem
			: public CItemTemplate<ITEM_RECORD>
		{
		public:
			CRecordItem(CInformationPanel *pPanel, bool fVisible);

			LPCTSTR GetName() const override { return TEXT("Record"); }
			void Reset() override;
			bool Update() override;
			void Draw(HDC hdc, const RECT &Rect) override;

		private:
			bool m_fRecording;
			LONGLONG m_WroteSize;
			CRecordTask::DurationType m_RecordTime;
			LONGLONG m_DiskFreeSpace;
		};

		class CServiceItem
			: public CItemTemplate<ITEM_SERVICE>
		{
		public:
			CServiceItem(CInformationPanel *pPanel, bool fVisible);

			LPCTSTR GetName() const override { return TEXT("Service"); }
			void Reset() override;
			bool Update() override;
			void Draw(HDC hdc, const RECT &Rect) override;

		private:
			String m_ServiceName;
		};

		class CProgramInfoItem
			: public CItemTemplate<ITEM_PROGRAMINFO>
		{
		public:
			CProgramInfoItem(CInformationPanel *pPanel, bool fVisible);

			LPCTSTR GetName() const override { return TEXT("ProgramInfo"); }
			bool IsSingleRow() const override { return false; }
			void Reset() override;
			bool Update() override;
			void Draw(HDC hdc, const RECT &Rect) override;
			void DrawButton(HDC hdc, Theme::CThemeDraw &ThemeDraw,
							const Theme::ForegroundStyle Style,
							const RECT &ButtonRect, const RECT &TextRect,
							int Button) override;
			int GetButtonCount() const override { return 2; }
			bool GetButtonRect(int Button, RECT *pRect) const override;
			bool IsButtonEnabled(int Button) const override;
			bool OnButtonPushed(int Button) override;
			bool GetButtonTipText(int Button, LPTSTR pszText, int MaxText) const override;
			const String &GetInfoText() const { return m_InfoText; }
			void SetNext(bool fNext);
			bool IsNext() const { return m_fNext; }

		private:
			enum {
				BUTTON_PREV,
				BUTTON_NEXT
			};

			String m_InfoText;
			bool m_fNext;
		};

		static bool Initialize(HINSTANCE hinst);

		CInformationPanel();
		~CInformationPanel();

	// CBasicWindow
		bool Create(HWND hwndParent, DWORD Style, DWORD ExStyle = 0, int ID = 0) override;

	// CUIBase
		void SetStyle(const Style::CStyleManager *pStyleManager) override;
		void NormalizeStyle(
			const Style::CStyleManager *pStyleManager,
			const Style::CStyleScaling *pStyleScaling) override;
		void SetTheme(const Theme::CThemeManager *pThemeManager) override;

	// CPage
		bool SetFont(const Style::Font &Font) override;

	// CSettingsBase
		bool ReadSettings(CSettings &Settings) override;
		bool WriteSettings(CSettings &Settings) override;

	// CInformationPanel
		bool IsVisible() const;
		CItem *GetItem(int Item);
		const CItem *GetItem(int Item) const;
		template<typename T> T *GetItem() { return static_cast<T*>(GetItem(T::ID)); }
		template<typename T> const T *GetItem() const { return static_cast<const T*>(GetItem(T::ID)); }
		bool SetItemVisible(int Item, bool fVisible);
		bool IsItemVisible(int Item) const;
		bool UpdateItem(int Item);
		void RedrawItem(int Item);
		bool ResetItem(int Item);
		bool UpdateAllItems();
		bool SetProgramInfoRichEdit(bool fRichEdit);

	private:
		struct InformationPanelStyle
		{
			Style::Size ButtonSize{16, 16};
			Style::IntValue LineSpacing{1};
			Style::Margins ItemButtonMargin{4, 0, 0, 0};
			Style::Margins ItemButtonPadding{2};

			void SetStyle(const Style::CStyleManager *pStyleManager);
			void NormalizeStyle(
				const Style::CStyleManager *pStyleManager,
				const Style::CStyleScaling *pStyleScaling);
		};

		struct InformationPanelTheme
		{
			Theme::Style Style;
			Theme::Style ProgramInfoStyle;
			Theme::Style ButtonStyle;
			Theme::Style ButtonHotStyle;
		};

		template<typename T> void RegisterItem(bool fVisible = true)
		{
			T *pItem = new T(this, fVisible);
			m_ItemList[pItem->GetID()].reset(pItem);
		}

		class CProgramInfoSubclass
			: public CWindowSubclass
		{
		public:
			CProgramInfoSubclass(CInformationPanel *pInfoPanel);

		private:
			LRESULT OnMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

			CInformationPanel *m_pInfoPanel;
		};

		struct ItemButtonNumber
		{
			int Item = -1;
			int Button = -1;

			ItemButtonNumber() = default;
			ItemButtonNumber(int item, int button) : Item(item), Button(button) {}

			bool operator==(const ItemButtonNumber &rhs) const noexcept = default;

			bool IsValid() const { return Item >= 0 && Button >= 0; }
		};

		static const LPCTSTR m_pszClassName;
		static HINSTANCE m_hinst;

		std::unique_ptr<CItem> m_ItemList[NUM_ITEMS];
		InformationPanelStyle m_Style;
		InformationPanelTheme m_Theme;
		DrawUtil::CBrush m_BackBrush;
		DrawUtil::CBrush m_ProgramInfoBackBrush;
		Style::Font m_StyleFont;
		DrawUtil::CFont m_Font;
		int m_FontHeight = 0;
		DrawUtil::CFont m_IconFont;
		DrawUtil::COffscreen m_Offscreen;
		CTooltip m_Tooltip;
		int m_ItemButtonWidth = 0;

		HWND m_hwndProgramInfo = nullptr;
		CProgramInfoSubclass m_ProgramInfoSubclass{this};
		CRichEditUtil m_RichEditUtil;
		bool m_fUseRichEdit = true;
		CRichEditLinkHandler m_RichEditLink;
		ItemButtonNumber m_HotButton;

		void UpdateProgramInfoText();
		bool CreateProgramInfoEdit();
		void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
		void GetItemRect(int Item, RECT *pRect) const;
		void CalcFontHeight();
		void Draw(HDC hdc, const RECT &PaintRect);
		bool GetDrawItemRect(int Item, RECT *pRect, const RECT &PaintRect) const;
		void DrawItem(CItem *pItem, HDC hdc, LPCTSTR pszText, const RECT &Rect);
		void RedrawButton(ItemButtonNumber Button);
		void RedrawButton(int Item, int Button) { RedrawButton(ItemButtonNumber(Item, Button)); }
		ItemButtonNumber ButtonHitTest(int x, int y) const;
		void SetHotButton(ItemButtonNumber Button);

	// CCustomWindow
		LRESULT OnMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

	// CUIBase
		void ApplyStyle() override;
		void RealizeStyle() override;
	};

} // namespace TVTest


#endif
