#include "stdafx.h"
#include "TVTest.h"
#include "GUIUtil.h"
#include "Common/DebugDef.h"


namespace TVTest
{


CIcon::CIcon()
	: m_hico(nullptr)
{
}


CIcon::CIcon(const CIcon &Src)
	: m_hico(nullptr)
{
	*this=Src;
}


CIcon::CIcon(HICON hico)
	: m_hico(hico)
{
}


CIcon::~CIcon()
{
	Destroy();
}


CIcon &CIcon::operator=(const CIcon &Src)
{
	if (&Src!=this) {
		Destroy();

		if (Src.m_hico!=nullptr)
			m_hico=::CopyIcon(Src.m_hico);
	}

	return *this;
}


bool CIcon::Load(HINSTANCE hinst,LPCTSTR pszName)
{
	Destroy();

	m_hico=::LoadIcon(hinst,pszName);

	return m_hico!=nullptr;
}


void CIcon::Attach(HICON hico)
{
	Destroy();
	m_hico=hico;
}


HICON CIcon::Detach()
{
	HICON hico=m_hico;

	m_hico=nullptr;

	return hico;
}


void CIcon::Destroy()
{
	if (m_hico!=nullptr) {
		::DestroyIcon(m_hico);
		m_hico=nullptr;
	}
}




HIMAGELIST CreateImageListFromIcons(
	HINSTANCE hinst,const LPCTSTR *ppszIcons,int IconCount,IconSizeType Size)
{
	if (ppszIcons==nullptr || IconCount<=0)
		return nullptr;

	int IconWidth,IconHeight;

	GetStandardIconSize(Size,&IconWidth,&IconHeight);

	HIMAGELIST himl=::ImageList_Create(IconWidth,IconHeight,ILC_COLOR32 | ILC_MASK,IconCount,1);
	if (himl==nullptr)
		return nullptr;

	for (int i=0;i<IconCount;i++) {
		HICON hicon=nullptr;

		if (ppszIcons[i]!=nullptr)
			hicon=LoadIconStandardSize(hinst,ppszIcons[i],Size);
		if (hicon==nullptr)
			hicon=CreateEmptyIcon(IconWidth,IconHeight,32);
		::ImageList_AddIcon(himl,hicon);
		::DestroyIcon(hicon);
	}

	return himl;
}


void SetWindowIcon(HWND hwnd,HINSTANCE hinst,LPCTSTR pszIcon)
{
	HICON hico=(HICON)::LoadImage(hinst,pszIcon,IMAGE_ICON,
								  0,0,LR_DEFAULTSIZE | LR_SHARED);
	::SendMessage(hwnd,WM_SETICON,ICON_BIG,reinterpret_cast<LPARAM>(hico));
	hico=(HICON)::LoadImage(hinst,pszIcon,IMAGE_ICON,
							::GetSystemMetrics(SM_CXSMICON),
							::GetSystemMetrics(SM_CYSMICON),
							LR_SHARED);
	::SendMessage(hwnd,WM_SETICON,ICON_SMALL,reinterpret_cast<LPARAM>(hico));
}


}	// namespace TVTest
