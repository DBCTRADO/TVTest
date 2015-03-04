#include "stdafx.h"
#include "TVTest.h"
#include "UIBase.h"
#include "AppMain.h"
#include "Common/DebugDef.h"


namespace TVTest
{


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


}	// namespace TVTest
