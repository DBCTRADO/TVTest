#include "stdafx.h"
#include "TVTest.h"
#include "UIBase.h"
#include "AppMain.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


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
