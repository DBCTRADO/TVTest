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
#include <cctype>
#include <cwctype>
#include "TVTest.h"
#include "Style.h"
#include "Settings.h"
#include "DPIUtil.h"
#include "Common/DebugDef.h"


namespace TVTest
{

namespace Style
{


CStyleManager::CStyleManager()
	: m_DPI(TVTest::GetSystemDPI())
{
}


bool CStyleManager::Load(LPCTSTR pszFileName)
{
	if (IsStringEmpty(pszFileName))
		return false;

	TRACE(TEXT("CStyleManager::Load() : \"{}\"\n"), pszFileName);

	CSettings Settings;

	if (!Settings.Open(pszFileName, CSettings::OpenFlag::Read))
		return false;

	if (Settings.SetSection(TEXT("Settings"))) {
		int Value;

		if (Settings.Read(TEXT("DPI"), &Value) && Value > 0) {
			m_ForcedDPI = Value;
			m_DPI = m_ForcedDPI;
		}

		Settings.Read(TEXT("HandleDPIChanged"), &m_fHandleDPIChanged);
		Settings.Read(TEXT("ScaleFont"), &m_fScaleFont);
		Settings.Read(TEXT("UseDarkMenu"), &m_fUseDarkMenu);
		Settings.Read(TEXT("DarkDialog"), &m_fDarkDialog);
	}

	if (Settings.SetSection(TEXT("Styles"))) {
		CSettings::EntryList Entries;
		if (!Settings.GetEntries(&Entries))
			return false;

		for (auto &e : Entries) {
			if (!e.Name.empty() && !e.Value.empty()) {
				StringUtility::Trim(e.Value);
				if (!e.Value.empty()) {
					if (std::_istdigit(e.Value[0])
							|| e.Value[0] == _T('-') || e.Value[0] == _T('+')) {
						IntValue Value;
						if (ParseValue(e.Value.c_str(), &Value))
							Set(e.Name.c_str(), Value);
					} else if (StringUtility::IsEqualNoCase(e.Value, TEXT("true"))
							|| StringUtility::IsEqualNoCase(e.Value, TEXT("false"))) {
						Set(e.Name.c_str(), e.Value[0] == _T('t'));
					} else if (e.Value.length() >= 2
							&& e.Value.front() == _T('\"')
							&& e.Value.back() == _T('\"')) {
						Set(e.Name.c_str(), e.Value.substr(1, e.Value.length() - 2));
					}
#ifdef _DEBUG
					else {
						TRACE(TEXT("Invalid style : {}={}\n"), e.Name, e.Value);
					}
#endif
				}
			}
		}
	}

	return true;
}


bool CStyleManager::Set(const StyleInfo &Info)
{
	if (Info.Name.empty())
		return false;

	auto itr = m_StyleMap.find(Info.Name);
	if (itr == m_StyleMap.end()) {
		m_StyleMap.emplace(Info.Name, Info);
	} else {
		itr->second = Info;
	}

	return true;
}


bool CStyleManager::Get(LPCTSTR pszName, StyleInfo *pInfo) const
{
	if (IsStringEmpty(pszName) || pInfo == nullptr)
		return false;

	auto itr = m_StyleMap.find(String(pszName));
	if (itr == m_StyleMap.end())
		return false;

	*pInfo = itr->second;

	return true;
}


bool CStyleManager::Set(LPCTSTR pszName, const IntValue &Value)
{
	if (IsStringEmpty(pszName))
		return false;

	StyleInfo Style;
	Style.Name = pszName;
	auto itr = m_StyleMap.find(Style.Name);
	if (itr == m_StyleMap.end()) {
		Style.Type = ValueType::Int;
		Style.Value.Int = Value.Value;
		Style.Unit = Value.Unit;
		m_StyleMap.emplace(Style.Name, Style);
	} else {
		itr->second.Type = ValueType::Int;
		itr->second.Value.Int = Value.Value;
		itr->second.Unit = Value.Unit;
	}

	return true;
}


bool CStyleManager::Get(LPCTSTR pszName, IntValue *pValue) const
{
	if (IsStringEmpty(pszName) || pValue == nullptr)
		return false;

	auto itr = m_StyleMap.find(String(pszName));
	if (itr == m_StyleMap.end() || itr->second.Type != ValueType::Int)
		return false;

	pValue->Value = itr->second.Value.Int;
	pValue->Unit = itr->second.Unit;

	return true;
}


bool CStyleManager::Set(LPCTSTR pszName, bool fValue)
{
	if (IsStringEmpty(pszName))
		return false;

	StyleInfo Style;
	Style.Name = pszName;
	auto itr = m_StyleMap.find(Style.Name);
	if (itr == m_StyleMap.end()) {
		Style.Type = ValueType::Bool;
		Style.Value.Bool = fValue;
		m_StyleMap.emplace(Style.Name, Style);
	} else {
		itr->second.Type = ValueType::Bool;
		itr->second.Value.Bool = fValue;
	}

	return true;
}


bool CStyleManager::Get(LPCTSTR pszName, bool *pfValue) const
{
	if (IsStringEmpty(pszName) || pfValue == nullptr)
		return false;

	auto itr = m_StyleMap.find(String(pszName));
	if (itr == m_StyleMap.end() || itr->second.Type != ValueType::Bool)
		return false;

	*pfValue = itr->second.Value.Bool;

	return true;
}


bool CStyleManager::Set(LPCTSTR pszName, const String &Value)
{
	if (IsStringEmpty(pszName))
		return false;

	StyleInfo Style;
	Style.Name = pszName;
	auto itr = m_StyleMap.find(Style.Name);
	if (itr == m_StyleMap.end()) {
		Style.Type = ValueType::String;
		Style.Value.String = Value;
		m_StyleMap.emplace(Style.Name, Style);
	} else {
		itr->second.Type = ValueType::String;
		itr->second.Value.String = Value;
	}

	return true;
}


bool CStyleManager::Get(LPCTSTR pszName, String *pValue) const
{
	if (IsStringEmpty(pszName) || pValue == nullptr)
		return false;

	auto itr = m_StyleMap.find(String(pszName));
	if (itr == m_StyleMap.end() || itr->second.Type != ValueType::String)
		return false;

	*pValue = itr->second.Value.String;

	return true;
}


bool CStyleManager::Set(LPCTSTR pszName, const Size &Value)
{
	if (IsStringEmpty(pszName))
		return false;

	String Name;

	Name = pszName;
	Name += TEXT(".width");
	Set(Name.c_str(), Value.Width);
	Name = pszName;
	Name += TEXT(".height");
	Set(Name.c_str(), Value.Height);

	return true;
}


bool CStyleManager::Get(LPCTSTR pszName, Size *pValue) const
{
	if (IsStringEmpty(pszName) || pValue == nullptr)
		return false;

	String Name;

	Name = pszName;
	Name += TEXT(".width");
	const bool fWidth = Get(Name.c_str(), &pValue->Width);
	Name = pszName;
	Name += TEXT(".height");
	const bool fHeight = Get(Name.c_str(), &pValue->Height);

	return fWidth || fHeight;
}


bool CStyleManager::Set(LPCTSTR pszName, const Margins &Value)
{
	if (IsStringEmpty(pszName))
		return false;

	String Name;

	Name = pszName;
	Name += TEXT(".left");
	Set(Name.c_str(), Value.Left);
	Name = pszName;
	Name += TEXT(".top");
	Set(Name.c_str(), Value.Top);
	Name = pszName;
	Name += TEXT(".right");
	Set(Name.c_str(), Value.Right);
	Name = pszName;
	Name += TEXT(".bottom");
	Set(Name.c_str(), Value.Bottom);

	return true;
}


bool CStyleManager::Get(LPCTSTR pszName, Margins *pValue) const
{
	if (IsStringEmpty(pszName) || pValue == nullptr)
		return false;

	String Name;
	bool fOK = false;

	IntValue Margin;
	if (Get(pszName, &Margin)) {
		pValue->Left = Margin;
		pValue->Top = Margin;
		pValue->Right = Margin;
		pValue->Bottom = Margin;
		fOK = true;
	}

	Name = pszName;
	Name += TEXT(".left");
	if (Get(Name.c_str(), &pValue->Left))
		fOK = true;
	Name = pszName;
	Name += TEXT(".top");
	if (Get(Name.c_str(), &pValue->Top))
		fOK = true;
	Name = pszName;
	Name += TEXT(".right");
	if (Get(Name.c_str(), &pValue->Right))
		fOK = true;
	Name = pszName;
	Name += TEXT(".bottom");
	if (Get(Name.c_str(), &pValue->Bottom))
		fOK = true;

	return fOK;
}


bool CStyleManager::InitStyleScaling(CStyleScaling *pScaling, HMONITOR hMonitor) const
{
	if (pScaling == nullptr)
		return false;

	const int SystemDPI = GetSystemDPI();
	int DPI;

	if (m_ForcedDPI > 0) {
		DPI = m_ForcedDPI;
	} else {
		DPI = SystemDPI;

		if (hMonitor != nullptr) {
			const int MonitorDPI = GetMonitorDPI(hMonitor);
			if (MonitorDPI != 0) {
				DPI = MonitorDPI;
			}
		}
	}

	pScaling->SetDPI(DPI);
	pScaling->SetSystemDPI(SystemDPI);
	pScaling->SetScaleFont(m_fScaleFont);

	return true;
}


bool CStyleManager::InitStyleScaling(CStyleScaling *pScaling, HWND hwnd) const
{
	return InitStyleScaling(pScaling, ::MonitorFromWindow(::GetAncestor(hwnd, GA_ROOT), MONITOR_DEFAULTTONULL));
}


bool CStyleManager::InitStyleScaling(CStyleScaling *pScaling, const RECT &Position) const
{
	return InitStyleScaling(pScaling, ::MonitorFromRect(&Position, MONITOR_DEFAULTTONULL));
}


bool CStyleManager::InitStyleScaling(CStyleScaling *pScaling) const
{
	return InitStyleScaling(pScaling, static_cast<HMONITOR>(nullptr));
}


int CStyleManager::GetSystemDPI() const
{
	return TVTest::GetSystemDPI();
}


int CStyleManager::GetForcedDPI() const
{
	return m_ForcedDPI;
}


bool CStyleManager::IsHandleDPIChanged() const
{
	return m_fHandleDPIChanged;
}


bool CStyleManager::IsUseDarkMenu() const
{
	return m_fUseDarkMenu;
}


bool CStyleManager::IsDarkDialog() const
{
	return m_fDarkDialog;
}


bool CStyleManager::AssignFontSizeFromLogFont(Font *pFont)
{
	if (pFont == nullptr)
		return false;

	pFont->Size.Value = ::MulDiv(std::abs(pFont->LogFont.lfHeight), 72, TVTest::GetSystemDPI());
	if (pFont->Size.Value != 0)
		pFont->Size.Unit = Style::UnitType::Point;
	else
		pFont->Size.Unit = Style::UnitType::Undefined;

	return true;
}


bool CStyleManager::ParseValue(LPCTSTR pszText, IntValue *pValue)
{
	if (IsStringEmpty(pszText) || pValue == nullptr)
		return false;

	if (!std::_istdigit(pszText[0]) && pszText[0] != _T('-') && pszText[0] != _T('+'))
		return false;

	LPTSTR pEnd;
	const int Value = static_cast<int>(std::_tcstol(pszText, &pEnd, 10));
	const UnitType Unit = ParseUnit(pEnd);
	if (Unit == UnitType::Undefined)
		return false;

	*pValue = IntValue(Value, Unit);

	return true;
}


UnitType CStyleManager::ParseUnit(LPCTSTR pszUnit)
{
	if (IsStringEmpty(pszUnit))
		return UnitType::LogicalPixel;
	if (::lstrcmpi(pszUnit, TEXT("px")) == 0)
		return UnitType::PhysicalPixel;
	if (::lstrcmpi(pszUnit, TEXT("pt")) == 0)
		return UnitType::Point;
	if (::lstrcmpi(pszUnit, TEXT("dp")) == 0)
		return UnitType::DIP;

	return UnitType::Undefined;
}




bool CStyleScaling::SetDPI(int DPI)
{
	if (DPI < 1)
		return false;

	m_DPI = DPI;

	return true;
}


int CStyleScaling::GetDPI() const
{
	return m_DPI;
}


bool CStyleScaling::SetSystemDPI(int DPI)
{
	if (DPI < 1)
		return false;

	m_SystemDPI = DPI;

	return true;
}


int CStyleScaling::GetSystemDPI() const
{
	return m_SystemDPI;
}


void CStyleScaling::SetScaleFont(bool fScale)
{
	m_fScaleFont = fScale;
}


bool CStyleScaling::ToPixels(IntValue *pValue) const
{
	if (pValue == nullptr)
		return false;

	switch (pValue->Unit) {
	case UnitType::LogicalPixel:
		pValue->Value = LogicalPixelsToPhysicalPixels(pValue->Value);
		break;

	case UnitType::PhysicalPixel:
		return true;

	case UnitType::Point:
		pValue->Value = PointsToPixels(pValue->Value);
		break;

	case UnitType::DIP:
		pValue->Value = DipToPixels(pValue->Value);
		break;

	default:
		return false;
	}

	pValue->Unit = UnitType::PhysicalPixel;

	return true;
}


bool CStyleScaling::ToPixels(Size *pValue) const
{
	if (pValue == nullptr)
		return false;

	ToPixels(&pValue->Width);
	ToPixels(&pValue->Height);

	return true;
}


bool CStyleScaling::ToPixels(Margins *pValue) const
{
	if (pValue == nullptr)
		return false;

	ToPixels(&pValue->Left);
	ToPixels(&pValue->Top);
	ToPixels(&pValue->Right);
	ToPixels(&pValue->Bottom);

	return true;
}


int CStyleScaling::ToPixels(int Value, UnitType Unit) const
{
	switch (Unit) {
	case UnitType::LogicalPixel:
		return LogicalPixelsToPhysicalPixels(Value);

	case UnitType::PhysicalPixel:
		return Value;

	case UnitType::Point:
		return PointsToPixels(Value);

	case UnitType::DIP:
		return DipToPixels(Value);
	}

	return 0;
}


int CStyleScaling::LogicalPixelsToPhysicalPixels(int Pixels) const
{
	return ::MulDiv(Pixels, m_DPI, 96);
}


int CStyleScaling::PointsToPixels(int Points) const
{
	// 1pt = 1/72in
	return ::MulDiv(Points, m_DPI, 72);
}


int CStyleScaling::DipToPixels(int Dip) const
{
	// 1dp = 1/160in
	return ::MulDiv(Dip, m_DPI, 160);
}


int CStyleScaling::ConvertUnit(int Value, UnitType SrcUnit, UnitType DstUnit) const
{
	switch (DstUnit) {
	case UnitType::LogicalPixel:
		return ::MulDiv(ToPixels(Value, SrcUnit), 96, m_DPI);

	case UnitType::PhysicalPixel:
		return ToPixels(Value, SrcUnit);

	case UnitType::Point:
		return ::MulDiv(ToPixels(Value, SrcUnit), 72, m_DPI);

	case UnitType::DIP:
		return ::MulDiv(ToPixels(Value, SrcUnit), 160, m_DPI);
	}

	return Value;
}


bool CStyleScaling::RealizeFontSize(Font *pFont) const
{
	if (pFont == nullptr)
		return false;

	if (!m_fScaleFont)
		return false;

	const int Size = ToPixels(pFont->Size.Value, pFont->Size.Unit);
	if (Size == 0)
		return false;

	pFont->LogFont.lfHeight =
		pFont->LogFont.lfHeight >= 0 ? Size : -Size;

	return true;
}


int CStyleScaling::GetScaledSystemMetrics(int Index, bool fFallbackScaling) const
{
	return GetSystemMetricsWithDPI(Index, m_DPI, fFallbackScaling);
}


bool CStyleScaling::AdjustWindowRect(HWND hwnd, RECT *pRect) const
{
	return AdjustWindowRectWithDPI(
		pRect,
		::GetWindowLong(hwnd, GWL_STYLE),
		::GetWindowLong(hwnd, GWL_EXSTYLE),
		false,
		m_DPI);
}




void Add(RECT *pRect, const Margins &margins)
{
	pRect->left -= margins.Left;
	pRect->top -= margins.Top;
	pRect->right += margins.Right;
	pRect->bottom += margins.Bottom;
}


void Subtract(RECT *pRect, const Margins &margins)
{
	pRect->left += margins.Left;
	pRect->top += margins.Top;
	pRect->right -= margins.Right;
	pRect->bottom -= margins.Bottom;
	if (pRect->right < pRect->left)
		pRect->right = pRect->left;
	if (pRect->bottom < pRect->top)
		pRect->bottom = pRect->top;
}


int GetFontHeight(HDC hdc, HFONT hfont, const IntValue &Extra, TEXTMETRIC *pTextMetric)
{
	if (hdc == nullptr || hfont == nullptr)
		return 0;

	const HGDIOBJ hOldFont = ::SelectObject(hdc, hfont);
	TEXTMETRIC tm;
	::GetTextMetrics(hdc, &tm);
	::SelectObject(hdc, hOldFont);

	if (pTextMetric != nullptr)
		*pTextMetric = tm;

	if (tm.tmInternalLeading < Extra)
		tm.tmHeight += Extra - tm.tmInternalLeading;

	return tm.tmHeight;
}


int GetFontHeight(HWND hwnd, HFONT hfont, const IntValue &Extra, TEXTMETRIC *pTextMetric)
{
	if (hwnd == nullptr || hfont == nullptr)
		return 0;

	const HDC hdc = ::GetDC(hwnd);
	if (hdc == nullptr)
		return 0;
	const int Height = GetFontHeight(hdc, hfont, Extra, pTextMetric);
	::ReleaseDC(hwnd, hdc);

	return Height;
}


} // namespace Style

} // namespace TVTest
