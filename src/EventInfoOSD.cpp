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


#include "stdafx.h"
#include "TVTest.h"
#include "EventInfoOSD.h"
#include "AppMain.h"
#include "EpgUtil.h"
#include "Graphics.h"
#include "Common/DebugDef.h"


namespace TVTest
{

namespace
{


Graphics::CColor GraphicsColorFromThemeColor(const Theme::ThemeColor &Color)
{
	return Graphics::CColor(Color.Red, Color.Green, Color.Blue, Color.Alpha);
}


}




bool CEventInfoOSD::Show(HWND hwndParent, DWORD Time)
{
	if (!m_OSD.Create(hwndParent, true))
		return false;

	int Width, Height;
	GetPosition(nullptr, nullptr, &Width, &Height);

	if (!CreateBitmap(Width, Height)) {
		m_OSD.Destroy();
		return false;
	}

	m_OSD.SetImage(
		m_Bitmap.GetHandle(),
		CPseudoOSD::ImageEffect::None,
		CPseudoOSD::ImageFlag::DirectSource);

	return m_OSD.Show(Time);
}


bool CEventInfoOSD::Hide()
{
	m_OSD.Destroy();
	m_OSD.SetImage(nullptr);
	m_Bitmap.Destroy();
	return true;
}


bool CEventInfoOSD::IsVisible() const
{
	return m_OSD.IsVisible();
}


bool CEventInfoOSD::IsCreated() const
{
	return m_OSD.IsCreated();
}


bool CEventInfoOSD::Update()
{
	if (m_OSD.IsCreated()) {
		int Width, Height;
		m_OSD.GetPosition(nullptr, nullptr, &Width, &Height);
		if (!CreateBitmap(Width, Height)) {
			m_OSD.SetImage(nullptr);
			return false;
		}
		m_OSD.SetImage(
			m_Bitmap.GetHandle(),
			CPseudoOSD::ImageEffect::None,
			CPseudoOSD::ImageFlag::DirectSource);
		m_OSD.Update();
	}

	return true;
}


bool CEventInfoOSD::SetEventInfo(const LibISDB::EventInfo *pEventInfo)
{
	if (pEventInfo == nullptr)
		return false;

	m_EventInfo = *pEventInfo;

	return true;
}


bool CEventInfoOSD::SetPosition(int Left, int Top, int Width, int Height)
{
	if (m_OSD.IsCreated()) {
		int OldWidth, OldHeight;
		m_OSD.GetPosition(nullptr, nullptr, &OldWidth, &OldHeight);
		if (Width != OldWidth || Height != OldHeight) {
			if (CreateBitmap(Width, Height)) {
				m_OSD.SetImage(
					m_Bitmap.GetHandle(),
					CPseudoOSD::ImageEffect::None,
					CPseudoOSD::ImageFlag::DirectSource);
			} else {
				m_OSD.SetImage(nullptr);
			}
		}
	}

	return m_OSD.SetPosition(Left, Top, Width, Height);
}


void CEventInfoOSD::GetPosition(int *pLeft, int *pTop, int *pWidth, int *pHeight) const
{
	m_OSD.GetPosition(pLeft, pTop, pWidth, pHeight);
}


bool CEventInfoOSD::AdjustPosition(RECT *pRect) const
{
	const int Width = pRect->right - pRect->left;
	const int Height = pRect->bottom - pRect->top;

	pRect->left += ::MulDiv(Width, m_Style.Margin.Left, 100);
	pRect->top += ::MulDiv(Height, m_Style.Margin.Top, 100);
	pRect->right -= ::MulDiv(Width, m_Style.Margin.Right, 100);
	pRect->bottom -= ::MulDiv(Height, m_Style.Margin.Bottom, 100);

	return pRect->left < pRect->right && pRect->top < pRect->bottom;
}


void CEventInfoOSD::SetColorScheme(const ColorScheme &Colors)
{
	m_ColorScheme = Colors;
}


void CEventInfoOSD::SetFont(const LOGFONT &Font)
{
	m_Font = Font;
}


void CEventInfoOSD::SetTitleFont(const LOGFONT &Font)
{
	m_TitleFont = Font;
}


void CEventInfoOSD::OnParentMove()
{
	m_OSD.OnParentMove();
}


void CEventInfoOSD::SetStyle(const Style::CStyleManager *pStyleManager)
{
	m_Style = {};

	pStyleManager->Get(TEXT("event-osd.margin"), &m_Style.Margin);
	pStyleManager->Get(TEXT("event-osd.padding"), &m_Style.Padding);
	Style::IntValue Value;
	if (pStyleManager->Get(TEXT("event-osd.text-size-ratio"), &Value) && Value.Value > 0)
		m_Style.TextSizeRatio = Value;
	pStyleManager->Get(TEXT("event-osd.text-size-min"), &m_Style.TextSizeMin);
	pStyleManager->Get(TEXT("event-osd.text-size-max"), &m_Style.TextSizeMax);
	pStyleManager->Get(TEXT("event-osd.text-outline"), &m_Style.TextOutline);
	pStyleManager->Get(TEXT("event-osd.use-hinting"), &m_Style.fUseHinting);
	pStyleManager->Get(TEXT("event-osd.use-path"), &m_Style.fUsePath);
	pStyleManager->Get(TEXT("event-osd.logo.show"), &m_Style.fShowLogo);
}


void CEventInfoOSD::NormalizeStyle(
	const Style::CStyleManager *pStyleManager,
	const Style::CStyleScaling *pStyleScaling)
{
	pStyleScaling->ToPixels(&m_Style.Padding);
	pStyleScaling->ToPixels(&m_Style.TextSizeMin);
	pStyleScaling->ToPixels(&m_Style.TextSizeMax);
}


bool CEventInfoOSD::CreateBitmap(int Width, int Height)
{
	Graphics::CImage Image;
	if (!Image.Create(Width, Height, 32))
		return false;

	{
		Graphics::CCanvas Canvas(&Image);

		Draw(Canvas, {0, 0, Width, Height});
	}

	HBITMAP hbm = Image.CreateBitmap();
	if (hbm == nullptr)
		return false;

	m_Bitmap.Attach(hbm);

	return true;
}


void CEventInfoOSD::Draw(Graphics::CCanvas &Canvas, const RECT &Rect) const
{
	Canvas.Clear(m_ColorScheme.Back.Red, m_ColorScheme.Back.Green, m_ColorScheme.Back.Blue, m_ColorScheme.Back.Alpha);

	RECT ContentRect = Rect;
	Style::Subtract(&ContentRect, m_Style.Padding);
	if (ContentRect.left >= ContentRect.right || ContentRect.top >= ContentRect.bottom)
		return;

	String Text;

	{
		TCHAR EventTimeText[EpgUtil::MAX_EVENT_TIME_LENGTH];
		if (EpgUtil::FormatEventTime(
					m_EventInfo, EventTimeText, lengthof(EventTimeText),
					EpgUtil::FormatEventTimeFlag::UndecidedText) > 0) {
			Text += EventTimeText;
			Text += _T(' ');
		}
	}
	Text += m_EventInfo.EventName;

	const int FontSize = std::clamp<int>(
		(ContentRect.right - ContentRect.left) / m_Style.TextSizeRatio,
		m_Style.TextSizeMin, m_Style.TextSizeMax);
	LOGFONT lf = m_TitleFont;
	lf.lfHeight = -FontSize;
	lf.lfWidth = 0;
	Graphics::CFont Font(lf);

	const float OutlineWidth = static_cast<float>(m_Style.TextOutline.Value * FontSize) / 100.0f;

	Graphics::TextFlag TextFlags = Graphics::TextFlag::Draw_Antialias;
	if (m_Style.fUseHinting)
		TextFlags |= Graphics::TextFlag::Draw_Hinting;
	if (m_Style.fUsePath)
		TextFlags |= Graphics::TextFlag::Draw_Path;
	const Graphics::TextFlag DrawTextFlags = TextFlags | Graphics::TextFlag::Format_EndEllipsis | Graphics::TextFlag::Format_ClipLastLine;

	RECT TitleRect = ContentRect;

	if (m_Style.fShowLogo) {
		const int LogoHeight = FontSize;
		const int LogoWidth = ::MulDiv(LogoHeight, 16, 9);
		const Graphics::CImage *pImage = GetAppClass().LogoManager.GetAssociatedLogoImage(
			m_EventInfo.NetworkID, m_EventInfo.ServiceID, CLogoManager::LOGOTYPE_BIG);
		if (pImage != nullptr) {
			Canvas.DrawImage(
				TitleRect.left,
				TitleRect.top + std::max((static_cast<int>(Canvas.GetLineSpacing(Font)) - LogoHeight) / 2, 0),
				LogoWidth, LogoHeight,
				pImage, 0, 0, pImage->GetWidth(), pImage->GetHeight());
			TitleRect.left += LogoWidth + FontSize / 4;
		}
	}

	SIZE TitleSize = {TitleRect.right - TitleRect.left, TitleRect.bottom - TitleRect.top};
	if (OutlineWidth > 0.0f)
		Canvas.GetOutlineTextSize(Text.c_str(), Font, OutlineWidth, TextFlags, &TitleSize);
	else
		Canvas.GetTextSize(Text.c_str(), Font, TextFlags, &TitleSize);

	Graphics::CBrush Brush(GraphicsColorFromThemeColor(m_ColorScheme.Title));
	// ぴったりのサイズで指定すると最後の行が表示されないことがある
	//TitleRect.bottom = TitleRect.top + TitleSize.cy;
	if (OutlineWidth > 0.0f) {
		Canvas.DrawOutlineText(
			Text.c_str(), Font, TitleRect, &Brush,
			GraphicsColorFromThemeColor(m_ColorScheme.TitleOutline), OutlineWidth,
			DrawTextFlags);
	} else {
		Canvas.DrawText(Text.c_str(), Font, TitleRect, &Brush, DrawTextFlags);
	}
	TitleRect.bottom = TitleRect.top + TitleSize.cy;

	if (ContentRect.bottom > TitleRect.bottom) {
		Text = m_EventInfo.EventText;
		StringUtility::Trim(Text, TEXT(" \t\r\n"));

		if (!m_EventInfo.ExtendedText.empty()) {
			LibISDB::EventInfo::ExtendedTextInfoList TextList = m_EventInfo.ExtendedText;
			String ExtendedText;

			// 「番組内容」などを優先して表示する
			for (auto it = TextList.begin(); it != TextList.end();) {
				if (it->Description.find(TEXT("内容")) != String::npos
						|| it->Description.find(TEXT("概要")) != String::npos) {
					StringUtility::Trim(it->Text, TEXT(" \t\r\n"));
					if (!it->Text.empty()) {
						ExtendedText += it->Text;
						ExtendedText += TEXT("\r\n");
					}
					it = TextList.erase(it);
				} else {
					++it;
				}
			}

			for (auto it = TextList.begin(); it != TextList.end(); ++it) {
				StringUtility::TrimEnd(it->Text, TEXT(" \t\r\n"));
				if (!it->Text.empty()) {
					if (!it->Description.empty()) {
						ExtendedText += it->Description;
						ExtendedText += TEXT("\r\n");
					}
					ExtendedText += it->Text;
					ExtendedText += TEXT("\r\n");
				}
			}

			StringUtility::TrimEnd(ExtendedText, TEXT("\r\n"));

			if (!ExtendedText.empty()) {
				if (!Text.empty())
					Text += TEXT("\r\n\r\n");
				Text += ExtendedText;
			}
		}

		if (!Text.empty()) {
			lf = m_Font;
			lf.lfHeight = -FontSize;
			lf.lfWidth = 0;
			if (!CompareLogFont(&lf, &m_Font))
				Font.Create(lf);

			Brush.CreateSolidBrush(GraphicsColorFromThemeColor(m_ColorScheme.Text));
			const RECT TextRect = {ContentRect.left, TitleRect.bottom, ContentRect.right, ContentRect.bottom};
			if (OutlineWidth > 0.0f) {
				Canvas.DrawOutlineText(
					Text.c_str(), Font, TextRect, &Brush,
					GraphicsColorFromThemeColor(m_ColorScheme.TextOutline), OutlineWidth,
					DrawTextFlags);
			} else {
				Canvas.DrawText(Text.c_str(), Font, TextRect, &Brush, DrawTextFlags);
			}
		}
	}
}


} // namespace TVTest
