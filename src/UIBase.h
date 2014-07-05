#ifndef TVTEST_UI_BASE_H
#define TVTEST_UI_BASE_H


#include "Style.h"


namespace TVTest
{

	class CUIBase
	{
	public:
		virtual ~CUIBase() = 0;
		virtual void SetStyle(const Style::CStyleManager *pStyleManager);
		virtual void NormalizeStyle(const Style::CStyleManager *pStyleManager);

	protected:
		void InitializeUI();
		const Style::CStyleManager *GetStyleManager() const;
		void UpdateStyle();
	};

}	// namespace TVTest


#endif	// ndef TVTEST_UI_BASE_H
