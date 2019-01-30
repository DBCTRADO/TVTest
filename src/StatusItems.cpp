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


#include "stdafx.h"
#include "TVTest.h"
#include "StatusItems.h"
#include "AppMain.h"
#include "Menu.h"
#include "resource.h"
#include "Common/DebugDef.h"


namespace TVTest
{


CChannelStatusItem::CChannelStatusItem()
	: CStatusItem(STATUS_ITEM_CHANNEL, SizeValue(9 * EM_FACTOR, SizeUnit::EM))
{
}

void CChannelStatusItem::Draw(HDC hdc, const RECT &ItemRect, const RECT &DrawRect, unsigned int Flags)
{
	if ((Flags & DRAW_PREVIEW) != 0) {
		DrawText(hdc, DrawRect, TEXT("アフリカ中央テレビ"));
		return;
	}

	CAppMain &App = GetAppClass();
	const CChannelManager &ChannelManager = App.ChannelManager;
	const CChannelInfo *pInfo;
	TCHAR szText[4 + MAX_CHANNEL_NAME];

	if (App.UICore.GetSkin()->IsWheelChannelChanging()) {
		COLORREF crText, crBack;

		crText = ::GetTextColor(hdc);
		crBack = ::GetBkColor(hdc);
		::SetTextColor(hdc, MixColor(crText, crBack, 128));
		pInfo = ChannelManager.GetChangingChannelInfo();
		StringPrintf(
			szText, TEXT("%d: %s"),
			pInfo->GetChannelNo(), pInfo->GetName());
	} else if ((pInfo = ChannelManager.GetCurrentChannelInfo()) != nullptr) {
		TCHAR szService[MAX_CHANNEL_NAME];
		StringPrintf(
			szText, TEXT("%d: %s"),
			pInfo->GetChannelNo(),
			App.Core.GetCurrentServiceName(szService, lengthof(szService)) ? szService : pInfo->GetName());
	} else {
		StringCopy(szText, TEXT("<チャンネル>"));
	}
	DrawText(hdc, DrawRect, szText);
}

void CChannelStatusItem::OnLButtonDown(int x, int y)
{
	POINT pt;
	UINT Flags;
	RECT rc;

	GetMenuPos(&pt, &Flags, &rc);
	GetAppClass().UICore.PopupSubMenu(
		CMainMenu::SUBMENU_CHANNEL, &pt, Flags | TPM_RIGHTBUTTON, &rc);
}

void CChannelStatusItem::OnRButtonDown(int x, int y)
{
	POINT pt;
	UINT Flags;
	RECT rc;

	GetMenuPos(&pt, &Flags, &rc);
	GetAppClass().UICore.PopupSubMenu(CMainMenu::SUBMENU_SERVICE, &pt, Flags | TPM_RIGHTBUTTON, &rc);
}

bool CChannelStatusItem::OnMouseWheel(int x, int y, bool fHorz, int Delta, int *pCommand)
{
	*pCommand = CM_WHEEL_CHANNEL;
	return true;
}


CVideoSizeStatusItem::CVideoSizeStatusItem()
	: CStatusItem(STATUS_ITEM_VIDEOSIZE, SizeValue(11 * EM_FACTOR, SizeUnit::EM))
	, m_OriginalVideoWidth(0)
	, m_OriginalVideoHeight(0)
	, m_ZoomPercentage(0)
{
}

bool CVideoSizeStatusItem::UpdateContent()
{
	const CAppMain &App = GetAppClass();
	const CCoreEngine &CoreEngine = App.CoreEngine;
	const int OriginalVideoWidth = CoreEngine.GetOriginalVideoWidth();
	const int OriginalVideoHeight = CoreEngine.GetOriginalVideoHeight();
	const int ZoomPercentage = App.UICore.GetZoomPercentage();

	if (OriginalVideoWidth == m_OriginalVideoWidth
			&& OriginalVideoHeight == m_OriginalVideoHeight
			&& ZoomPercentage == m_ZoomPercentage)
		return false;

	m_OriginalVideoWidth = OriginalVideoWidth;
	m_OriginalVideoHeight = OriginalVideoHeight;
	m_ZoomPercentage = ZoomPercentage;

	return true;
}

void CVideoSizeStatusItem::Draw(HDC hdc, const RECT &ItemRect, const RECT &DrawRect, unsigned int Flags)
{
	if ((Flags & DRAW_PREVIEW) == 0) {
		TCHAR szText[64];

		StringPrintf(
			szText, TEXT("%d x %d (%d %%)"),
			m_OriginalVideoWidth, m_OriginalVideoHeight,
			m_ZoomPercentage);
		DrawText(hdc, DrawRect, szText);
	} else {
		DrawText(
			hdc, DrawRect,
#ifndef TVTEST_FOR_1SEG
			TEXT("1920 x 1080 (100 %)")
#else
			TEXT("320 x 180 (100 %)")
#endif
			);
	}
}

void CVideoSizeStatusItem::OnLButtonDown(int x, int y)
{
	POINT pt;
	UINT Flags;
	RECT rc;

	GetMenuPos(&pt, &Flags, &rc);
	GetAppClass().UICore.PopupSubMenu(
		CMainMenu::SUBMENU_ZOOM, &pt, Flags | TPM_RIGHTBUTTON, &rc);
}

void CVideoSizeStatusItem::OnRButtonDown(int x, int y)
{
	POINT pt;
	UINT Flags;
	RECT rc;

	GetMenuPos(&pt, &Flags, &rc);
	GetAppClass().UICore.PopupSubMenu(
		CMainMenu::SUBMENU_ASPECTRATIO, &pt, Flags | TPM_RIGHTBUTTON, &rc);
}


CVolumeStatusItem::CVolumeStatusItem()
	: CStatusItem(STATUS_ITEM_VOLUME, SizeValue(7 * EM_FACTOR, SizeUnit::EM))
{
}

void CVolumeStatusItem::Draw(HDC hdc, const RECT &ItemRect, const RECT &DrawRect, unsigned int Flags)
{
	CUICore *pUICore = &GetAppClass().UICore;
	LOGBRUSH lb;
	HPEN hpen, hpenOld;
	HBRUSH hbrOld;
	RECT rc;
	COLORREF crText = ::GetTextColor(hdc), crBar;

	lb.lbStyle = BS_SOLID;
	lb.lbColor = crText;
	lb.lbHatch = 0;
	hpen = ::ExtCreatePen(
		PS_GEOMETRIC | PS_SOLID | PS_INSIDEFRAME | PS_JOIN_MITER,
		m_Style.BarBorderWidth, &lb, 0, nullptr);
	hpenOld = SelectPen(hdc, hpen);
	hbrOld = SelectBrush(hdc, ::GetStockObject(NULL_BRUSH));
	rc.left = DrawRect.left;
	rc.top = DrawRect.top + ((DrawRect.bottom - DrawRect.top) - m_Style.BarHeight) / 2;
	rc.right = DrawRect.right;
	rc.bottom = rc.top + m_Style.BarHeight;
	::Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);
	::SelectObject(hdc, hbrOld);
	::SelectObject(hdc, hpenOld);
	::DeleteObject(hpen);
	if (!pUICore->GetMute())
		crBar = crText;
	else
		crBar = MixColor(crText, ::GetBkColor(hdc), 128);
	::InflateRect(&rc, -m_Style.BarBorderWidth, -m_Style.BarBorderWidth);
	Style::Subtract(&rc, m_Style.BarPadding);
	rc.right = rc.left + (rc.right - rc.left) * pUICore->GetVolume() / CCoreEngine::MAX_VOLUME;
	DrawUtil::Fill(hdc, &rc, crBar);
}

