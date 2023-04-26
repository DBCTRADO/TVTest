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
#include "ComUtility.h"
#include "Dialog.h"
#include "DialogUtil.h"
#include "DPIUtil.h"
#include "TVTestInterface.h"
#include "resource.h"
#include "Common/DebugDef.h"


namespace TVTest
{


CVariant::CVariant()
{
	::VariantInit(this);
}


CVariant::CVariant(const CVariant &var)
{
	::VariantInit(this);
	*this = var;
}


CVariant::CVariant(const VARIANT &var)
{
	::VariantInit(this);
	*this = var;
}


CVariant::CVariant(VARIANT &&var) noexcept
{
	*static_cast<VARIANT*>(this) = var;
	::VariantInit(&var);
}


CVariant::~CVariant()
{
	::VariantClear(this);
}


CVariant &CVariant::operator=(const CVariant &var)
{
	if (&var != this) {
		Assign(var);
	}

	return *this;
}


CVariant &CVariant::operator=(CVariant &&var) noexcept
{
	if (&var != this) {
		*static_cast<VARIANT*>(this) = var;
		::VariantInit(&var);
	}

	return *this;
}


CVariant &CVariant::operator=(const VARIANT &var)
{
	if (&var != this) {
		Assign(var);
	}

	return *this;
}


CVariant &CVariant::operator=(VARIANT &&var) noexcept
{
	if (&var != this) {
		*static_cast<VARIANT*>(this) = var;
		::VariantInit(&var);
	}

	return *this;
}


HRESULT CVariant::Assign(const VARIANT &var)
{
	return ::VariantCopy(this, &var);
}


void CVariant::Clear()
{
	::VariantClear(this);
}


HRESULT CVariant::ChangeType(VARTYPE Type)
{
	return ::VariantChangeType(this, this, VARIANT_ALPHABOOL, Type);
}


HRESULT CVariant::ToString(String *pStr) const
{
	if (pStr == nullptr)
		return E_POINTER;

	VARIANT var;

	::VariantInit(&var);
	const HRESULT hr = ::VariantChangeType(&var, this, VARIANT_ALPHABOOL, VT_BSTR);
	if (FAILED(hr))
		return hr;

	*pStr = var.bstrVal;

	::VariantClear(&var);

	return S_OK;
}


HRESULT CVariant::FromString(const String &Str)
{
	::VariantClear(this);

	bstrVal = ::SysAllocString(Str.c_str());
	if (bstrVal == nullptr)
		return E_OUTOFMEMORY;
	vt = VT_BSTR;

	return S_OK;
}




STDMETHODIMP CPropertyBag::QueryInterface(REFIID riid, void **ppvObject)
{
	if (ppvObject == nullptr)
		return E_POINTER;

	if (riid == IID_IUnknown || riid == IID_IPropertyBag) {
		*ppvObject = static_cast<IPropertyBag*>(this);
	} else {
		*ppvObject = nullptr;
		return E_NOINTERFACE;
	}

	AddRef();

	return S_OK;
}


STDMETHODIMP CPropertyBag::Read(LPCOLESTR pszPropName, VARIANT *pVar, IErrorLog *pErrorLog)
{
	if (pszPropName == nullptr)
		return E_POINTER;

	auto it = m_Properties.find(String(pszPropName));
	if (it == m_Properties.end())
		return E_INVALIDARG;

	if (pVar->vt == VT_EMPTY || pVar->vt == it->second.vt)
		return ::VariantCopy(pVar, &it->second);

	return ::VariantChangeType(pVar, &it->second, VARIANT_ALPHABOOL, pVar->vt);
}


STDMETHODIMP CPropertyBag::Write(LPCOLESTR pszPropName, VARIANT *pVar)
{
	if (pszPropName == nullptr || pVar == nullptr)
		return E_POINTER;

	CVariant var;
	const HRESULT hr = var.Assign(*pVar);
	if (FAILED(hr))
		return hr;

	auto it = m_Properties.find(String(pszPropName));

	if (it == m_Properties.end()) {
		it = m_Properties.emplace(
			std::piecewise_construct,
			std::forward_as_tuple(pszPropName),
			std::forward_as_tuple()).first;
	}
	it->second = std::move(var);

	return S_OK;
}




class CPropertyPageSite
	: public IPropertyPageSite
	, protected CIUnknownImpl
{
public:
// IUnknown
	STDMETHODIMP QueryInterface(REFIID riid, void **ppvObject) override;
	STDMETHODIMP_(ULONG) AddRef() override { return AddRefImpl(); }
	STDMETHODIMP_(ULONG) Release() override { return ReleaseImpl(); }

// IPropertyPageSite
	STDMETHODIMP OnStatusChange(DWORD dwFlags) override { return S_OK; }
	STDMETHODIMP GetLocaleID(LCID *pLocaleID) override;
	STDMETHODIMP GetPageContainer(IUnknown **ppUnk) override { return E_NOTIMPL; }
	STDMETHODIMP TranslateAccelerator(MSG *pMsg) override { return E_NOTIMPL; }

private:
	~CPropertyPageSite() = default;
};


STDMETHODIMP CPropertyPageSite::QueryInterface(REFIID riid, void **ppvObject)
{
	if (ppvObject == nullptr)
		return E_POINTER;

	if (riid == IID_IUnknown || riid == IID_IPropertyPageSite) {
		*ppvObject = static_cast<IPropertyPageSite*>(this);
	} else {
		*ppvObject = nullptr;
		return E_NOINTERFACE;
	}

	AddRef();

	return S_OK;
}


STDMETHODIMP CPropertyPageSite::GetLocaleID(LCID *pLocaleID)
{
	if (pLocaleID == nullptr)
		return E_POINTER;

	*pLocaleID = ::GetUserDefaultLCID();

	return S_OK;
}




class CPropertyPageFrame
	: public CBasicDialog
{
public:
	CPropertyPageFrame(IPropertyPage **ppPropPages, int NumPages, CPropertyPageSite *pPageSite);
	~CPropertyPageFrame();
	bool Show(HWND hwndOwner, HINSTANCE hinst);

private:
	struct PageInfo
	{
		IPropertyPage *pPropPage;
		String Title;
		SIZE Size;
		bool fActivated;
	};

	CPropertyPageSite *m_pPageSite;
	std::vector<PageInfo> m_PageList;
	bool m_fTabInitialized = false;
	int m_CurTab = -1;
	RECT m_PageRect;

	INT_PTR DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) override;
	void SetCurTab(int Tab);
};


