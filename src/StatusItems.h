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


#ifndef TVTEST_STATUS_ITEMS_H
#define TVTEST_STATUS_ITEMS_H


#include "StatusView.h"
#include "Tooltip.h"
#include "EventInfoPopup.h"
#include "EpgUtil.h"


namespace TVTest
{

	enum {
		STATUS_ITEM_CHANNEL,
		STATUS_ITEM_VIDEOSIZE,
		STATUS_ITEM_VOLUME,
		STATUS_ITEM_AUDIOCHANNEL,
		STATUS_ITEM_RECORD,
		STATUS_ITEM_CAPTURE,
		STATUS_ITEM_ERROR,
		STATUS_ITEM_SIGNALLEVEL,
		STATUS_ITEM_CLOCK,
		STATUS_ITEM_PROGRAMINFO,
		STATUS_ITEM_BUFFERING,
		STATUS_ITEM_TUNER,
		STATUS_ITEM_MEDIABITRATE,
		STATUS_ITEM_FAVORITES,
		STATUS_ITEM_FIRST = STATUS_ITEM_CHANNEL,
		STATUS_ITEM_LAST  = STATUS_ITEM_FAVORITES
	};


	class CChannelStatusItem
		: public CStatusItem
	{
	public:
		CChannelStatusItem();

	// CStatusItem
		LPCTSTR GetIDText() const override { return TEXT("Channel"); }
		LPCTSTR GetName() const override { return TEXT("チャンネル"); }
		void Draw(HDC hdc, const RECT &ItemRect, const RECT &DrawRect, DrawFlag Flags) override;
		void OnLButtonDown(int x, int y) override;
		void OnRButtonDown(int x, int y) override;
		bool OnMouseWheel(int x, int y, bool fHorz, int Delta, int *pCommand) override;
	};

	class CVideoSizeStatusItem
		: public CStatusItem
	{
	public:
		CVideoSizeStatusItem();

	// CStatusItem
		LPCTSTR GetIDText() const override { return TEXT("Video"); }
		LPCTSTR GetName() const override { return TEXT("映像サイズ"); }
		bool UpdateContent() override;
		void Draw(HDC hdc, const RECT &ItemRect, const RECT &DrawRect, DrawFlag Flags) override;
		void OnLButtonDown(int x, int y) override;
		void OnRButtonDown(int x, int y) override;

	private:
		int m_OriginalVideoWidth = 0;
		int m_OriginalVideoHeight = 0;
		int m_ZoomPercentage = 0;
	};

	class CVolumeStatusItem
		: public CStatusItem
	{
	public:
		CVolumeStatusItem();

	// CStatusItem
		LPCTSTR GetIDText() const override { return TEXT("Volume"); }
		LPCTSTR GetName() const override { return TEXT("音量"); }
		void Draw(HDC hdc, const RECT &ItemRect, const RECT &DrawRect, DrawFlag Flags) override;
		void OnLButtonDown(int x, int y) override;
		void OnRButtonDown(int x, int y) override;
		void OnMouseMove(int x, int y) override;
		bool OnMouseWheel(int x, int y, bool fHorz, int Delta, int *pCommand) override;
		void SetStyle(const Style::CStyleManager *pStyleManager) override;
		void NormalizeStyle(
			const Style::CStyleManager *pStyleManager,
			const Style::CStyleScaling *pStyleScaling) override;

	private:
		struct VolumeStatusStyle
		{
			Style::IntValue BarHeight{8};
			Style::Margins BarPadding{1};
			Style::IntValue BarBorderWidth{1};
		};

		VolumeStatusStyle m_Style;
	};

	class CAudioChannelStatusItem
		: public CStatusItem
	{
	public:
		CAudioChannelStatusItem();

	// CStatusItem
		LPCTSTR GetIDText() const override { return TEXT("Audio"); }
		LPCTSTR GetName() const override { return TEXT("音声"); }
		void Draw(HDC hdc, const RECT &ItemRect, const RECT &DrawRect, DrawFlag Flags) override;
		void OnLButtonDown(int x, int y) override;
		void OnRButtonDown(int x, int y) override;
		bool OnMouseWheel(int x, int y, bool fHorz, int Delta, int *pCommand) override;

	private:
		Theme::IconList m_Icons;
	};

