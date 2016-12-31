#ifndef TVTEST_GUI_UTIL_H
#define TVTEST_GUI_UTIL_H


#include <commctrl.h>


namespace TVTest
{

	class CIcon
	{
	public:
		CIcon();
		CIcon(const CIcon &Src);
		CIcon(HICON hico);
		~CIcon();
		CIcon &operator=(const CIcon &Src);
		operator HICON() const { return m_hico; }
		operator bool() const { return m_hico!=nullptr; }
		bool Load(HINSTANCE hinst,LPCTSTR pszName);
		void Attach(HICON hico);
		HICON Detach();
		void Destroy();
		bool IsCreated() const { return m_hico!=nullptr; }
		HICON GetHandle() const { return m_hico; }

	private:
		HICON m_hico;
	};


	HIMAGELIST CreateImageListFromIcons(
		HINSTANCE hinst,const LPCTSTR *ppszIcons,int IconCount,IconSizeType Size);
	void SetWindowIcon(HWND hwnd,HINSTANCE hinst,LPCTSTR pszIcon);

}	// namespace TVTest


#endif
