#include "stdafx.h"
#include <cctype>
#include <cwctype>
#include "TVTest.h"
#include "Style.h"
#include "Settings.h"
#include "Common/DebugDef.h"


namespace TVTest
{

namespace Style
{


int CStyleManager::m_SystemResolutionX=96;
int CStyleManager::m_SystemResolutionY=96;


CStyleManager::CStyleManager()
	: m_ForcedResolutionX(0)
	, m_ForcedResolutionY(0)
	, m_fScaleFont(true)
	, m_fHandleDPIChanged(true)
{
	HDC hdc=::GetDC(nullptr);
	if (hdc!=nullptr) {
		m_SystemResolutionX=::GetDeviceCaps(hdc,LOGPIXELSX);
		m_SystemResolutionY=::GetDeviceCaps(hdc,LOGPIXELSY);
		TRACE(TEXT("System DPI : %d:%d\n"),m_SystemResolutionX,m_SystemResolutionY);
		::ReleaseDC(nullptr,hdc);
	} else {
		m_SystemResolutionX=96;
		m_SystemResolutionY=96;
	}

	m_ResolutionX=m_SystemResolutionX;
	m_ResolutionY=m_SystemResolutionY;
}


bool CStyleManager::Load(LPCTSTR pszFileName)
{
	if (IsStringEmpty(pszFileName))
		return false;

	TRACE(TEXT("CStyleManager::Load() : \"%s\"\n"),pszFileName);

	CSettings Settings;

	if (!Settings.Open(pszFileName,CSettings::OPEN_READ))
		return false;

	if (Settings.SetSection(TEXT("Settings"))) {
		int Value;

		if (Settings.Read(TEXT("DPI"),&Value) && Value>0) {
			m_ForcedResolutionX=Value;
			m_ForcedResolutionY=Value;
			m_ResolutionX=m_ForcedResolutionX;
			m_ResolutionY=m_ForcedResolutionY;
		}

		Settings.Read(TEXT("HandleDPIChanged"),&m_fHandleDPIChanged);
		Settings.Read(TEXT("ScaleFont"),&m_fScaleFont);
	}

	if (Settings.SetSection(TEXT("Styles"))) {
		CSettings::EntryList Entries;
		if (!Settings.GetEntries(&Entries))
			return false;

		for (auto itr=Entries.begin();itr!=Entries.end();++itr) {
			if (!itr->Name.empty() && !itr->Value.empty()) {
				StringUtility::Trim(itr->Value);
				if (!itr->Value.empty()) {
					if (std::_istdigit(itr->Value[0])
							|| itr->Value[0]==_T('-') || itr->Value[0]==_T('+')) {
						IntValue Value;
						if (ParseValue(itr->Value.c_str(),&Value))
							Set(itr->Name.c_str(),Value);
					} else if (StringUtility::CompareNoCase(itr->Value,TEXT("true"))==0
							|| StringUtility::CompareNoCase(itr->Value,TEXT("false"))==0) {
						Set(itr->Name.c_str(),itr->Value[0]==_T('t'));
					} else if (itr->Value.length()>=2
							&& itr->Value.front()==_T('\"')
							&& itr->Value.back()==_T('\"')) {
						Set(itr->Name.c_str(),itr->Value.substr(1,itr->Value.length()-2));
					}
#ifdef _DEBUG
					else {
						TRACE(TEXT("Invalid style : %s=%s\n"),
							  itr->Name.c_str(),itr->Value.c_str());
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

	auto itr=m_StyleMap.find(Info.Name);
	if (itr==m_StyleMap.end()) {
		m_StyleMap.insert(std::pair<String,StyleInfo>(Info.Name,Info));
	} else {
		itr->second=Info;
	}

	return true;
}


bool CStyleManager::Get(LPCTSTR pszName,StyleInfo *pInfo) const
{
	if (IsStringEmpty(pszName) || pInfo==nullptr)
		return false;

	auto itr=m_StyleMap.find(String(pszName));
	if (itr==m_StyleMap.end())
		return false;

	*pInfo=itr->second;

	return true;
}


bool CStyleManager::Set(LPCTSTR pszName,const IntValue &Value)
{
	if (IsStringEmpty(pszName))
		return false;

	StyleInfo Style;
	Style.Name=pszName;
	auto itr=m_StyleMap.find(Style.Name);
	if (itr==m_StyleMap.end()) {
		Style.Type=TYPE_INT;
		Style.Value.Int=Value.Value;
		Style.Unit=Value.Unit;
		m_StyleMap.insert(std::pair<String,StyleInfo>(Style.Name,Style));
	} else {
		itr->second.Type=TYPE_INT;
		itr->second.Value.Int=Value.Value;
		itr->second.Unit=Value.Unit;
	}

	return true;
}


bool CStyleManager::Get(LPCTSTR pszName,IntValue *pValue) const
{
	if (IsStringEmpty(pszName) || pValue==nullptr)
		return false;

	auto itr=m_StyleMap.find(String(pszName));
	if (itr==m_StyleMap.end() || itr->second.Type!=TYPE_INT)
		return false;

	pValue->Value=itr->second.Value.Int;
	pValue->Unit=itr->second.Unit;

	return true;
}


bool CStyleManager::Set(LPCTSTR pszName,bool fValue)
{
	if (IsStringEmpty(pszName))
		return false;

	StyleInfo Style;
	Style.Name=pszName;
	auto itr=m_StyleMap.find(Style.Name);
	if (itr==m_StyleMap.end()) {
		Style.Type=TYPE_BOOL;
		Style.Value.Bool=fValue;
		m_StyleMap.insert(std::pair<String,StyleInfo>(Style.Name,Style));
	} else {
		itr->second.Type=TYPE_BOOL;
		itr->second.Value.Bool=fValue;
	}

	return true;
}


bool CStyleManager::Get(LPCTSTR pszName,bool *pfValue) const
{
	if (IsStringEmpty(pszName) || pfValue==nullptr)
		return false;

	auto itr=m_StyleMap.find(String(pszName));
	if (itr==m_StyleMap.end() || itr->second.Type!=TYPE_BOOL)
		return false;

	*pfValue=itr->second.Value.Bool;

	return true;
}


bool CStyleManager::Set(LPCTSTR pszName,const String &Value)
{
	if (IsStringEmpty(pszName))
		return false;

	StyleInfo Style;
	Style.Name=pszName;
	auto itr=m_StyleMap.find(Style.Name);
	if (itr==m_StyleMap.end()) {
		Style.Type=TYPE_STRING;
		Style.Value.String=Value;
		m_StyleMap.insert(std::pair<String,StyleInfo>(Style.Name,Style));
	} else {
		itr->second.Type=TYPE_STRING;
		itr->second.Value.String=Value;
	}

	return true;
}


bool CStyleManager::Get(LPCTSTR pszName,String *pValue) const
{
	if (IsStringEmpty(pszName) || pValue==nullptr)
		return false;

	auto itr=m_StyleMap.find(String(pszName));
	if (itr==m_StyleMap.end() || itr->second.Type!=TYPE_STRING)
		return false;

	*pValue=itr->second.Value.String;

	return true;
}


bool CStyleManager::Set(LPCTSTR pszName,const Size &Value)
{
	if (IsStringEmpty(pszName))
		return false;

	String Name;

	Name=pszName;
	Name+=TEXT(".width");
	Set(Name.c_str(),Value.Width);
	Name=pszName;
	Name+=TEXT(".height");
	Set(Name.c_str(),Value.Height);

	return true;
}


bool CStyleManager::Get(LPCTSTR pszName,Size *pValue) const
{
	if (IsStringEmpty(pszName) || pValue==nullptr)
		return false;

	String Name;

	Name=pszName;
	Name+=TEXT(".width");
	bool fWidth=Get(Name.c_str(),&pValue->Width);
	Name=pszName;
	Name+=TEXT(".height");
	bool fHeight=Get(Name.c_str(),&pValue->Height);

	return fWidth || fHeight;
}


bool CStyleManager::Set(LPCTSTR pszName,const Margins &Value)
{
	if (IsStringEmpty(pszName))
		return false;

	String Name;

	Name=pszName;
	Name+=TEXT(".left");
	Set(Name.c_str(),Value.Left);
	Name=pszName;
	Name+=TEXT(".top");
	Set(Name.c_str(),Value.Top);
	Name=pszName;
	Name+=TEXT(".right");
	Set(Name.c_str(),Value.Right);
	Name=pszName;
	Name+=TEXT(".bottom");
	Set(Name.c_str(),Value.Bottom);

	return true;
}


bool CStyleManager::Get(LPCTSTR pszName,Margins *pValue) const
{
	if (IsStringEmpty(pszName) || pValue==nullptr)
		return false;

	String Name;
	bool fOK=false;

	IntValue Margin;
	if (Get(pszName,&Margin)) {
		pValue->Left=Margin;
		pValue->Top=Margin;
		pValue->Right=Margin;
		pValue->Bottom=Margin;
		fOK=true;
	}

	Name=pszName;
	Name+=TEXT(".left");
	if (Get(Name.c_str(),&pValue->Left))
		fOK=true;
	Name=pszName;
	Name+=TEXT(".top");
	if (Get(Name.c_str(),&pValue->Top))
		fOK=true;
	Name=pszName;
	Name+=TEXT(".right");
	if (Get(Name.c_str(),&pValue->Right))
		fOK=true;
	Name=pszName;
	Name+=TEXT(".bottom");
	if (Get(Name.c_str(),&pValue->Bottom))
		fOK=true;

	return fOK;
}


bool CStyleManager::InitStyleScaling(CStyleScaling *pScaling,HMONITOR hMonitor) const
{
	if (pScaling==nullptr)
		return false;

	int ResolutionX,ResolutionY;

	if (m_ForcedResolutionX>0 && m_ForcedResolutionY>0) {
		ResolutionX=m_ForcedResolutionX;
		ResolutionY=m_ForcedResolutionY;
	} else {
		ResolutionX=m_SystemResolutionX;
		ResolutionY=m_SystemResolutionY;

		if (hMonitor!=nullptr) {
			UINT DpiX,DpiY;
			if (Util::GetMonitorDPI(hMonitor,&DpiX,&DpiY)!=0) {
				ResolutionX=DpiX;
				ResolutionY=DpiY;
			}
		}
	}

	pScaling->SetDPI(ResolutionX,ResolutionY);
	pScaling->SetSystemDPI(m_SystemResolutionX,m_SystemResolutionY);
	pScaling->SetScaleFont(m_fScaleFont);

	return true;
}


bool CStyleManager::InitStyleScaling(CStyleScaling *pScaling,HWND hwnd) const
{
	return InitStyleScaling(pScaling,::MonitorFromWindow(::GetAncestor(hwnd,GA_ROOT),MONITOR_DEFAULTTONULL));
}


bool CStyleManager::InitStyleScaling(CStyleScaling *pScaling,const RECT &Position) const
{
	return InitStyleScaling(pScaling,::MonitorFromRect(&Position,MONITOR_DEFAULTTONULL));
}


bool CStyleManager::InitStyleScaling(CStyleScaling *pScaling) const
{
	return InitStyleScaling(pScaling,static_cast<HMONITOR>(nullptr));
}


int CStyleManager::GetSystemDPI() const
{
	return m_SystemResolutionY;
}


int CStyleManager::GetForcedDPI() const
{
	return m_ForcedResolutionY;
}


bool CStyleManager::IsHandleDPIChanged() const
{
	return m_fHandleDPIChanged;
}


bool CStyleManager::AssignFontSizeFromLogFont(Font *pFont)
{
	if (pFont==nullptr)
		return false;

	pFont->Size.Value=::MulDiv(abs(pFont->LogFont.lfHeight),72,m_SystemResolutionY);
	if (pFont->Size.Value!=0)
		pFont->Size.Unit=TVTest::Style::UNIT_POINT;
	else
		pFont->Size.Unit=TVTest::Style::UNIT_UNDEFINED;

	return true;
}


bool CStyleManager::ParseValue(LPCTSTR pszText,IntValue *pValue)
{
	if (IsStringEmpty(pszText) || pValue==nullptr)
		return false;

	if (!std::_istdigit(pszText[0]) && pszText[0]!=_T('-') && pszText[0]!=_T('+'))
		return false;

	LPTSTR pEnd;
	int Value=static_cast<int>(std::_tcstol(pszText,&pEnd,10));
	UnitType Unit=ParseUnit(pEnd);
	if (Unit==UNIT_UNDEFINED)
		return false;

	*pValue=IntValue(Value,Unit);

	return true;
}


UnitType CStyleManager::ParseUnit(LPCTSTR pszUnit)
{
	if (IsStringEmpty(pszUnit))
		return UNIT_LOGICAL_PIXEL;
	if (::lstrcmpi(pszUnit,TEXT("px"))==0)
		return UNIT_PHYSICAL_PIXEL;
	if (::lstrcmpi(pszUnit,TEXT("pt"))==0)
		return UNIT_POINT;
	if (::lstrcmpi(pszUnit,TEXT("dp"))==0)
		return UNIT_DIP;

	return UNIT_UNDEFINED;
}




CStyleScaling::CStyleScaling()
	: m_ResolutionX(96)
	, m_ResolutionY(96)
	, m_SystemResolutionX(96)
	, m_SystemResolutionY(96)
	, m_fScaleFont(true)
{
}


bool CStyleScaling::SetDPI(int ResolutionX,int ResolutionY)
{
	if (ResolutionX<1 || ResolutionY<1)
		return false;

	m_ResolutionX=ResolutionX;
	m_ResolutionY=ResolutionY;

	return true;
}


int CStyleScaling::GetDPI() const
{
	return m_ResolutionY;
}


bool CStyleScaling::SetSystemDPI(int ResolutionX,int ResolutionY)
{
	if (ResolutionX<1 || ResolutionY<1)
		return false;

	m_SystemResolutionX=ResolutionX;
	m_SystemResolutionY=ResolutionY;

	return true;
}


int CStyleScaling::GetSystemDPI() const
{
	return m_SystemResolutionY;
}


void CStyleScaling::SetScaleFont(bool fScale)
{
	m_fScaleFont=fScale;
}


bool CStyleScaling::ToPixels(IntValue *pValue) const
{
	if (pValue==nullptr)
		return false;

	switch (pValue->Unit) {
	case UNIT_LOGICAL_PIXEL:
		pValue->Value=LogicalPixelsToPhysicalPixels(pValue->Value);
		break;

	case UNIT_PHYSICAL_PIXEL:
		return true;

	case UNIT_POINT:
		pValue->Value=PointsToPixels(pValue->Value);
		break;

	case UNIT_DIP:
		pValue->Value=DipToPixels(pValue->Value);
		break;

	default:
		return false;
	}

	pValue->Unit=UNIT_PHYSICAL_PIXEL;

	return true;
}


bool CStyleScaling::ToPixels(Size *pValue) const
{
	if (pValue==nullptr)
		return false;

	ToPixels(&pValue->Width);
	ToPixels(&pValue->Height);

	return true;
}


bool CStyleScaling::ToPixels(Margins *pValue) const
{
	if (pValue==nullptr)
		return false;

	ToPixels(&pValue->Left);
	ToPixels(&pValue->Top);
	ToPixels(&pValue->Right);
	ToPixels(&pValue->Bottom);

	return true;
}


int CStyleScaling::ToPixels(int Value,UnitType Unit) const
{
	switch (Unit) {
	case UNIT_LOGICAL_PIXEL:
		return LogicalPixelsToPhysicalPixels(Value);

	case UNIT_PHYSICAL_PIXEL:
		return Value;

	case UNIT_POINT:
		return PointsToPixels(Value);

	case UNIT_DIP:
		return DipToPixels(Value);
	}

	return 0;
}


int CStyleScaling::LogicalPixelsToPhysicalPixels(int Pixels) const
{
	return ::MulDiv(Pixels,m_ResolutionY,96);
}


int CStyleScaling::PointsToPixels(int Points) const
{
	// 1pt = 1/72in
	return ::MulDiv(Points,m_ResolutionY,72);
}


int CStyleScaling::DipToPixels(int Dip) const
{
	// 1dp = 1/160in
	return ::MulDiv(Dip,m_ResolutionY,160);
}


int CStyleScaling::ConvertUnit(int Value,UnitType SrcUnit,UnitType DstUnit) const
{
	switch (DstUnit) {
	case UNIT_LOGICAL_PIXEL:
		return ::MulDiv(ToPixels(Value,SrcUnit),96,m_ResolutionY);

	case UNIT_PHYSICAL_PIXEL:
		return ToPixels(Value,SrcUnit);

	case UNIT_POINT:
		return ::MulDiv(ToPixels(Value,SrcUnit),72,m_ResolutionY);

	case UNIT_DIP:
		return ::MulDiv(ToPixels(Value,SrcUnit),160,m_ResolutionY);
	}

	return Value;
}


bool CStyleScaling::RealizeFontSize(Font *pFont) const
{
	if (pFont==nullptr)
		return false;

	if (!m_fScaleFont)
		return false;

	int Size=ToPixels(pFont->Size.Value,pFont->Size.Unit);
	if (Size==0)
		return false;

	pFont->LogFont.lfHeight=
		pFont->LogFont.lfHeight>=0 ? Size : -Size;

	return true;
}


int CStyleScaling::GetScaledSystemMetrics(int Index) const
{
	return ::MulDiv(::GetSystemMetrics(Index),m_ResolutionY,m_SystemResolutionY);
}




void Add(RECT *pRect,const Margins &margins)
{
	pRect->left-=margins.Left;
	pRect->top-=margins.Top;
	pRect->right+=margins.Right;
	pRect->bottom+=margins.Bottom;
}


void Subtract(RECT *pRect,const Margins &margins)
{
	pRect->left+=margins.Left;
	pRect->top+=margins.Top;
	pRect->right-=margins.Right;
	pRect->bottom-=margins.Bottom;
	if (pRect->right<pRect->left)
		pRect->right=pRect->left;
	if (pRect->bottom<pRect->top)
		pRect->bottom=pRect->top;
}


int GetFontHeight(HDC hdc,HFONT hfont,const IntValue &Extra,TEXTMETRIC *pTextMetric)
{
	if (hdc==nullptr || hfont==nullptr)
		return 0;

	HGDIOBJ hOldFont=::SelectObject(hdc,hfont);
	TEXTMETRIC tm;
	::GetTextMetrics(hdc,&tm);
	::SelectObject(hdc,hOldFont);

	if (pTextMetric!=nullptr)
		*pTextMetric=tm;

	if (tm.tmInternalLeading<Extra)
		tm.tmHeight+=Extra-tm.tmInternalLeading;

	return tm.tmHeight;
}


int GetFontHeight(HWND hwnd,HFONT hfont,const IntValue &Extra,TEXTMETRIC *pTextMetric)
{
	if (hwnd==nullptr || hfont==nullptr)
		return 0;

	HDC hdc=::GetDC(hwnd);
	if (hdc==nullptr)
		return 0;
	int Height=GetFontHeight(hdc,hfont,Extra,pTextMetric);
	::ReleaseDC(hwnd,hdc);

	return Height;
}


}	// namespace Style

}	// namespace TVTest
