// IBonDriver2.h: IBonDriver2 クラスのインターフェイス
//
//////////////////////////////////////////////////////////////////////

#if !defined(_IBONDRIVER2_H_)
#define _IBONDRIVER2_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#include "IBonDriver.h"


// 凡ドライバインタフェース2
class IBonDriver2 : public IBonDriver
{
public:
	virtual LPCTSTR GetTunerName(void) = 0;

	virtual const BOOL IsTunerOpening(void) = 0;
	
	virtual LPCTSTR EnumTuningSpace(const DWORD dwSpace) = 0;
	virtual LPCTSTR EnumChannelName(const DWORD dwSpace, const DWORD dwChannel) = 0;

	virtual const BOOL SetChannel(const DWORD dwSpace, const DWORD dwChannel) = 0;
	
	virtual const DWORD GetCurSpace(void) = 0;
	virtual const DWORD GetCurChannel(void) = 0;
	
// IBonDriver
	virtual void Release(void) = 0;
};

#endif // !defined(_IBONDRIVER2_H_)