	class CRecordStatusItem
		: public CStatusItem
	{
		COLORREF m_CircleColor = RGB(223, 63, 0);
		bool m_fRemain = false;
		CTooltip m_Tooltip;

		size_t GetTipText(LPTSTR pszText, size_t MaxLength);
		void SetTipFont();

	public:
		CRecordStatusItem();

	// CStatusItem
		LPCTSTR GetIDText() const override { return TEXT("Record"); }
		LPCTSTR GetName() const override { return TEXT("録画"); }
		void Draw(HDC hdc, const RECT &ItemRect, const RECT &DrawRect, DrawFlag Flags) override;
		void OnLButtonDown(int x, int y) override;
		void OnRButtonDown(int x, int y) override;
		bool OnMouseHover(int x, int y) override;
		void OnFocus(bool fFocus) override;
		LRESULT OnNotifyMessage(LPNMHDR pnmh) override;
		void OnFontChanged() override;

	// CUIBase
		void SetTheme(const Theme::CThemeManager *pThemeManager) override;

	// CRecordStatusItem
		void ShowRemainTime(bool fRemain);
		void SetCircleColor(COLORREF Color);
	};

	class CCaptureStatusItem
		: public CIconStatusItem
	{
	public:
		CCaptureStatusItem();

	// CStatusItem
		LPCTSTR GetIDText() const override { return TEXT("Capture"); }
		LPCTSTR GetName() const override { return TEXT("キャプチャ"); }
		void Draw(HDC hdc, const RECT &ItemRect, const RECT &DrawRect, DrawFlag Flags) override;
		void OnLButtonDown(int x, int y) override;
		void OnRButtonDown(int x, int y) override;

	private:
		Theme::IconList m_Icons;
	};

	class CErrorStatusItem
		: public CStatusItem
	{
	public:
		CErrorStatusItem();

	// CStatusItem
		LPCTSTR GetIDText() const override { return TEXT("Error"); }
		LPCTSTR GetName() const override { return TEXT("エラー"); }
		bool UpdateContent() override;
		void Draw(HDC hdc, const RECT &ItemRect, const RECT &DrawRect, DrawFlag Flags) override;
		void OnLButtonDown(int x, int y) override;
		void OnRButtonDown(int x, int y) override;

	private:
		ULONGLONG m_ContinuityErrorPacketCount = 0;
		ULONGLONG m_ErrorPacketCount = 0;
		ULONGLONG m_ScramblePacketCount = 0;
	};

	class CSignalLevelStatusItem
		: public CStatusItem
	{
	public:
		CSignalLevelStatusItem();

	// CStatusItem
		LPCTSTR GetIDText() const override { return TEXT("Signal"); }
		LPCTSTR GetName() const override { return TEXT("信号レベル"); }
		bool UpdateContent() override;
		void Draw(HDC hdc, const RECT &ItemRect, const RECT &DrawRect, DrawFlag Flags) override;

	// CSignalLevelStatusItem
		void ShowSignalLevel(bool fShow);

	private:
		bool m_fShowSignalLevel = true;
		float m_SignalLevel = 0.0f;
		DWORD m_BitRate = 0;
	};

	class CClockStatusItem
		: public CStatusItem
	{
	public:
		CClockStatusItem();

	// CStatusItem
		LPCTSTR GetIDText() const override { return TEXT("Clock"); }
		LPCTSTR GetName() const override { return TEXT("時計"); }
		bool UpdateContent() override;
		void Draw(HDC hdc, const RECT &ItemRect, const RECT &DrawRect, DrawFlag Flags) override;
		void OnLButtonDown(int x, int y) override;
		void OnRButtonDown(int x, int y) override;

	// CClockStatusItem
		void SetTOT(bool fTOT);
		bool IsTOT() const { return m_fTOT; }
		void SetInterpolateTOT(bool fInterpolate);
		bool IsInterpolateTOT() const { return m_fInterpolateTOT; }

	private:
		void FormatTime(const LibISDB::DateTime &Time, LPTSTR pszText, int MaxLength) const;

		bool m_fTOT = false;
		bool m_fInterpolateTOT = true;
		LibISDB::DateTime m_Time;
		LibISDB::MutexLock m_Lock;
	};