CPropertyPageFrame::CPropertyPageFrame(IPropertyPage **ppPropPages, int NumPages, CPropertyPageSite *pPageSite)
	: m_pPageSite(pPageSite)
{
	m_fDisableDarkMode = true;

	m_PageList.reserve(NumPages);

	for (int i = 0; i < NumPages; i++) {
		PageInfo Info;

		Info.pPropPage = ppPropPages[i];
		Info.pPropPage->AddRef();
		Info.fActivated = false;
		m_PageList.push_back(Info);
	}
}


CPropertyPageFrame::~CPropertyPageFrame()
{
	for (auto &e : m_PageList) {
		e.pPropPage->SetObjects(0, nullptr);
		e.pPropPage->SetPageSite(nullptr);
		e.pPropPage->Release();
	}

	m_pPageSite->Release();
}


bool CPropertyPageFrame::Show(HWND hwndOwner, HINSTANCE hinst)
{
	return ShowDialog(hwndOwner, hinst, MAKEINTRESOURCE(IDD_PROPERTYPAGEFRAME)) == IDOK;
}


INT_PTR CPropertyPageFrame::DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			m_fTabInitialized = false;
			m_CurTab = -1;

			const HWND hwndTab = ::GetDlgItem(hDlg, IDC_PROPERTYPAGEFRAME_TAB);
			SIZE MaxSize = {0, 0};
			TCITEM tci;

			tci.mask = TCIF_TEXT | TCIF_PARAM;

			for (size_t i = 0; i < m_PageList.size(); i++) {
				PageInfo &Info = m_PageList[i];
				PROPPAGEINFO PropPageInfo = {sizeof(PROPPAGEINFO)};
				const HRESULT hr = Info.pPropPage->GetPageInfo(&PropPageInfo);
				if (FAILED(hr))
					return TRUE;
				Info.Size = PropPageInfo.size;
				if (PropPageInfo.size.cx > MaxSize.cx)
					MaxSize.cx = PropPageInfo.size.cx;
				if (PropPageInfo.size.cy > MaxSize.cy)
					MaxSize.cy = PropPageInfo.size.cy;
				if (PropPageInfo.pszTitle != nullptr) {
					Info.Title = PropPageInfo.pszTitle;
					::CoTaskMemFree(PropPageInfo.pszTitle);
				}
				if (PropPageInfo.pszDocString != nullptr)
					::CoTaskMemFree(PropPageInfo.pszDocString);
				if (PropPageInfo.pszHelpFile != nullptr)
					::CoTaskMemFree(PropPageInfo.pszHelpFile);

				tci.pszText = const_cast<LPTSTR>(Info.Title.c_str());
				tci.lParam = i;
				TabCtrl_InsertItem(hwndTab, i, &tci);
			}

			RECT rc;
			::SetRect(&rc, 0, 0, MaxSize.cx, MaxSize.cy);
			TabCtrl_AdjustRect(hwndTab, TRUE, &rc);

			RECT rcTab;
			GetDlgItemRect(hDlg, IDC_PROPERTYPAGEFRAME_TAB, &rcTab);
			int XOffset = (rc.right - rc.left) - (rcTab.right - rcTab.left);
			int YOffset = (rc.bottom - rc.top) - (rcTab.bottom - rcTab.top);
			if (XOffset > 0)
				rcTab.right += XOffset;
			else
				XOffset = 0;
			if (YOffset > 0)
				rcTab.bottom += YOffset;
			else
				YOffset = 0;
			::GetWindowRect(hDlg, &rc);
			::MoveWindow(
				hDlg, rc.left, rc.top,
				rc.right - rc.left + XOffset,
				rc.bottom - rc.top + YOffset,
				FALSE);
			::MoveWindow(
				hwndTab,
				rcTab.left, rcTab.top,
				rcTab.right - rcTab.left, rcTab.bottom - rcTab.top, FALSE);
			GetDlgItemRect(hDlg, IDOK, &rc);
			::MoveWindow(
				::GetDlgItem(hDlg, IDOK),
				rc.left + XOffset, rc.top + YOffset,
				rc.right - rc.left, rc.bottom - rc.top, FALSE);
			GetDlgItemRect(hDlg, IDCANCEL, &rc);
			::MoveWindow(
				::GetDlgItem(hDlg, IDCANCEL),
				rc.left + XOffset, rc.top + YOffset,
				rc.right - rc.left, rc.bottom - rc.top, FALSE);

			m_PageRect = rcTab;
			TabCtrl_AdjustRect(hwndTab, FALSE, &m_PageRect);

			m_fTabInitialized = true;

			SetCurTab(0);

			AdjustDialogPos(::GetParent(hDlg), hDlg);
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			for (auto &e : m_PageList) {
				if (e.fActivated)
					e.pPropPage->Apply();
			}
			[[fallthrough]];
		case IDCANCEL:
			::EndDialog(hDlg, LOWORD(wParam));
			return TRUE;
		}
		return TRUE;

	case WM_NOTIFY:
		{
			const NMHDR *pnmh = reinterpret_cast<const NMHDR*>(lParam);

			switch (pnmh->code) {
			case TCN_SELCHANGE:
				if (m_fTabInitialized)
					SetCurTab(TabCtrl_GetCurSel(pnmh->hwndFrom));
				return TRUE;
			}
		}
		break;

	case WM_DESTROY:
		for (auto &e : m_PageList) {
			if (e.fActivated) {
				e.pPropPage->Deactivate();
				e.fActivated = false;
			}
		}
		return TRUE;
	}

	return FALSE;
}


