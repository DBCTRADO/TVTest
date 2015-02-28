#include "stdafx.h"
#include "GUIUtil.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


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


}	// namespace TVTest