void CVolumeStatusItem::OnLButtonDown(int x, int y)
{
	::SetCapture(m_pStatus->GetHandle());
	OnMouseMove(x, y);
}

void CVolumeStatusItem::OnRButtonDown(int x, int y)
{
	CUICore *pUICore = &GetAppClass().UICore;
	pUICore->SetMute(!pUICore->GetMute());
}

void CVolumeStatusItem::OnMouseMove(int x, int y)
{
	if (::GetCapture() != m_pStatus->GetHandle())
		return;

	CUICore *pUICore = &GetAppClass().UICore;
	RECT rcItem, rcClient;
	int Volume;

	GetRect(&rcItem);
	GetClientRect(&rcClient);
	Style::Subtract(&rcClient, m_Style.BarPadding);
	Volume = (x - (rcClient.left - rcItem.left)) * CCoreEngine::MAX_VOLUME / ((rcClient.right - rcClient.left) - 1);
	if (Volume < 0)
		Volume = 0;
	else if (Volume > CCoreEngine::MAX_VOLUME)
		Volume = CCoreEngine::MAX_VOLUME;
	if (pUICore->GetMute() || Volume != pUICore->GetVolume())
		pUICore->SetVolume(Volume, false);
}

bool CVolumeStatusItem::OnMouseWheel(int x, int y, bool fHorz, int Delta, int *pCommand)
{
	*pCommand = CM_WHEEL_VOLUME;
	return true;
}

void CVolumeStatusItem::SetStyle(const Style::CStyleManager *pStyleManager)
{
	m_Style = VolumeStatusStyle();
	pStyleManager->Get(TEXT("status-bar.volume.bar.height"), &m_Style.BarHeight);
	pStyleManager->Get(TEXT("status-bar.volume.bar.padding"), &m_Style.BarPadding);
	pStyleManager->Get(TEXT("status-bar.volume.bar.border.width"), &m_Style.BarBorderWidth);
}

void CVolumeStatusItem::NormalizeStyle(
	const Style::CStyleManager *pStyleManager,
	const Style::CStyleScaling *pStyleScaling)
{
	pStyleScaling->ToPixels(&m_Style.BarHeight);
	pStyleScaling->ToPixels(&m_Style.BarPadding);
	pStyleScaling->ToPixels(&m_Style.BarBorderWidth);
}

CVolumeStatusItem::VolumeStatusStyle::VolumeStatusStyle()
	: BarHeight(8)
	, BarPadding(1)
	, BarBorderWidth(1)
{
}


CAudioChannelStatusItem::CAudioChannelStatusItem()
	: CStatusItem(STATUS_ITEM_AUDIOCHANNEL, SizeValue(7 * EM_FACTOR, SizeUnit::EM))
{
}

void CAudioChannelStatusItem::Draw(HDC hdc, const RECT &ItemRect, const RECT &DrawRect, unsigned int Flags)
{
	if ((Flags & DRAW_PREVIEW) != 0) {
		DrawText(hdc, DrawRect, TEXT("Stereo"));
		return;
	}

	CAppMain &App = GetAppClass();
	RECT rc = DrawRect;

	const LibISDB::ViewerFilter *pViewer = App.CoreEngine.GetFilter<LibISDB::ViewerFilter>();
	if (pViewer != nullptr && pViewer->IsSPDIFPassthrough()) {
		Style::Size IconSize = m_pStatus->GetIconSize();
		if (!m_Icons.IsCreated()) {
			static const Theme::IconList::ResourceInfo ResourceList[] = {
				{MAKEINTRESOURCE(IDB_PASSTHROUGH16), 16, 16},
				{MAKEINTRESOURCE(IDB_PASSTHROUGH32), 32, 32},
			};
			m_Icons.Load(
				App.GetResourceInstance(),
				IconSize.Width, IconSize.Height,
				ResourceList, lengthof(ResourceList));
		}
		RECT rcIcon = rc;
		rcIcon.right = rcIcon.left + IconSize.Width;
		DrawIcon(hdc, rcIcon, m_Icons);
		rc.left += IconSize.Width + IconSize.Width / 4;
	}

	TCHAR szText[64];
	if (App.UICore.FormatCurrentAudioText(szText, lengthof(szText)) <= 0)
		StringCopy(szText, TEXT("<音声>"));
	DrawText(hdc, rc, szText);
}

