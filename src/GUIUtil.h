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


#ifndef TVTEST_GUI_UTIL_H
#define TVTEST_GUI_UTIL_H


#include <commctrl.h>


namespace TVTest
{

	class CIcon
	{
	public:
		CIcon() = default;
		CIcon(const CIcon &Src);
		CIcon(HICON hico);
		~CIcon();

		CIcon &operator=(const CIcon &Src);

		operator HICON() const { return m_hico; }
		operator bool() const { return m_hico != nullptr; }

		bool Load(HINSTANCE hinst, LPCTSTR pszName);
		void Attach(HICON hico);
		HICON Detach();
		void Destroy();
		bool IsCreated() const { return m_hico != nullptr; }
		HICON GetHandle() const { return m_hico; }

	private:
		HICON m_hico = nullptr;
	};


	HIMAGELIST CreateImageListFromIcons(
		HINSTANCE hinst, const LPCTSTR *ppszIcons, int IconCount, int Width, int Height);
	HIMAGELIST CreateImageListFromIcons(
		HINSTANCE hinst, const LPCTSTR *ppszIcons, int IconCount, IconSizeType Size);
	void SetWindowIcon(HWND hwnd, HINSTANCE hinst, LPCTSTR pszIcon);

}	// namespace TVTest


#endif