void CPropertyPageFrame::SetCurTab(int Tab)
{
	if (m_CurTab == Tab)
		return;
	if (Tab < 0 || static_cast<size_t>(Tab) >= m_PageList.size())
		return;

	PageInfo &Page = m_PageList[Tab];

	if (!Page.fActivated) {
		if (FAILED(Page.pPropPage->Activate(m_hDlg, &m_PageRect, TRUE)))
			return;
		Page.fActivated = true;
	}

	if (m_CurTab >= 0) {
		PageInfo &OldPage = m_PageList[m_CurTab];
		if (OldPage.fActivated)
			OldPage.pPropPage->Show(SW_HIDE);
	}

	Page.pPropPage->Show(SW_SHOW);

	m_CurTab = Tab;
}




HRESULT ShowPropertyPageFrame(
	IPropertyPage **ppPropPages, int NumPages,
	IUnknown *pObject, HWND hwndOwner, HINSTANCE hinst)
{
	if (ppPropPages == nullptr)
		return E_POINTER;
	if (NumPages < 1)
		return E_INVALIDARG;

	CPropertyPageSite *pPageSite = new CPropertyPageSite;
	for (int i = 0; i < NumPages; i++) {
		const HRESULT hr = ppPropPages[i]->SetPageSite(pPageSite);
		if (FAILED(hr)) {
			for (i--; i >= 0; i--)
				ppPropPages[i]->SetPageSite(nullptr);
			pPageSite->Release();
			return hr;
		}
	}

	if (pObject != nullptr) {
		for (int i = 0; i < NumPages; i++)
			ppPropPages[i]->SetObjects(1, &pObject);
	}

	// TVTest DTV Video Decoder のダイアログが欠けるのでとりあえず System DPI とする
	SystemDPIBlock SystemDPI;

	CPropertyPageFrame Frame(ppPropPages, NumPages, pPageSite);

	Frame.Show(hwndOwner, hinst);

	return S_OK;
}


HRESULT ShowPropertyPageFrame(IUnknown *pObject, HWND hwndOwner, HINSTANCE hinst)
{
	if (pObject == nullptr)
		return E_POINTER;

	HRESULT hr;
	ISpecifyPropertyPages2 *pSpecifyPropPages2;

	hr = pObject->QueryInterface(IID_PPV_ARGS(&pSpecifyPropPages2));

	if (SUCCEEDED(hr)) {
		CAUUID Pages = {0, nullptr};

		hr = pSpecifyPropPages2->GetPages(&Pages);
		if (SUCCEEDED(hr) && Pages.pElems != nullptr) {
			if (Pages.cElems > 0) {
				std::vector<IPropertyPage*> PropPages(Pages.cElems);
				ULONG j;

				for (j = 0; j < Pages.cElems; j++) {
					hr = pSpecifyPropPages2->CreatePage(Pages.pElems[j], &PropPages[j]);
					if (FAILED(hr))
						break;
				}
				if (SUCCEEDED(hr))
					hr = ShowPropertyPageFrame(PropPages.data(), Pages.cElems, pObject, hwndOwner, hinst);
				while (j > 0)
					PropPages[--j]->Release();
			}
			::CoTaskMemFree(Pages.pElems);
		}
		pSpecifyPropPages2->Release();
	}

	return hr;
}


}	// namespace TVTest