void CAudioChannelStatusItem::OnLButtonDown(int x, int y)
{
	if (!GetAppClass().UICore.SwitchAudio())
		OnRButtonDown(x, y);
}

void CAudioChannelStatusItem::OnRButtonDown(int x, int y)
{
	POINT pt;
	UINT Flags;
	RECT rc;

	GetMenuPos(&pt, &Flags, &rc);
	GetAppClass().UICore.PopupSubMenu(
		CMainMenu::SUBMENU_AUDIO, &pt, Flags | TPM_RIGHTBUTTON, &rc);
}

bool CAudioChannelStatusItem::OnMouseWheel(int x, int y, bool fHorz, int Delta, int *pCommand)
{
	*pCommand = CM_WHEEL_AUDIO;
	return true;
}


CRecordStatusItem::CRecordStatusItem()
	: CStatusItem(STATUS_ITEM_RECORD, SizeValue(6 * EM_FACTOR, SizeUnit::EM))
	, m_fRemain(false)
	, m_CircleColor(RGB(223, 63, 0))
{
}

void CRecordStatusItem::Draw(HDC hdc, const RECT &ItemRect, const RECT &DrawRect, unsigned int Flags)
{
	if ((Flags & DRAW_PREVIEW) != 0) {
		RECT rc = DrawRect;
		COLORREF OldTextColor = ::SetTextColor(hdc, m_CircleColor);
		::DrawText(hdc, TEXT("●"), -1, &rc, DT_LEFT | DT_SINGLELINE | DT_VCENTER);
		::SetTextColor(hdc, OldTextColor);
		rc.left += m_pStatus->GetFontHeight() + 4;
		DrawText(hdc, rc, TEXT("0:24:15"));
		return;
	}

	const CRecordManager &RecordManager = GetAppClass().RecordManager;
	const int FontHeight = m_pStatus->GetFontHeight();
	RECT rc;
	TCHAR szText[32];

	rc = DrawRect;
	if (RecordManager.IsRecording()) {
		if (RecordManager.IsPaused()) {
			HBRUSH hbr = ::CreateSolidBrush(::GetTextColor(hdc));
			RECT rc1;

			rc1.left = rc.left;
			rc1.top = rc.top + ((rc.bottom - rc.top) - FontHeight) / 2;
			rc1.right = rc1.left + FontHeight / 2 - 1;
			rc1.bottom = rc1.top + FontHeight;
			::FillRect(hdc, &rc1, hbr);
			rc1.left = rc1.right + 2;
			rc1.right = rc.left + FontHeight;
			::FillRect(hdc, &rc1, hbr);
			::DeleteObject(hbr);
		} else {
#if 0
			// Ellipseで小さい丸を描くと汚い
			HBRUSH hbr = ::CreateSolidBrush(m_CircleColor);
			HBRUSH hbrOld;
			HPEN hpenOld;

			rc1.right = rc1.left + FontHeight;
			hbrOld = SelectBrush(hdc, hbr);
			hpenOld = SelectPen(hdc, ::GetStockObject(NULL_PEN));
			::Ellipse(hdc, rc1.left, rc1.top, rc1.right, rc1.bottom);
			SelectPen(hdc, hpenOld);
			SelectBrush(hdc, hbrOld);
			::DeleteObject(hbr);
#else
			COLORREF OldTextColor = ::SetTextColor(hdc, m_CircleColor);
			::DrawText(
				hdc, TEXT("●"), -1, &rc,
				DT_LEFT | DT_SINGLELINE | DT_VCENTER);
			::SetTextColor(hdc, OldTextColor);
#endif
		}
		rc.left += FontHeight + 4;
		bool fRemain = m_fRemain && RecordManager.IsStopTimeSpecified();
		int RecordSec;
		if (fRemain) {
			RecordSec = (int)(RecordManager.GetRemainTime() / 1000);
			if (RecordSec < 0)
				RecordSec = 0;
		} else {
			RecordSec = (int)(RecordManager.GetRecordTime() / 1000);
		}
		StringPrintf(
			szText, TEXT("%s%d:%02d:%02d"),
			fRemain ? TEXT("-") : TEXT(""),
			RecordSec / (60 * 60), (RecordSec / 60) % 60, RecordSec % 60);
	} else if (RecordManager.IsReserved()) {
		StringCopy(szText, TEXT("■ 録画待機"));
	} else {
		StringCopy(szText, TEXT("■ <録画>"));
	}
	DrawText(hdc, rc, szText);
}

void CRecordStatusItem::OnLButtonDown(int x, int y)
{
	CAppMain &App = GetAppClass();
	const CRecordManager &RecordManager = App.RecordManager;
	CUICore &UICore = App.UICore;
	bool fRecording = RecordManager.IsRecording();

	if (fRecording && !RecordManager.IsPaused()) {
		if (!UICore.ConfirmStopRecording())
			return;
	}
	UICore.DoCommand(
		RecordManager.IsReserved() ?
			CM_RECORDOPTION :
		!fRecording ?
			CM_STATUSBARRECORD :
			CM_RECORD);
}

void CRecordStatusItem::OnRButtonDown(int x, int y)
{
	POINT pt;
	UINT Flags;
	RECT rc;

	GetMenuPos(&pt, &Flags, &rc);
	GetAppClass().UICore.ShowSpecialMenu(
		CUICore::MenuType::Record, &pt, Flags | TPM_RIGHTBUTTON, &rc);
}

bool CRecordStatusItem::OnMouseHover(int x, int y)
{
	const CRecordManager &RecordManager = GetAppClass().RecordManager;

	if (RecordManager.IsRecording()) {
		const HWND hwndStatus = m_pStatus->GetHandle();

		if (!m_Tooltip.IsCreated()) {
			m_Tooltip.Create(hwndStatus);
			m_Tooltip.AddTrackingTip(1);
			SetTipFont();
		}

		TCHAR szText[256];
		GetTipText(szText, lengthof(szText));
		m_Tooltip.SetText(1, szText);
		if (!m_Tooltip.IsVisible())
			m_Tooltip.TrackActivate(1, true);
	}
	return true;
}

