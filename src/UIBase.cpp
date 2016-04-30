#include "stdafx.h"
#include "TVTest.h"
#include "UIBase.h"
#include "AppMain.h"
#include "StyleUtil.h"
#include "Common/DebugDef.h"


namespace TVTest
{


	Style::Font CUIBase::m_DefaultFont;
	bool CUIBase::m_fValidDefaultFont=false;


	CUIBase::~CUIBase()
	{
	}


	void CUIBase::SetStyle(const Style::CStyleManager *pStyleManager)
	{
	}


	void CUIBase::NormalizeStyle(const Style::CStyleManager *pStyleManager)
	{
	}


	void CUIBase::SetTheme(const Theme::CThemeManager *pThemeManager)
	{
	}


	void CUIBase::InitializeUI()
	{
		UpdateStyle();
	}


	const Style::CStyleManager *CUIBase::GetStyleManager() const
	{
		return &GetAppClass().StyleManager;
	}


	void CUIBase::UpdateStyle()
	{
		const Style::CStyleManager *pStyleManager=GetStyleManager();
		SetStyle(pStyleManager);
		NormalizeStyle(pStyleManager);
	}


	bool CUIBase::GetDefaultFont(Style::Font *pFont) const
	{
		if (pFont==nullptr)
			return false;

		if (!m_fValidDefaultFont) {
			if (!StyleUtil::GetDefaultUIFont(&m_DefaultFont))
				return false;
			m_fValidDefaultFont=true;
		}

		*pFont=m_DefaultFont;

		return true;
	}


	bool CUIBase::GetSystemFont(DrawUtil::FontType Type,Style::Font *pFont) const
	{
		return StyleUtil::GetSystemFont(Type,pFont);
	}


	bool CUIBase::CreateDrawFont(const Style::Font &Font,DrawUtil::CFont *pDrawFont) const
	{
		if (pDrawFont==nullptr)
			return false;

		Style::Font f=Font;

		GetStyleManager()->RealizeFontSize(&f);

		return pDrawFont->Create(&f.LogFont);
	}


	bool CUIBase::CreateDefaultFont(DrawUtil::CFont *pDefaultFont) const
	{
		Style::Font Font;

		if (!GetDefaultFont(&Font))
			return false;

		return CreateDrawFont(Font,pDefaultFont);
	}


	bool CUIBase::CreateDefaultFontAndBoldFont(DrawUtil::CFont *pDefaultFont,DrawUtil::CFont *pBoldFont) const
	{
		if (!CreateDefaultFont(pDefaultFont))
			return false;

		if (pBoldFont!=nullptr) {
			LOGFONT lf;

			if (!pDefaultFont->GetLogFont(&lf))
				return false;

			lf.lfWeight=FW_BOLD;

			if (!pBoldFont->Create(&lf))
				return false;
		}

		return true;
	}


	HCURSOR CUIBase::GetActionCursor() const
	{
		return GetAppClass().UICore.GetActionCursor();
	}


	HCURSOR CUIBase::GetLinkCursor() const
	{
		return GetAppClass().UICore.GetLinkCursor();
	}


	bool CUIBase::ConvertBorderWidthsInPixels(Theme::BorderStyle *pStyle) const
	{
		if (pStyle==nullptr)
			return false;

		const Style::CStyleManager *pStyleManager=GetStyleManager();

		pStyleManager->ToPixels(&pStyle->Width.Left);
		pStyleManager->ToPixels(&pStyle->Width.Top);
		pStyleManager->ToPixels(&pStyle->Width.Right);
		pStyleManager->ToPixels(&pStyle->Width.Bottom);

		return true;
	}


	bool CUIBase::GetBorderWidthsInPixels(const Theme::BorderStyle &Style,RECT *pWidths) const
	{
		Theme::BorderStyle Border=Style;

		ConvertBorderWidthsInPixels(&Border);
		return Theme::GetBorderWidths(Border,pWidths);
	}


	int CUIBase::GetHairlineWidth() const
	{
		int Width=GetStyleManager()->ToPixels(1,Style::UNIT_LOGICAL_PIXEL);
		return max(Width,1);
	}


	void CUIBase::ResetDefaultFont()
	{
		m_fValidDefaultFont=false;
	}


}	// namespace TVTest
