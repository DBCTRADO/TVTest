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


#ifndef TVTEST_COM_UTILITY_H
#define TVTEST_COM_UTILITY_H


#include <map>


namespace TVTest
{

	template<typename T> inline void SafeRelease(T **ppObj)
	{
		if (*ppObj != nullptr) {
			(*ppObj)->Release();
			*ppObj = nullptr;
		}
	}

	class CIUnknownImpl
	{
	public:
		ULONG AddRefImpl() {
			return ::InterlockedIncrement(&m_RefCount);
		}

		ULONG ReleaseImpl() {
			const LONG Count = ::InterlockedDecrement(&m_RefCount);
			if (Count == 0)
				delete this;
			return Count;
		}

	protected:
		LONG m_RefCount = 1;

		virtual ~CIUnknownImpl() = default;
	};

	class CVariant
		: public VARIANT
	{
	public:
		CVariant();
		CVariant(const CVariant &var);
		CVariant(const VARIANT &var);
		CVariant(VARIANT &&var) noexcept;
		~CVariant();
		CVariant &operator=(const CVariant &var);
		CVariant &operator=(CVariant &&var) noexcept;
		CVariant &operator=(const VARIANT &var);
		CVariant &operator=(VARIANT &&var) noexcept;
		HRESULT Assign(const VARIANT &var);
		void Clear();
		HRESULT ChangeType(VARTYPE Type);
		HRESULT ToString(String *pStr) const;
		HRESULT FromString(const String &Str);
	};

	class CPropertyBag
		: public IPropertyBag
		, protected CIUnknownImpl
	{
	public:
		typedef std::map<String, CVariant> PropertyListType;

		// IUnknown
		STDMETHODIMP QueryInterface(REFIID riid, void **ppvObject) override;
		STDMETHODIMP_(ULONG) AddRef() override { return AddRefImpl(); }
		STDMETHODIMP_(ULONG) Release() override { return ReleaseImpl(); }

		// IPropertyBag
		STDMETHODIMP Read(LPCOLESTR pszPropName, VARIANT *pVar, IErrorLog *pErrorLog) override;
		STDMETHODIMP Write(LPCOLESTR pszPropName, VARIANT *pVar) override;

		// CPropertyBag
		PropertyListType::iterator begin() { return m_Properties.begin(); }
		PropertyListType::iterator end() { return m_Properties.end(); }

	protected:
		PropertyListType m_Properties;

		~CPropertyBag() = default;
	};


	HRESULT ShowPropertyPageFrame(
		IPropertyPage **pPropPages, int NumPages,
		IUnknown *pObject, HWND hwndOwner, HINSTANCE hinst);
	HRESULT ShowPropertyPageFrame(
		IUnknown *pObject, HWND hwndOwner, HINSTANCE hinst);

}	// namespace TVTest


#endif