int CRecordStatusItem::GetTipText(LPTSTR pszText, int MaxLength)
{
	const CRecordManager &RecordManager = GetAppClass().RecordManager;

	if (RecordManager.IsRecording()) {
		const CRecordTask *pRecordTask = RecordManager.GetRecordTask();
		int Length;

		unsigned int RecordSec = (unsigned int)(pRecordTask->GetRecordTime() / 1000);
		Length = StringPrintf(
			pszText, MaxLength,
			TEXT("● %d:%02d:%02d"),
			RecordSec / (60 * 60), (RecordSec / 60) % 60, RecordSec % 60);

		LONGLONG WroteSize = pRecordTask->GetWroteSize();
		if (WroteSize >= 0) {
			unsigned int Size = (unsigned int)(WroteSize / (1024 * 1024 / 100));
			Length += StringPrintf(
				pszText + Length, MaxLength - Length,
				TEXT("\r\nサイズ: %d.%02d MB"),
				Size / 100, Size % 100);
		}

		LONGLONG DiskFreeSpace = pRecordTask->GetFreeSpace();
		if (DiskFreeSpace > 0) {
			unsigned int FreeSpace = (unsigned int)(DiskFreeSpace / (ULONGLONG)(1024 * 1024 * 1024 / 100));
			Length += StringPrintf(
				pszText + Length, MaxLength - Length,
				TEXT("\r\n空き容量: %d.%02d GB"),
				FreeSpace / 100, FreeSpace % 100);
		}

		return Length;
	}

	pszText[0] = '\0';
	return 0;
}

void CRecordStatusItem::SetTipFont()
{
	m_Tooltip.SetFont(m_pStatus->GetFont());
	m_Tooltip.SetMaxWidth(m_pStatus->GetFontHeight() * 20);
}

void CRecordStatusItem::OnFocus(bool fFocus)
{
	if (!fFocus && m_Tooltip.IsCreated())
		m_Tooltip.TrackActivate(1, false);
}

