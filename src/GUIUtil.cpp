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
#include "GUIUtil.h"
#include "Common/DebugDef.h"


namespace TVTest
{


CIcon::CIcon(const CIcon &Src)
{
	*this = Src;
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
	if (&Src != this) {
		Destroy();

		if (Src.m_hico != nullptr)
			m_hico = ::CopyIcon(Src.m_hico);
	}

	return *this;
}


bool CIcon::Load(HINSTANCE hinst, LPCTSTR pszName)
{
	Destroy();

	m_hico = ::LoadIcon(hinst, pszName);

	return m_hico != nullptr;
}


void CIcon::Attach(HICON hico)
{
	Destroy();
	m_hico = hico;
}


HICON CIcon::Detach()
{
	const HICON hico = m_hico;

	m_hico = nullptr;

	return hico;
}


void CIcon::Destroy()
{
	if (m_hico != nullptr) {
		::DestroyIcon(m_hico);
		m_hico = nullptr;
	}
}




HIMAGELIST CreateImageListFromIcons(
	HINSTANCE hinst, const LPCTSTR *ppszIcons, int IconCount, int Width, int Height)
{
	if (ppszIcons == nullptr || IconCount <= 0)
		return nullptr;

	const HIMAGELIST himl = ::ImageList_Create(Width, Height, ILC_COLOR32 | ILC_MASK, IconCount, 1);
	if (himl == nullptr)
		return nullptr;

	for (int i = 0; i < IconCount; i++) {
		HICON hicon = nullptr;

		if (ppszIcons[i] != nullptr)
			hicon = LoadIconSpecificSize(hinst, ppszIcons[i], Width, Height);
		if (hicon == nullptr)
			hicon = CreateEmptyIcon(Width, Height, 32);
		::ImageList_AddIcon(himl, hicon);
		::DestroyIcon(hicon);
	}

	return himl;
}


HIMAGELIST CreateImageListFromIcons(
	HINSTANCE hinst, const LPCTSTR *ppszIcons, int IconCount, IconSizeType Size)
{
	int IconWidth, IconHeight;

	GetStandardIconSize(Size, &IconWidth, &IconHeight);

	return CreateImageListFromIcons(hinst, ppszIcons, IconCount, IconWidth, IconHeight);
}


void SetWindowIcon(HWND hwnd, HINSTANCE hinst, LPCTSTR pszIcon)
{
	HICON hico = static_cast<HICON>(::LoadImage(hinst, pszIcon, IMAGE_ICON, 0, 0, LR_DEFAULTSIZE | LR_SHARED));
	::SendMessage(hwnd, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(hico));
	hico = static_cast<HICON>(::LoadImage(
		hinst, pszIcon, IMAGE_ICON,
		::GetSystemMetrics(SM_CXSMICON),
		::GetSystemMetrics(SM_CYSMICON),
		LR_SHARED));
	::SendMessage(hwnd, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(hico));
}


}	// namespace TVTest
