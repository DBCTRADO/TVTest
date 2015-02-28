#ifndef TVTEST_COM_UTILITY_H
#define TVTEST_COM_UTILITY_H


#include <map>


namespace TVTest
{

	class CIUnknownImpl
	{
	public:
		CIUnknownImpl() : m_RefCount(1) {}

		ULONG AddRefImpl() {
			return ::InterlockedIncrement(&m_RefCount);
		}

		ULONG ReleaseImpl() {
			LONG Count = ::InterlockedDecrement(&m_RefCount);
			if (Count == 0)
				delete this;
			return Count;
		}

	protected:
		LONG m_RefCount;

		virtual ~CIUnknownImpl() {}
	};

	class CVariant : public VARIANT
	{
	public:
		CVariant();
		CVariant(const CVariant &var);
		CVariant(const VARIANT &var);
		CVariant(VARIANT &&var);
		~CVariant();
		CVariant &operator=(const CVariant &var);
		CVariant &operator=(CVariant &&var);
		CVariant &operator=(const VARIANT &var);
		CVariant &operator=(VARIANT &&var);
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
		typedef std::map<String,CVariant> PropertyListType;

		CPropertyBag();

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

		~CPropertyBag();
	};


	HRESULT ShowPropertyPageFrame(IPropertyPage **pPropPages, int NumPages,
								  IUnknown *pObject, HWND hwndOwner, HINSTANCE hinst);

}	// namespace TVTest


#endif