LRESULT CRecordStatusItem::OnNotifyMessage(LPNMHDR pnmh)
{
	if (pnmh->hwndFrom == m_Tooltip.GetHandle()) {
		if (pnmh->code == TTN_SHOW) {
			RECT rc, rcTip;
			int x, y;

			GetRect(&rc);
			MapWindowRect(m_pStatus->GetHandle(), nullptr, &rc);
			::GetWindowRect(pnmh->hwndFrom, &rcTip);
			x = rc.left + ((rc.right - rc.left) - (rcTip.right - rcTip.left)) / 2;
			y = rc.top - (rcTip.bottom - rcTip.top);
			::SendMessage(pnmh->hwndFrom, TTM_TRACKPOSITION, 0, MAKELONG(x, y));
			::SetWindowPos(
				pnmh->hwndFrom, nullptr, x, y, 0, 0,
				SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
			return TRUE;
		}
	}
	return 0;
}

void CRecordStatusItem::OnFontChanged()
{
	if (m_Tooltip.IsCreated())
		SetTipFont();
}

void CRecordStatusItem::SetTheme(const Theme::CThemeManager *pThemeManager)
{
	SetCircleColor(pThemeManager->GetColor(CColorScheme::COLOR_STATUSRECORDINGCIRCLE));
}

void CRecordStatusItem::ShowRemainTime(bool fRemain)
{
	if (m_fRemain != fRemain) {
		m_fRemain = fRemain;
		Update();
	}
}

void CRecordStatusItem::SetCircleColor(COLORREF Color)
{
	if (m_CircleColor != Color) {
		m_CircleColor = Color;
		Update();
	}
}


CCaptureStatusItem::CCaptureStatusItem()
	: CIconStatusItem(STATUS_ITEM_CAPTURE, 16)
{
}

void CCaptureStatusItem::Draw(HDC hdc, const RECT &ItemRect, const RECT &DrawRect, unsigned int Flags)
{
	if (!m_Icons.IsCreated()) {
		static const Theme::IconList::ResourceInfo ResourceList[] = {
			{MAKEINTRESOURCE(IDB_CAPTURE16), 16, 16},
			{MAKEINTRESOURCE(IDB_CAPTURE32), 32, 32},
		};
		Style::Size IconSize = m_pStatus->GetIconSize();
		m_Icons.Load(
			GetAppClass().GetResourceInstance(),
			IconSize.Width, IconSize.Height,
			ResourceList, lengthof(ResourceList));
	}
	DrawIcon(hdc, DrawRect, m_Icons);
}

void CCaptureStatusItem::OnLButtonDown(int x, int y)
{
	GetAppClass().UICore.DoCommand(CM_CAPTURE);
}

void CCaptureStatusItem::OnRButtonDown(int x, int y)
{
	POINT pt;
	UINT Flags;
	RECT rc;

	GetMenuPos(&pt, &Flags, &rc);
	GetAppClass().UICore.ShowSpecialMenu(
		CUICore::MenuType::Capture, &pt, Flags | TPM_RIGHTBUTTON, &rc);
}


CErrorStatusItem::CErrorStatusItem()
	: CStatusItem(STATUS_ITEM_ERROR, SizeValue(11 * EM_FACTOR, SizeUnit::EM))
	, m_ContinuityErrorPacketCount(0)
	, m_ErrorPacketCount(0)
	, m_ScramblePacketCount(0)
{
}

bool CErrorStatusItem::UpdateContent()
{
	const CCoreEngine &CoreEngine = GetAppClass().CoreEngine;
	const ULONGLONG ContinuityErrorPacketCount = CoreEngine.GetContinuityErrorPacketCount();
	const ULONGLONG ErrorPacketCount = CoreEngine.GetErrorPacketCount();
	const ULONGLONG ScramblePacketCount = CoreEngine.GetScramblePacketCount();

	if (ContinuityErrorPacketCount == m_ContinuityErrorPacketCount
			&& ErrorPacketCount == m_ErrorPacketCount
			&& ScramblePacketCount == m_ScramblePacketCount)
		return false;

	m_ContinuityErrorPacketCount = ContinuityErrorPacketCount;
	m_ErrorPacketCount = ErrorPacketCount;
	m_ScramblePacketCount = ScramblePacketCount;

	return true;
}

void CErrorStatusItem::Draw(HDC hdc, const RECT &ItemRect, const RECT &DrawRect, unsigned int Flags)
{
	if ((Flags & DRAW_PREVIEW) == 0) {
		TCHAR szText[80];

		StringPrintf(
			szText, TEXT("D %llu / E %llu / S %llu"),
			m_ContinuityErrorPacketCount,
			m_ErrorPacketCount,
			m_ScramblePacketCount);
		DrawText(hdc, DrawRect, szText);
	} else {
		DrawText(hdc, DrawRect, TEXT("D 2 / E 0 / S 127"));
	}
}

void CErrorStatusItem::OnLButtonDown(int x, int y)
{
	GetAppClass().UICore.DoCommand(CM_RESETERRORCOUNT);
}

void CErrorStatusItem::OnRButtonDown(int x, int y)
{
	POINT pt;
	UINT Flags;
	RECT rc;

	GetMenuPos(&pt, &Flags, &rc);
	GetAppClass().UICore.ShowSpecialMenu(
		CUICore::MenuType::StreamError, &pt, Flags | TPM_RIGHTBUTTON, &rc);
}


CSignalLevelStatusItem::CSignalLevelStatusItem()
	: CStatusItem(STATUS_ITEM_SIGNALLEVEL, SizeValue(11 * EM_FACTOR, SizeUnit::EM))
	, m_fShowSignalLevel(true)
	, m_SignalLevel(0.0f)
	, m_BitRate(0)
{
}

bool CSignalLevelStatusItem::UpdateContent()
{
	const CCoreEngine &CoreEngine = GetAppClass().CoreEngine;
	const float SignalLevel =
		m_fShowSignalLevel ? CoreEngine.GetSignalLevel() : 0.0f;
	const DWORD BitRate = CoreEngine.GetBitRate();

	if (SignalLevel == m_SignalLevel && BitRate == m_BitRate)
		return false;

	m_SignalLevel = SignalLevel;
	m_BitRate = BitRate;

	return true;
}

void CSignalLevelStatusItem::Draw(HDC hdc, const RECT &ItemRect, const RECT &DrawRect, unsigned int Flags)
{
	const CCoreEngine &CoreEngine = GetAppClass().CoreEngine;
	TCHAR szText[64];

	if ((Flags & DRAW_PREVIEW) == 0) {
		int Length = 0;

		if (m_fShowSignalLevel) {
			TCHAR szSignalLevel[32];
			CoreEngine.GetSignalLevelText(m_SignalLevel, szSignalLevel, lengthof(szSignalLevel));
			Length = StringPrintf(szText, TEXT("%s / "), szSignalLevel);
		}
		CoreEngine.GetBitRateText(m_BitRate, szText + Length, lengthof(szText) - Length);
	} else {
		TCHAR szSignalLevel[32], szBitRate[32];

		CoreEngine.GetSignalLevelText(24.52f, szSignalLevel, lengthof(szSignalLevel));
		CoreEngine.GetBitRateText(16.73f, szBitRate, lengthof(szBitRate));
		StringPrintf(szText, TEXT("%s / %s"), szSignalLevel, szBitRate);
	}

	DrawText(hdc, DrawRect, szText);
}

void CSignalLevelStatusItem::ShowSignalLevel(bool fShow)
{
	if (m_fShowSignalLevel != fShow) {
		m_fShowSignalLevel = fShow;
		UpdateContent();
		Redraw();
	}
}


CClockStatusItem::CClockStatusItem()
	: CStatusItem(STATUS_ITEM_CLOCK, SizeValue(5 * EM_FACTOR, SizeUnit::EM))
	, m_fTOT(false)
	, m_fInterpolateTOT(true)
{
}

bool CClockStatusItem::UpdateContent()
{
	LibISDB::BlockLock Lock(m_Lock);

	LibISDB::DateTime Time;

	if (m_fTOT) {
		const LibISDB::AnalyzerFilter *pAnalyzer =
			GetAppClass().CoreEngine.GetFilter<LibISDB::AnalyzerFilter>();
		bool fResult;

		if (m_fInterpolateTOT)
			fResult = pAnalyzer->GetInterpolatedTOTTime(&Time);
		else
			fResult = pAnalyzer->GetTOTTime(&Time);
		if (!fResult)
			Time.Reset();
	} else {
		Time.NowLocal();
	}

	Time.Millisecond = 0;

	if (Time == m_Time)
		return false;

	m_Time = Time;

	return true;
}

void CClockStatusItem::Draw(HDC hdc, const RECT &ItemRect, const RECT &DrawRect, unsigned int Flags)
{
	LibISDB::BlockLock Lock(m_Lock);

	TCHAR szText[64];

	if ((Flags & DRAW_PREVIEW) == 0) {
		if (!m_Time.IsValid())
			return;
		FormatTime(m_Time, szText, lengthof(szText));
	} else {
		LibISDB::DateTime Time;
		Time.Year = 2112;
		Time.Month = 9;
		Time.Day = 3;
		Time.Hour = 17;
		Time.Minute = 31;
		Time.Second = 15;
		Time.SetDayOfWeek();
		FormatTime(Time, szText, lengthof(szText));
	}

	DrawText(hdc, DrawRect, szText);
}

void CClockStatusItem::OnLButtonDown(int x, int y)
{
	GetAppClass().UICore.DoCommand(CM_SHOWTOTTIME);
}

void CClockStatusItem::OnRButtonDown(int x, int y)
{
	POINT pt;
	UINT Flags;
	RECT rc;

	GetMenuPos(&pt, &Flags, &rc);
	GetAppClass().UICore.ShowSpecialMenu(
		CUICore::MenuType::Clock, &pt, Flags | TPM_RIGHTBUTTON, &rc);
}

void CClockStatusItem::SetTOT(bool fTOT)
{
	LibISDB::BlockLock Lock(m_Lock);

	if (m_fTOT != fTOT) {
		m_fTOT = fTOT;
		m_Time.Reset();
		Update();
	}
}

void CClockStatusItem::SetInterpolateTOT(bool fInterpolate)
{
	LibISDB::BlockLock Lock(m_Lock);

	m_fInterpolateTOT = fInterpolate;
}

void CClockStatusItem::FormatTime(const LibISDB::DateTime &Time, LPTSTR pszText, int MaxLength) const
{
#if 0
	if (m_fTOT) {
		StringPrintf(
			pszText, MaxLength, TEXT("TOT %d/%d/%d %d:%02d:%02d"),
			Time.Year, Time.Month, Time.Day,
			Time.Hour, Time.Minute, Time.Second);
	} else {
		StringPrintf(
			pszText, MaxLength, TEXT("%d:%02d:%02d"),
			Time.Hour, Time.Minute, Time.Second);
	}
#else
	SYSTEMTIME st = Time.ToSYSTEMTIME();
	if (m_fTOT) {
		TCHAR szDate[32], szTime[32];
		if (::GetDateFormat(
					LOCALE_USER_DEFAULT, DATE_SHORTDATE,
					&st, nullptr, szDate, lengthof(szDate)) == 0)
			szDate[0] = _T('\0');
		if (::GetTimeFormat(
					LOCALE_USER_DEFAULT,
					TIME_FORCE24HOURFORMAT | TIME_NOTIMEMARKER,
					&st, nullptr, szTime, lengthof(szTime)) == 0)
			szTime[0] = _T('\0');
		StringPrintf(
			pszText, MaxLength,
			TEXT("TOT %s %s"), szDate, szTime);
	} else {
		::GetTimeFormat(LOCALE_USER_DEFAULT, 0, &st, nullptr, pszText, MaxLength);
	}
#endif
}


CProgramInfoStatusItem::CProgramInfoStatusItem()
	: CStatusItem(STATUS_ITEM_PROGRAMINFO, SizeValue(20 * EM_FACTOR, SizeUnit::EM))
	, m_fNext(false)
	, m_fShowProgress(true)
	, m_fEnablePopupInfo(true)
	, m_ProgressBackStyle(Theme::FillStyle(Theme::SolidStyle(Theme::ThemeColor(160, 160, 160))))
	, m_ProgressElapsedStyle(Theme::FillStyle(Theme::SolidStyle(Theme::ThemeColor(0, 0, 128))))
	, m_fValidProgress(false)
{
}

void CProgramInfoStatusItem::Draw(HDC hdc, const RECT &ItemRect, const RECT &DrawRect, unsigned int Flags)
{
	if ((Flags & DRAW_PREVIEW) != 0) {
		DrawText(hdc, DrawRect, TEXT("1:00～1:30 今日のニュース"));
		return;
	}

	if (m_fShowProgress && m_fValidProgress) {
		Theme::CThemeDraw ThemeDraw(BeginThemeDraw(hdc));
		RECT rcProgress = DrawRect;
		rcProgress.top = rcProgress.bottom - (DrawRect.bottom - DrawRect.top) / 3;
		ThemeDraw.Draw(m_ProgressBackStyle, &rcProgress);
		long long Elapsed = m_CurTime.DiffSeconds(m_EventInfo.StartTime);
		if (Elapsed > 0) {
			if (m_EventInfo.Duration > 0 && Elapsed < static_cast<long long>(m_EventInfo.Duration)) {
				rcProgress.right =
					rcProgress.left +
					::MulDiv(rcProgress.right - rcProgress.left, (int)Elapsed, m_EventInfo.Duration);
			}
			ThemeDraw.Draw(m_ProgressElapsedStyle, rcProgress);
		}
	}

	if (!m_Text.empty())
		DrawText(hdc, DrawRect, m_Text.c_str());
}

bool CProgramInfoStatusItem::UpdateContent()
{
	CCoreEngine &CoreEngine = GetAppClass().CoreEngine;

	CoreEngine.GetCurrentEventInfo(&m_EventInfo, m_fNext);

	TCHAR szText[256];
	CStaticStringFormatter Formatter(szText, lengthof(szText));

	if (m_fNext)
		Formatter.Append(TEXT("次: "));
	if (m_EventInfo.StartTime.IsValid()) {
		TCHAR szTime[EpgUtil::MAX_EVENT_TIME_LENGTH];
		if (EpgUtil::FormatEventTime(m_EventInfo, szTime, lengthof(szTime)) > 0) {
			Formatter.Append(szTime);
			Formatter.Append(TEXT(" "));
		}
	}
	if (!m_EventInfo.EventName.empty())
		Formatter.Append(m_EventInfo.EventName.c_str());

	bool fUpdated = false;

	if (m_Text.compare(Formatter.GetString()) != 0) {
		m_Text = Formatter.GetString();
		fUpdated = true;
	}

	if (UpdateProgress())
		fUpdated = true;

	return fUpdated;
}

void CProgramInfoStatusItem::OnLButtonDown(int x, int y)
{
	if (!m_fEnablePopupInfo) {
		if (!m_EventInfoPopup.IsVisible())
			ShowPopupInfo();
		else
			m_EventInfoPopup.Hide();
	} else {
		m_fNext = !m_fNext;
		m_EventInfoPopup.Hide();
		Update();
	}
}

void CProgramInfoStatusItem::OnRButtonDown(int x, int y)
{
	m_EventInfoPopup.Hide();

	POINT pt;
	UINT Flags;
	RECT rc;

	GetMenuPos(&pt, &Flags, &rc);
	GetAppClass().UICore.ShowSpecialMenu(
		CUICore::MenuType::ProgramInfo, &pt, Flags | TPM_RIGHTBUTTON, &rc);
}

void CProgramInfoStatusItem::OnLButtonDoubleClick(int x, int y)
{
	if (!m_fEnablePopupInfo) {
		m_fNext = !m_fNext;
		Update();
		m_EventInfoPopup.Hide();
		ShowPopupInfo();
	} else {
		OnLButtonDown(x, y);
	}
}

void CProgramInfoStatusItem::OnFocus(bool fFocus)
{
	if (!fFocus && m_EventInfoPopup.IsVisible()) {
		POINT pt;

		::GetCursorPos(&pt);
		if (!m_EventInfoPopup.IsOwnWindow(::WindowFromPoint(pt)))
			m_EventInfoPopup.Hide();
	}
}

bool CProgramInfoStatusItem::OnMouseHover(int x, int y)
{
	if (!m_fEnablePopupInfo
			|| ::GetActiveWindow() != ::GetForegroundWindow())
		return true;

	ShowPopupInfo();
	return true;
}

void CProgramInfoStatusItem::SetTheme(const Theme::CThemeManager *pThemeManager)
{
	m_EpgTheme.SetTheme(pThemeManager);

	pThemeManager->GetBackgroundStyle(
		Theme::CThemeManager::STYLE_STATUSBAR_EVENT_PROGRESS,
		&m_ProgressBackStyle);
	pThemeManager->GetBackgroundStyle(
		Theme::CThemeManager::STYLE_STATUSBAR_EVENT_PROGRESS_ELAPSED,
		&m_ProgressElapsedStyle);
}

void CProgramInfoStatusItem::SetShowProgress(bool fShow)
{
	if (m_fShowProgress != fShow) {
		m_fShowProgress = fShow;
		UpdateProgress();
		Redraw();
	}
}

void CProgramInfoStatusItem::EnablePopupInfo(bool fEnable)
{
	m_fEnablePopupInfo = fEnable;
	if (!fEnable)
		m_EventInfoPopup.Hide();
}

bool CProgramInfoStatusItem::UpdateProgress()
{
	if (m_fShowProgress) {
		const LibISDB::AnalyzerFilter *pAnalyzer =
			GetAppClass().CoreEngine.GetFilter<LibISDB::AnalyzerFilter>();
		LibISDB::DateTime CurTime;
		bool fValid =
			m_EventInfo.StartTime.IsValid() &&
			pAnalyzer != nullptr &&
			pAnalyzer->GetTOTTime(&CurTime);
		bool fUpdated;

		if (fValid) {
			fUpdated = !m_fValidProgress || m_CurTime != CurTime;
			m_CurTime = CurTime;
		} else {
			fUpdated = m_fValidProgress;
		}
		m_fValidProgress = fValid;

		return fUpdated;
	}

	return false;
}

void CProgramInfoStatusItem::ShowPopupInfo()
{
	if (m_EventInfoPopup.IsVisible())
		return;

	CAppMain &App = GetAppClass();
	CChannelInfo ChInfo;

	if (App.Core.GetCurrentStreamChannelInfo(&ChInfo)
			&& ChInfo.GetServiceID() != 0) {
		LibISDB::EventInfo EventInfo;

		if (App.CoreEngine.GetCurrentEventInfo(
					&EventInfo, ChInfo.GetServiceID(), m_fNext)) {
			RECT rc;
			POINT pt;
			int Width, Height;

			GetRect(&rc);
			pt.x = rc.left;
			pt.y = rc.top;
			::ClientToScreen(m_pStatus->GetHandle(), &pt);
			m_EventInfoPopup.GetSize(&Width, &Height);
			::SetRect(&rc, pt.x, pt.y - Height, pt.x + Width, pt.y);

			m_EventInfoPopup.SetTitleColor(
				m_EpgTheme.GetGenreColor(EventInfo),
				m_EpgTheme.GetColor(CEpgTheme::COLOR_EVENTNAME));

			int IconWidth, IconHeight;
			m_EventInfoPopup.GetPreferredIconSize(&IconWidth, &IconHeight);
			HICON hIcon = App.LogoManager.CreateLogoIcon(
				ChInfo.GetNetworkID(), ChInfo.GetServiceID(),
				IconWidth, IconHeight);

			if (!m_EventInfoPopup.Show(&EventInfo, &rc, hIcon, ChInfo.GetName())) {
				if (hIcon != nullptr)
					::DestroyIcon(hIcon);
			}
		}
	}
}

void CProgramInfoStatusItem::SetPopupInfoSize(int Width, int Height)
{
	if (Width > 0 && Height > 0)
		m_EventInfoPopup.SetSize(Width, Height);
}

void CProgramInfoStatusItem::GetPopupInfoSize(int *pWidth, int *pHeight) const
{
	m_EventInfoPopup.GetSize(pWidth, pHeight);
}


CBufferingStatusItem::CBufferingStatusItem()
	: CStatusItem(STATUS_ITEM_BUFFERING, SizeValue(7 * EM_FACTOR, SizeUnit::EM))
	, m_StreamRemain(0)
	, m_PacketBufferUsedPercentage(0)
{
}

bool CBufferingStatusItem::UpdateContent()
{
	CCoreEngine &CoreEngine = GetAppClass().CoreEngine;
	const DWORD StreamRemain = CoreEngine.GetStreamRemain();
	const int PacketBufferUsedPercentage = CoreEngine.GetPacketBufferUsedPercentage();

	if (StreamRemain == m_StreamRemain
			&& PacketBufferUsedPercentage == m_PacketBufferUsedPercentage)
		return false;

	m_StreamRemain = StreamRemain;
	m_PacketBufferUsedPercentage = PacketBufferUsedPercentage;

	return true;
}

void CBufferingStatusItem::Draw(HDC hdc, const RECT &ItemRect, const RECT &DrawRect, unsigned int Flags)
{
	if ((Flags & DRAW_PREVIEW) != 0) {
		DrawText(hdc, DrawRect, TEXT("R 2 / B 48%"));
		return;
	}

	TCHAR szText[32];

	StringPrintf(
		szText, TEXT("R %u / B %d%%"),
		static_cast<unsigned int>(m_StreamRemain),
		m_PacketBufferUsedPercentage);
	DrawText(hdc, DrawRect, szText);
}

void CBufferingStatusItem::OnLButtonDown(int x, int y)
{
	POINT pt;
	UINT Flags;
	RECT rc;

	GetMenuPos(&pt, &Flags, &rc);
	GetAppClass().UICore.ShowSpecialMenu(
		CUICore::MenuType::Buffering, &pt, Flags | TPM_RIGHTBUTTON, &rc);
}


CTunerStatusItem::CTunerStatusItem()
	: CStatusItem(STATUS_ITEM_TUNER, SizeValue(7 * EM_FACTOR, SizeUnit::EM))
{
}

void CTunerStatusItem::Draw(HDC hdc, const RECT &ItemRect, const RECT &DrawRect, unsigned int Flags)
{
	if ((Flags & DRAW_PREVIEW) != 0) {
		DrawText(hdc, DrawRect, TEXT("地デジ"));
		return;
	}

	const CChannelManager &ChannelManager = GetAppClass().ChannelManager;
	const CChannelInfo *pChInfo = ChannelManager.GetCurrentChannelInfo();
	LPCTSTR pszText;

	if (pChInfo != nullptr || ChannelManager.GetCurrentSpace() >= 0) {
		pszText =
			ChannelManager.GetTuningSpaceName(
				pChInfo != nullptr ? pChInfo->GetSpace() : ChannelManager.GetCurrentSpace());
		if (pszText == nullptr)
			pszText = TEXT("<チューナー>");
	} else if (ChannelManager.GetCurrentSpace() == CChannelManager::SPACE_ALL) {
		pszText = TEXT("すべて");
	} else {
		pszText = TEXT("<チューナー>");
	}
	DrawText(hdc, DrawRect, pszText);
}

void CTunerStatusItem::OnLButtonDown(int x, int y)
{
	POINT pt;
	UINT Flags;
	RECT rc;

	GetMenuPos(&pt, &Flags, &rc);
	GetAppClass().UICore.PopupSubMenu(
		CMainMenu::SUBMENU_SPACE, &pt, Flags | TPM_RIGHTBUTTON, &rc);
}

void CTunerStatusItem::OnRButtonDown(int x, int y)
{
	POINT pt;
	UINT Flags;
	RECT rc;

	GetMenuPos(&pt, &Flags, &rc);
	GetAppClass().UICore.ShowSpecialMenu(
		CUICore::MenuType::TunerSelect, &pt, Flags | TPM_RIGHTBUTTON, &rc);
}


CMediaBitRateStatusItem::CMediaBitRateStatusItem()
	: CStatusItem(STATUS_ITEM_MEDIABITRATE, SizeValue(13 * EM_FACTOR, SizeUnit::EM))
	, m_VideoBitRate(0)
	, m_AudioBitRate(0)
{
}

bool CMediaBitRateStatusItem::UpdateContent()
{
	const LibISDB::TSPacketCounterFilter *pPacketCounter =
		GetAppClass().CoreEngine.GetFilter<LibISDB::TSPacketCounterFilter>();
	DWORD VideoBitRate, AudioBitRate;

	if (pPacketCounter != nullptr) {
		VideoBitRate = pPacketCounter->GetVideoBitRate();
		AudioBitRate = pPacketCounter->GetAudioBitRate();
	} else {
		VideoBitRate = 0;
		AudioBitRate = 0;
	}

	if (VideoBitRate == m_VideoBitRate && AudioBitRate == m_AudioBitRate)
		return false;

	m_VideoBitRate = VideoBitRate;
	m_AudioBitRate = AudioBitRate;

	return true;
}

void CMediaBitRateStatusItem::Draw(HDC hdc, const RECT &ItemRect, const RECT &DrawRect, unsigned int Flags)
{
	if ((Flags & DRAW_PREVIEW) != 0) {
		DrawText(hdc, DrawRect, TEXT("V 13.25 Mbps / A 185 kbps"));
		return;
	}

	TCHAR szText[64];
	int Length;

	if (m_VideoBitRate < 1000 * 1000) {
		Length = StringPrintf(
			szText,
			TEXT("V %u kbps"),
			(m_VideoBitRate + 500) / 1000);
	} else {
		Length = StringPrintf(
			szText,
			TEXT("V %.2f Mbps"),
			(double)(m_VideoBitRate) / (double)(1000 * 1000));
	}
	StringPrintf(
		szText + Length, lengthof(szText) - Length,
		TEXT(" / A %u kbps"),
		(m_AudioBitRate + 500) / 1000);

	DrawText(hdc, DrawRect, szText);
}


CFavoritesStatusItem::CFavoritesStatusItem()
	: CIconStatusItem(STATUS_ITEM_FAVORITES, 16)
{
}

void CFavoritesStatusItem::Draw(HDC hdc, const RECT &ItemRect, const RECT &DrawRect, unsigned int Flags)
{
	if (!m_Icons.IsCreated()) {
		static const Theme::IconList::ResourceInfo ResourceList[] = {
			{MAKEINTRESOURCE(IDB_STATUSBAR_FAVORITES16), 16, 16},
			{MAKEINTRESOURCE(IDB_STATUSBAR_FAVORITES32), 32, 32},
		};
		Style::Size IconSize = m_pStatus->GetIconSize();
		m_Icons.Load(
			GetAppClass().GetResourceInstance(),
			IconSize.Width, IconSize.Height,
			ResourceList, lengthof(ResourceList));
	}
	DrawIcon(hdc, DrawRect, m_Icons);
}

void CFavoritesStatusItem::OnLButtonDown(int x, int y)
{
	POINT pt;
	UINT Flags;
	RECT rc;

	GetMenuPos(&pt, &Flags, &rc);
	GetAppClass().UICore.PopupSubMenu(
		CMainMenu::SUBMENU_FAVORITES, &pt, Flags | TPM_RIGHTBUTTON, &rc);
}

void CFavoritesStatusItem::OnRButtonDown(int x, int y)
{
	GetAppClass().UICore.DoCommand(CM_ADDTOFAVORITES);
}


}	// namespace TVTest