	class CProgramInfoStatusItem
		: public CStatusItem
	{
		bool m_fNext = false;
		bool m_fShowProgress = true;
		bool m_fEnablePopupInfo = true;
		CEventInfoPopup m_EventInfoPopup;
		String m_Text;
		Theme::BackgroundStyle m_ProgressBackStyle{Theme::FillStyle(Theme::SolidStyle(Theme::ThemeColor(160, 160, 160)))};
		Theme::BackgroundStyle m_ProgressElapsedStyle{Theme::FillStyle(Theme::SolidStyle(Theme::ThemeColor(0, 0, 128)))};
		bool m_fValidProgress = false;
		LibISDB::EventInfo m_EventInfo;
		LibISDB::DateTime m_CurTime;

		void ShowPopupInfo();

	public:
		CProgramInfoStatusItem();

	// CStatusItem
		LPCTSTR GetIDText() const override { return TEXT("Program"); }
		LPCTSTR GetName() const override { return TEXT("番組情報"); }
		void Draw(HDC hdc, const RECT &ItemRect, const RECT &DrawRect, DrawFlag Flags) override;
		bool UpdateContent() override;
		void OnLButtonDown(int x, int y) override;
		void OnRButtonDown(int x, int y) override;
		void OnLButtonDoubleClick(int x, int y) override;
		void OnFocus(bool fFocus) override;
		bool OnMouseHover(int x, int y) override;

	// CUIBase
		void SetTheme(const Theme::CThemeManager *pThemeManager) override;

	// CProgramInfoStatusItem
		void SetShowProgress(bool fShow);
		bool GetShowProgress() const { return m_fShowProgress; }
		void EnablePopupInfo(bool fEnable);
		bool UpdateProgress();
		bool SetEventInfoFont(const Style::Font &Font) {
			return m_EventInfoPopup.SetFont(Font);
		}
		void SetPopupInfoSize(int Width, int Height);
		void GetPopupInfoSize(int *pWidth, int *pHeight) const;
	};

	class CBufferingStatusItem
		: public CStatusItem
	{
	public:
		CBufferingStatusItem();

	// CStatusItem
		LPCTSTR GetIDText() const override { return TEXT("Buffering"); }
		LPCTSTR GetName() const override { return TEXT("バッファリング"); }
		bool UpdateContent() override;
		void Draw(HDC hdc, const RECT &ItemRect, const RECT &DrawRect, DrawFlag Flags) override;
		void OnLButtonDown(int x, int y) override;

	private:
		DWORD m_StreamRemain = 0;
		int m_PacketBufferUsedPercentage = 0;
	};

	class CTunerStatusItem
		: public CStatusItem
	{
	public:
		CTunerStatusItem();

	// CStatusItem
		LPCTSTR GetIDText() const override { return TEXT("Tuner"); }
		LPCTSTR GetName() const override { return TEXT("チューナー"); }
		void Draw(HDC hdc, const RECT &ItemRect, const RECT &DrawRect, DrawFlag Flags) override;
		void OnLButtonDown(int x, int y) override;
		void OnRButtonDown(int x, int y) override;
	};

	class CMediaBitRateStatusItem
		: public CStatusItem
	{
	public:
		CMediaBitRateStatusItem();

	// CStatusItem
		LPCTSTR GetIDText() const override { return TEXT("Bitrate"); }
		LPCTSTR GetName() const override { return TEXT("ビットレート"); }
		bool UpdateContent() override;
		void Draw(HDC hdc, const RECT &ItemRect, const RECT &DrawRect, DrawFlag Flags) override;

	private:
		DWORD m_VideoBitRate = 0;
		DWORD m_AudioBitRate = 0;
	};

	class CFavoritesStatusItem
		: public CIconStatusItem
	{
	public:
		CFavoritesStatusItem();

	// CStatusItem
		LPCTSTR GetIDText() const override { return TEXT("Favorites"); }
		LPCTSTR GetName() const override { return TEXT("お気に入り"); }
		void Draw(HDC hdc, const RECT &ItemRect, const RECT &DrawRect, DrawFlag Flags) override;
		void OnLButtonDown(int x, int y) override;
		void OnRButtonDown(int x, int y) override;

	private:
		Theme::IconList m_Icons;
	};

} // namespace TVTest


#endif
